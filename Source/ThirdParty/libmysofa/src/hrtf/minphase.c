/*
 * loudness.c
 *
 *  Created on: 17.01.2017
 *      Author: hoene
 */

#include "mysofa.h"
#include "mysofa_export.h"
#include "tools.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void trunk(float *in, int size, int *start, int *end, float threshold) {
  float energy = 0;
  int s = 0;
  int e = size - 1;
  float ss, ee;

  float l = loudness(in, size);
  threshold = threshold * l;

  ss = in[s] * in[s];
  ee = in[e] * in[e];
  while (s < e) {
    if (ss <= ee) {
      if (energy + ss > threshold)
        break;
      energy += ss;
      s++;
      ss = in[s] * in[s];
    } else {
      if (energy + ee > threshold)
        break;
      energy += ee;
      e--;
      ee = in[e] * in[e];
    }
  }
  *start = s;
  *end = e + 1;
}

MYSOFA_EXPORT int mysofa_minphase(struct MYSOFA_HRTF *hrtf, float threshold) {
  int i;
  int max = 0;
  int filters;
  int *start;
  int *end;
  float samplerate;
  float d[2];

  if (hrtf->DataDelay.elements != 2)
    return -1;

  filters = hrtf->M * hrtf->R;
  start = malloc(filters * sizeof(int));
  end = malloc(filters * sizeof(int));

  /*
   * find maximal length of a filter
   */
  for (i = 0; i < filters; i++) {
    trunk(hrtf->DataIR.values + i * hrtf->N, hrtf->N, start + i, end + i,
          threshold);
    if (end[i] - start[i] > max)
      max = end[i] - start[i];
  }

  if (max == hrtf->N) {
    free(start);
    free(end);
    return max;
  }

  /*
   * update delay and filters
   */
  samplerate = hrtf->DataSamplingRate.values[0];
  d[0] = hrtf->DataDelay.values[0];
  d[1] = hrtf->DataDelay.values[1];
  hrtf->DataDelay.elements = filters;
  hrtf->DataDelay.values =
      realloc(hrtf->DataDelay.values, sizeof(float) * filters);
  for (i = 0; i < filters; i++) {
    if (start[i] + max > hrtf->N)
      start[i] = hrtf->N - max;
    hrtf->DataDelay.values[i] = d[i % 1] + (start[i] / samplerate);
    memmove(hrtf->DataIR.values + i * max,
            hrtf->DataIR.values + i * hrtf->N + start[i], max * sizeof(float));
  }

  /*
   * update hrtf structure
   */
  hrtf->N = max;
  hrtf->DataIR.elements = max * filters;
  hrtf->DataIR.values =
      realloc(hrtf->DataIR.values, sizeof(float) * hrtf->DataIR.elements);

  free(start);
  free(end);
  return max;
}
