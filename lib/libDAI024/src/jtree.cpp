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
#include <stack>
#include <dai/jtree.h>


namespace dai {


using namespace std;


const char *JTree::Name = "JTREE";


void JTree::setProperties( const PropertySet &opts ) {
    DAI_ASSERT( opts.hasKey("verbose") );
    DAI_ASSERT( opts.hasKey("updates") );

    props.verbose = opts.getStringAs<size_t>("verbose");
    props.updates = opts.getStringAs<Properties::UpdateType>("updates");
    if( opts.hasKey("inference") )
        props.inference = opts.getStringAs<Properties::InfType>("inference");
    else
        props.inference = Properties::InfType::SUMPROD;
    if( opts.hasKey("heuristic") )
        props.heuristic = opts.getStringAs<Properties::HeuristicType>("heuristic");
    else
        props.heuristic = Properties::HeuristicType::MINFILL;
}


PropertySet JTree::getProperties() const {
    PropertySet opts;
    opts.Set( "verbose", props.verbose );
    opts.Set( "updates", props.updates );
    opts.Set( "inference", props.inference );
    opts.Set( "heuristic", props.heuristic );
    return opts;
}


string JTree::printProperties() const {
    stringstream s( stringstream::out );
    s << "[";
    s << "verbose=" << props.verbose << ",";
    s << "updates=" << props.updates << ",";
    s << "heuristic=" << props.heuristic << ",";
    s << "inference=" << props.inference << "]";
    return s.str();
}


JTree::JTree( const FactorGraph &fg, const PropertySet &opts, bool automatic ) : DAIAlgRG(fg), _mes(), _logZ(), RTree(), Qa(), Qb(), props() {
    setProperties( opts );

    if( !isConnected() )
       DAI_THROW(FACTORGRAPH_NOT_CONNECTED);

    if( automatic ) {
        // Create ClusterGraph which contains factors as clusters
        vector<VarSet> cl;
        cl.reserve( fg.nrFactors() );
        for( size_t I = 0; I < nrFactors(); I++ )
            cl.push_back( factor(I).vars() );
        ClusterGraph _cg( cl );

        if( props.verbose >= 3 )
            cerr << "Initial clusters: " << _cg << endl;

        // Retain only maximal clusters
        _cg.eraseNonMaximal();
        if( props.verbose >= 3 )
            cerr << "Maximal clusters: " << _cg << endl;

        // Use heuristic to guess optimal elimination sequence
        greedyVariableElimination::eliminationCostFunction ec(NULL);
        switch( (size_t)props.heuristic ) {
            case Properties::HeuristicType::MINNEIGHBORS:
                ec = eliminationCost_MinNeighbors;
                break;
            case Properties::HeuristicType::MINWEIGHT:
                ec = eliminationCost_MinWeight;
                break;
            case Properties::HeuristicType::MINFILL:
                ec = eliminationCost_MinFill;
                break;
            case Properties::HeuristicType::WEIGHTEDMINFILL:
                ec = eliminationCost_WeightedMinFill;
                break;
            default:
                DAI_THROW(UNKNOWN_ENUM_VALUE);
        }
        vector<VarSet> ElimVec = _cg.VarElim( greedyVariableElimination( ec ) ).eraseNonMaximal().toVector();
        if( props.verbose >= 3 )
            cerr << "VarElim result: " << ElimVec << endl;

        // Generate the junction tree corresponding to the elimination sequence
        GenerateJT( ElimVec );
    }
}


void JTree::construct( const std::vector<VarSet> &cl, bool verify ) {
    // Construct a weighted graph (each edge is weighted with the cardinality
    // of the intersection of the nodes, where the nodes are the elements of cl).
    WeightedGraph<int> JuncGraph;
    for( size_t i = 0; i < cl.size(); i++ )
        for( size_t j = i+1; j < cl.size(); j++ ) {
            size_t w = (cl[i] & cl[j]).size();
            if( w )
                JuncGraph[UEdge(i,j)] = w;
        }

    // Construct maximal spanning tree using Prim's algorithm
    RTree = MaxSpanningTreePrims( JuncGraph );

    // Construct corresponding region graph

    // Create outer regions
    ORs.clear();
    ORs.reserve( cl.size() );
    for( size_t i = 0; i < cl.size(); i++ )
        ORs.push_back( FRegion( Factor(cl[i], 1.0), 1.0 ) );

    // For each factor, find an outer region that subsumes that factor.
    // Then, multiply the outer region with that factor.
    fac2OR.clear();
    fac2OR.resize( nrFactors(), -1U );
    for( size_t I =