/*  This file is part of libDAI - http://www.libdai.org/
 *
 *  libDAI is licensed under the terms of the GNU General Public License version
 *  2, or (at your option) any later version. libDAI is distributed without any
 *  warranty. See the file COPYING for more details.
 *
 *  Copyright (C) 2006-2010  Joris Mooij  [joris dot mooij at libdai dot org]
 *  Copyright (C) 2006-2007  Radboud University Nijmegen, The Netherlands
 */


#include <map>
#include <dai/hak.h>
#include <dai/util.h>
#include <dai/exceptions.h>


namespace dai {


using namespace std;


const char *HAK::Name = "HAK";


/// Sets factor entries that lie between 0 and \a epsilon to \a epsilon
template <class T>
TFactor<T>& makePositive( TFactor<T> &f, T epsilon ) {
    for( size_t t = 0; t < f.states(); t++ )
        if( (0 < f[t]) && (f[t] < epsilon) )
            f[t] = epsilon;
    return f;
}

/// Sets factor entries that are smaller (in absolute value) than \a epsilon to 0
template <class T>
TFactor<T>& makeZero( TFactor<T> &f, T epsilon ) {
    for( size_t t = 0; t < f.states(); t++ )
        if( f[t] < epsilon && f[t] > -epsilon )
            f[t] = 0;
    return f;
}


void HAK::setProperties( const PropertySet &opts ) {
    DAI_ASSERT( opts.hasKey("tol") );
    DAI_ASSERT( opts.hasKey("maxiter") );
    DAI_ASSERT( opts.hasKey("verbose") );
    DAI_ASSERT( opts.hasKey("doubleloop") );
    DAI_ASSERT( opts.hasKey("clusters") );

    props.tol = opts.getStringAs<Real>("tol");
    props.maxiter = opts.getStringAs<size_t>("maxiter");
    props.verbose = opts.getStringAs<size_t>("verbose");
    props.doubleloop = opts.getStringAs<bool>("doubleloop");
    props.clusters = opts.getStringAs<Properties::ClustersType>("clusters");

    if( opts.hasKey("loopdepth") )
        props.loopdepth = opts.getStringAs<size_t>("loopdepth");
    else
        DAI_ASSERT( props.clusters != Properties::ClustersType::LOOP );
    if( opts.hasKey("damping") )
        props.damping = opts.getStringAs<Real>("damping");
    else
        props.damping = 0.0;
    if( opts.hasKey("init") )
        props.init = opts.getStringAs<Properties::InitType>("init");
    else
        props.init = Properties::InitType::UNIFORM;
}


PropertySet HAK::getProperties() const {
    PropertySet opts;
    opts.Set( "tol", props.tol );
    opts.Set( "maxiter", props.maxiter );
    opts.Set( "verbose", props.verbose );
    opts.Set( "doubleloop", props.doubleloop );
    opts.Set( "clusters", props.clusters );
    opts.Set( "init", props.init );
    opts.Set( "loopdepth", props.loopdepth );
    opts.Set( "damping", props.damping );
    return opts;
}


string HAK::printProperties() const {
    stringstream s( stringstream::out );
    s << "[";
    s << "tol=" << props.tol << ",";
    s << "maxiter=" << props.maxiter << ",";
    s << "verbose=" << props.verbose << ",";
    s << "doubleloop=" << props.doubleloop << ",";
    s << "clusters=" << props.clusters << ",";
    s << "init=" << props.init << ",";
    s << "loopdepth=" << props.loopdepth << ",";
    s << "damping=" << props.damping << "]";
    return s.str();
}


void HAK::construct() {
    // Create outer beliefs
    _Qa.clear();
    _Qa.reserve(nrORs());
    for( size_t alpha = 0; alpha < nrORs(); alpha++ )
        _Qa.push_back( Factor( OR(alpha) ) );

    // Create inner beliefs
    _Qb.clear();
    _Qb.reserve(nrIRs());
    for( size_t beta = 0; beta < nrIRs(); beta++ )
        _Qb.push_back( Factor( IR(beta) ) );

    // Create messages
    _muab.clear();
    _muab.reserve( nrORs() );
    _muba.clear();
    _muba.reserve( nrORs() );
    for( size_t alpha = 0; alpha < nrORs(); alpha++ ) {
        _muab.push_back( vector<Factor>() );
        _muba.push_back( vector<Factor>() );
        _muab[alpha].reserve( nbOR(alpha).size() );
        _muba[alpha].reserve( nbOR(alpha).size() );
        daiforeach( const Neighbor &beta, nbOR(alpha) ) {
            _muab[alpha].push_back( Factor( IR(beta) ) );
            _muba[alpha].push_back( Factor( IR(beta) ) );
        }
    }
}


HAK::HAK( const RegionGraph &rg, const PropertySe