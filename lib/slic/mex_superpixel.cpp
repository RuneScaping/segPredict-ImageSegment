//========================================================================
// This is a sample main function for using the implementation and header
// files LKM.cpp and LKM.h respectively
//========================================================================

#include <vector>
#include <string>
#include "LKM.h"
#include "mex.h"
#ifdef WINDOWS
#include "BMPhandler.h"
#else
#include <cv.h>
#include <highgui.h>
#include <fstream>
//#include "utils.h"
#endif

using namespace std;


string getNameFromPathWithoutExtension(string path){
  string nameWith =  path.substr(path.find_last_of("/\\")+1);
  string nameWithout = nameWith.substr(0,nameWith.find_last_of("."));
  return nameWithout;
}

//=================================================================================
/// DrawContoursAroundSegments
///
/// Internal contour drawing option exists. One only needs to comment the 'if'
/// statement inside the loop that looks at neighbourhood.
//=================================================================================
void DrawContoursAroundSegments(
                                UINT*			img,//contours will be drawn on this image
                                sidType*			labels,
                                const int&				width,
                                const int&				height,
                                const UINT&				color )
{
  const int dx8[8] = {-1, -1,  0,  1, 1, 1, 0, -1};
  const int dy8[8] = { 0, -1, -1, -1, 0, 1, 1,  1};

  int sz = width*height;

  vector<bool> istaken(sz, false);

  int mainindex(0);
  int cind(0);
  for( int j = 0; j < height; j++ )
    {
      for( int k = 0; k < width; k++ )
        {
          int np(0);
          for( int i = 0; i < 8; i++ )
            {
              int x = k + dx8[i];
              int y = j + dy8[i];

              if( (x >= 0 && x < width) && (y >= 0 && y < height) )
                {
                  int index = y*width + x;

                  if( false == istaken[index] )//comment this to obtain internal contours
                    {
                      if( labels[mainindex] != labels[index] ) np++;
                    }
                }
            }
          if( np > 1 )
            {
              istaken[mainindex] = true;
              img[mainindex] = color;
              cind++;
            }
          mainindex++;
        }
    }
}

#ifndef WINDOWS

//===========================================================================
///	SaveUINTBuffer
///
///	Provides UINT buffer after the necessary vertical flip and BRG to RGB ordering
//===========================================================================
void SaveImage(
               UINT*	ubuff,				// RGB buffer
               const int&			width,				// size
               const int&			height,
               const string&		fileName)			// filename to be given; even if whole path is given, it is still the filename that is used
{
  IplImage* img=cvCreateImage(cvSize(width,height),IPL_DEPTH_8U,3); 
  uchar* pValue;
  int idx = 0;

  for(int j=0;j<img->height;j++)
    for(int i=0;i<img->width;i++)
      {
        pValue = &((uchar*)(img->imageData + img->widthStep*(j)))[(i)*img->nChannels];
        pValue[0] = ubuff[idx] & 0xff;
        pValue[1] = (ubuff[idx] >> 8) & 0xff;
        pValue[2] = (ubuff[idx] >>16) & 0xff;
        idx++;
      }

  cvSaveImage(fileName.c_str(),img);
}

//===========================================================================
///	SaveUINTBuffer
///
///	Provides UINT buffer after the necessary vertical flip and BRG to RGB ordering
//===========================================================================
void SaveUINTBuffer(
                    UINT*	ubuff,				// RGB buffer
                    const int&			width,