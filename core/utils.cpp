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

#include <algorithm>
#include <errno.h>
#include <iostream>
#include <iterator>
#include <limits.h>
#include <locale>
#include <string>
#include <sstream>
//MSVS
//#include <values.h>
#define isnan(x) _isnan(x)

// zlib
#include "zlib.h"

// SliceMe
#include "utils.h"
#include "globalsE.h"
#include "Feature.h"
#include "F_Precomputed.h"
#include "Slice.h"
#include "Slice3d.h"
#include "Slice_P.h"

//---------------------------------------------------------------------FUNCTIONS

bool isDirectory(string path)
{
  return isDirectory(path.c_str());
}

bool isDirectory(const char* path)
{
  bool test = false;
  DIR* dir;
  dir = opendir(path);
  if (dir) {
    test = true;
  }
  closedir(dir);
  return test;
}

bool fileExists(const char* filename)
{
  ifstream inp;
  inp.open(filename, ifstream::in);
  if(inp.fail()){
    inp.close();
    return false;
  }
  inp.close();
  return true;
}

bool fileExists(string filename) {
  return fileExists(filename.c_str());
}

string getDirectoryFromPath(string path) {
  size_t pos = path.find_last_of("/\\");
  if(pos == string::npos) {
    return "./";
  } else {
    return path.substr(0,pos+1);
  }
}

string getLastDirectoryFromPath(string path) {
  int pos1 =  path.find_last_of("/\\");
  int pos2 =  path.find_last_of("/\\", pos1-1);
  int len = pos1-pos2-1;
  assert(pos2>0 && len>0);
  return path.substr(pos2+1, len);
}

string getNameFromPath(string path) {
  return path.substr(path.find_last_of("/\\")+1);
}

string getNameFromPathWithoutExtension(string path) {
  string nameWith =  path.substr(path.find_last_of("/\\")+1);
  string nameWithout = nameWith.substr(0,nameWith.find_last_of("."));
  return nameWithout;
}

string getNonExistingName(string name) {
  string name_no_ext = getNameFromPathWithoutExtension(name);
  string ext = getExtension(name);
  string new_name = name;
  ofstream ofs("test.txt", ios::app);
  ofs << name_no_ext << "\n";
  ofs << ext << "\n";
  ofs << new_name << "\n";
  int idx = 0;
  while(fileExists(new_name)) {
    stringstream sout;
    sout << name_no_ext;
    sout << idx;
    sout << ".";
    sout << ext;
    ++idx;
    new_name = sout.str();
    ofs << new_name;
  }
  ofs.close();
  return new_name;
}

bool containsImageExtension(string path) {
  string ext = path.substr(path.find_last_of(".") + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  if(ext == "png" || ext == "tif" || ext == "bmp") {
    return true;
  } else {
    return false;
  }
}

int enumerate_files_in_dir(const char* dir, vector<string> &files, const char* pattern)
{
  int id = 0;
  char* filename = new char[strlen(dir)+strlen(pattern)+250];
  for(;;)
    {
      sprintf(filename,pattern,id);
      if(fileExists(filename))
        files.push_back((string)filename);
      else
        break;
      id++;
    }

  sort(files.begin(),files.end());

  delete[] filename;
  return 0;
}

int getFilesInDirRec(const char* dir, vector<string> &files,
                     const char* ext)
{
  DIR *dp;
  struct dirent *dirp;
  if((dp  = opendir(dir)) == NULL)
    {
      //printf("%s is not a directory\n", dir);
      return 1;
    }

  string sext;
  if(ext!=0)
     sext = (string)ext;
  string sdir(dir);
  if(dir[strlen(dir)-1] != '/')
    sdir += '/';
  while ((dirp = readdir(dp)) != NULL)
    {
      string s(dirp->d_name);
      if(s.c_str()[0] == '.')
        continue;
      s = sdir + s;
      //printf("%s\n",s.c_str());
      if(getFilesInDirRec(s.c_str(),files,ext) == 0)
        continue;
      if(ext==0 || getExtension(s)==sext)
        files.push_back(s);
    }

  //sort(files.begin(),files.end());

  closedir(dp);
  return 0;
}

int getFilesInDir(const char* dir, vector<string> &files,
                  const int firstIdx, const char* ext, bool includePath)
{
  vector<string> _files;
  getFilesInDir(dir, _files, ext, includePath);
  for (int i = firstIdx; i < (int)_files.size(); ++i) {
    files.push_back(_files[i]);
  }
  return 0;
}

int getFilesInDir(const char* dir, vector<string> &files,
                  const char* ext, bool includePath)
{
 if(ext != 0 && strchr(ext,'%')!=0)
    return enumerate_files_in_dir(dir,files,ext);

  DIR *dp;
  struct dirent *dirp;
  if((dp  = opendir(dir)) == NULL) {
    cout << "Error(" << errno << ") opening " << dir << endl;
    return errno;
  }

  locale loc;
  string sext;
  if(ext != 0) {
     sext = (string)ext;
     //sext = tolower(sext, loc);
  }
  while ((dirp = readdir(dp)) != NULL) {
    string sname(dirp->d_name);
    if(includePath) {
      sname = string(dir) + sname;
    }
    string sname_ext = getExtension(sname);
    //sname_ext = tolower(sname_ext, loc);
    //printf("ext %s %s\n", sname_ext.c_str(), sext.c_str());
    if((ext == 0) || (sname_ext == sext)) {
      files.push_back(sname);
    }
  }

  sort(files.begin(),files.end());

  closedir(dp);
  return 0;
}

string getExtension(string path){
  return path.substr(path.find_last_of(".")+1);
}

int loadImagesInDir(const char* dir, vector<IplImage*>& lImages,
                    const char* ext, bool includePath)
{
  vector<string> lImageNames;
  if(getFilesInDir(dir,lImageNames,ext,includePath) != 0)
    return -1;

  for(vector<string>::iterator itFilename = lImageNames.begin();
      itFilename != lImageNames.end(); itFilename++)
    {
      IplImage* img = cvLoadImage(itFilename->c_str());
      if(!img)
        {
          printf("[Utils] Error in loadImagesInDir while loading %s\n",itFilename->c_str());
        }
      else
        lImages.push_back(img);
    }

  return 0;
}

string findLastFile(const string& file_pattern, const string& extension, int* _idx)
{
  int idx = 0;
  if(_idx) {
    idx = *_idx;
  }
  string last_file;
  do {
    last_file = file_pattern;
    last_file += VarToString(idx);
    last_file += extension;
    if(fileExists(last_file)) {
    //if(isDirectory(last_file)) {
      ++idx;
    } else {
      break;
    }
  } while(1);
  --idx;
  if(_idx) {
    *_idx = idx;
  }
  last_file = file_pattern;
  last_file += VarToString(idx);
  last_file += extension;
  return last_file; 
}

/*
 * Only compare first channel of the 2 given images
 */
int compareBWImages(const char* imageModelName,
                    const char* imageName,
                    float& true_neg,
                    float& true_pos,
                    float& false_neg,
                    float& false_pos,
                    bool normalize)
{
  IplImage *ptrModel = cvLoadImage(imageModelName);
  if(!ptrModel)
    {
      printf("[utils] Error while loading image %s\n",imageModelName);
      return -1;
    }

  IplImage *ptrImg = cvLoadImage(imageName);
  if(!ptrImg)
    {
      printf("[utils] Error while loading image %s\n",imageName);
      return -1;
    }


  int ret = compareBWImages(ptrModel, ptrImg,
                            true_neg, true_pos,
                            false_neg, false_pos,
                            normalize);

  cvReleaseImage(&ptrImg);
  cvReleaseImage(&ptrModel);

  return ret;
}

/*
 * Only compare first channel of the 2 given images
 */
int compareBWImages(IplImage *ptrModel,
                    IplImage *ptrImg,
                    float& true_neg,
                    float& true_pos,
                    