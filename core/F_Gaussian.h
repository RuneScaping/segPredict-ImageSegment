
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

#ifndef F_GAUSSIAN_H
#define F_GAUSSIAN_H

#include "Slice.h"
#include "Slice3d.h"
#include "Feature.h"

//-------------------------------------------------------------------------TYPES

//-------------------------------------------------------------------------CLASS

class F_Gaussian : public Feature
{
 public:	

  F_Gaussian();

  int getSizeFeatureVector();

  /**
   * Extract a feature vector for a given supernode in a 2d slice
   */
  bool getFeatureVectorForOneSupernode(osvm_node *x,
                                       Slice* slice,
                                       const int supernodeId);

  /**
   * Extract a feature vector for a given supernode in a 3d volume
   */
  bool getFeatureVectorForOneSupernode(osvm_node *x,
                                       Slice3d* slice3d,
                                       const int supernodeId);
};

#endif // F_GAUSSIAN_H