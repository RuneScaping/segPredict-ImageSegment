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

#include <fstream>

// SliceMe
#include "Config.h"
#include "F_LoadFromFile.h"
#include "oSVM.h"
#include "utils.h"

//------------------------------------------------------------------------------

#define OUTPUT_FEATURES_TO_TXT_FILE 0

#define UPSIDE_DOWN_FEATURES 0

//------------------------------------------------------------------------------

F_LoadFromFile::F_LoadFromFile() {
  featureSize = 0;
  nFeatures = 0;
  featurePath = "";
  initialized = false;

#if UPSIDE_DOWN_FEATURES
  printf("[F_LoadFromFile] UPSIDE_DOWN_FEATURES is set to 1\n");
#endif
}

void F_LoadFromFile::loadFeatureFilenames(const char* filename,
                                          vector<string>* lFeatureFilenames)
{
  ifstream ifs(filename);
  if(ifs.fail()) {
    printf("[F_LoadFromFile] Error while loading %s\n", filename);
    exit(-1);
  }
  
  // Load path containing files
  getline(ifs, featurePath);

  // Load name of the files containing the features
  string fn;
  while(!ifs.eof()) {
    getline(ifs,fn);
    if(!fn.empty()) {
      lFeatureFilenames->push_back(fn);
    }
  }
  ifs.close();
}

string F_LoadFromFile::getAbsoluteFeaturePath(const string& featureFilename,
                                              const string& inputDir)
{
  string fullpath = featureFilename;
  string inputDir_rel = inputDir;
  if(inputDir_rel[inputDir_rel.length()-1] == '/')
    inputDir_rel = inputDir_rel.substr(0,inputDir_rel.length()-1);
  inputDir_rel = inputDir_rel.substr(inputDir_rel.rfind('/'));
  fullpath += inputDir_rel;
  fullpath += "/";
  return fullpath;
}

void F_LoadFromFile::init(Slice3d& slice3d, const char* filename,
                          vector<sidType>& lNodes)
{
  if(initialized) {
    printf("[F_LoadFromFile] Warning: Features were already loaded.\n");
    return;
  }

  vector<string> lFeatureFilenames;
  loadFeatureFilenames(filename, &lFeatureFilenames);

  if(lFeatureFilenames.size() == 0) {
    // no feature filename specified. Check if there is a file with a name
    // matching filename
    string featName = getNameFromPathWithoutExtension(slice3d.getName());
    string fullpath = featurePath + "/";
    fullpath += featName;
    printf("[F_LoadFromFile] Checking binary file %s\n", fullpath.c_str());
    if(fileExists(fullpath)) {
      lFeatureFilenames.push_back(featName);
      loadTextFeatures(slice3d, lFeatureFilenames);
    } else {
      string fullpath_with_ext = fullpath + ".bin";
      featName += ".bin";
      if(fileExists(fullpath_with_ext)) {
        printf("[F_LoadFromFile] Loading features from binary file %s\n", fullpath_with_ext.c_str());
        vector<string> lFiles;
        lFiles.push_back(featName);
        loadSupervoxelBasedFeaturesFromBinary(slice3d, lFiles, lNodes);
      } else {
        printf("[F_LoadFromFile] No features to be loaded in %s\n", filename);
      }
    }
  } else {
    string ext = getExtension(lFeatureFilenames[0]);
    if(ext == "tif") {
      PRINT_MESSAGE("[F_LoadFromFile] Loading features from TIF file %s\n", lFeatureFilenames[0].c_str());
      loadSupervoxelBasedFeaturesFromTIF(slice3d, lFeatureFilenames, lNodes);
    } else {
      if(ext == "bin") {
        printf("[F_LoadFromFile] Loading features from binary file %s\n", lFeatureFilenames[0].c_str());
        loadSupervoxelBasedFeaturesFromBinary(slice3d, lFeatureFilenames, lNodes);
      } else {
        loadTextFeatures(slice3d, lFeatureFilenames);
      }
    }
  }

  initialized = true;
}

void F_LoadFromFile::loadSupervoxelBasedFeaturesFromTIF(Slice3d& slice3d,
                                                        const vector<string>& lFeatureFilenames,
                                                        vector<sidType>& lNodes)
{
  featureSize = lFeatureFilenames.size();
  nFeatures = lNodes.size();
  PRINT_MESSAGE("[F_LoadFromFile] Allocating memory for %ld nodes and %d features\n",
                lNodes.size(), featureSize);
#if !USE_SPARSE_STRUCTURE
  features = new fileFeatureType*[nFeatures];
#endif
  for(vector<sidType>::iterator itNode = lNodes.begin();
      itNode != lNodes.end(); itNode++) {
    features[*itNode] = new fileFeatureType[featureSize];
  }

  // store features in memory
  fileFeatureType value;
  ulong idx;
  int fileId = 0;
  supernode* s;
  node center;
  for(vector<string>::const_iterator itFile = lFeatureFilenames.begin();
      itFile != lFeatureFilenames.end(); itFile++)
    {

#if OUTPUT_FEATURES_TO_TXT_FILE
      stringstream sout;
      sout << "features_" << fileId << ".txt";
      ofstream ofsFeat(sout.str().c_str());
#endif

      string fullpath = getAbsoluteFeaturePath(featurePath, slice3d.inputDir);
      fullpath += *itFile;
      PRINT_MESSAGE("[F_LoadFromFile] Loading %s\n", fullpath.c_str());
      Slice3d* inputCube = new Slice3d(fullpath.c_str());
      for(vector<sidType>::iterator itNode = lNodes.begin();
          itNode != lNodes.end(); itNode++)
        {
          s = slice3d.getSupernode(*itNode);
          s->getCenter(center);

#if UPSIDE_DOWN_FEATURES
          // Bug fix : Feture cubes from Yunpeng are upside-down
          center.y = slice3d.height - center.y;
#endif

          idx = slice3d.getIndex(center.x,center.y,center.z);
          // read data
          value = inputCube->raw_data[idx];
          features[*itNode][fileId] = (fileFeatureType)value;

#if OUTPUT_FEATURES_TO_TXT_FILE
          ofsFeat << (double) value << endl;
#endif          
        }
      delete inputCube;

#if OUTPUT_FEATURES_TO_TXT_FILE
      ofsFeat.close();
#endif

      fileId++;
    }
  PRINT_MESSAGE("[F_LoadFromFile] All feature files are now loaded in memory\n");
}

void F_LoadFromFile::loadSupervoxelBasedFeaturesFromSetOfBinaries(Slice3d& slice3d,
                                                                  const vector<string>& lFeatureFilenames,
                                                                  vector<sidType>& lNodes)
{
  featureSize = lFeatureFilenames.size();
  nFeatures = lNodes.size();
  PRINT_MESSAGE("[F_LoadFromFile] Allocating memory for %ld nodes and %d features\n",
                lNodes.size(), featureSize);
#if !USE_SPARSE_STRUCTURE
  features = new fileFeatureType*[nFeatures];
#endif
  for(vector<sidType>::iterator itNode = lNodes.begin();
      itNode != lNodes.end(); itNode++) {
    features[*itNode] = new fileFeatureType[featureSize];
  }

  // store features in memory
  fileFeatureType value;
  ulong idx;
  int fileId = 0;
  supernode* s;
  node center;
  for(vector<string>::const_iterator itFile = lFeatureFilenames.begin();
      itFile != lFeatureFilenames.end(); itFile++)
    {
      ifstream ifsF(itFile->c_str(), ios::binary);
      if(ifsF.fail()) {
        printf("[F_LoadFromFile] Failed to load %s\n", itFile->c_str());
        exit(-1);
      }
      for(vector<sidType>::iterator itNode = lNodes.begin();
          itNode != lNodes.end(); itNode++) {
        s = slice3d.getSupernode(*itNode);
        s->getCenter(center);
        idx = slice3d.getIndex(center.x,center.y,center.z);
        ifsF.seekg (idx, ios::beg);
        // read data
        ifsF.read((char*)&value,sizeof(fileFeatureType));
        features[*itNode][fileId] = value;
        //printf("f %ld %d %d %d %d\n",idx,(int)value,*itNode,fileId,(int)features[*itNode][fileId]);
      }
      ifsF.close();
      fileId++;
    }
  PRINT_MESSAGE("[F_LoadFromFile] All feature files are now loaded in memory\n");
}


void F_LoadFromFile::loadSupervoxelBasedFeaturesFromBinary(Slice3d& slice3d,
                                                           const vector<string>& lFeatureFilenames,
                                                           vector<sidType>& lNodes)
{
  uint featureSizePerFile = 0;
  string config_tmp;
  if(Config::Instance()->getParameter("featureSizePerFile", config_tmp)) {
    featureSizePerFile = atoi(config_tmp.c_str());
  }

  featureSize = featureSizePerFile;
  nFeatures = lNodes.size();
  PRINT_MESSAGE("[F_LoadFromFile] Allocating memory for %ld nodes and %d features\n",
                lNodes.size(), featureSize);
#if !USE_SPARSE_STRUCTURE
  features = new fileFeatureType*[nFeatures];
#endif
  for(vector<sidType>::iterator itNode = lNodes.begin();
      itNode != lNodes.end(); itNode++) {
    features[*itNode] = new fileFeatureType[featureSize];
  }

  for(vector<string>::const_iterator itFile = lFeatureFilenames.begin();
      itFile != lFeatureFilenames.end(); itFile++)
    {
      printf("[F_LoadFromFile] Loading %s\n", itFile->c_str());

      /*
       * File format :
       * <unsigned int numRows>
       * <unsigned int numCols>
       * <numRows * numCols float values (32-bit float data)>
       * The data is in column-major format, and each row belongs to a given supervoxel.
       */

      string fulllpath = featurePath + *itFile;
      ifstream ifsF(fulllpath.c_str(), ios::binary);
      if(ifsF.fail()) {
        printf("[F_LoadFromFile] Failed to load %s\n", itFile->c_str());
        exit(-1);
      }
      uint nRows;
      uint nCols;
      ifsF.read((char*)&nRows, sizeof(uint));
      ifsF.read((char*)&nCols, sizeof(uint));
      ulong n = nRows*nCols;
      printf("[F_LoadFromFile] nRows = %d, nCols = %d, nSupernodes = %ld, featureSize = %d. Need %f Mb\n",
             nRows, nCols, lNodes.size(), featureSize, n/(1024.0*1024.0));
      assert(nRows >= lNodes.size());
      assert(nCols >= (uint)featureSize);
      bool useSparseFeatures = lNodes.size() < nRows;
      float* _features = new float[n];
      ifsF.read((char*)_features, sizeof(float)*n);

#if 1
      if(useSparseFeatures) {
        for(int i = 0; i < featureSize; ++i) {
          ulong idx = i*nRows;
          for(vector<sidType>::iterator itNode = lNodes.begin();
              itNode != lNodes.end(); itNode++) {
            ulong fidx = idx + *itNode;
            if(fidx > n) {
              printf("f %d/%d, %d/%d, %ld %ld %ld\n", i, featureSize, *itNode, nRows, idx, fidx, n); 
              exit(-1);
            }
            assert(fidx < n);
            features[*itNode][i] = _features[fidx];
          }
        }
      } else {
        ulong fidx = 0;
        for(int j = 0; j < featureSize; ++j) {
          for(uint i = 0; i < nRows; ++i) {
            features[i][j] = _features[fidx];
            ++fidx;
          }
        }
      }
#else
      if(useSparseFeatures) {
        for(vector<sidType>::iterator itNode = lNodes.begin();
            itNode != lNodes.end(); itNode++) {
          ulong idx = (*itNode)*nCols;
          for(int i = 0; i < featureSize; ++i) {
            ulong fidx = idx + i;
            if(fidx > n) {
              printf("f %d/%d, %d/%d, %ld %ld %ld\n", i, featureSize, *itNode, nRows, idx, fidx, n); 
              exit(-1);
            }
            assert(fidx < n);
            features[*itNode][i] = _features[fidx];
          }
        }
      } else {
        ulong fidx = 0;
        for(uint i = 0; i < nRows; ++i) {
          for(int j = 0; j < featureSize; ++j) {
            features[i][j] = _features[fidx];
            ++fidx;
          }
        }
      }
#endif

      ifsF.close();
      delete[] _features;
    }

  PRINT_MESSAGE("[F_LoadFromFile] Rescaling features...\n");
  // rescale features
  string range_filename = slice3d.getName() + ".range";
  ofstream ofs_range(range_filename.c_str());
  for(int i = 0; i < featureSize; ++i) {
    fileFeatureType min_value = features[lNodes[0]][i];
    fileFeatureType max_value = features[lNodes[0]][i];
    for(int featIdx = 0; featIdx < nFeatures; ++featIdx) {
      fileFeatureType* _feat = features[featIdx];
      if(_feat[i] < min_value) {
        min_value = _feat[i];
      }
      if(_feat[i] > max_value) {
        max_value = _feat[i];
      }
    }
    PRINT_MESSAGE("[F_LoadFromFile] Feature %d : (min,max)=(%g,%g)\n", i,
                  min_value, max_value);

    ofs_range << min_value << " " << max_value << endl;

    fileFeatureType df = max_value - min_value;
    for(int featIdx = 0; featIdx < nFeatures; ++featIdx) {
      fileFeatureType* _feat = features[featIdx];
      _feat[i] = (_feat[i]-min_value)/df;
    }
  }
  ofs_range.close();

  PRINT_MESSAGE("[F_LoadFromFile] All feature files are now loaded in memory\n");
}

void F_LoadFromFile::init(Slice3d& slice3d, const char* filename,
                          map<sidType, sidType>& sid_mapping)
{
  if(initialized) {
    printf("[F_LoadFromFile] Warning: Features were already loaded.\n");
    return;
  }

  vector<string> lFeatureFilenames;
  loadFeatureFilenames(filename, &lFeatureFilenames);

  if(lFeatureFilenames.size() == 0) {
    // no feature filename specified. Check if there is a file with a name
    // matching filename
    string featName = getNameFromPathWithoutExtension(slice3d.getName());
    featName += ".bin";
    string fullpath = featurePath + "/";
    fullpath += featName;
    printf("[F_LoadFromFile] Checking binary file %s\n", fullpath.c_str());
    if(fileExists(fullpath)) {
      printf("[F_LoadFromFile] Loading features from binary file %s\n", fullpath.c_str());
      vector<string> lFiles;
      lFiles.push_back(featName);
      vector<sidType> lSamples;
      for(map<sidType, supernode* >::iterator it = slice3d.mSupervoxels->begin();
          it != slice3d.mSupervoxels->end(); it++) {
        lSamples.push_back(it->first);
      }
      loadSupervoxelBasedFeaturesFromBinary(slice3d, lFiles, lSamples, sid_mapping);
    } else {
      printf("[F_LoadFromFile] No features to be loaded in %s\n", filename);
    }
  } else {
    string ext = getExtension(lFeatureFilenames[0]);
    if(ext == "tif") {
      //PRINT_MESSAGE("[F_LoadFromFile] Loading feature cube %s\n", lFeatureFilenames[0].c_str());
      //loadVoxelBasedFeaturesFromTIF(slice3d, lFeatureFilenames);
      printf("[F_LoadFromFile] Not implemented yet\n");
      exit(-1);
    } else {
      if(ext == "bin") {
        PRINT_MESSAGE("[F_LoadFromFile] Loading features from binary file %s\n", lFeatureFilenames[0].c_str());
        vector<sidType> lSamples;
        for(map<sidType, supernode* >::iterator it = slice3d.mSupervoxels->begin();
            it != slice3d.mSupervoxels->end(); it++) {
          lSamples.push_back(it->first);
        }
        loadSupervoxelBasedFeaturesFromBinary(slice3d, lFeatureFilenames, lSamples, sid_mapping);
      } else {
        PRINT_MESSAGE("[F_LoadFromFile] Loading features from text file %s\n", lFeatureFilenames[0].c_str());
        loadTextFeatures(slice3d, lFeatureFilenames);
      }
    }
  }

  initialized = true;
}

void F_LoadFromFile::loadSupervoxelBasedFeaturesFromBinary(Slice3d& slice3d,
                                                           const vector<string>& lFeatureFilenames,
                                                           vector<sidType>& lNodes,
                                                           map<sidType, sidType>& sid_mapping)
{
  uint featureSizePerFile = 0;
  string config_tmp;
  if(Config::Instance()->getParameter("featureSizePerFile", config_tmp)) {
    featureSizePerFile = atoi(config_tmp.c_str());
  }

  featureSize = featureSizePerFile;
  nFeatures = lNodes.size();
  PRINT_MESSAGE("[F_LoadFromFile] Allocating memory for %ld nodes and %d features\n",
                lNodes.size(), featureSize);
#if !USE_SPARSE_STRUCTURE
  features = new fileFeatureType*[nFeatures];
#endif
  for(vector<sidType>::iterator itNode = lNodes.begin();
      itNode != lNodes.end(); itNode++) {
    features[*itNode] = new fileFeatureType[featureSize];
  }

  for(vector<string>::const_iterator itFile = lFeatureFilenames.begin();
      itFile != lFeatureFilenames.end(); itFile++)
    {
      printf("[F_LoadFromFile] Loading %s\n", itFile->c_str());

      /*
       * File format :
       * <unsigned int numRows>
       * <unsigned int numCols>
       * <numRows * numCols float values (32-bit float data)>
       * The data is in column-major format, and each row belongs to a given supervoxel.
       */

      string fulllpath = featurePath + *itFile;
      ifstream ifsF(fulllpath.c_str(), ios::binary);
      if(ifsF.fail()) {
        printf("[F_LoadFromFile] Failed to load %s\n", itFile->c_str());
        exit(-1);
      }
      uint nRows;
      uint nCols;
      ifsF.read((char*)&nRows, sizeof(uint));
      ifsF.read((char*)&nCols, sizeof(uint));
      ulong n = nRows*nCols;
      printf("[F_LoadFromFile] nRows = %d, nCols = %d, nSupernodes = %ld, featureSize = %d. Need %f Mb\n",
             nRows, nCols, lNodes.size(), featureSize, n/(1024.0*1024.0));
      assert(nRows >= lNodes.size());
      assert(nCols >= (uint)featureSize);
      bool useSparseFeatures = lNodes.size() < nRows;
      assert(useSparseFeatures);
      float* _features = new float[n];
      ifsF.read((char*)_features, sizeof(float)*n);

#if 1
      for(int i = 0; i < featureSize; ++i) {
        ulong idx = i*nRows;
        for(vector<sidType>::iterator itNode = lNodes.begin();
            itNode != lNodes.end(); itNode++) {
          ulong fidx = idx + sid_mapping[*itNode];
          if(fidx > n) {
            printf("f %d/%d, %d/%d, %ld %ld %ld\n", i, featureSize, *itNode, nRows, idx, fidx, n); 
            exit(-1);
          }
          assert(fidx < n);
          features[*itNode][i] = _features[fidx];
        }
      }
#else
      for(vector<sidType>::iterator itNode = lNodes.begin();
          itNode != lNodes.end(); itNode++) {
        ulong idx = sid_mapping[*itNode]*nCols;
        for(int i = 0; i < featureSize; ++i) {
          ulong fidx = idx + i;
          if(fidx > n) {
            printf("f %d/%d, %d/%d, %ld %ld %ld\n", i, featureSize, *itNode, nRows, idx, fidx, n); 
            exit(-1);
          }
          assert(fidx < n);
          features[*itNode][i] = _features[fidx];
        }
      }
#endif

      ifsF.close();
      delete[] _features;
    }

  PRINT_MESSAGE("[F_LoadFromFile] Rescaling features...\n");
  // rescale features
  string range_filename = slice3d.getName() + ".range";
  ofstream ofs_range(range_filename.c_str());
  for(int i = 0; i < featureSize; ++i) {
    fileFeatureType min_value = features[lNodes[0]][i];
    fileFeatureType max_value = features[lNodes[0]][i];
    for(int featIdx = 0; featIdx < nFeatures; ++featIdx) {
      fileFeatureType* _feat = features[featIdx];
      if(_feat[i] < min_value) {
        min_value = _feat[i];
      }
      if(_feat[i] > max_value) {
        max_value = _feat[i];
      }
    }
    PRINT_MESSAGE("[F_LoadFromFile] Feature %d : (min,max)=(%g,%g)\n", i,
                  min_value, max_value);

    ofs_range << min_value << " " << max_value << endl;

    fileFeatureType df = max_value - min_value;
    for(int featIdx = 0; featIdx < nFeatures; ++featIdx) {
      fileFeatureType* _feat = features[featIdx];
      _feat[i] = (_feat[i]-min_value)/df;
    }
  }
  ofs_range.close();

  PRINT_MESSAGE("[F_LoadFromFile] All feature files are now loaded in memory\n");
}

void F_LoadFromFile::init(Slice_P& slice_p, const char* filename)
{
  switch(slice_p.getType()) {
  case SLICEP_SLICE:
    {
      Slice* slice = static_cast<Slice*>(&slice_p);
      init(*slice, filename);
    }
    break;
  case SLICEP_SLICE3D:
    {
      Slice3d* slice3d = static_cast<Slice3d*>(&slice_p);
      init(*slice3d, filename);
    }
    break;
  default:
    break;
  }
}

void F_L