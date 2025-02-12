/*
 * loudness.c
 *
 *  Created on: 17.01.2017
 *      Author: hoene
 */

#include "../resampler/speex_resampler.h"
#include "mysofa.h"
#include "mysofa_export.h"
#include "tools.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MYSOFA_EXPORT float mysofa_loudness(struct MYSOFA_HRTF *hrtf) {
  float c[3], factor;
  float min = FLT_MAX;
  int radius = 0;
  unsigned int i, index = 0;
  int cartesian =
      verifyAttribute(hrtf->SourcePosition.attributes, "Type", "cartesian");

  /*
   * find frontal source position
   */
  for (i = 0; i + 2 < hrtf->SourcePosition.elements; i += hrtf->C) {
    c[0] = hrtf->SourcePosition.values[i];
    c[1] = hrtf->SourcePosition.values[i + 1];
    c[2] = hrtf->SourcePosition.values[i + 2];

    if (cartesian)
      mysofa_c2s(c);

    if (min > c[0] + c[1]) {
      min = c[0] + c[1];
      radius = c[2];
      index = i;
    } else if (min == c[0] + c[1] && radius < c[2]) {
      radius = c[2];
      index = i;
    }
  }

  /* get loudness of frontal fir filter, for both channels */
  factor = loudness(hrtf->DataIR.values + (index / hrtf->C) * hrtf->N * hrtf->R,
                    hrtf->N * hrtf->R);
  factor = sqrtf(2 / factor);
  if (fequals(factor, 1.f))
    return 1.f;

  scaleArray(hrtf->DataIR.values, hrtf->DataIR.elements, factor);

  return factor;
}
