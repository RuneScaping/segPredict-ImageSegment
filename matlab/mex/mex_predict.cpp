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
// Contact aurelien.lucchi (at) gmail.com                              // 
// for comments & bug reports                                          //
/////////////////////////////////////////////////////////////////////////

#include <mex.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

// SliceMe
#include <Config.h>
#include <Feature.h>
#include <Slice.h>

#include <energyParam.h>
#include <graphInference.h>
#include <inference.h>
#include <svm_struct_api.h>

#include <globals.h>


void mexFunction(int nlhs,       mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
  char *pParamFilename;
  uc