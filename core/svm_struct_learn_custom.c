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

// This code is based on the template provided by Thorsten Joachims.

#include <iomanip>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#define mkdir(x,y) _mkdir(x)
#include "gettimeofday.h"
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include "constraint_set.h"
#include "label_cache.h"
#include "svm_struct_learn_custom.h"
#include "svm_struct_api.h"
#include "svm_light/svm_common.h"
#include "svm_struct/svm_struct_common.h"
#include "svm_struct/svm_struct_learn.h"
#include "svm_struct_globals.h"

#include "Config.h"

#include "highgui.h"

#include "energyParam.h"
#include "graphInference.h"
#include "inference.h"

//------------------------------------------------------------------------MACROS

#define BUFFER_SIZE 250

// If greater than 1, output dscore, norm(dfy), loss
// If greater than 2, output dfy
#define CUSTOM_VERBOSITY 3

#define CUSTOM_VERBOSITY_F(X, Y) if(CUSTOM_VERBOSITY > X) { Y }

//---------------------------------------------------------------------FUNCTIONS

void write_vector(const char* filename, double* v, int size_v)
{
  ofstream ofs(filename, ios::app);
  for(int i = 0; i < size_v; ++i) {
    ofs << v[i] << " ";
  }
  ofs << endl;
  ofs.close();
}

/**
 * Write vector to a file (don't overwrite but append new line).
 */
void write_vector(const char* filename, SWORD* v)
{
  ofstream ofs(filename, ios::app);
  SWORD* w = v;
  while (w->wnum) {
    ofs << w->weight << " ";
    ++w;
  }
  ofs << endl;
  ofs.close();
}

/**
 * Write set of scalar values to a file (don't overwrite but append new lines).
 */
void write_scalars(const char* filename, double* v, int size_v)
{
  ofstream ofs(filename, ios::app);
  for(int i = 0; i < size_v; ++i) {
    ofs << v[i] << endl;
  }
  ofs.close();
}

/**
 * Write scalar value to a file (don't overwrite but append new line).
 */
void write_scalar(const char* filename, double v)
{
  ofstream ofs(filename, ios::app);
  ofs << v << endl;
  ofs.close();
}

/**
 * Returns squared norm
 */
double get_sq_norm(double* v, int _sizePsi)
{
  double sq_norm_v = 0;
  for(int i = 1; i < _sizePsi; ++i) {
    sq_norm_v += v[i]*v[i];
  }
  return sq_norm_v;
}

double get_norm(double* v, int _sizePsi)
{
  double norm_v = 0;
  for(int i = 1; i < _sizePsi; ++i) {
    norm_v += v[i]*v[i];
  }
  return sqrt(norm_v);
}

/**
 * Compute the average norm of psi over the training data
 */
double get_norm_psi_gt(STRUCT_LEARN_PARM *sparm, STRUCTMODEL *sm, EXAMPLE *examples, long nExamples)
{
  int _sizePsi = sm->sizePsi + 1;
  SWORD* fy_to = new SWORD[_sizePsi];
  double avg_norm = 0;

  for(long i = 0; i < nExamples; ++i) {
    computePsi(fy_to, examples[i].x, examples[i].y, sm, sparm);
    double norm_wy_to = 0;
    SWORD* wy_to = fy_to;
    while (wy_to->wnum) {
      norm_wy_to += wy_to->weight*wy_to->weight;
      ++wy_to;
    }
    norm_wy_to = sqrt(norm_wy_to);
    avg_norm += norm_wy_to;
  }
  avg_norm /= nExamples;

  delete[] fy_to;
  return avg_norm;
}

/**
 * accumulate gradient in dfy
 */
void compute_gradient_accumulate(STRUCTMODEL *sm, GRADIENT_PARM* gparm,
                                 SWORD* fy_to, SWORD* fy_away, double *dfy,
                                 const double loss, const double dfy_weight)
{
#if CUSTOM_VERBOSITY > 2
  double score_y = 0;
  double score_y_away = 0;
#endif

  SWORD* wy_to = fy_to;
  SWORD* wy_away = fy_away;
  switch(gparm->loss_type)
    {
    case LOG_LOSS:
      {
        // L(w) = log(1+e(m(x)))
        // where m(x) = (loss(y,y_bar) + score(x,y_bar)) - score(x,y)
        // and score(x,y) = w^T*psi(x,y)
        // dL(w)/dw = ( m'(x) e(m(x)) ) / ( 1 + e(m(x)))
        // m'(x) = psi(x,y_bar) - psi(x,y)
        double m = 0;
        double dm;
        while (wy_to->wnum) {
          while(wy_away->wnum && (wy_away->wnum < wy_to->wnum)) {
            ++wy_away;
          }

          if(wy_to->wnum == wy_away->wnum) {
            dm = wy_away->weight - wy_to->weight;
          } else {
            dm = - wy_to->weight;
          }
          m += (sm->w[wy_to->wnum]*dm);
          ++wy_to;
        }
        m += loss;
        double e_m = 0;
        if(m < 100) {
          e_m = exp(m);
        }

        wy_to = fy_to;
        wy_away = fy_away;
        while (wy_to->wnum) {
          while(wy_away->wnum && (wy_away->wnum < wy_to->wnum)) {
            ++wy_away;
          }
          if(wy_to->wnum == wy_away->wnum) {
            dm = wy_away->weight - wy_to->weight;
          } else {
            dm = - wy_to->weight;
          }

          if(m >= 100) {
            dfy[wy_to->wnum] += dfy_weight * dm;
          } else {
            dfy[wy_to->wnum] += dfy_weight * (dm*e_m / (e_m + 1));
          }
#if CUSTOM_VERBOSITY > 2
          score_y += sm->w[wy_to->wnum]*wy_to->weight;
          score_y_away += sm->w[wy_to->wnum]*wy_away->weight;
#endif
          ++wy_to;
        }
      }
      break;
    case HINGE_LOSS:
      {
        // L(w) = (loss(y,y_bar) + score(x,y_bar)) - score(x,y)
        // where score(x,y) = w^T*psi(x,y)
        // dL(w)/dw = psi(x,y_bar) - psi(x,y)
        double dm;
        while (wy_to->wnum) {
          while(wy_away->wnum && (wy_away->wnum < wy_to->wnum)) {
            ++wy_away;
          }

          if(wy_to->wnum == wy_away->wnum) {
            dm = wy_away->weight - wy_to->weight;
          } else {
            dm = - wy_to->weight;
          }

          dfy[wy_to->wnum] += dfy_weight * dm;
#if CUSTOM_VERBOSITY > 2
          score_y += sm->w[wy_to->wnum]*wy_to->weight;
          score_y_away += sm->w[wy_to->wnum]*wy_away->weight;
#endif
          ++wy_to;
        }
      }
      break;
    case SQUARE_HINGE_LOSS:
      {
        // L(w) = log(1+e(m(x)))
        // where m(x) = (loss(y,y_bar) + score(x,y_bar)) - score(x,y)
        // and score(x,y) = w^T*psi(x,y)
        // dL(w)/dw = ( m'(x) e(m(x)) ) / ( 1 + e(m(x)))
        // m'(x) = psi(x,y_bar) - psi(x,y)
        double m = 0;
        double dm;
        while (wy_to->wnum) {
          while(wy_away->wnum && (wy_away->wnum < wy_to->wnum)) {
            ++wy_away;
          }

          if(wy_to->wnum == wy_away->wnum) {
            dm = wy_away->weight - wy_to->weight;
          } else {
            dm = - wy_to->weight;
          }
          m += (sm->w[wy_to->wnum]*dm);
          ++wy_to;
        }
        m += loss;

        wy_to = fy_to;
        wy_away = fy_away;
        while (wy_to->wnum) {
          while(wy_away->wnum && (wy_away->wnum < wy_to->wnum)) {
            ++wy_away;
          }
          if(wy_to->wnum == wy_away->wnum) {
            dm = wy_away->weight - wy_to->weight;
          } else {
            dm = - wy_to->weight;
          }

          dfy[wy_to->wnum] += 1e-30 * dfy_weight * dm * m;
#if CUSTOM_VERBOSITY > 2
          score_y += sm->w[wy_to->wnum]*wy_to->weight;
          score_y_away += sm->w[wy_to->wnum]*wy_away->weight;
#endif
          ++wy_to;
        }
      }
      break;
    default:
      printf("[svm_struct_custom] Unknown loss type %d\n", gparm->loss_type);
      exit(-1);
      break;
    }

#if CUSTOM_VERBOSITY > 2
  ofstream ofs_score_y("score_y.txt", ios::app);
  ofs_score_y << score_y << endl;
  ofs_score_y.close();

  ofstream ofs_score_y_away("score_y_away.txt", ios::app);
  ofs_score_y_away << score_y_away << endl;
  ofs_score_y_away.close();
#endif
}

void compute_psi(STRUCT_LEARN_PARM *sparm, STRUCTMODEL *sm,
                   EXAMPLE* ex, LABEL* y_bar, LABEL* y_direct,
                   GRADIENT_PARM* gparm, SWORD* fy_to, SWORD* fy_away,
                   double* loss)
{
  labelType* y_to = 0;
  labelType* y_away = 0;
  switch(gparm->gradient_type) {
  case GRADIENT_GT:
    // moves toward ground truth, away from larger loss
    y_to = ex->y.nodeLabels;
    y_away = y_bar->nodeLabels;
    computePsi(fy_to, ex->x, ex->y, sm, sparm);
    computePsi(fy_away, ex->x, *y_bar, sm, sparm);
    break;
  case GRADIENT_DIRECT_ADD:
    // moves away from larger loss
    y_to = y_direct->nodeLabels;
    y_away = y_bar->nodeLabels;
    computePsi(fy_to, ex->x, *y_direct, sm, sparm);
    computePsi(fy_away, ex->x, *y_bar, sm, sparm);
    break;
  case GRADIENT_DIRECT_SUBTRACT:
    // moves toward better label
    y_to = y_direct->nodeLabels;
    y_away = y_bar->nodeLabels;
    computePsi(fy_to, ex->x, *y_direct, sm, sparm);
    computePsi(fy_away, ex->x, *y_bar, sm, sparm);
    break;
  default:
    printf("[svm_struct_custom] Unknown gradient type\n");
    exit(-1);
    break;
  }

  if(!gparm->ignore_loss) {
    int nDiff;
    double _loss;
    computeLoss(y_to, y_away, ex->y.nNodes, sparm, _loss, nDiff);
    if(loss) {
      *loss = _loss;
    }
  }
}

void compute_psi_to(STRUCT_LEARN_PARM *sparm, STRUCTMODEL *sm,
                    EXAMPLE* ex, GRADIENT_PARM* gparm, SWORD* fy_to)
{
  switch(gparm->gradient_type) {
  case GRADIENT_GT:
    // moves toward ground truth, away from larger loss
    computePsi(fy_to, ex->x, ex->y, sm, sparm);
    break;
    /*
  case GRADIENT_DIRECT_ADD:
    // moves away from larger loss
    computePsi(fy_to, ex->x, *y_direct, sm, sparm);
    break;
  case GRADIENT_DIRECT_SUBTRACT:
    // moves toward better label
    computePsi(fy_to, ex->x, *y_direct, sm, sparm);
    break;
    */
  default:
    printf("[svm_struct_custom] Unknown gradient type\n");
    exit(-1);
    break;
  }
}

double compute_gradient_accumulate(STRUCT_LEARN_PARM *sparm, STRUCTMODEL *sm,
                                   EXAMPLE* ex, LABEL* y_bar, LABEL* y_direct,
                                   GRADIENT_PARM* gparm, SWORD* fy_to, SWORD* fy_away,
                                   double *dfy, doub