#include "mysofa.h"
#include "mysofa_export.h"
#include "tools.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void convertArray(struct MYSOFA_ARRAY *array) {
  if (!changeAttribute(array->attributes, "Type", "cartesian", "spherical"))
    return;

  changeAttribute(array->attributes, "Units", NULL, "degree, degree, meter");

  convertCartesianToSpherical(array->values, array->elements);
}

MYSOFA_EXPORT void mysofa_tospherical(struct MYSOFA_HRTF *hrtf) {
  convertArray(&hrtf->ListenerView);
  convertArray(&hrtf->ListenerUp);
  convertArray(&hrtf->ListenerPosition);
  convertArray(&hrtf->EmitterPosition);
  convertArray(&hrtf->ReceiverPosition);
  convertArray(&hrtf->SourcePosition);
}

static void convertArray2(struct MYSOFA_ARRAY *array) {
  if (!changeAttribute(array->attributes, "Type", "spherical", "cartesian"))
    return;

  changeAttribute(array->attributes, "Units", NULL, "meter");

  convertSphericalToCartesian(array->values, array->elements);
}

MYSOFA_EXPORT void mysofa_tocartesian(struct MYSOFA_HRTF *hrtf) {
  convertArray2(&hrtf->ListenerView);
  convertArray2(&hrtf->ListenerUp);
  convertArray2(&hrtf->ListenerPosition);
  convertArray2(&hrtf->EmitterPosition);
  convertArray2(&hrtf->ReceiverPosition);
  convertArray2(&hrtf->SourcePosition);
}
