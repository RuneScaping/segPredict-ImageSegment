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

#include "graphInference.h"

#include "globalsE.h"
#include "Config.h"

//------------------------------------------------------------------------------

map<ulong, labelType> GraphInference::classIdxToLabel;

//------------------------------------------------------------------------------

GraphInference::GraphInference(Slice_P* _slice,
                               const EnergyParam* _param,
                               double* _smw,
                               Feature* _feature,
                               map<sidType, nodeCoeffType>* _nodeCoeffs,
                               map<sidType, edgeCoeffType>* _edgeCoeffs
                               )
{
  init();

  slice = _slice;
  param = _param;
  smw = _smw;
  feature = _feature;
  nodeCoeffs = _nodeCoeffs;
  edgeCoeffs = _edgeCoeffs;
}

GraphInference::~GraphInference()
{
}

void GraphInference::init()
{
  nodeCoeffs = 0;
  edgeCoeffs = 0;
  lossPerLabel = 0;
}

/**
 * Compute energy for a given configuration nodeLabels
 */
double GraphInference::computeEnergy(labelType* nodeLabels)
{
  // Compute energy
  double energyU = 0.0;
  double energyP = 0.0;
  double loss = 0.0;
  int nDiff = 0;

  int fvSize = feature->getSizeFeatureVector();
  int label = 0;
  int nSupernodes = slice->getNbSupernodes();
  double energy