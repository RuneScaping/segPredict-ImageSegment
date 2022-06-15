////////////////////////////////////////////////////////////////////////
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

#include "Slice.h"
#include "utils.h"

#include "LKM.h"

// standard libraries
#include <limits.h>
#include <sstream>
#include <time.h>

//------------------------------------------------------------------------------

#define USE_UNDIRECTED_GRAPH 0

//------------------------------------------------------------------------------

void Slice::init(int width, int height)
{
  PRINT_MESSAGE("[Slice] Initializing slice. width %d height %d\n",width,height);
  img_width = width;
  img_height = height;
  min_sid = INT_MAX; //numeric_limits<int>::max();

  // Populating mSupernodes
  map<sidType, supernode* >::iterator iLabel;
  int iBuffer = 0;
  sidType sid;  // supernode id
  for(int y=0;y<img_height;y++) {
    for(int x=0;x<img_width;x++) {
      node* p = new node;
      p->x = x;
      p->y = y;
      p->z = 0;
      
      sid = pixelLabels[iBuffer];
      iLabel = mSupernodes.find(sid);
      if(iLabel != mSupernodes.end()) {
        iLabel->second->addNode(p);
      } else {
        // Create a new supernode
        supernode* s = new supernode;
        s->id = sid;
        s->addNode(p);
        mSupernodes[sid] = s;

        if(s->id < min_sid)
          min_sid = s->id;
      }

      iBuffer++;
    }
  }

  if(min_sid > 0) {
    printf("[Slice] WARNING : min_sid equals %d. Should be 0 ?\n", min_sid);
  }

  // initialize random seed
  srand(time(0));

  // edges are set by loadNeighborhoodMap
  nbEdges = 0;
  avgIntensity = -1;
  supernodeLabelsLoaded = false;
  neighborhoodMapLoaded = false;

  minPercentToAssignLabel = MIN_PERCENT_TO_ASSIGN_LABEL;
  includeOtherLabel = minPercentToAssignLabel != 0;

  // load a color image as well
  generateColorImage();
}

Slice::Slice(const char* fn_label, int awidth, int aheight)
{
  img = 0;
  colorImg = 0;

  // Loading file containing a superpixel label for each pixel
  int bufferSize = awidth*aheight;
  pixelLabels = new sidType[bufferSize];
  ifstream ifslb(fn_label, ios::in | ios::binary);
  if(ifslb.fail()) {
    printf("[Slice] Error while loading %s\n",fn_label);
    return;
  }
  ifslb.read((char*)pixelLabels, bufferSize*sizeof(sidType));
  ifslb.close();

  init(awidth,aheight);
}

Slice::Slice(const char* a_image_name,
             const char* fn_label,
             int superpixelStepSize,
             float M)
{
  initSuperpixels(a_image_name, fn_label, superpixelStepSize, M, true);
}

Slice::Slice(const char* a_image_name, const char* fn_label,
             int superpixelStepSize, float M, bool bGenerateNeighborhoodMap)
{
  initSuperpixels(a_image_name, fn_label, superpixelStepSize, M,
                  bGenerateNeighborhoodMap);
}

void Slice::initSuperpixels(const char* a_image_name, const char* fn_label,
                            int superpixelStepSize, float M,
                            bool bGenerateNeighborhoodMap)
{
  colorImg = 0;
  supernode_step = superpixelStepSize;
  cubeness = M;

  bool superpixelLoaded = false;
  if(fn_label != 0 && fileExists(fn_label)) {
    image_name = (string)a_image_name;
    img = cvLoadImage(a_image_name,CV_LOAD_IMAGE_COLOR);
    eraseImage = true;
    if(!img) {
      printf("[Slice] Error : input image %s was not found\n", a_image_name);
      return;
    }

    // Loading file containing a superpixel label for each pixel
    int bufferSize = img->width*img->height;
    pixelLabels = new sidType[bufferSize];

    string labelFilename;
    if(isDirectory(fn_label)) {
      labelFilename = fn_label;
      labelFilename += "/";
      labelFilename += getNameFromPathWithoutExtension(a_image_name);
      labelFilename += ".dat";
    } else {
      labelFilename = fn_label;
    }

    PRINT_MESSAGE("[Slice] Loading %s\n", labelFilename.c_str());
    ifstream ifslb(labelFilename.c_str(), ios::in | ios::binary);
    if(ifslb.fail()) {
      printf("[Slice] Error while loading %s\n", labelFilename.c_str());
    } else {
      string imageNameWithoutPath = getNameFromPath(string(a_image_name));
      ifslb.read((char*)pixelLabels, bufferSize*sizeof(sidType));
      ifslb.close();

      init(img->width,img->height);

      if(bGenerateNeighborhoodMap) {
        generateNeighborhoodMap(pixelLabels, img->width, img->height);
        neighborhoodMapLoaded = true;
      }
      superpixelLoaded = true;
    }
  }
  
  if (!superpixelLoaded) {
      generateSuperpixels(a_image_name, superpixelStepSize, M);

      if(bGenerateNeighborhoodMap) {
        generateNeighborhoodMap(pixelLabels, img_width, img_height);
        neighborhoodMapLoaded = true;
      }
    }
}

Slice::Slice(IplImage* _img, const char* fn_label, int superpixelStepSize, float M)
{
  colorImg = 0;
  image_name = "";
  img = _img;
  eraseImage = false;
  supernode_step = superpixelStepSize;
  cubeness = M;

  if(fn_label != 0 && fileExists(fn_label)) {
    // Loading file containing a superpixel label for each pixel
    int bufferSize = img->width*img->height;
    pixelLabels = new sidType[bufferSize];
    ifstream ifslb(fn_label, ios::in | ios::binary);
    if(ifslb.fail()) {
      printf("[Slice] Error while loading %s\n",fn_label);
      return;
    }
    ifslb.read((char*)pixelLabels, bufferSize*sizeof(sidType));
    ifslb.close();
    init(img->width,img->height);
  } else {
    generateSuperpixels(superpixelStepSize, M);
  }

  generateNeighborhoodMap(pixelLabels, img_width, img_height);
  neighborhoodMapLoaded = true;
}

Slice::Slice(const char* a_img_name, int superpixelStepSize, float M)
{
  colorImg = 0;
  eraseImage = false;
  supernode_step = superpixelStepSize;
  cubeness = M;
  generateSuperpixels(a_img_name, superpixelStepSize, M);
  generateNeighborhoodMap(pixelLabels, img_width, img_height);
  neighborhoodMapLoaded = true;
}

void Slice::generateSuperpixels(const char* a_img_name, int superpixelStepSize, float M)
{
  image_name = (string)a_img_name;
  img = cvLoadImage(a_img_name,CV_LOAD_IMAGE_COLOR);
  eraseImage = true;
  if(!img) {
    printf("[Slice] Error : input image %s was not found\n", a_img_name);
    return;
  }
  generateSuperpixels(superpixelStepSize, M);
}

void Slice::generateSuperpixels(int superpixelStepSize, float M)
{
  img_width = img->width;
  img_height = img->height;
  supernode_step = superpixelStepSize;
  cubeness = M;

  switch(cubeness) {
  case SUPERPIXEL_IMAGE:
    {
      uint img_size = img_width*img_height;
      pixelLabels = new sidType[img_size];
      for(int i =0; i < img_size; ++i) {
        pixelLabels[i] = 0;
      }
      init(img->width,img->height);
    }    
    break;
  case SUPERPIXEL_CUBE:
    {
      // create cubes
      pixelLabels = new sidType[img_width*img_height];
      int sid = 0;
      for(int x = 0; x < img_width; x += superpixelStepSize) {
        for(int y = 0; y< img_height; y += superpixelStepSize) {
          for(int sx=x;sx<min(img_width,(sizeSliceType)x+superpixelStepSize);sx++) {
            for(int sy=y;sy<min(img_height,(sizeSliceType)y+superpixelStepSize);sy++) {
              pixelLabels[sy*img_width+sx] = sid;
            }
          }
          sid++;
        }
      }
      init(img->width,img->height);

    }
    break;
  default:
    {
      int n = img->height*img->width;
      uint* ubuff