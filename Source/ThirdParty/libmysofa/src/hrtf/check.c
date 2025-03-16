#include "../hdf/reader.h"
#include "mysofa.h"
#include "mysofa_export.h"
#include "tools.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int compareValues(struct MYSOFA_ARRAY *array, const float *compare,
                         int elements, int size) {
  int i, j;
  if (array->values == NULL || array->elements != elements * size)
    return 0;
  for (j = 0; j < array->elements;)
    for (i = 0; i < elements; i++, j++)
      if (!fequals(array->values[j], compare[i]))
        return 0;
  return 1;
}

static const float array000[] = {0, 0, 0};
static const float array001[] = {0, 0, 1};
static const float array100[] = {1, 0, 0};

MYSOFA_EXPORT int mysofa_check(struct MYSOFA_HRTF *hrtf) {

  /* check for valid parameter ranges */
  /*
   Attributes":{
   "APIName":"ARI SOFA API for Matlab\/Octave",
   "APIVersion":"0.4.0",
   "ApplicationName":"Demo of the SOFA API",
   "ApplicationVersion":"0.4.0",
   "AuthorContact":"piotr@majdak.com",
   "Comment":"",
   "Conventions":"SOFA",
   "DataType":"FIR",
   "DatabaseName":"ARI",
   "DateCreated":"2014-03-20 17:35:22",
   "DateModified":"2014-03-20 17:35:22",
   "History":"Converted from the ARI format",
   "License":"No license provided, ask the author for permission",
   "ListenerShortName":"",
   "Organization":"Acoustics Research Institute",
   "Origin":"",
   "References":"",
   "RoomType":"free field",
   "SOFAConventions":"SimpleFreeFieldHRIR",
   "SOFAConventionsVersion":"0.4",
   "Title":"",
   "Version":"0.6"
   },
   */
  if (!verifyAttribute(hrtf->attributes, "Conventions", "SOFA") ||
      !verifyAttribute(hrtf->attributes, "SOFAConventions",
                       "SimpleFreeFieldHRIR") ||
      /* TODO: Support FT too */
      !verifyAttribute(hrtf->attributes, "DataType", "FIR"))
    return MYSOFA_INVALID_ATTRIBUTES; // LCOV_EXCL_LINE

  /*==============================================================================
   dimensions
   ==============================================================================
 */

  if (hrtf->C != 3 || hrtf->I != 1 || hrtf->E != 1 || hrtf->R != 2 ||
      hrtf->M == 0)
    return MYSOFA_INVALID_DIMENSIONS; // LCOV_EXCL_LINE

  /* verify format */

  if (hrtf->ListenerView.values) {
    int m = 1;
    if (!verifyAttribute(hrtf->ListenerView.attributes, "DIMENSION_LIST",
                         "I,C")) {
      if (!verifyAttribute(hrtf->ListenerView.attributes, "DIMENSION_LIST",
                           "M,C")) {
        return MYSOFA_INVALID_DIMENSION_LIST; // LCOV_EXCL_LINE
      }
      m = hrtf->M;
    }
    if (verifyAttribute(hrtf->ListenerView.attributes, "Type", "cartesian")) {
      if (!compareValues(&hrtf->ListenerView, array100, 3, m))
        return MYSOFA_INVALID_FORMAT; // LCOV_EXCL_LINE
    } else if (verifyAttribute(hrtf->ListenerView.attributes, "Type",
                               "spherical")) {
      if (!compareValues(&hrtf->ListenerView, array001, 3, m))
        return MYSOFA_INVALID_FORMAT; // LCOV_EXCL_LINE
    } else
      return MYSOFA_INVALID_COORDINATE_TYPE; // LCOV_EXCL_LINE
  }

#if 0
	if(hrtf->ListenerUp.values) {
		if(!verifyAttribute(hrtf->ListenerUp.attributes,"DIMENSION_LIST","I,C"))
		return MYSOFA_INVALID_FORMAT;
		if(verifyAttribute(hrtf->ListenerUp.attributes,"Type","cartesian")) {
			if(!compareValues(&hrtf->ListenerUp,array001,3))
			return MYSOFA_INVALID_FORMAT;
		}
		else if(verifyAttribute(hrtf->ListenerUp.attributes,"Type","spherical")) {
			if(!compareValues(&hrtf->ListenerUp,array0901,3))
			return MYSOFA_INVALID_FORMAT;
		}
	}

	/* TODO. support M,C too */
	if(!verifyAttribute(hrtf->ListenerPosition.attributes,"DIMENSION_LIST","I,C"))
	return MYSOFA_INVALID_FORMAT;
	if(!compareValues(&hrtf->ListenerPosition,array000,3))
	return MYSOFA_INVALID_FORMAT;
#endif

  int m = 1;
  if (!verifyAttribute(hrtf->EmitterPosition.attributes, "DIMENSION_LIST",
                       "E,C,I")) {
    if (!verifyAttribute(hrtf->EmitterPosition.attributes, "DIMENSION_LIST",
                         "E,C,M")) {
      return MYSOFA_ONLY_EMITTER_WITH_ECI_SUPPORTED; // LCOV_EXCL_LINE
    }
    m = hrtf->M;
  }

  if (!compareValues(&hrtf->EmitterPosition, array000, 3, m))
    return MYSOFA_ONLY_EMITTER_WITH_ECI_SUPPORTED; // LCOV_EXCL_LINE

  if (hrtf->DataDelay.values) {
    if (!verifyAttribute(hrtf->DataDelay.attributes, "DIMENSION_LIST", "I,R") &&
        !verifyAttribute(hrtf->DataDelay.attributes, "DIMENSION_LIST", "M,R"))
      return MYSOFA_ONLY_DELAYS_WITH_IR_OR_MR_SUPPORTED; // LCOV_EXCL_LINE
  }
  /* TODO: Support different sampling rate per measurement, support default
   sampling rate of 48000 However, so far, I have not seen any sofa files with
   a format other then I */
  if (!verifyAttribute(hrtf->DataSamplingRate.attributes, "DIMENSION_LIST",
                       "I"))
    return MYSOFA_ONLY_THE_SAME_SAMPLING_RATE_SUPPORTED; // LCOV_EXCL_LINE

  if (verifyAttribute(hrtf->ReceiverPosition.attributes, "DIMENSION_LIST",
                      "R,C,I")) {
    // do nothing
  } else if (verifyAttribute(hrtf->ReceiverPosition.attributes,
                             "DIMENSION_LIST", "R,C,M")) {
    if (hrtf->ReceiverPosition.elements != hrtf->C * hrtf->R * hrtf->M)
      return MYSOFA_INVALID_RECEIVER_POSITIONS;

    for (int i = 0; i < hrtf->C * hrtf->R; i++) {
      int offset = i * hrtf->M;
      double receiverPosition = hrtf->ReceiverPosition.values[offset];
      for (int j = 1; j < hrtf->M; j++)
        if (!fequals(receiverPosition,
                     hrtf->ReceiverPosition.values[offset + j]))
          return MYSOFA_RECEIVERS_WITH_RCI_SUPPORTED; // LCOV_EXCL_LINE
    }
  } else {
    return MYSOFA_RECEIVERS_WITH_RCI_SUPPORTED; // LCOV_EXCL_LINE
  }

  if (!verifyAttribute(hrtf->ReceiverPosition.attributes, "Type", "cartesian"))
    return MYSOFA_RECEIVERS_WITH_CARTESIAN_SUPPORTED; // LCOV_EXCL_LINE

  if (hrtf->ReceiverPosition.elements < hrtf->C * hrtf->R ||
      fabs(hrtf->ReceiverPosition.values[0]) >= 0.02f || // we assumes a somehow symetrical face
      fabs(hrtf->ReceiverPosition.values[2]) >= 0.02f ||
      fabs(hrtf->ReceiverPosition.values[3]) >= 0.02f ||
      fabs(hrtf->ReceiverPosition.values[5]) >= 0.02f)
  {
    return MYSOFA_INVALID_RECEIVER_POSITIONS; // LCOV_EXCL_LINE
  }
  if (fabs(hrtf->ReceiverPosition.values[4] +
           hrtf->ReceiverPosition.values[1]) >= 0.02f)
    return MYSOFA_INVALID_RECEIVER_POSITIONS; // LCOV_EXCL_LINE
  if (hrtf->ReceiverPosition.values[1] < 0) {
    if (!verifyAttribute(hrtf->attributes, "APIName",
                         "ARI SOFA API for Matlab/Octave"))
      return MYSOFA_INVALID_RECEIVER_POSITIONS; // LCOV_EXCL_LINE

    const char *version = mysofa_getAttribute(hrtf->attributes, "APIVersion");
    if (version == NULL)
      return MYSOFA_INVALID_RECEIVER_POSITIONS; // LCOV_EXCL_LINE

    int a, b, c;
    int res = sscanf(version, "%d.%d.%d", &a, &b, &c);
    if (res != 3)
      return MYSOFA_INVALID_RECEIVER_POSITIONS; // LCOV_EXCL_LINE
    if (a > 1)
      return MYSOFA_INVALID_RECEIVER_POSITIONS; // LCOV_EXCL_LINE
    if (a == 1 && b > 1)
      return MYSOFA_INVALID_RECEIVER_POSITIONS; // LCOV_EXCL_LINE
    if (a == 1 && b == 1 && c > 0)
      return MYSOFA_INVALID_RECEIVER_POSITIONS; // LCOV_EXCL_LINE

    if (hrtf->ReceiverPosition.values[1] >= 0)
      return MYSOFA_INVALID_RECEIVER_POSITIONS; // LCOV_EXCL_LINE

    // old versions of sofaapi sometimes has been used wrongly. Thus, they wrote
    // left and right ears to different possitions
    mylog("WARNING: SOFA file is written with wrong receiver positions. %d "
          "%d.%d.%d %f<>%f\n",
          res, a, b, c, hrtf->ReceiverPosition.values[1],
          hrtf->ReceiverPosition.values[4]);
  }

  /* read source positions */
  if (!verifyAttribute(hrtf->SourcePosition.attributes, "DIMENSION_LIST",
                       "M,C"))
    return MYSOFA_ONLY_SOURCES_WITH_MC_SUPPORTED; // LCOV_EXCL_LINE

  return MYSOFA_OK;
}
