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


// standard libraries
#include <sstream>
#include <time.h>

// Third-party libraries
#include "LKM.h"

// SliceMe
#include "Slice3d.h"
#include "globalsE.h"
#include "utils.h"

#define USE_RUN_LENGTH_ENCODING

Slice3d::Slice3d(unsigned char* a_raw_data,
                 int awidth, int aheight,
                 int adepth,
                 sizeSliceType vstep,
                 int anChannels,
                 bool _loadNeighbors)
{
  init();
  delete_raw_data = false; // caller is responsible for freeing memory
  width = awidth;
  height = aheight;
  depth = adepth;
  raw_data = a_raw_data;
  nChannels = anChannels;
  loadNeighbors = _loadNeighbors;

  if(vstep > depth) {
    supernode_step = depth;
  } else {
    supernode_step = vstep;
  }

  sliceSize = width*height;
}

Slice3d::Slice3d(const char* input_dir,
                 int vstep,
                 int nImgs,
                 bool _loadNeighbors)
{
  width = UNITIALIZED_SIZE;
  height = UNITIALIZED_SIZE;
  depth = UNITIALIZED_SIZE;
  nChannels = 1;
  loadNeighbors = _loadNeighbors;

  inputDir = string(input_dir);
  init();
  loadFromDir(input_dir, nImgs);

  sliceSize = width*height;

  if(vstep > depth) {
    supernode_step = depth;
  } else {
    supernode_step = vstep;
  }
}

Slice3d::Slice3d(const char* input_dir,
                 int awidth,
                 int aheight,
                 int adepth,
                 int vstep,
                 bool _loadNeighbors)
{
  init();
  width = awidth;
  height = aheight;
  depth = adepth;
  nChannels = 1;
  loadNeighbors = _loadNeighbors;

  inputDir = string(input_dir);
  loadFromDir(input_dir, depth);

  sliceSize = width*height;

  if(vstep > depth) {
    supernode_step = depth;
  } else {
    supernode_step = vstep;
  }
}

Slice3d::Slice3d(const char* input_dir,
                 const node& _start,
                 const node& _end,
                 int vstep,
                 bool _loadNeighbors)
{
  init();
  width = _end.x - _start.x;
  height = _end.y - _start.y;
  depth = _end.z - _start.z;
  start_x = _start.x;
  start_y = _start.y;
  start_z = _start.z;
  nChannels = 1;
  loadNeighbors = _loadNeighbors;

  inputDir = string(input_dir);
  loadFromDir(input_dir, depth);

  sliceSize = width*height;

  if(vstep > depth) {
    supernode_step = depth;
  } else {
    supernode_step = vstep;
  }
}

Slice3d::Slice3d() {
  init();
}

void Slice3d::init()
{
  mSupervoxels = 0;
  nbEdges = 0;
  supernodeLabelsLoaded = false;
  maxDegree = -1;
  minPercentToAssignLabel = MIN_PERCENT_TO_ASSIGN_LABEL;
  nLabels = 0;
  raw_data = 0;
  includeOtherLabel = true;
  delete_raw_data = true;
  cubeness = SUPERVOXEL_DEFAULT_CUBENESS;

  start_x = 0;
  start_y = 0;
  start_z = 0;

#ifdef USE_REVERSE_INDEXING
  klabels = 0;
#endif
}

Slice3d::~Slice3d()
{
  if(mSupervoxels) {
    for(map< sidType, supernode* >::iterator it = mSupervoxels->begin();
        it != mSupervoxels->end();it++) {
      delete it->second;
    }
    delete mSupervoxels;
  }
  
  if(delete_raw_data && raw_data) {
    delete[] raw_data;
  }
}

uchar Slice3d::at(int x, int y, int z)
{
  return raw_data[z*sliceSize+y*width+x];
}

int Slice3d::raw2Double(double**& ptr_data)
{
  // supervoxel library needs a cube made of ints so we have to convert the cube
  // ask for enough memory for the texels and make sure we got it before proceeding
  ptr_data = new double* [depth];
  if (ptr_data == 0) {
    printf("[Slice3d] Error while allocating memory for 3d volume\n");
    return -1;
  }

  int i = 0;
  for(int z = 0;z < depth; z++) {
    ptr_data[z] = new double[sliceSize];
    for(int xy = 0; xy < sliceSize; xy++) {
      ptr_data[z][xy] = (double)raw_data[i];
      i++;
    }
  }
  return 0;
}

int Slice3d::raw2RGB(unsigned int**& ptr_data)
{
  // supervoxel library needs a cube made of ints so we have to convert the cube
  // ask for enough memory for the texels and make sure we got it before proceeding
  ptr_data = new unsigned int* [depth];
  if (ptr_data == 0) {
    printf("[Slice3d] Error while allocating memory for 3d volume\n");
    return -1;
  }

  int i = 0;
  char c;
  for(int z=0;z<depth;z++) {
    ptr_data[z] = new unsigned int[sliceSize];
    for(int xy = 0;xy<sliceSize;xy++) {
      c = raw_data[i];
      ptr_data[z][xy] = c | (c << 8) | (c << 16);
      i++;
    }
  }
  return 0;
}

void Slice3d::loadSupervoxels(const char* imageDir)
{
  cubeness = SUPERVOXEL_DEFAULT_CUBENESS;
  loadSupervoxels(imageDir, DEFAULT_VOXEL_STEP, SUPERVOXEL_DEFAULT_CUBENESS);
}

void Slice3d::loadSupervoxels(const char* imageDir, const int voxel_step, const float _cubeness)
{
  cubeness = _cubeness;
  supernode_step = voxel_step;
  stringstream soutSupervoxels_nrrd;
  soutSupervoxels_nrrd << imageDir << "supervoxels_" << voxel_step << "_" << cubeness << ".nrrd";
  if(fileExists(soutSupervoxels_nrrd.str().c_str())) {
    uint* outputData = 0;
    int width;
    int height;
    int depth;
    PRINT_MESSAGE("[Slice3d] Loading supervoxels from nrrd file %s\n", soutSupervoxels_nrrd.str().c_str());
#ifdef USE_ITK
    importNRRDCube_uint(soutSupervoxels_nrrd.str().c_str(), outputData, width, height, depth);
#else
    PRINT_MESSAGE("[Slice3d] Set USE_ITK to true to import NRRD cubes\n");
    exit(-1);
#endif
    importSupervoxelsFromBuffer((const uint*)outputData, width, height, depth);
    delete[] outputData;
  } else {
    stringstream soutSupervoxels;
    soutSupervoxels << imageDir << "supervoxels_" << voxel_step << "_" << cubeness;
    if(fileExists(soutSupervoxels.str().c_str())) {
      PRINT_MESSAGE("[Slice3d] Loading supervoxels from %s\n", soutSupervoxels.str().c_str());
      importSupervoxelsFromBinaryFile(soutSupervoxels.str().c_str());
    } else {
      PRINT_MESSAGE("[Slice3d] Generating supervoxels (cubeness=%d)\n", cubeness);
      generateSupervoxels(cubeness);
      if(strlen(imageDir) > 0) {
        PRINT_MESSAGE("[Slice3d] Exporting supervoxels to %s\n", soutSupervoxels.str().c_str());
        exportSupervoxelsToBinaryFile(soutSupervoxels.str().c_str());
      }
    }
  }
}

void Slice3d::generateSupervoxels(const double _cubeness)
{
  cubeness = _cubeness;
  int slice_size = width*height;

  // voxel step should not be greater than the number of slices
  if(supernode_step > depth) {
    supernode_step = depth;
  }

  // klabels is a 2d array indexed by z coordinates. Slices are ordered by yx.
#ifndef USE_REVERSE_INDEXING
  sidType** klabels;
#endif

  PRINT_MESSAGE("[Slice3d] Generating supervoxels. vol_size=(%d, %d, %d). voxel_step=%d. cubeness=%d, %fMb needed\n",
                width,height,depth,supernode_step,cubeness,slice_size*depth/(1024.0*1024.0));

  if(cubeness == -1)
    {
      // use cubical supervoxels
      PRINT_MESSAGE("[Slice3d] Uniform sampling...\n");
      klabels = new sidType*[depth];
      for(int z=0;z<depth;z++) {
        klabels[z] = new sidType[slice_size];
      }

      int sid = 0;
      for(int z=0;z<depth; z += supernode_step)
        {
          for(int x=0;x<width; x+= supernode_step)
            for(int y=0;y<height; y += supernode_step)
              {
                // do not use voxel+step
                for(int sz=z;sz<min(depth,(sizeSliceType)z+supernode_step);sz++)
                  {
                    for(int sx=x;sx<min(width,(sizeSliceType)x+supernode_step);sx++)
                      for(int sy=y;sy<min(height,(sizeSliceType)y+supernode_step);sy++)
                        klabels[sz][sy*width+sx] = sid;
                  }
                sid++;
              }
        }
      nLabels = sid;
      PRINT_MESSAGE("[Slice3d] Uniform sampling done. %d labels created\n",nLabels);
    }
  else  
    {
      double** ptr_data;
      raw2Double(ptr_data);
      LKM* lkm = new LKM(false); // do not free memory
      lkm->DoSupervoxelSegmentationForGrayVolume(ptr_data,
                                                 (int)width,(int)height,(int)depth,
                                                 klabels,
                                                 nLabels,
                                                 (int)supernode_step,
                                                 cubeness);

      PRINT_MESSAGE("[Slice3d] Supervoxelization done\n");
      for(int z=0;z<depth;z++) {
        delete[] ptr_data[z];
      }
      delete[] ptr_data;

      delete lkm;
    }

  createIndexingStructures(klabels);
}


void Slice3d::createIndexingStructures(sidType** _klabels, bool force)
{
  if(mSupervoxels !=0) {
    if(force) {
      for(map< sidType, supernode* >::iterator it = mSupervoxels->begin();
          it != mSupervoxels->end();it++) {
        delete it->second;
      }
      delete mSupervoxels;
    } else {
      printf("[Slice3d] Error in createIndexingStructures : structures already existing\n");
      return;
    }
  }

  ulong slice_size = width*height;
  // Creating indexation structure
  PRINT_MESSAGE("[Slice3d] Creating indexing structure. %fMb needed\n",
                (sizeof(supernode)*slice_size*depth/(supernode_step*supernode_step)
                 + sizeof(node) * slice_size*depth)/(1024.0*1024.0));

  mSupervoxels = new map< sidType, supernode* >;
  supernode* s;
  map<sidType,supernode*>::iterator itVoxel;

  PRINT_MESSAGE("[Slice3d] Cube size = (%d,%d,%d)=%ld voxels\n", width, height, depth,slice_size*depth);

  sidType sid;

#ifdef USE_RUN_LENGTH_ENCODING
  sidType previousSid;
  lineContainer* line;

  for(int d=0;d<depth;d++) { 
    for(int y=0;y<height;y++) {

      // create new line
      line = new lineContainer;
      line->coord.x = 0;
      line->coord.y = y;
      line->coord.z = d;
      sid = _klabels[d][y*width]; // x=0

      // add line to supernode
      itVoxel = mSupervoxels->find(sid);
      if(itVoxel == mSupervoxels->end()) {
        // Create new supernode and add it to the list
        s = new supernode;
        s->id = sid;
        (*mSupervoxels)[sid] = s;
      } else {
        // Supernode already exists
        s = itVoxel->second;
      }

      s->addLine(line);

      previousSid = sid;

      for(int x = 0;x < width;x++) {
        sid = _klabels[d][y*width+x];
        if(sid == previousSid) {
          line->length++;
        } else {
          // create new line
          line = new lineContainer;
          line->coord.x = x;
          line->coord.y = y;
          line->coord.z = d;
          line->length = 1;
          previousSid = sid;

          // add line to supernode
          itVoxel = mSupervoxels->find(sid);
          if(itVoxel == mSupervoxels->end()) {
            // Create new supernode and add it to the list
            s = new supernode;
            s->id = sid;
            (*mSupervoxels)[sid] = s;
          } else {
            // Supernode already exists
            s = itVoxel->second;
          }

          s->addLine(line);
        }
      }
    }
  }
#else
  for(int d=0;d<depth;d++)
    for(int y=0;y<height;y++)
      for(int x=0;x<width;x++) {
        sid = _klabels[d][y*width+x];
        itVoxel = mSupervoxels->find(sid);
        if(itVoxel == mSupervoxels->end()) {
          // Create new supernode and add it to the list
          s = new supernode;
          s->id = sid;
          (*mSupervoxels)[sid] = s;
        } else {
          // Supernode already exists
          s = itVoxel->second;
        }

        // Add node to the supernode
        node* p = new node;
        p->z = d;
        p->y = y;
        p->x = x;
        s->addNode(p);
      }
#endif

  PRINT_MESSAGE("[Slice3d] %d supervoxels created\n", (int)mSupervoxels->size());

  /*
#if USE_LONG_RANGE_EDGES
  PRINT_MESSAGE("[Slice3d] Adding long range edges...\n");
  addLongRangeEdges();
  return;
#endif
  */

  if(loadNeighbors) {
    PRINT_MESSAGE("[Slice3d] Indexing neighbors...\n");

    stringstream sout_neighbors;
    sout_neighbors << inputDir << "neighbors_" << supernode_step << "_" << cubeness;
    if(fileExists(sout_neighbors.str().c_str())) {
      PRINT_MESSAGE("[Slice3d] Loading neighbors from %s\n", sout_neighbors.str().c_str());
      ifstream ifs(sout_neighbors.str().c_str());
      string line;
      while(getline(ifs,line)) {
        vector<string> tokens;
        splitString(line, tokens);
        int sid = atoi(tokens[0].c_str());
        supernode* s = (*mSupervoxels)[sid];
        vector<supernode*>* ptrNeighbors = &(s->neighbors);
        for(int i = 1; i < tokens.size(); ++i) {
          int nsid = atoi(tokens[i].c_str());
          supernode* sn = (*mSupervoxels)[nsid];
          s->neighbors.push_back(sn);
        }
      }
      ifs.close();
    } else {
      const int nh_size = 1; // neighborhood size
      sidType nsid;
      vector<supernode*>* ptrNeighbors;
      supernode* sn;
      bool existingSupernode;
      nbEdges = 0;
      for(int z = nh_size; z < depth - nh_size; z++) {
        for(int x = nh_size; x < width - nh_size; x++) {
          for(int y = nh_size; y < height - nh_size; y++) {
            sid = _klabels[z][y*width+x];
            for(int nx = x-nh_size; nx <= x+nh_size; nx++) {
              for(int ny = y-nh_size; ny <= y+nh_size; ny++) {
                for(int nz = z-nh_size; nz <= z+nh_size; nz++) {
                  nsid = _klabels[nz][ny*width+nx];
                  if(sid > nsid) {
                    s = (*mSupervoxels)[sid];
                    sn = (*mSupervoxels)[nsid];
                    if(sn == 0) {
                      printf("[Slice3d] Error : supernode %d is null (coordinate=(%d,%d,%d))\n",nsid,nx,ny,nz);
                      exit(-1);
                    }
                    ptrNeighbors = &(s->neighbors);
                    existingSupernode = false;
                    // Searching in a hash map would be faster but it would also consume
                    // more memory ...
                    for(vector<supernode*>::iterator itN = ptrNeighbors->begin();
                        itN != ptrNeighbors->end(); itN++) {
                      if((*itN)->id == nsid) {
                        existingSupernode = true;
                        break;
                      }
                    }
                    if(!existingSupernode) {
                      s->neighbors.push_back(sn);
                      sn->neighbors.push_back(s);
                      nbEdges++;
                    }
                  }
                }
              }
            }
          }
        }
      }

      PRINT_MESSAGE("Exporting neighbors to %s\n", sout_neighbors.str().c_str());
      ofstream ofs(sout_neighbors.str().c_str());
      for(map<sidType, supernode* >::iterator it = mSupervoxels->begin();
          it != mSupervoxels->end(); it++) {
        s = it->second;
        stringstream sout;
        sout << s->id;
        for(vector < supernode* >::iterator itN = s->neighbors.begin();
            itN != s->neighbors.end();itN++) {
          supernode* ns = *itN;
          sout << " " << ns->id;
        }
        ofs << sout.str() << endl;
      }
      ofs.close();

    }

    maxDegree = -1;
    int d;
    for(map<sidType, supernode* >::iterator it = mSupervoxels->begin();
        it != mSupervoxels->end(); it++) {
      d = it->second->neighbors.size();
      if(maxDegree < d) {
        maxDegree = d;
      }
    }    
    PRINT_MESSAGE("[Slice3d] %ld undirected edges created. Maximum degree = %d\n", nbEdges, maxDegree);
  }
}

uchar* Slice3d::createNodeLabelVolume()
{
  ulong sliceSize = width*height;
  ulong volSize = sliceSize*depth;
  uchar* labelVolume = new uchar[volSize];
  memcpy(labelVolume,raw_data,sizeof(char)*volSize);

  // Loop through supernodes and their neighbors
  supernode* s;
  node n;
  int superpixelLabel;

  for(map<sidType, supernode* >::iterator it = mSupervoxels->begin();
      it != mSupervoxels->end(); it++) {
    s = it->second;
    superpixelLabel = s->getLabel();

    nodeIterator ni = s->getIterator();
    ni.goToBegin();
    while(!ni.isAtEnd()) {
      ni.get(n);
      ni.next();
      labelVolume[n.z*sliceSize
                  +n.y*width
                  +n.x] = superpixelLabel*(255.0f/NUMBER_TYPE);
    }  
  }

  return labelVolume;
}

int Slice3d::getIntensity(int x, int y, int z)
{
  ulong sliceSize = width*height;
  return raw_data[z*sliceSize + y*width + x];
}

// TODO : Using the first channel is only OK for grey images
float Slice3d::getAvgIntensity(sidType supernodeId)
{
  map<sidType, supernode* >::iterator iLabel = mSupervoxels->find(supernodeId);
  if(iLabel == mSupervoxels->end()) {
    printf("[Slice3d] Error for sid %d : iLabel==g->mSupernodes.end()\n",supernodeId);
    return -1;
  }

  float intensity = 0;
  ulong idx = 0;
  supernode* s = iLabel->second;
  node n;
  nodeIterator ni = s->getIterator();
  ni.goToBegin();
  while(!ni.isAtEnd())
    {
      ni.get(n);
      ni.next();
      idx = ((ulong)n.z)*width*height + (n.y*width) + n.x;
      intensity += (float) raw_data[idx];
    }
  intensity /= s->size();
  return intensity;
}

void Slice3d::importSupervoxelsFromBinaryFile(const char* filename)
{
#ifndef USE_REVERSE_INDEXING
  sidType** klabels = 0;
#endif

  if(klabels != 0) {
    printf("[Slice3d] Error in importSupervoxels : supervoxels have already been generated\n");
    return;
  }
  PRINT_MESSAGE("[Slice3d] Importing supervoxel labels from binary file %s. depth=%d, height=%d, width=%d, supernode_step=%d\n",
                filename,depth,height,width,supernode_step);

  klabels = new sidType*[depth];
  ulong sliceSize = width*height;

  ifstream ifs(filename, ios::binary);
  for(int z = 0; z < depth;z++) {
    klabels[z] = new sidType[sliceSize];
    ifs.read((char*)klabels[z],sliceSize*sizeof(sidType));
  }
  ifs.close();

  createIndexingStructures(klabels);

#ifndef USE_REVERSE_INDEXING
  for(int z = 0; z < depth;z++) {
    delete[] klabels[z];
  }
  delete[] klabels;
#endif
}

void Slice3d::importSupervoxelsFromBuffer(const uint* buffer, int _width, int _height, int _depth)
{
#ifndef USE_REVERSE_INDEXING
  sidType** klabels = 0;
#endif

  printf("[Slice3d] Importing supervoxel labels from buffer. size = (%d,%d,%d) =? (%d,%d,%d), supernode_step=%d\n",
         width, height, depth, _width, _height, _depth, supernode_step);

  assert(_width == width);
  assert(_height == height);
  assert(_depth == depth);

  if(klabels != 0) {
    printf("[Slice3d] Error in importSupervoxels : supervoxels have already been generated\n");
    return;
  }

  klabels = new sidType*[depth];
  ulong sliceSize = width*height;

  ulong idx = 0;
  for(int z=0; z<depth; z++) {
    klabels[z] = new sidType[sliceSize];
    for(ulong s = 0; s < sliceSize; ++s) {
      klabels[z][s] = buffer[idx];
      ++idx;
    }
  }

  createIndexingStructures(klabels);

#ifndef USE_REVERSE_INDEXING
  for(int z = 0; z < depth;z++)
    delete[] klabels[z];
  delete[] klabels;
#endif
}

void Slice3d::importSupervoxels(const char* filename)
{
#ifndef USE_REVERSE_INDEXING
  sidType** klabels = 0;
#endif

  if(klabels != 0) {
    printf("[Slice3d] Error in importSupervoxels : supervoxels have already been generated\n");
    return;
  }

  const int MAX_SIZE = 100;
  char line[MAX_SIZE];

  ifstream ifs(filename);
  ifs.getline(line,MAX_SIZE);
  sscanf(line,"%d %d %d %d",&depth,&height,&width,&supernode_step);

  PRINT_MESSAGE("[Slice3d] Importing supervoxel labels from %s. depth=%d, height=%d, width=%d, supernode_step=%d\n",
                filename,depth,height,width,supernode_step);

  klabels = new sidType*[depth];
  ulong slice_size = width*height;
  for(int z=0;z<depth;z++)
    klabels[z] = new sidType[slice_size];

  for(int z = 0;z<depth;z++) {
    for(int y = 0;y<height;y++) {
      for(int x = 0;x<width;x++) {
        ifs.getline(line,MAX_SIZE);
        klabels[z][y*width+x] = (sidType)atoi(line);
      }
    }
  }
  ifs.close();

  createIndexingStructures(klabels);

#ifndef USE_REVERSE_INDEXING
  for(int z = 0;z < depth;z++)
    delete[] klabels[z];
  delete[] klabels;
#endif
}

void Slice3d::createReverseIndexing(sidType**& _klabels)
{
  _klabels = new sidType*[depth];
  ulong slice_size = width*height;
  for(int z=0;z<depth;z++) {
    _klabels[z] = new sidType[slice_size];
  }

  supernode* s;
  int sid = 0;
  node n;
  for(map<sidType, supernode* >::iterator it = mSupervoxels->begin();
      it != mSupervoxels->end(); it++) {
    sid = it->first;
    s = it->second;
    nodeI