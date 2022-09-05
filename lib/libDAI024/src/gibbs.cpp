/*  This file is part of libDAI - http://www.libdai.org/
 *
 *  libDAI is licensed under the terms of the GNU General Public License version
 *  2, or (at your option) any later version. libDAI is distributed without any
 *  warranty. See the file COPYING for more details.
 *
 *  Copyright (C) 2008  Frederik Eaton  [frederik at ofb dot net]
 *  Copyright (C) 2008-2010  Joris Mooij  [joris dot mooij at libdai dot org]
 */


#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <algorithm>
#include <dai/gibbs.h>
#include <dai/util.h>
#include <dai/properties.h>


namespace dai {


using namespace std;


const char *Gibbs::Name = "GIBBS";


vo