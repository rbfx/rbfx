/*
 * neighbor.c
 *
 *  Created on: 17.01.2017
 *      Author: hoene
 */

#include "mysofa.h"
#include "mysofa_export.h"
#include "tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #define VDEBUG */

MYSOFA_EXPORT float *mysofa_interpolate(struct MYSOFA_HRTF *hrtf,
                                        float *cordinate, int nearest,
                                        int *neighborhood, float *fir,
                                        float *delays) {
  int i, use[6];
  float d, d6[6];
  float weight;
  int size = hrtf->N * hrtf->R;

  d = distance(cordinate, hrtf->SourcePosition.values + nearest * hrtf->C);
  if (fequals(d, 0)) {
    if (hrtf->DataDelay.elements > hrtf->R) {
      delays[0] = hrtf->DataDelay.values[nearest * hrtf->R];
      delays[1] = hrtf->DataDelay.values[nearest * hrtf->R + 1];
    } else {
      delays[0] = hrtf->DataDelay.values[0];
      delays[1] = hrtf->DataDelay.values[1];
    }
    float *ret = hrtf->DataIR.values + nearest * size;
    copyFromFloat(fir, ret, size);
    return ret;
  }

  for (i = 0; i < 6; i++) {
    use[i] = 0;
    d6[i] = 1;
  }

  if (neighborhood[0] >= 0 && neighborhood[1] >= 0) {
    d6[0] = distance(cordinate,
                     hrtf->SourcePosition.values + neighborhood[0] * hrtf->C);
    d6[1] = distance(cordinate,
                     hrtf->SourcePosition.values + neighborhood[1] * hrtf->C);

    if (!fequals(d6[0], d6[1])) {
      if (d6[0] < d6[1])
        use[0] = 1;
      else
        use[1] = 1;
    }
  } else if (neighborhood[0] >= 0) {
    d6[0] = distance(cordinate,
                     hrtf->SourcePosition.values + neighborhood[0] * hrtf->C);
    use[0] = 1;
  } else if (neighborhood[1] >= 0) {
    d6[1] = distance(cordinate,
                     hrtf->SourcePosition.values + neighborhood[1] * hrtf->C);
    use[1] = 1;
  }

  if (neighborhood[2] >= 0 && neighborhood[3] >= 0) {
    d6[2] = distance(cordinate,
                     hrtf->SourcePosition.values + neighborhood[2] * hrtf->C);
    d6[3] = distance(cordinate,
                     hrtf->SourcePosition.values + neighborhood[3] * hrtf->C);
    if (!fequals(d6[2], d6[3])) {
      if (d6[2] < d6[3])
        use[2] = 1;
      else
        use[3] = 1;
    }
  } else if (neighborhood[2] >= 0) {
    d6[2] = distance(cordinate,
                     hrtf->SourcePosition.values + neighborhood[2] * hrtf->C);
    use[2] = 1;
  } else if (neighborhood[3] >= 0) {
    d6[3] = distance(cordinate,
                     hrtf->SourcePosition.values + neighborhood[3] * hrtf->C);
    use[3] = 1;
  }

  if (neighborhood[4] >= 0 && neighborhood[5] >= 0) {
    d6[4] = distance(cordinate,
                     hrtf->SourcePosition.values + neighborhood[4] * hrtf->C);
    d6[5] = distance(cordinate,
                     hrtf->SourcePosition.values + neighborhood[5] * hrtf->C);
    if (!fequals(d6[4], d6[5])) {
      if (d6[4] < d6[5])
        use[4] = 1;
      else
        use[5] = 1;
    }
  } else if (neighborhood[4] >= 0) {
    d6[4] = distance(cordinate,
                     hrtf->SourcePosition.values + neighborhood[4] * hrtf->C);
    use[4] = 1;
  } else if (neighborhood[5] >= 0) {
    d6[5] = distance(cordinate,
                     hrtf->SourcePosition.values + neighborhood[5] * hrtf->C);
    use[5] = 1;
  }

  weight = 1 / d;
  copyArrayWeighted(fir, hrtf->DataIR.values + nearest * size, size, weight);
  if (hrtf->DataDelay.elements > hrtf->R) {
    delays[0] = hrtf->DataDelay.values[nearest * hrtf->R] * weight;
    delays[1] = hrtf->DataDelay.values[nearest * hrtf->R + 1] * weight;
  } else {
    delays[0] = hrtf->DataDelay.values[0] * weight;
    delays[1] = hrtf->DataDelay.values[1] * weight;
  }
#ifdef VDEBUG
  printf("%d ! %f ", nearest, d);
#endif
  for (i = 0; i < 6; i++) {
    if (use[i]) {
      float w = 1 / d6[i];
#ifdef VDEBUG
      printf("%d - %f ", neighborhood[i], d6[i]);
#endif
      addArrayWeighted(fir, hrtf->DataIR.values + neighborhood[i] * size, size,
                       w);
      weight += w;
      if (hrtf->DataDelay.elements > hrtf->R) {
        delays[0] += hrtf->DataDelay.values[neighborhood[i] * hrtf->R] * w;
        delays[1] += hrtf->DataDelay.values[neighborhood[i] * hrtf->R + 1] * w;
      }
    }
  }
#ifdef VDEBUG
  printf("\n");
#endif
  weight = 1 / weight;
  scaleArray(fir, size, weight);
  delays[0] *= weight;
  delays[1] *= weight;
  return fir;
}
