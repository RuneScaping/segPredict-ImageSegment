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

#include "gi_sampling.h"

// SliceMe
#include "Config.h"
#include "utils.h"

#include "inference_globals.h"

#if USE_GSL_DEBUG
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#else
#ifdef _WIN32
#include "gettimeofday.h"
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#endif

//------------------------------------------------------------------------------

#if USE_GSL_DEBUG
#define GI_SAMPLING_GET_RAND_UNIFORM gsl_rng_uniform(rng);
#define GI_SAMPLING_GET_RAND(n) gsl_rng_uniform_int(rng, n);
#else
#define GI_SAMPLING_GET_RAND_UNIFORM rand() / (double)RAND_MAX;
#define GI_SAMPLING_GET_RAND(n) rand() * ((double)n/(double)RAND_MAX);
#endif


//---------------------