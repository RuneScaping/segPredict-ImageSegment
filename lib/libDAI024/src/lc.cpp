/*  This file is part of libDAI - http://www.libdai.org/
 *
 *  libDAI is licensed under the terms of the GNU General Public License version
 *  2, or (at your option) any later version. libDAI is distributed without any
 *  warranty. See the file COPYING for more details.
 *
 *  Copyright (C) 2006-2010  Joris Mooij  [joris dot mooij at libdai dot org]
 *  Copyright (C) 2006-2007  Radboud University Nijmegen, The Netherlands
 */


#include <iostream>
#include <algorithm>
#include <map>
#include <set>
#include <dai/lc.h>
#include <dai/util.h>
#include <dai/alldai.h>


namespace dai {


using namespace std;


const char *LC::Name = "LC";


void LC::setProperties( const PropertySet &opts ) {
    DAI_ASSERT( opts.hasKey("tol") );
    DAI_ASSERT( opts.hasKey("maxiter") );
    DAI_ASSERT( opts.hasKey("verbose") );
    DAI_ASSERT( opts.hasKey("cavity") );
    DAI_ASSERT( opts.hasKey("updates") );

    props.tol = opts.getStringAs<Real>("tol");
    props.maxiter = opts.getStringAs<size_t>("maxiter");
    props.verbose = opts.getStringAs<size_t>("verbose");
    props.cavity = opts.getStringAs<Properties::CavityType>("cavity");
    props.updates = opts.getStringAs<Properties::UpdateType>("updates");
    if( opts.hasKey("cavainame") )
        props.cavainame = opts.getStringAs<string>("cavainame");
    if( opts.hasKey("cavaiopts") )
        props.cavaiopts = opts.getStringAs<PropertySet>("cavaiopts");
    if( opts.hasKey("reinit") )
        props.reinit = opts.getStringAs<bool>("reinit");
    if( opts.hasKey("damping") )
        props.damping = opts.getStringAs<Real>("damping");
    else
        props.damping = 0.0;
}


PropertySet LC::getProperties() const {
    PropertySet opts;
    opts.Set( "tol", props.tol );
    opts.Set( "maxiter", props.maxiter );
    opts.Set( "verbose", props.verbose );
    opts.Set( "cavity", props.cavity );
    opts.Set( "updates", props.updates );
    opts.Set( "cavainame", props.cavainame );
    opts.Set( "cavaiopts", props.cavaiopts );
    opts.Set( "reinit", props.reinit );
    opts.Set( "damping", props.damping );
    return opts;
}


string LC::printProperties() const {
    stringstream s( stringstream::out );
    s << "[";
    s << "tol=" << props.tol << ",";
    s << "maxiter=" << props.maxiter << ",";
    s << "verbose=" << props.verbose << ",";
    s << "cavity=" << props.cavity << ",";
    s << "updates=" << props.updates << ",";
    s << "cavainame=" << props.cavainame << ",";
    s << "cavaiopts=" << props.cavaiopts << ",";
    s << "reinit=" << props.reinit << ",";
    s << "damping=" << props.damping << "]";
    return s.str();
}


LC::LC( const FactorGraph & fg, const PropertySet &opts ) : DAIAlgFG(fg), _pancakes(), _cavitydists(), _phis(), _beliefs(), _maxdiff(0.0), _iters(0), props() {
    setProperties( opts );

    // create pancakes
    _pancakes.resize( nrVars() );

    // create cavitydists
    for( size_t i=0; i < nrVars(); i++ )
        _cavitydists.push_back(Factor( delta(i) ));

    // create phis
    _phis.reserve( nrVars() );
    for( size_t i = 0; i < nrVars(); i++ ) {
        _phis.push_back( vector<Factor>() );
        _phis[i].reserve( nbV(i).size() );
        daiforeach( const Neighbor &I, nbV(i) )
            _phis[i].push_back( Factor( factor(I).vars() / var(i) ) );
    }

    // create beliefs
    _beliefs.reserve( nrVars() );
    for( size_t i=0; i < nrVars(); i++ )
        _beliefs.push_back(Factor(var(i)));
}


string LC::identify() const {
    return string(Name) + printProperties();
}


void LC::CalcBelief (size_t i) {
    _beliefs[i] = _pancakes[i].marginal(var(i));
}


Real LC::CalcCavityDist (size_t i, con