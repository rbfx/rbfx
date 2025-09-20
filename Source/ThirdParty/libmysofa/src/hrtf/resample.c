/*
 * resample.c
 *
 *  Created on: 17.01.2017
 *      Author: hoene
 */

#include "../resampler/speex_resampler.h"
#include "mysofa.h"
#include "mysofa_export.h"
#include "tools.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MYSOFA_EXPORT int mysofa_resample(struct MYSOFA_HRTF *hrtf, float samplerate) {
  int i, err;
  float factor;
  unsigned newN;
  float *values;
  SpeexResamplerState *resampler;
  float zero[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  if (hrtf->DataSamplingRate.elements != 1 || samplerate < 8000. ||
      hrtf->DataIR.elements != hrtf->R * hrtf->M * hrtf->N)
    return MYSOFA_INVALID_FORMAT;

  if (samplerate == hrtf->DataSamplingRate.values[0])
    return MYSOFA_OK;

  factor = samplerate / hrtf->DataSamplingRate.values[0];
  newN = ceil(hrtf->N * factor);

  /*
   * resample FIR filter
   */

  values = malloc(newN * hrtf->R * hrtf->M * sizeof(float));
  if (values == NULL)
    return MYSOFA_NO_MEMORY;

  resampler = speex_resampler_init(1, hrtf->DataSamplingRate.values[0],
                                   samplerate, 10, &err);
  if (resampler == NULL) {
    free(values);
    return err;
  }

  if (hrtf->N) {
    for (i = 0; i < hrtf->R * hrtf->M; i++) {
      unsigned inlen = hrtf->N;
      unsigned outlen = newN;
      speex_resampler_reset_mem(resampler);
      speex_resampler_skip_zeros(resampler);
      speex_resampler_process_float(resampler, 0,
                                    hrtf->DataIR.values + i * hrtf->N, &inlen,
                                    values + i * newN, &outlen);
      assert(inlen == hrtf->N);
      while (outlen < newN) {
        unsigned difflen = newN - outlen;
        inlen = 10;
        speex_resampler_process_float(resampler, 0, zero, &inlen,
                                      values + i * newN + outlen, &difflen);
        outlen += difflen;
      }
      assert(outlen == newN);
    }
  }
  speex_resampler_destroy(resampler);

  free(hrtf->DataIR.values);
  hrtf->DataIR.values = values;
  hrtf->DataIR.elements = newN * hrtf->R * hrtf->M;

  /*
   * update delay values
   */
  for (i = 0; i < hrtf->DataDelay.elements; i++)
    hrtf->DataDelay.values[i] *= factor;

  /*
   * update sample rate
   */
  hrtf->DataSamplingRate.values[0] = samplerate;
  hrtf->N = newN;

  return MYSOFA_OK;
}
