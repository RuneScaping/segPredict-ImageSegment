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
  typedef uchar TInputPixelType;
  typedef itk::Image< TInputPixelType, dimension > InputImageType;
  typedef itk::Image< TInputPixelType, dimension > OutputImageType;
  typedef itk::ImportImageFilter< TInputPixelType, dimension > ImportFilterType;
  ImportFilterType::Pointer importFilter = ImportFilterType::New();
		
  ImportFilterType::SizeType size;
  size[0] = cubeWidth;
  size[1] = cubeHeight;
  size[2] = cubeDepth;
		
  ImportFilterType::IndexType start;
  start.Fill(0);
		
  ImportFilterType::RegionType region;
  region.SetIndex(start);
  region.SetSize(  size  );
		
  importFilter->SetRegion( region );	
		
  InputImageType::PointType origin;
  origin.Fill(0.0);
		
  importFilter->SetOrigin( origin );
				
  ImportFilterType::SpacingType spacing;
  spacing.Fill(1.0);
		
  importFilter->SetSpacing( spacing );
  importFilter->SetImportPointer(rawData, 0, false);

  stringstream sout;
  sout << filename;
  int n = strlen(filename);
  if(n < 4 || strcmp(filename+n-4,".tif")!=0)
     sout << ".tif";
  printf("[Utils] Writing output cube %s of size (%d,%d,%d)\n",
         sout.str().c_str(), cubeWidth, cubeHeight, cubeDepth);
  typedef itk::ImageFileWriter< OutputImageType > WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(sout.str().c_str());
  writer->SetInput(importFilter->GetOutput());
  writer->Update();
}

void exportColorTIFCube(uchar* rawData,
                        const char* filename,
                        int cubeDepth,
                        int cubeHeight,
                        int cubeWidth)
{
#if USE_ITK
  // import data to an itk image
  const int dimension = 3;
  typedef uchar TInputPixelType;
  typedef itk::RGBPixel<TInputPixelType> RGBPixelType;
  typedef itk::Image< RGBPixelType, dimension > InputImageType;
  typedef itk::Image< RGBPixelType, dimension > OutputImageType;
  typedef itk::ImportImageFilter< RGBPixelType, dimension > ImportFilterType;
  ImportFilterType::Pointer importFilter = ImportFilterType::New();
		
  ImportFilterType::SizeType size;
  size[0] = cubeWidth;
  size[1] = cubeHeight;
  size[2] = cubeDepth;
		
  ImportFilterType::IndexType start;
  start.Fill(0);
		
  ImportFilterType::RegionType region;
  region.SetIndex(start);
  region.SetSize(  size  );
		
  importFilter->SetRegion( region );	
		
  InputImageType::PointType origin;
  origin.Fill(0.0);
		
  importFilter->SetOrigin( origin );
				
  ImportFilterType::SpacingType spacing;
  spacing.Fill(1.0);
		
  importFilter->SetSpacing( spacing );
  RGBPixelType* localBuffer = reinterpret_cast<RGBPixelType* >(rawData);
  importFilter->SetImportPointer(localBuffer, 0, false);

  stringstream sout;
  sout << filename << ".tif";
  //printf("[Utils] Writing output cube %s\n", sout.str().c_str());
  typedef itk::ImageFileWriter< OutputImageType > WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(sout.str().c_str());
  writer->SetInput(importFilter->GetOutput());
  writer->Update();
#else
  assert(0);
#endif
}

void exportNRRDCube(uint* rawData,
                    const char* filename,
                    int cubeDepth,
                    int cubeHeight,
                    int cubeWidth)
{
  // import data to an itk image
  const int dimension = 3;
  typedef uint TInputPixelType;
  typedef itk::Image< TInputPixelType, dimension > InputImageType;
  typedef itk::Image< TInputPixelType, dimension > OutputImageType;
  typedef itk::ImportImageFilter< TInputPixelType, dimension > ImportFilterType;
  ImportFilterType::Pointer importFilter = ImportFilterType::New();
		
  ImportFilterType::SizeType size;
  size[0] = cubeWidth;
  size[1] = cubeHeight;
  size[2] = cubeDepth;
		
  ImportFilterType::IndexType start;
  start.Fill(0);
		
  ImportFilterType::RegionType region;
  region.SetIndex(start);
  region.SetSize(  size  );
		
  importFilter->SetRegion( region );	
		
  InputImageType::PointType origin;
  origin.Fill(0.0);
		
  importFilter->SetOrigin( origin );
				
  ImportFilterType::SpacingType spacing;
  spacing.Fill(1.0);
		
  importFilter->SetSpacing( spacing );
  importFilter->SetImportPointer(rawData, 0, false);

  stringstream sout;
  sout << filename;
  int n = strlen(filename);
  if(n < 5 || strcmp(filename+n-5,".nrrd")!=0)
     sout << ".nrrd";
  //printf("[Utils] Writing output cube %s\n", sout.str().c_str());
  typedef itk::ImageFileWriter< OutputImageType > WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(sout.str().c_str());
  writer->SetInput(importFilter->GetOutput());
  writer->Update();
}

void exportNRRDCube(uchar* rawData,
                    const char* filename,
                    int cubeDepth,
                    int cubeHeight,
                    int cubeWidth)
{
  // import data to an itk image
  const int dimension = 3;
  typedef uchar TInputPixelType;
  typedef itk::Image< TInputPixelType, dimension > InputImageType;
  typedef itk::Image< TInputPixelType, dimension > OutputImageType;
  typedef itk::ImportImageFilter< TInputPixelType, dimension > ImportFilterType;
  ImportFilterType::Pointer importFilter = ImportFilterType::New();
		
  ImportFilterType::SizeType size;
  size[0] = cubeWidth;
  size[1] = cubeHeight;
  size[2] = cubeDepth;
		
  ImportFilterType::IndexType start;
  start.Fill(0);
		
  ImportFilterType::RegionType region;
  region.SetIndex(start);
  region.SetSize(  size  );
		
  importFilter->SetRegion( region );	
		
  InputImageType::PointType origin;
  origin.Fill(0.0);
		
  importFilter->SetOrigin( origin );
				
  ImportFilterType::SpacingType spacing;
  spacing.Fill(1.0);
		
  importFilter->SetSpacing( spacing );
  importFilter->SetImportPointer(rawData, 0, false);

  stringstream sout;
  sout << filename;
  int n = strlen(filename);
  if(n < 5 || strcmp(filename+n-5,".nrrd")!=0)
     sout << ".nrrd";
  //printf("[Utils] Writing output cube %s\n", sout.str().c_str());
  typedef itk::ImageFileWriter< OutputImageType > WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(sout.str().c_str());
  writer->SetInput(importFilter->GetOutput());
  writer->Update();
}

#endif // USE_ITK

void exportVIVACube(float* rawData,
                    const char* filename,
                    int cubeDepth,
                    int cubeHeight,
                    int cubeWidth)
{
  ofstream ofs(filename);
  ofs.write((char*)rawData,cubeDepth*cubeHeight*cubeWidth*sizeof(float));
  ofs.close();

  // NFO file used by VIVA
  char* nfoFilename = new char[strlen(filename)+5];
  sprintf(nfoFilename,"%s.nfo", filename);
  ofstream nfo(nfoFilename);
  nfo << "voxelDepth 0.1" << endl;
  nfo << "voxelHeight 0.1" << endl;
  nfo << "voxelWidth 0.1" << endl;
  nfo << "cubeDepth " << cubeDepth << endl;
  nfo << "cubeHeight " << cubeHeight << endl;
  nfo << "cubeWidth " << cubeWidth << endl;
  nfo << "x_offset 0" << endl;
  nfo << "y_offset 0" << endl;
  nfo << "z_offset 0" << endl;
  //nfo << "cubeFile " << getNameFromPath() << endl;
  nfo << "cubeFile " << filename << endl;
  nfo << "type float" << endl;
  nfo.close();
  delete[] nfoFilename;
}

void exportVIVACube(uchar* rawData,
                    const char* filename,
                    int cubeDepth,
                    int cubeHeight,
                    int cubeWidth)
{
  ofstream ofs(filename);
  ofs.write((char*)rawData,cubeDepth*cubeHeight*cubeWidth*sizeof(char));
  ofs.close();

  // NFO file used by VIVA    
  char* nfoFilename = new char[strlen(filename)+5];
  sprintf(nfoFilename,"%s.nfo", filename);

  ofstream nfo(nfoFilename);
  nfo << "voxelDepth 0.1" << endl;
  nfo << "voxelHeight 0.1" << endl;
  nfo << "voxelWidth 0.1" << endl;
  nfo << "cubeDepth " << cubeDepth << endl;
  nfo << "cubeHeight " << cubeHeight << endl;
  nfo << "cubeWidth " << cubeWidth << endl;
  nfo << "x_offset 0" << endl;
  nfo << "y_offset 0" << endl;
  nfo << "z_offset 0" << endl;
  //nfo << "cubeFile " << getNameFromPath() << endl;
  nfo << "cubeFile " << filename << endl;
  nfo << "type uchar" << endl;
  nfo.close();
  delete[] nfoFilename;
}

#ifdef USE_ITK
void importTIFCube(const char* imgFileName,
		   uchar*& outputData,
                   int& width,
                   int& height,
                   int& depth)
{
  const int Dimension = 3;
  typedef unsigned char PixelType;
  typedef itk::Image< PixelType, Dimension >   ImageType;
  typedef itk::ImageFileReader< ImageType >  ImageReaderType;
  typedef ImageType::RegionType RegionType; 

  ImageReaderType::Pointer imageReader = ImageReaderType::New();
  imageReader->SetFileName(imgFileName);
 
  //Load it
  ImageType *img = imageReader->GetOutput();
  img->SetBufferedRegion(img->GetLargestPossibleRegion());
  try {
    imageReader->Update();
  } catch( itk::ExceptionObject & excep ) {
    cout << "[Utils] Exception Caught !" << std::endl;
    cout << excep << std::endl;
    exit(-1);
  }

  ImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();
  ulong totalSize = size[0]*size[1]*size[2];
  outputData = new uchar[totalSize];
  memcpy(outputData,img->GetBufferPointer(),totalSize);

  width = size[0];
  height = size[1];
  depth = size[2];

  /*
  //ImageReader->GetOutput()->ReleaseDataFlagOn();
  ImageType *itkImg = imageReader->GetOutput();
  RegionType region = itkImg->GetRegion();
  int totalSize = region[0]*region[1]*region[2];

  outputData = new uchar[totalSize];
  memcpy(outputData,itkImg->GetOutput());
  */
}

void importTIFCube_noAllocation(const char* imgFileName,
                                uchar*& outputData,
                                int& width,
                                int& height,
                                int& depth)
{
  const int Dimension = 3;
  typedef unsigned char PixelType;
  typedef itk::Image< PixelType, Dimension >   ImageType;
  typedef itk::ImageFileReader< ImageType >  ImageReaderType;
  typedef ImageType::RegionType RegionType; 

  ImageReaderType::Pointer imageReader = ImageReaderType::New();
  imageReader->SetFileName(imgFileName);
 
  //Load it
  ImageType *img = imageReader->GetOutput();
  img->SetBufferedRegion(img->GetLargestPossibleRegion());
  try {
    imageReader->Update();
  } catch( itk::ExceptionObject & excep ) {
    cout << "[Utils] Exception Caught !" << std::endl;
    cout << excep << std::endl;
    exit(-1);
  }

  ImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();
  ulong totalSize = size[0]*size[1]*size[2];
  memcpy(outputData,img->GetBufferPointer(),totalSize);

  width = size[0];
  height = size[1];
  depth = size[2];
}

void importCube(const char* imgFileName,
                uchar*& outputData,
                int& width,
                int& height,
                int& depth)
{
  const int Dimension = 3;
  //typedef unsigned char PixelType;
  typedef float PixelType;
  typedef itk::Image< PixelType, Dimension >   ImageType;
  typedef itk::ImageFileReader< ImageType >  ImageReaderType;
  typedef ImageType::RegionType RegionType; 

  ImageReaderType::Pointer imageReader = ImageReaderType::New();
  imageReader->SetFileName(imgFileName);
 
  //Load it
  ImageType *img = imageReader->GetOutput();
  img->SetBufferedRegion(img->GetLargestPossibleRegion());
  try  {
    imageReader->Update();
  }
  catch( itk::ExceptionObject & excep ) {
    cout << "[Utils] Exception Caught !" << std::endl;
    cout << excep << std::endl;
    exit(-1);
  }

  ImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();
  ulong totalSize = size[0]*size[1]*size[2];
  outputData = new uchar[totalSize];

  PixelType* ptrData = img->GetBufferPointer();
  double minValue=-0.1;
  double maxValue=0.1;
  for(ulong i = 0; i < totalSize; i++)
    {
      if(ptrData[i] < minValue)
	minValue=ptrData[i];
      if(ptrData[i]>maxValue)
	maxValue = ptrData[i];
    }

  //printf("[utils] Range=(%g,%g)\n", minValue, maxValue);

  /*

  //TODO : Create vector and store all the elements in that vector.

  ulong idx_min = 0.01*nPixels;
  ulong idx_max = 0.99* nPixels;
  for(int i=0;i<fvSize;i++)
    {  
      nth_element(features[i].begin(), features[i].begin()+idx_min, features[i].end());
      minValue = features[i][idx_min];

      nth_element(features[i].begin(), features[i].begin()+idx_max, features[i].end());
      maxValue= features[i][idx_max];
    }
  */

  double scale = (255.0 / (double)maxValue);

  for(ulong i=0; i < totalSize; i++) {
    outputData[i] = (ptrData[i] - minValue)*scale; 
  }

  width = size[0];
  height = size[1];
  depth = size[2];

  /*
  //ImageReader->GetOutput()->ReleaseDataFlagOn();
  ImageType *itkImg = imageReader->GetOutput();
  RegionType region = itkImg->GetRegion();
  int totalSize = region[0]*region[1]*region[2];

  outputData = new uchar[totalSize];
  memcpy(outputData,itkImg->GetOutput());
  */
}

void importNRRDCube_uint(const char* imgFileName,
                         uint*& outputData,
                         int& width,
                         int& height,
                         int& depth)
{
#if 1
  const int Dimension = 3;
  typedef unsigned int PixelType;
  typedef itk::Image< PixelType, Dimension >   ImageType;
  typedef itk::ImageFileReader< ImageType >  ImageReaderType;
  typedef ImageType::RegionType RegionType; 

  ImageReaderType::Pointer imageReader = ImageReaderType::New();
  imageReader->SetFileName(imgFileName);
 
  //Load it
  ImageType *img = imageReader->GetOutput();
  img->SetBufferedRegion(img->GetLargestPossibleRegion());
  try 
    {
      imageReader->Update();
    }
  catch( itk::ExceptionObject & excep )
    {
      cout << "[Utils] Exception Caught in importNRRDCube_uint !" << std::endl;
      cout << excep << std::endl;
      exit(-1);
    }

  ImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();
  ulong totalSize = size[0]*size[1]*size[2];
  outputData = new uint[totalSize];

  PixelType* ptrData = img->GetBufferPointer();

  /*
  double minValue=-0.1;
  double maxValue=0.1;
  for(ulong i = 0; i < totalSize; i++)
    {
      if(ptrData[i] < minValue)
	minValue=ptrData[i];
      if(ptrData[i]>maxValue)
	maxValue = ptrData[i];
    }

  double scale = (255.0 / (double)maxValue);

  for(ulong i=0; i < totalSize; i++) {
    outputData[i] = (ptrData[i] - minValue)*scale; 
  }
  */

  for(ulong i=0; i < totalSize; i++) {
    outputData[i] = ptrData[i];
  }

  width = size[0];
  height = size[1];
  depth = size[2];

#else

  typedef unsigned int PixelType;
  typedef itk::Image<PixelType, 3> ItkImageType;

  itk::ImageFileReader<ItkImageType>::Pointer reader = itk::ImageFileReader<ItkImageType>::New();
  printf("Loading %s\n", imgFileName);
  reader->SetFileName(imgFileName);
  reader->Update();
  printf("HERE\n");

  ItkImageType::Pointer img = reader->GetOutput();

  ItkImageType::IndexType index;
  index[0] = index[1] = index[2] = 0;

  //PixelType *mData = &img->GetPixel( index );
  outputData = &img->GetPixel( index );

  ItkImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();
  width = size[0];
  height = size[1];
  depth = size[2];


#endif
}

#endif

/**
 * true positive (TP) : eqv. with hit
 * true negative (TN) : eqv. with correct rejection
 * false positive (FP) : eqv. with false alarm, Type I error
 * false negative (FN) : eqv. with miss, Type II error
 * sensitivity or true positive rate (TPR) : eqv. with hit rate, recall
 * TPR = TP / P = TP / (TP + FN)
 * false positive rate (FPR) : eqv. with fall-out
 * FPR = FP / N = FP / (FP + TN)
 * accuracy (ACC) : ACC = (TP + TN) / (P + N)
 * 
 * Source : http://en.wikipedia.org/wiki/Receiver_operating_characteristic
 */
void compareVolumes(uchar* annotationData,
                    uchar* data,
                    int width,
                    int height,
                    int depth,
                    float& true_neg,
                    float& true_pos,
                    float& false_neg,
                    float& false_pos,
                    bool normalize,
                    bool useColorAnnotations,
                    ulong* TP,
                    ulong* TN)
{
  /*
  int POS_VALUE = FOREGROUND_MASKVALUE;
  if(useColorAnnotations)
    POS_VALUE = FOREGROUND_ADVANCED_MASKVALUE;
  int NEG_VALUE = BACKGROUND_MASKVALUE;
  */

  const int POS_VALUE = 255;
  const int NEG_VALUE = 0;

  ulong total_pos = 0;
  ulong total_neg = 0;
  unsigned long int itrue_pos = 0;
  unsigned long int itrue_neg = 0;
  unsigned long int ifalse_pos = 0;
  unsigned long int ifalse_neg = 0;

  uchar annotation_value;
  int idx = 0;
  for(int z = 0;z < depth; z++)
    {
      for(int y = 0;y < height; y++)
        for(int x = 0;x < width; x++)
          {
            annotation_value = annotationData[idx];

            if(annotation_value != NEG_VALUE)
              {
                // positive annotation
                total_pos++;
                
                if(data[idx] == POS_VALUE)
                  itrue_pos++; // correctly predicted
                else
                  ifalse_neg++; // wrongly predicted
              }
            else
              {
                // negative annotation
                total_neg++;
                
                if(data[idx] == POS_VALUE)
                  ifalse_pos++; // wrongly predicted
                else
                  itrue_neg++; // correctly predicted
              }

            idx++;
          }
    }

  /*
  uchar annotation_value;
  int idx = 0;
  for(int z = 0;z < depth; z++)
    {
      for(int y = 0;y < height; y++)
        for(int x = 0;x < width; x++)
          {
            annotation_value = annotationData[idx];

            //if(annotation_value != NEG_VALUE)
            if(annotation_value == POS_VALUE)
              {
                // positive annotation
                total_pos++;
                
                //if(data[idx] == POS_VALUE)
                if(data[idx] != NEG_VALUE)
                  itrue_pos++; // correctly predicted
                else
                  ifalse_neg++; // wrongly predicted
              }
            else
              {
                // negative annotation
                total_neg++;
                
                //if(data[idx] == POS_VALUE)
                if(data[idx] != NEG_VALUE)
                  ifalse_pos++; // wrongly predicted
                else
                  itrue_neg++; // correctly predicted
              }

            idx++;
          }
    }
  */  

  /*
  printf("[Util] total_pos=%d total_neg=%d total=%d=%d ? TP=%lu FN=%lu FP=%lu\n",
         total_pos, total_neg,
         total_pos+total_neg,width*height*depth,
         itrue_pos, ifalse_neg, ifalse_pos);
  */

  if(normalize)
    {
      if(total_pos != 0)
        {
          true_pos = itrue_pos*(100.0f/total_pos); // TPR = TP / P
          false_neg = ifalse_neg*(100.0f/total_pos); // FNR = FN / P
        }
      if(total_neg != 0)
        {
          false_pos = ifalse_pos*(100.0f/total_neg); // FPR = FP / N
          true_neg = itrue_neg*(100.0f/total_neg); // TNR = TN / N
        }
    }
  else
    {
      true_pos = itrue_pos;
      true_neg = itrue_neg;
      false_neg = ifalse_neg;
      false_pos = ifalse_pos;
    }

  if(TP)
    *TP = total_pos;
  if(TN)
    *TN = total_neg;
}

void compareMultiLabelVolumes(Slice_P& slice_GT,
                              const labelType* labels,
                              const int class_label,
                              float& true_neg,
                              float& true_pos,
                              float& false_neg,
                              float& false_pos,
                              bool normalize,
                              bool useColorAnnotations,
                              ulong* TP,
                              ulong* TN)
{
  ulong total_pos = 0;
  ulong total_neg = 0;
  unsigned long int itrue_pos = 0;
  unsigned long int itrue_neg = 0;
  unsigned long int ifalse_pos = 0;
  unsigned long int ifalse_neg = 0;

  uchar annotation_value;
  sidType sid;
  const map<int, supernode* >& _supernodes = slice_GT.getSupernodes();
  for(map<int, supernode* >::const_iterator its = _supernodes.begin();
      its != _supernodes.end(); its++)
    {
      sid = its->first;
      annotation_value = its->second->getLabel();
      uint count = its->second->size();

      if(annotation_value == class_label)
        {
          // positive annotation
          total_pos += count;

          if(labels[sid] == class_label)
            i