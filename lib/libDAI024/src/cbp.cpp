/*  This file is part of libDAI - http://www.libdai.org/
 *
 *  libDAI is licensed under the terms of the GNU General Public License version
 *  2, or (at your option) any later version. libDAI is distributed without any
 *  warranty. See the file COPYING for more details.
 *
 *  Copyright (C) 2009  Frederik Eaton [frederik at ofb dot net]
 */


#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <algorithm>

#include <dai/util.h>
#include <dai/properties.h>
#include <dai/gibbs.h>
#include <dai/bp.h>
#include <dai/cbp.h>
#include <dai/bbp.h>


namespace dai {


using namespace std;
using boost::shared_ptr;


const char *CBP::Name = "CBP";


/// Given a sorted vector of states \a xis and total state count \a n_states, return a vector of states not in \a xis
vector<size_t> complement( vector<size_t> &xis, size_t n_states ) {
    vector<size_t> cmp_xis( 0 );
    size_t j = 0;
    for( size_t xi = 0; xi < n_states; xi++ ) {
        while( j < xis.size() && xis[j] < xi )
            j++;
        if( j >= xis.size() || xis[j] > xi )
            cmp_xis.push_back(xi);
    }
    DAI_ASSERT( xis.size()+cmp_xis.size() == n_states );
    return cmp_xis;
}


/// Computes \f$\frac{\exp(a)}{\exp(a)+\exp(b)}\f$
Real unSoftMax( Real a, Real b ) {
    if( a > b )
        return 1.0 / (1.0 + exp(b-a));
    else
        return exp(a-b) / (exp(a-b) + 1.0);
}


/// Computes log of sum of exponents, i.e., \f$\log\left(\exp(a) + \exp(b)\right)\f$
Real logSumExp( Real a, Real b ) {
    if( a > b )
        return a + log1p( exp( b-a ) );
    else
        return b + log1p( exp( a-b ) );
}


/// Compute sum of pairwise L-infinity distances of the first \a nv factors in each vector
Real dist( const vector<Factor> &b1, const vector<Factor> &b2, size_t nv ) {
    Real d = 0.0;
    for( size_t k = 0; k < nv; k++ )
        d += dist( b1[k], b2[k], Prob::DISTLINF );
    return d;
}


void CBP::setBeliefs( const std::vector<Factor> &bs, Real logZ ) {
    size_t i = 0;
    _beliefsV.clear();
    _beliefsV.reserve( nrVars() );
    _beliefsF.clear();
    _beliefsF.reserve( nrFactors() );
    for( i = 0; i < nrVars(); i++ )
        _beliefsV.push_back( bs[i] );
    for( ; i < nrVars() + nrFactors(); i++ )
        _beliefsF.push_back( bs[i] );
    _logZ = logZ;
}


void CBP::construct() {
    _beliefsV.clear();
    _beliefsV.reserve(nrVars());
    for( size_t i = 0; i < nrVars(); i++ )
        _beliefsV.push_back( Factor(var(i)).normalized() );

    _beliefsF.clear();
    _beliefsF.reserve(nrFactors());
    for( size_t I = 0; I < nrFactors(); I++ ) {
        Factor f = factor(I);
        f.fill(1); f.normalize();
        _beliefsF.push_back( f );
    }

    // to compute average level
    _sum_level = 0;
    _num_leaves = 0;

    _maxdiff = 0;
    _iters = 0;

    if( props.clamp_outfile.length() > 0 ) {
        _clamp_ofstream = shared_ptr<ofstream>(new ofstream( props.clamp_outfile.c_str(), ios_base::out|ios_base::trunc ));
        *_clamp_ofstream << "# COUNT LEVEL VAR STATE" << endl;
    }
}


/// Calculates a vector of mixtures p * b + (1-p) * c
static vector<Factor> mixBeliefs( Real p, const vector<Factor> &b, const vector<Factor> &c ) {
    vector<Factor> out;
    DAI_ASSERT( b.size() == c.size() );
    out.reserve( b.size() );
    Real pc = 1 - p;
    for( size_t i = 0; i < b.size(); i++ )
        // probably already normalized, but do it again just in case
        out.push_back( b[i].normalized() * p + c[i].normalized() * pc );
    return out;
}


Real CBP::run() {
    size_t seed = props.rand_seed;
    if( seed > 0 )
        rnd_seed( seed );

    InfAlg *bp = getInfAlg();
    bp->init();
    bp->run();
    _iters += bp->Iterations();

    vector<Factor> beliefs_out;
    Real lz_out;
    size_t choose_count=0;
    runRecurse( bp, bp->logZ(), vector<size_t>(0), _num_leaves, choose_count, _sum_level, lz_out, beliefs_out );
    if( props.verbose >= 1 )
        cerr << "CBP average levels = " << (_sum_level / _num_leaves) << ", leaves = " << _num_leaves << endl;
    setBeliefs( beliefs_out, lz_out );
    return 0.0;
}


InfAlg* CBP::getInfAlg() {
    PropertySet bpProps;
    bpProps.Set("updates", props.updates);
    bpProps.Set("tol", props.tol);
    bpProps.Set("maxiter", props.maxiter);
    bpProps.Set("verbose", props.verbose);
    bpProps.Set("logdomain", false);
    bpProps.Set("damping", (Real)0.0);
    BP *bp = new BP( *this, bpProps );
    bp->recordSentMessages = true;
    bp->init();
    return bp;
}


void CBP::runRecurse( InfAlg *bp, Real orig_logZ, vector<size_t> clamped_vars_list, size_t &num_leaves,
                      size_t &choose_count, Real &sum_level, Real &lz_out, vector<Factor>& beliefs_out) {
    // choose a variable/states to clamp:
    size_t i;
    vector<size_t> xis;
    Real maxVar = 0.0;
    bool found;
    bool clampingVar = (props.clamp == Properties::ClampType::CLAMP_VAR);

    if( props.recursion == Properties::RecurseType::REC_LOGZ && props.rec_tol > 0 && exp( bp->logZ() - orig_logZ ) < props.rec_tol )
        found = false;
    else
        found = chooseNextClampVar( bp, clamped_vars_list, i, xis, &maxVar );

    if( !found ) {
        num_leaves++;
        sum_level += clamped_vars_list.size();
        beliefs_out = bp->beliefs();
        lz_out = bp->logZ();
        return;
    }

    choose_count++;
    if( props.clamp_outfile.length() > 0 )
        *_clamp_ofstream << choose_count << "\t" << clamped_vars_list.size() << "\t" << i << "\t" << xis[0] << endl;

    if( clampingVar )
        daiforeach( size_t xi, xis )
            DAI_ASSERT(/*0<=xi &&*/ xi < var(i).states() );
    else
        daiforeach( size_t xI, xis )
            DAI_ASSERT(/*0<=xI &&*/ xI < factor(i).states() );
    // - otherwise, clamp and recurse, saving margin estimates for each
    // clamp setting. afterwards, combine estimates.

    // compute complement of 'xis'
    vector<size_t> cmp_xis = complement( xis, clampingVar ? var(i).states() : factor(i).states() );

    /// \idea dai::CBP::runRecurse() could be implemented more efficiently with a nesting version of backupFactors/restoreFactors
    // this improvement could also be done locally: backup the clamped factor in a local variable,
    // and restore it just before we return.
    Real lz;
    vector<Factor> b;
    InfAlg *bp_c = bp->clone();
    if( clampingVar ) {
        bp_c->fg().clampVar( i, xis );
        bp_c->init( var(i) );
    } else {
        bp_c->fg().clampFactor( i, xis );
        bp_c->init( factor(i).vars() );
    }
    bp_c->run();
    _iters += bp_c->Iterations();

    lz = bp_c->logZ();
    b = bp_c->beliefs();

    Real cmp_lz;
    vector<Factor> cmp_b;
    InfAlg *cmp_bp_c = bp->clone();
    if( clampingVar ) {
        cmp_bp_c->fg().clampVar( i, cmp_xis );
        cmp_bp_c->init(var(i));
    } else {
        cmp_bp_c->fg().clampFactor( i, cmp_xis );
        cmp_bp_c->init( factor(i).vars() );
    }
    cmp_bp_c->run();
    _iters += cmp_bp_c->Iterations();

    cmp_lz = cmp_bp_c->logZ();
    cmp_b = cmp_bp_c->beliefs();

    Real p = unSoftMax( lz, cmp_lz );
    Real bp__d = 0.0;

    if( props.recursion == Properties::RecurseType::REC_BDIFF && props.rec_tol > 0 ) {
        vector<Factor> combined_b( mixBeliefs( p, b, cmp_b ) );
        Real new_lz = logSumExp( lz,cmp_lz );
        bp__d = dist( bp->beliefs(), combined_b, nrVars() );
        if( exp( new_lz - orig_logZ) * bp__d < props.rec_tol ) {
            num_leaves++;
            sum_level += clamped_vars_list.size();
            beliefs_out = combined_b;
            lz_out = new_lz;
            return;
        }
    }

    // either we are not doing REC_BDIFF or the distance was large
    // enough to recurse:
    runRecurse( bp_c, orig_logZ, clamped_vars_list, num_leaves, choose_count, sum_level, lz, b );
    runRecurse( cmp_bp_c, orig_logZ, clamped_vars_list, num_leaves, choose_count, sum_level, cmp_lz, cmp_b );

    p = unSoftMax( lz, cmp_lz );

    beliefs_out = mixBeliefs( p, b, cmp_b );
    lz_out = logSumExp( lz, cmp_lz );

    if( props.verbose >= 2 ) {
        Real d = dist( bp->beliefs(), beliefs_out, nrVars() );
        cerr << "Distance (clamping " << i << "): " << d;
        if( props.recursion == Properties::RecurseType::REC_BDIFF )
            cerr << "; bp_dual predicted " << bp__d;
        cerr << "; max_adjoint = " << maxVar << "; logZ = " << lz_out << " (in " << bp->logZ() << ") (orig " << orig_logZ << "); p = " << p << "; level = " << clamped_vars_list.size() << endl;
    }

    delete bp_c;
    delete cmp_bp_c;
}


// 'xis' must be sorted
bool CBP::chooseNextClampVar( InfAlg *bp, vector<size_t> &clamped_vars_list, size_t &i, vector<size_t> &xis, Real *maxVarOut ) {
    Real tiny = 1.0e-14;
    if( props.verbose >= 3 )
        cerr << "clamped_vars_list" << clamped_vars_list << endl;
    if( clamped_vars_list.size() >= props.max_levels )
        return false;
    if( props.choose == Properties::ChooseMethodType::CHOOSE_RANDOM ) {
        if( props.clamp == Properties::ClampType::CLAMP_VAR ) {
            int t = 0, t1 = 100;
            do {
                i = rnd( nrVars() );
                t++;
            } while( abs( bp->beliefV(i).p().max() - 1) < tiny && t < t1 );
            if( t == t1 ) {
                return false;
                // die("Too many levels requested in CBP");
            }
            // only pick probable values for variable
            size_t xi;
            do {
                xi = rnd( var(i).states() );
                t++;
            } while( bp->beliefV(i).p()[xi] < tiny && t < t1 );
            DAI_ASSERT( t < t1 );
            xis.resize( 1, xi );
            // DAI_ASSERT(!_clamped_vars.count(i)); // not true for >2-ary variables
            DAI_IFVERB(2, "CHOOSE_RANDOM at level " << clamped_vars_list.size() << " chose variable " << i << " state " << xis[0] << endl);
        } else {
            int t = 0, t1 = 100;
            do {
                i = rnd( nrFactors() );
                t++;
            } while( abs( bp->beliefF(i).p().max() - 1) < tiny && t < t1 );
            if( t == t1 )
                return false;
                // die("Too many levels requested in CBP");
            // only pick probable values for variable
            size_t xi;
            do {
                xi = rnd( factor(i).states() );
                t++;
            } while( bp->beliefF(i).p()[xi] < tiny && t < t1 );
            DAI_ASSERT( t < t1 );
            xis.resize( 1, xi );
            // DAI_ASSERT(!_clamped_vars.count(i)); // not true for >2-ary variables
            DAI_IFVERB(2, endl<<"CHOOSE_RANDOM chose factor "<<i<<" state "<<xis[0]<<endl);
        }
    } else if( props.choose == Properties::ChooseMethodType::CHOOSE_MAXENT ) {
        if( props.clamp == Properties::ClampType::CLAMP_VAR ) {
            Real max_ent = -1.0;
            int w