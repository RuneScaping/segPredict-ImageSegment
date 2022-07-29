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
                    float& false_neg,
                    float& false_pos,
                    bool normalize)
{
  const int POS_VALUE = 255;
  //const int NEG_VALUE = 0;
  uchar* ptrM;
  uchar* ptrI;

  if((ptrModel->width != ptrImg->width) || (ptrModel->height != ptrImg->height)
     || (ptrModel->nChannels != ptrImg->nChannels))
    {
      printf("[utils] Error : The 2 images have different attributes (width, height, number of channels)\n");
      return -1;
    }

  int total_pos = 0;
  int total_neg = 0;
  true_pos = 0.0f;
  true_neg = 0.0f;
  false_pos = 0.0f;
  false_neg = 0.0f;
  for(int u=0;u<ptrImg->width;u++)
    {
      for(int v=0;v<ptrImg->height;v++)
        {
          ptrM = &((uchar*)(ptrModel->imageData + ptrModel->widthStep*v))[u*ptrModel->nChannels];
          ptrI = &((uchar*)(ptrImg->imageData + ptrImg->widthStep*v))[u*ptrImg->nChannels];

          //printf("%d %d ",(int)ptrC,(int)ptrC2);

          if(ptrImg->nChannels == 3)
            {
              // color image
              if((*ptrM == POS_VALUE) && (*(ptrM+1) == POS_VALUE) && (*(ptrM+2) == POS_VALUE))
                {
                  // positive sample
                  total_pos++;

                  if((*ptrI == POS_VALUE) && (*(ptrI+1) == POS_VALUE) && (*(ptrI+2) == POS_VALUE))
                    true_pos++;
                  else
                    false_neg++;
                }
              else
                {
                  // negative sample
                  total_neg++;

                  if((*ptrI == POS_VALUE) && (*(ptrI+1) == POS_VALUE) && (*(ptrI+2) == POS_VALUE))
                    false_pos++;
                  else
                    true_neg++;
                }
            }
          else
            {
              // gray image
              if(*ptrM == POS_VALUE)
                {
                  total_pos++;

                  if(*ptrI == POS_VALUE)
                    true_pos++;
                  else
                    false_neg++;
                }
              else
                {
                  total_neg++;

                  if(*ptrI == POS_VALUE)
                    false_pos++;
                  else
                    true_neg++;
                }
            }
        }
    }

  //printf("Comparison %f %f %d %d\n",true_pos,false_neg,total_pos,total_neg);
  if(normalize)
    {
      if(total_pos != 0)
        {
          true_pos *= (100.0f/total_pos);
          false_neg *= (100.0f/total_pos);
        }
      if(total_neg != 0)
        {
          true_neg *= (100.0f/total_neg);
          false_pos *= (100.0f/total_neg);
        }
    }

  return 0;
}

void save16bitsImage(const char* filename, IplImage* img)
{
  ofstream ofs(filename, ios::out | ios::binary);
  ushort* ptrImg;
  for(int y=0;y<img->height;y++) {
    for(int x=0;x<img->width;x++) {
      ptrImg = ((ushort*)(img->imageData + img->widthStep*y)) + x*(img)->nChannels;
      ofs.write((char*)ptrImg,sizeof(ushort));
    }
  }
  ofs.close();
}

void save32bitsImage(const char* filename, IplImage* img)
{
  ofstream ofs(filename, ios::out | ios::binary);
  uint* ptrImg;
  for(int y=0;y<img->height;y++) {
    for(int x=0;x<img->width;x++) {
      ptrImg = ((uint*)(img->imageData + img->widthStep*y)) + x*(img)->nChannels;
      ofs.write((char*)ptrImg,sizeof(int));
    }
  }
  ofs.close();
}

IplImage* load32bitsImage(const char* filename, CvSize& size)
{
  IplImage* img = cvCreateImage(size, IPL_DEPTH_32S, 1);
  ifstream ifs(filename, ios::in | ios::binary);

  int* ptrImg;
  for(int y=0;y<img->height;y++)
    for(int x=0;x<img->width;x++)
      {
        ptrImg = ((int*)(img->imageData + img->widthStep*y)) + x*(img)->nChannels;
        ifs.read((char*)ptrImg,sizeof(int));
      }
  ifs.close();
  return img;
}

void saveDoubleImage(const char* filename, IplImage* img)
{
  ofstream ofs(filename, ios::out | ios::binary);

  double* ptrImg;
  // channels first (easier to load in matlab)
  for(int c = 0; c < img->nChannels; c++)
    for(int y=0;y<img->height;y++)
      for(int x=0;x<img->width;x++)
        {
          ptrImg = ((double*)(img->imageData + img->widthStep*y)) + x*(img)->nChannels + c;
          ofs.write((char*)ptrImg,sizeof(double));
        }
  ofs.close();
}

IplImage* loadDoubleImage(const char* filename, CvSize& size, int nChannels)
{
  IplImage* img = cvCreateImage(size, IPL_DEPTH_64F, nChannels);
  ifstream ifs(filename, ios::in | ios::binary);

  double* ptrImg;
  for(int c = 0; c < nChannels; c++)
    for(int y=0;y<img->height;y++)
      for(int x=0;x<img->width;x++)
        {
          ptrImg = ((double*)(img->imageData + img->widthStep*y)) + x*(img)->nChannels + c;
          ifs.read((char*)ptrImg,sizeof(double));
        }
  ifs.close();
  return img;
}

void saveFloatImage(const char* filename, IplImage* img)
{
  ofstream ofs(filename, ios::out | ios::binary);

  float* ptrImg;
  // channels first (easier to load in matlab)
  for(int c = 0; c < img->nChannels; c++)
    for(int y=0;y<img->height;y++)
      for(int x=0;x<img->width;x++)
        {
          ptrImg = ((float*)(img->imageData + img->widthStep*y)) + x*(img)->nChannels + c;
          ofs.write((char*)ptrImg,sizeof(float));
        }
  ofs.close();
}

IplImage* loadFloatImage(const char* filename, CvSize& size, int nChannels)
{
  IplImage* img = cvCreateImage(size, IPL_DEPTH_64F, nChannels);
  ifstream ifs(filename, ios::in | ios::binary);

  float* ptrImg;
  for(int c = 0; c < nChannels; c++)
    for(int y=0;y<img->height;y++)
      for(int x=0;x<img->width;x++)
        {
          ptrImg = ((float*)(img->imageData + img->widthStep*y)) + x*(img)->nChannels + c;
          ifs.read((char*)ptrImg,sizeof(float));
        }
  ifs.close();
  return img;
}

void saveImage(const char* filename, IplImage* img, const char* ext)
{
  const char* raw_ext = ".raw";
  IplImage* img8U = 0;
  bool releaseImage = true;
  if(img->depth == IPL_DEPTH_64F)
    {
      stringstream sout;
      sout << filename << "D" << raw_ext;
      saveDoubleImage(sout.str().c_str(),img);
      double2ucharImage(img, img8U);
    }
  else if(img->depth == IPL_DEPTH_32F)
    {
      stringstream sout;
      sout << filename << "F" << raw_ext;
      saveFloatImage(sout.str().c_str(),img);
      float2ucharImage(img, img8U);
    }
  else if(img->depth == (int)IPL_DEPTH_32S)
    {
      stringstream sout;
      sout << filename << "S" << raw_ext;
      save32bitsImage(sout.str().c_str(),img);
    }
  else if(img->depth == IPL_DEPTH_8U)
    {
      img8U = img;
      releaseImage = false;
    }
  else
    {
      printf("[utils] saveImage : unknown image type\n");
    }

  if(img8U != 0)
    {
      stringstream sout;
      sout << filename << "U" << ext;
      cvSaveImage(sout.str().c_str(),img8U);
      if(releaseImage)
        cvReleaseImage(&img8U);
    }
}

void double2ucharImage(IplImage* imgIn, IplImage*& imgOut)
{
  double vImg;
  double maxValue = 0;
  imgOut = cvCreateImage(cvSize(imgIn->width,imgIn->height), IPL_DEPTH_8U, imgIn->nChannels);
  for(int c = 0; c < imgIn->nChannels; c++)
    for(int y=0;y<imgIn->height;y++)
      for(int x=0;x<imgIn->width;x++)
        {
          vImg = ((double*)(imgIn->imageData + imgIn->widthStep*y))[x*imgIn->nChannels + c];
          if(maxValue < vImg)
            maxValue = vImg;
        }

  //printf("double2ucharImage %f\n",maxValue);
  cvConvertScale(imgIn,imgOut,255.0/maxValue);
}

void float2ucharImage(IplImage* imgIn, IplImage*& imgOut)
{
  float vImg;
  float maxValue = 0;
  imgOut = cvCreateImage(cvSize(imgIn->width,imgIn->height), IPL_DEPTH_8U, imgIn->nChannels);
  for(int c = 0; c < imgIn->nChannels; c++)
    for(int y=0;y<imgIn->height;y++)
      for(int x=0;x<imgIn->width;x++)
        {
          vImg = ((float*)(imgIn->imageData + imgIn->widthStep*y))[x*imgIn->nChannels + c];
          if(maxValue < vImg)
            maxValue = vImg;
        }

  //printf("double2ucharImage %f\n",maxValue);
  cvConvertScale(imgIn,imgOut,255.0/maxValue);
}

int sign(int v)
{
  return v > 0 ? 1 : (v < 0 ? -1 : 0);
}

uint time_seed()
{
  time_t now = time ( 0 );
  unsigned char *p = (unsigned char *)&now;
  unsigned seed = 0;
  size_t i;
 
  for ( i = 0; i < sizeof now; i++ )
    seed = seed * ( UCHAR_MAX + 2U ) + p[i];
 
  return seed;
}

void exportCube(uchar* rawData,
                const char* filename,
                int cubeDepth,
                int cubeHeight,
                int cubeWidth)
{
#ifdef USE_ITK
  exportTIFCube(rawData,
                filename,
                cubeDepth,
                cubeHeight,
                cubeWidth);
#else
  exportVIVACube(rawData,
                 filename,
                 cubeDepth,
                 cubeHeight,
                 cubeWidth);
#endif
}

void exportCube(float* rawData,
                const char* filename,
                int cubeDepth,
                int cubeHeight,
                int cubeWidth)
{
  exportVIVACube(rawData,
                 filename,
                 cubeDepth,
                 cubeHeight,
                 cubeWidth);
}

bool getGroundTruthName(string& groundTruthName, const string& maskDir, const string& filename)
{
  bool found_file = false;
  vector<string> list_ext;
  list_ext.push_back(".png");
  list_ext.push_back(".bmp");
  list_ext.push_back("_GT.bmp");
  list_ext.push_back(".jpg");
  list_ext.push_back(".tif");
  list_ext.push_back(".labels");

  string baseName = getNameFromPathWithoutExtension(filename);
  for(vector<string>::const_iterator it = list_ext.begin();
      it != list_ext.end(); ++it) {
    groundTruthName = maskDir + baseName + *it;
    if(fileExists(groundTruthName.c_str())) {
      found_file = true;
      break;      
    }
  }
  return found_file;
}

//---------------------------------------------------------------------------ITK

#ifdef USE_ITK

void exportTIFCube(uchar* rawData,
                   const char* filename,
                   int cubeDepth,
                   int cubeHeight,
                   int cubeWidth)
{
  // import data to an itk image
  const int dimension = 3;
  ty