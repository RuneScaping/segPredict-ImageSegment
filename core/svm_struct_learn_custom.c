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
                                   double *dfy, double* loss, const double dfy_weight)
{
  int _sizePsi = sm->sizePsi + 1;
  double _loss;
  compute_psi(sparm, sm, ex, y_bar, y_direct, gparm, fy_to, fy_away, &_loss);
  if(loss) {
    *loss = _loss;
  }

  compute_gradient_accumulate(sm, gparm, fy_to, fy_away, dfy, _loss, dfy_weight);

#if CUSTOM_VERBOSITY > 3
  write_vector("dfy.txt", dfy, _sizePsi);
#endif

  double dscore = 0;
  // do not add +1 here as dfy also has an additional dummy entry at index 0.
  double* smw = sm->w;
  for(int i = 0; i < _sizePsi; ++i) {
    dscore += smw[i]*dfy[i];
  }
  return dscore;
}

double compute_gradient(STRUCT_LEARN_PARM *sparm, STRUCTMODEL *sm,
                        EXAMPLE* ex, LABEL* y_bar, LABEL* y_direct,
                        GRADIENT_PARM* gparm, SWORD* fy_to, SWORD* fy_away,
                        double *dfy, double* loss, const double dfy_weight)
{
  // initialize dfy to 0
  int _sizePsi = sm->sizePsi + 1;
  for(int i = 0; i < _sizePsi; ++i) {
    dfy[i] = 0;
  }

  return compute_gradient_accumulate(sparm, sm, ex, y_bar, y_direct, gparm, fy_to, fy_away, dfy, loss, dfy_weight);
}

double compute_gradient(STRUCTMODEL *sm, GRADIENT_PARM* gparm,
                        SWORD* fy_to, SWORD* fy_away, double *dfy,
                        const double loss, const double dfy_weight)
{
  // initialize dfy to 0
  int _sizePsi = sm->sizePsi + 1;
  for(int i = 0; i < _sizePsi; ++i) {
    dfy[i] = 0;
  }

  compute_gradient_accumulate(sm, gparm, fy_to, fy_away, dfy, loss, dfy_weight);

  double dscore = 0;
  // do not add +1 here as dfy also has an additional dummy entry at index 0.
  double* smw = sm->w;
  for(int i = 0; i < _sizePsi; ++i) {
    dscore += smw[i]*dfy[i];
  }
  return dscore;
}

void exportLabels(STRUCT_LEARN_PARM *sparm, EXAMPLE* ex,
                  LABEL* y, const char* dir_name)
{
  string paramSlice3d;
  Config::Instance()->getParameter("slice3d", paramSlice3d);
  bool useSlice3d = paramSlice3d.c_str()[0] == '1';
  string paramVOC;
  Config::Instance()->getParameter("voc", paramVOC);
  bool useVOC = paramVOC.c_str()[0] == '1';

  stringstream ss_dir;
  ss_dir << dir_name;
  mkdir(ss_dir.str().c_str(), 0777);
  if(!useSlice3d) {
    //TODO : Remove !useVOC
    if(useVOC) {
      ss_dir << "x" << sparm->iterationId;
    }
    else {
      ss_dir << "x" << sparm->iterationId << "/";
    }
  }
  mkdir(ss_dir.str().c_str(), 0777);

  stringstream soutColoredImage;
  soutColoredImage << ss_dir.str();
  if(useSlice3d) {
    soutColoredImage << getNameFromPathWithoutExtension(ex->x.slice->getName());
    soutColoredImage << "_";
    soutColoredImage << sparm->iterationId;
  } else {
    soutColoredImage << ex->x.slice->getName();
  }

  ex->x.slice->exportSupernodeLabels(soutColoredImage.str().c_str(),
                                     sparm->nClasses,
                                     y->nodeLabels,
                                     y->nNodes,
                                     &(sparm->labelToClassIdx));

  if(useSlice3d) {
    zipAndDeleteCube(soutColoredImage.str().c_str());
  }
}

double do_gradient_step(STRUCT_LEARN_PARM *sparm,
                        STRUCTMODEL *sm, EXAMPLE *ex, long nExamples,
                        GRADIENT_PARM* gparm,
                        double* momentum, double& dscore, LABEL* y_bar)
{
  int _sizePsi = sm->sizePsi + 1;
  SWORD* fy_to = new SWORD[_sizePsi];
  SWORD* fy_away = new SWORD[_sizePsi];
  double* dfy = new double[_sizePsi];
  memset((void*)dfy, 0, sizeof(double)*(_sizePsi));

  double m = do_gradient_step(sparm, sm, ex, nExamples, gparm,
                              momentum, fy_to, fy_away, dfy, dscore, y_bar);
  delete[] fy_to;
  delete[] fy_away;
  delete[] dfy;
  return m;
}

double compute_gradient_with_history(STRUCT_LEARN_PARM *sparm, STRUCTMODEL *sm,
                                     EXAMPLE* ex,
                                     GRADIENT_PARM* gparm, SWORD* fy_to,
                                     double *dfy, double* loss)
{
  ConstraintSet* cs = ConstraintSet::Instance();
  const vector< constraint >* constraints = cs->getConstraints(ex->x.id);
  assert(constraints != 0);
  int n_cs = constraints->size();

  double* dfy_weights = new double[n_cs];
  if(gparm->use_random_weights) {
    double total_weights = 0;
    for(int c = 0; c < n_cs; ++c) {
      dfy_weights[c] = rand() * ((double)n_cs/(double)RAND_MAX);
      total_weights += dfy_weights[c];
    }
    for(int c = 0; c < n_cs; ++c) {
      dfy_weights[c] /= total_weights;
    }
  } else {
    double total_weights = 0;
    for(int c = 0; c < n_cs; ++c) {
      dfy_weights[c] = 1.0/(double)(n_cs);
      total_weights += dfy_weights[c];
    }
    for(int c = 0; c < n_cs; ++c) {
      dfy_weights[c] /= total_weights;
    }
  }

  int _sizePsi = sm->sizePsi + 1;
  // initialize dfy to 0
  for(int i = 0; i < _sizePsi; ++i) {
    dfy[i] = 0;
  }

  if(loss) {
    *loss = 0;
  }

  // add gradient for history of constraints
  if(gparm->loss_type != HINGE_LOSS && gparm->loss_type != SQUARE_HINGE_LOSS) {
    // use all the constraints in the set
    int c = 0;
    for(vector<constraint>::const_iterator it = constraints->begin();
        it != constraints->end(); ++it) {
      compute_gradient_accumulate(sm, gparm, fy_to,
                                  it->first->w, dfy, it->first->loss, dfy_weights[c]);
      if(loss) {
        *loss += it->first->loss;
      }      
      ++c;
    }
  } else {
    // only use violated constraints

    double score_gt = computeScore(sm, fy_to);
    int c = 0;
    for(vector<constraint>::const_iterator it = constraints->begin();
        it != constraints->end(); ++it) {
      // check if constraint is violated
      double score_cs = computeScore(sm, it->first->w);
      bool positive_margin = (score_cs - score_gt + it->first->loss) > 0;
      //printf("Margin constraint %d: score_cs = %g, score_gt = %g, loss = %g, margin = %g\n",
      //       c, score_cs, score_gt, it->first->loss, score_cs - score_gt + it->first->loss);

      if(positive_margin) {
        compute_gradient_accumulate(sm, gparm, fy_to,
                                    it->first->w, dfy, it->first->loss, dfy_weights[c]);
        if(loss) {
          *loss += it->first->loss;
        }
      }     
      ++c;
    }
  }

  double total_dscore = 0;
  // do not add +1 here as dfy also has an additional dummy entry at index 0.
  double* smw = sm->w;
  for(int i = 0; i < _sizePsi; ++i) {
    total_dscore += smw[i]*dfy[i];
  }

  delete[] dfy_weights;

  return total_dscore;
}

double compute_gradient_with_history(STRUCT_LEARN_PARM *sparm, STRUCTMODEL *sm,
                                     EXAMPLE* ex, LABEL* y_bar, LABEL* y_direct,
                                     GRADIENT_PARM* gparm, SWORD* fy_to, SWORD* fy_away,
                                     double *dfy, double* loss)
{
  double dfy_weight = 1.0;
  int _sizePsi = sm->sizePsi + 1;
  // initialize dfy to 0
  for(int i = 0; i < _sizePsi; ++i) {
    dfy[i] = 0;
  }

  double _loss;
  double _dscore = compute_gradient(sparm, sm, ex, y_bar, y_direct, gparm, fy_to,
                                    fy_away, dfy, &_loss, dfy_weight);
  if(loss) {
    *loss += _loss;
  }

  // add gradient for history of constraints
  ConstraintSet* cs = ConstraintSet::Instance();
  const vector< constraint >* constraints = cs->getConstraints(ex->x.id);
  if(constraints) {
    dfy_weight = 1.0/(double)(constraints->size()+1.0);
    for(vector<constraint>::const_iterator it = constraints->begin();
        it != constraints->end(); ++it) {
      compute_gradient_accumulate(sm, gparm, fy_to,
                                  it->first->w, dfy, it->first->loss, dfy_weight);
      if(loss) {
        *loss += it->first->loss;
      }
    }
  }

  double total_dscore = 0;
  // do not add +1 here as dfy also has an additional dummy entry at index 0.
  double* smw = sm->w;
  for(int i = 0; i < _sizePsi; ++i) {
    total_dscore += smw[i]*dfy[i];
  }
  total_dscore += _dscore;
  return total_dscore;
}

void update_w(STRUCT_LEARN_PARM *sparm, STRUCTMODEL *sm, GRADIENT_PARM* gparm,
              double* momentum, double *dfy)
{
  int _sizePsi = sm->sizePsi + 1;

  // do not add +1 here as dfy also has an additional dummy entry at index 0.
  double* smw = sm->w;
  if(momentum) {
    // update momentum
    for(int i = 1; i < _sizePsi; ++i) {
      momentum[i] = (gparm->learning_rate*(dfy[i] + (gparm->regularization_weight*smw[i])) + gparm->momentum_weight*momentum[i]);
    }
    for(int i = 1; i < _sizePsi; ++i) {
      smw[i] -= momentum[i];
    }
  } else {
    for(int i = 1; i < _sizePsi; ++i) {
      smw[i] -= gparm->learning_rate*(dfy[i]+(gparm->regularization_weight*smw[i]));
    }
  }
}

double do_gradient_step(STRUCT_LEARN_PARM *sparm,
                        STRUCTMODEL *sm, EXAMPLE *ex, long nExamples,
                        GRADIENT_PARM* gparm,
                        double* momentum,
                        SWORD* fy_to, SWORD* fy_away, double *dfy,
                        double& dscore,
                        LABEL* y_bar)
{
  int _sizePsi = sm->sizePsi + 1;
  LABEL* y_direct = 0;

  double* _lossPerLabel = sparm->lossPerLabel;
  if(gparm->ignore_loss) {
    sparm->lossPerLabel = 0;
  }

  // setting this to 1 will make the example loop below single thread so that
  // several threads can be run for different temperature while running the
  // samplign code.
#define USE_SAMPLING 0

#if USE_SAMPLING
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
#endif

  /*** precomputation step ***/
  for(int i = 0; i < nExamples; i++) {

#if USE_SAMPLING
#ifdef USE_OPENMP
    int threadId = omp_get_thread_num();
    printf("[svm_struct_custom] Thread %d/%d\n", threadId,omp_get_num_threads());
#endif
#endif

    if(sparm->loss_type == SLACK_RESCALING) {
      y_bar[i] = find_most_violated_constraint_slackrescaling(ex[i].x, ex[i].y,
                                                             sm, sparm);
    } else {
      y_bar[i] = find_most_violated_constraint_marginrescaling(ex[i].x, ex[i].y,
                                                              sm, sparm);
    }
  }

  if(gparm->ignore_loss) {
    sparm->lossPerLabel = _lossPerLabel;
  }

  if(gparm->gradient_type == GRADIENT_DIRECT_ADD ||
     gparm->gradient_type == GRADIENT_DIRECT_SUBTRAC