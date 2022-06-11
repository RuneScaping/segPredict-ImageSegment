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
        printf("[F_LoadFromFile] Failed to load %s\n", itFile->c_str(