/*
 * easy.c
 *
 *  Created on: 18.01.2017
 *      Author: hoene
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mysofa.h"
#include "mysofa_export.h"

static struct MYSOFA_EASY *easy_processing(struct MYSOFA_HRTF *hrtf,
                                           float samplerate, int *filterlength,
                                           int *err, bool applyNorm,
                                           float neighbor_angle_step,
                                           float neighbor_radius_step) {
  if (!hrtf)
    return NULL;

  struct MYSOFA_EASY *easy =
      (struct MYSOFA_EASY *)malloc(sizeof(struct MYSOFA_EASY));

  if (!easy) {
    *err = MYSOFA_NO_MEMORY;
    mysofa_free(hrtf);
    return NULL;
  }

  // set all values of struct to their default "0" (to avoid freeing unallocated
  // values in mysofa_free)
  *easy = (struct MYSOFA_EASY){0};
  easy->hrtf = hrtf;

  *err = mysofa_check(easy->hrtf);
  if (*err != MYSOFA_OK) {
    mysofa_close(easy);
    return NULL;
  }

  *err = mysofa_resample(easy->hrtf, samplerate);
  if (*err != MYSOFA_OK) {
    mysofa_close(easy);
    return NULL;
  }

  if (applyNorm) {
    mysofa_loudness(easy->hrtf);
  }

  /* does not sound well:
   mysofa_minphase(easy->hrtf,0.01);
   */

  mysofa_tocartesian(easy->hrtf);

  if (easy->hrtf->SourcePosition.elements != easy->hrtf->C * easy->hrtf->M) {
    *err = MYSOFA_INVALID_FORMAT;
    mysofa_close(easy);
    return NULL;
  }

  easy->lookup = mysofa_lookup_init(easy->hrtf);
  if (easy->lookup == NULL) {
    *err = MYSOFA_INTERNAL_ERROR;
    mysofa_close(easy);
    return NULL;
  }

  easy->neighborhood = mysofa_neighborhood_init_withstepdefine(
      easy->hrtf, easy->lookup, neighbor_angle_step, neighbor_radius_step);

  *filterlength = easy->hrtf->N;

  easy->fir = (float *)malloc(easy->hrtf->N * easy->hrtf->R * sizeof(float));
  assert(easy->fir);

  return easy;
}

MYSOFA_EXPORT struct MYSOFA_EASY *mysofa_open(const char *filename,
                                              float samplerate,
                                              int *filterlength, int *err) {
  return easy_processing(mysofa_load(filename, err), samplerate, filterlength,
                         err, true, MYSOFA_DEFAULT_NEIGH_STEP_ANGLE,
                         MYSOFA_DEFAULT_NEIGH_STEP_RADIUS);
}

MYSOFA_EXPORT struct MYSOFA_EASY *mysofa_open_no_norm(const char *filename,
                                                      float samplerate,
                                                      int *filterlength,
                                                      int *err) {
  return easy_processing(mysofa_load(filename, err), samplerate, filterlength,
                         err, false, MYSOFA_DEFAULT_NEIGH_STEP_ANGLE,
                         MYSOFA_DEFAULT_NEIGH_STEP_RADIUS);
}

MYSOFA_EXPORT struct MYSOFA_EASY *
mysofa_open_advanced(const char *filename, float samplerate, int *filterlength,
                     int *err, bool norm, float neighbor_angle_step,
                     float neighbor_radius_step) {
  return easy_processing(mysofa_load(filename, err), samplerate, filterlength,
                         err, norm, neighbor_angle_step, neighbor_radius_step);
}

MYSOFA_EXPORT struct MYSOFA_EASY *mysofa_open_data(const char *data, long size,
                                                   float samplerate,
                                                   int *filterlength,
                                                   int *err) {
  return easy_processing(
      mysofa_load_data(data, size, err), samplerate, filterlength, err, true,
      MYSOFA_DEFAULT_NEIGH_STEP_ANGLE, MYSOFA_DEFAULT_NEIGH_STEP_RADIUS);
}

MYSOFA_EXPORT struct MYSOFA_EASY *
mysofa_open_data_no_norm(const char *data, long size, float samplerate,
                         int *filterlength, int *err) {
  return easy_processing(
      mysofa_load_data(data, size, err), samplerate, filterlength, err, false,
      MYSOFA_DEFAULT_NEIGH_STEP_ANGLE, MYSOFA_DEFAULT_NEIGH_STEP_RADIUS);
}

MYSOFA_EXPORT struct MYSOFA_EASY *mysofa_open_data_advanced(
    const char *data, long size, float samplerate, int *filterlength, int *err,
    bool norm, float neighbor_angle_step, float neighbor_radius_step) {
  return easy_processing(mysofa_load_data(data, size, err), samplerate,
                         filterlength, err, norm, neighbor_angle_step,
                         neighbor_radius_step);
}

MYSOFA_EXPORT struct MYSOFA_EASY *mysofa_open_cached(const char *filename,
                                                     float samplerate,
                                                     int *filterlength,
                                                     int *err) {
  struct MYSOFA_EASY *res = mysofa_cache_lookup(filename, samplerate);
  if (res) {
    *filterlength = res->hrtf->N;
    *err = MYSOFA_OK;
    return res;
  }
  res = mysofa_open(filename, samplerate, filterlength, err);
  if (res) {
    res = mysofa_cache_store(res, filename, samplerate);
  }
  return res;
}

MYSOFA_EXPORT void mysofa_getfilter_short(struct MYSOFA_EASY *easy, float x,
                                          float y, float z, short *IRleft,
                                          short *IRright, int *delayLeft,
                                          int *delayRight) {
  float c[3];
  float delays[2];
  float *fl;
  float *fr;
  int nearest;
  int *neighbors;
  unsigned int i;

  c[0] = x;
  c[1] = y;
  c[2] = z;
  nearest = mysofa_lookup(easy->lookup, c);
  assert(nearest >= 0);
  neighbors = mysofa_neighborhood(easy->neighborhood, nearest);

  mysofa_interpolate(easy->hrtf, c, nearest, neighbors, easy->fir, delays);

  *delayLeft = delays[0] * easy->hrtf->DataSamplingRate.values[0];
  *delayRight = delays[1] * easy->hrtf->DataSamplingRate.values[0];

  fl = easy->fir;
  fr = easy->fir + easy->hrtf->N;
  for (i = easy->hrtf->N; i > 0; i--) {
    *IRleft++ = (short)(*fl++ * 32767.);
    *IRright++ = (short)(*fr++ * 32767.);
  }
}

MYSOFA_EXPORT void mysofa_getfilter_float_advanced(
    struct MYSOFA_EASY *easy, float x, float y, float z, float *IRleft,
    float *IRright, float *delayLeft, float *delayRight, bool interpolate) {
  float c[3];
  float delays[2];
  float *fl;
  float *fr;
  int nearest;
  int *neighbors;
  int i;

  c[0] = x;
  c[1] = y;
  c[2] = z;
  nearest = mysofa_lookup(easy->lookup, c);
  assert(nearest >= 0);
  neighbors = mysofa_neighborhood(easy->neighborhood, nearest);

  // bypass interpolate by forcing current cooordinates to nearest's
  if (!interpolate) {
    memcpy(c, easy->hrtf->SourcePosition.values + nearest * easy->hrtf->C,
           sizeof(float) * easy->hrtf->C);
  }

  float *res =
      mysofa_interpolate(easy->hrtf, c, nearest, neighbors, easy->fir, delays);

  *delayLeft = delays[0];
  *delayRight = delays[1];

  fl = res;
  fr = res + easy->hrtf->N;
  for (i = easy->hrtf->N; i > 0; i--) {
    *IRleft++ = *fl++;
    *IRright++ = *fr++;
  }
}

MYSOFA_EXPORT void mysofa_getfilter_float(struct MYSOFA_EASY *easy, float x,
                                          float y, float z, float *IRleft,
                                          float *IRright, float *delayLeft,
                                          float *delayRight) {
  mysofa_getfilter_float_advanced(easy, x, y, z, IRleft, IRright, delayLeft,
                                  delayRight, true);
}

MYSOFA_EXPORT void
mysofa_getfilter_float_nointerp(struct MYSOFA_EASY *easy, float x, float y,
                                float z, float *IRleft, float *IRright,
                                float *delayLeft, float *delayRight) {
  mysofa_getfilter_float_advanced(easy, x, y, z, IRleft, IRright, delayLeft,
                                  delayRight, false);
}

MYSOFA_EXPORT void mysofa_close(struct MYSOFA_EASY *easy) {
  if (easy) {
    if (easy->fir)
      free(easy->fir);
    if (easy->neighborhood)
      mysofa_neighborhood_free(easy->neighborhood);
    if (easy->lookup)
      mysofa_lookup_free(easy->lookup);
    if (easy->hrtf)
      mysofa_free(easy->hrtf);
    free(easy);
  }
}

MYSOFA_EXPORT void mysofa_close_cached(struct MYSOFA_EASY *easy) {
  mysofa_cache_release(easy);
}
