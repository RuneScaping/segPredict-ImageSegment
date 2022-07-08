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

#ifndef GRAPH_INFERENCE_H
#define GRAPH_INFERENCE_H

#include <stdio.h>

#include "Feature.h"
#include "Slice_P.h"
#include "globalsE.h"
#include "oSVM.h"

#include "inference_globals.h"
#include "energyParam.h"

//------------------------------------------------------------------------------

#define T_GI_LIBDAI 0
#define T_GI_MRF 1
#define T_GI_QPBO 2
//#define T_GI_FASTPD 3
#define T_GI_MAX 4
#define T_GI_MAXFLOW 5
#define T_GI_OPENGM 6
#define T_GI_SAMPLING 7
#define T_GI_ICM 8
#define T_GI_LIBDAI_ICM 9
#define T_GI_LIBDAI_ICM_QPBO 10
#define T_GI_MF 11
#define T_GI_MULTIOBJ 12

//------------------------------------------------------------------------------

class GraphInference
{
 public:

  GraphInference() {}
  GraphInference(Slice_P* _slice, 
                 const EnergyParam* _param,
                 double* _smw,
                 Feature* _feature,
                 map<sidType, nodeCoeffType>* _nodeCoeffs,
                 map<sidType, edgeCoeffType>* _edgeCoeffs
                 );

  virtual ~GraphInference();

  virtual void addLocalEdges() { return; }

  virtual void addUnaryNodes(labelType* globalNodeLabels = 0) { return; }

  /**
   * Compute energy for a given configuration nodeLabels
   */
  double computeEnergy(labelType* nodeLabels);

  void computeNodePotentials(double**& unaryPotentials, double& maxPotential);

  void init();

  virtual double run(labelType* inferredLabels,
                     int id,
                     size_t maxiter,
                     labelType* nodeLabelsGroundTruth = 0,
                     bool computeEnergyAtEachIteration = false,
                     double* _loss = 0) { assert(0); return 0; }

  static void setColormap(map<ulong, labelType>& _classIdxToLabel) { classIdxToLabel = _classIdxToLabel;}

  Slice_P* slice;
  double* smw;

  const EnergyParam* param;
  double* lossPerLabel;
  labelType* groundTruthLabels;   /* ground truth labels */
  Feature* feature;

  // compute pairwise potential for 2 given nodes s and sn
  inline double computePairwisePotential(Slice_P* slice, supernode* s,
                                         supernode* sn,
                                         labelType s_label,
                                         labelType sn_label);

  // compute pairwise potential for 2 given nodes s and sn
  // distance adaptive pairwise term
  inline double computePairwisePotential_distance(Slice_P* slice, supernode* s,
                                                  supernode* sn,
                                                  labelType s_label,
                                                  labelType sn_label);

  /**
   * Assume features were precomputed with the function precomputeFeatures
   */
  inline double computeUnary