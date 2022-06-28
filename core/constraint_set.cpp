/////////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or       //
// modify it under the terms of the GNU General Public License         //
// version 2 as published by the Free Software Foundation.             //
//                                                                     //
// This program is distributed in the hope that it will be useful, but //
// WITHOUT ANY WARRANTY; without even the implied warranty of          //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   //
// General Public License for more details.                            //
//                                                                     //
// Written and (C) by Aurelien Lucchi                                  //
// Contact <aurelien.lucchi@gmail.com> for comments & bug reports      //
/////////////////////////////////////////////////////////////////////////

#include "constraint_set.h"
#include "Config.h"
#include "svm_struct_globals.h" // for SSVM_PRINT

ConstraintSet* ConstraintSet::pInstance = 0; // initialize pointer

ConstraintSet::ConstraintSet()
{
  max_number_constraints = CONSTRAINT_SET_DEFAULT_SIZE;
  sortingType = CS_DISTANCE;
  string config_tmp;
  if(Config::Instance()->getParameter("cs_max_number_constraints", config_tmp)) {
    max_number_constraints = atoi(config_tmp.c_str());
    printf("[ConstraintSet] max_number_constraints = %ld\n", max_number_constraints);
  }
}

ConstraintSet::~ConstraintSet()
{
  clear();
}

void ConstraintSet::clear()
{
  for(map<cs_id_type, vector< constraint >* >::iterator itC = constraints.begin();
      itC != constraints.end(); ++itC) {
    for(vector<constraint>::iterator it = itC->second->begin();
        it != itC->second->end(); ++it) {
      delete[] it->first->w;
      delete[] it->first;
    }
    delete itC->second;
  }
  constraints.clear();
}

void ConstraintSet::getConstraints(vector< constraint >& all_cs)
{
  //todo
  for(map<cs_id_type, vector< constraint >* >::iterator itC = constraints.begin();
      itC != constraints.end(); ++itC) {
    for(vector<constraint>::iterator it = itC->second->begin();
        it != itC->second->end(); ++it) {
      all_cs.push_back(*it);
    }
  }
}

const vector< constraint >* ConstraintSet::getConstraints(cs_id_type id)
{
  if(constraints.find(id) != constraints.end()) {
    return constraints[id];
  } else {
    return 0;
  }
}

vector< constraint >* ConstraintSet::getMutableConstraints(cs_id_type id)
{
  assert(constraints.find(id) != constraints.end());
  return constraints[id];
}

const constraint* ConstraintSet::getMostViolatedConstraint(cs_id_type id, double* w, int* max_index)
{
  constraint* c = 