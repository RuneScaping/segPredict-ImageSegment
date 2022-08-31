/*  This file is part of libDAI - http://www.libdai.org/
 *
 *  libDAI is licensed under the terms of the GNU General Public License version
 *  2, or (at your option) any later version. libDAI is distributed without any
 *  warranty. See the file COPYING for more details.
 *
 *  Copyright (C) 2006-2010  Joris Mooij  [joris dot mooij at libdai dot org]
 *  Copyright (C) 2006-2007  Radboud University Nijmegen, The Netherlands
 */

#include <fstream>
#include <string>
#include <dai/alldai.h>
#include <dai/properties.h>
#include <dai/exceptions.h>

namespace dai {


using namespace std;


InfAlg *newInfAlg( const std::string &name, const FactorGraph &fg, const PropertySet &opts ) {
    if( name == ExactInf::Name )
        return new ExactInf (fg, opts);
#ifdef DAI_WITH_BP
    if( name == BP::Name )
        return new BP (fg, opts);
#endif
#ifdef DAI_WITH_FBP
    if( name == FBP::Name )
        return new FBP (fg, opts);
#endif
#ifdef DAI_WITH_TRWBP
    if( name == TRWBP::Name )
        return new TRWBP (fg, opts);
#endif
#ifdef DAI_WITH_MF
    if( name == MF::Name )
        return new MF (fg, opts);
#endif
#ifdef DAI_WITH_HAK
    if( name == HAK::Name )
        return new HAK (fg, opts);
#endif
#ifdef DAI_WITH_LC
    if( name == LC::Name )
        return new LC (fg, opts);
#endif
#ifdef DAI_WITH_TREEEP
    if( name == TreeEP::Name )
        return new TreeEP (fg, opts);
#endif
#ifdef DAI_WITH_JTREE
    if( name == JTree::Name )
        return new JTree (fg, opts);
#endif
#ifdef DAI_WITH_MR
    if( name == MR::Name )
        return new MR (fg, opts);
#endif
#ifdef DAI_WITH_GIBBS
    if( name == Gibbs::Name )
        return new Gibbs (fg, opts);
#endif
#ifdef DAI_WITH_CBP
    if( name == CBP::Name )
        return new CBP (fg, opts);
#endif
    DAI_THROWE(UNKNOWN_DAI_ALGORITHM,"Unknown libDAI algorithm: " + name);
}


InfAlg *newInfAlgFromString( const std::string &nameOpts, const FactorGraph &fg ) {
    pair<string,PropertySet> no = parseNameProperties( nameOpts );
    return newInfAlg( no.first, fg, no.second );
}


InfAlg *newInfAlgFromString( const std::string &nameOpts, const FactorGraph &fg, const std::map<std::string,std::string> &aliases ) {
    pair<string,PropertySet> no = parseNameProperties( nameOpts, aliases );
    return newInfAlg( no.first, fg, no.second );
}


std::pair<std::s