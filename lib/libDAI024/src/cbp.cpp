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


const char *CBP::Name = "