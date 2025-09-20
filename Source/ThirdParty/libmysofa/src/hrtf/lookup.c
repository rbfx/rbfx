/*
 * sort.c
 *
 *  Created on: 12.01.2017
 *      Author: hoene
 */

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "kdtree.h"
#include "mysofa.h"
#include "mysofa_export.h"
#include "tools.h"

MYSOFA_EXPORT struct MYSOFA_LOOKUP *
mysofa_lookup_init(struct MYSOFA_HRTF *hrtf) {
  int i;
  struct MYSOFA_LOOKUP *lookup;

  /*
   * alloc memory structure
   */
  if (!verifyAttribute(hrtf->SourcePosition.attributes, "Type", "cartesian"))
    return NULL;

  lookup = malloc(sizeof(struct MYSOFA_LOOKUP));
  if (!lookup)
    return NULL;

  /*
   * find smallest and largest phi, theta, and radius (to reduce neighbors table
   * init)
   */
  float *origin;
  origin = malloc(sizeof(float) * hrtf->C);
  lookup->phi_min = FLT_MAX;
  lookup->phi_max = FLT_MIN;
  lookup->theta_min = FLT_MAX;
  lookup->theta_max = FLT_MIN;
  lookup->radius_min = FLT_MAX;
  lookup->radius_max = FLT_MIN;
  for (i = 0; i < hrtf->M; i++) {
    memcpy(origin, hrtf->SourcePosition.values + i * hrtf->C,
           sizeof(float) * hrtf->C);
    convertCartesianToSpherical(origin, hrtf->C);
    if (origin[0] < lookup->phi_min) {
      lookup->phi_min = origin[0];
    }
    if (origin[0] > lookup->phi_max) {
      lookup->phi_max = origin[0];
    }
    if (origin[1] < lookup->theta_min) {
      lookup->theta_min = origin[1];
    }
    if (origin[1] > lookup->theta_max) {
      lookup->theta_max = origin[1];
    }
    if (origin[2] < lookup->radius_min) {
      lookup->radius_min = origin[2];
    }
    if (origin[2] > lookup->radius_max) {
      lookup->radius_max = origin[2];
    }
  }
  free(origin);

  /*
   * Allocate kd tree
   */
  lookup->kdtree = kd_create();
  if (!lookup->kdtree) {
    free(lookup);
    return NULL;
  }

  /*
   * add coordinates to the tree
   */
  for (i = 0; i < hrtf->M; i++) {
    float *f = hrtf->SourcePosition.values + i * hrtf->C;
    kd_insert((struct kdtree *)lookup->kdtree, f, (void *)(intptr_t)i);
  }

  return lookup;
}

/*
 * looks for a filter that is similar to the given Cartesian coordinate
 * BE AWARE: The coordinate vector will be normalized if required
 * A return value of -1 = MYSOFA_INTERNAL_ERROR indicates an error
 */
MYSOFA_EXPORT int mysofa_lookup(struct MYSOFA_LOOKUP *lookup,
                                float *coordinate) {

  int index;
  void *res;
  int success;
  float r = radius(coordinate);
  if (r > lookup->radius_max) {
    r = lookup->radius_max / r;
    coordinate[0] *= r;
    coordinate[1] *= r;
    coordinate[2] *= r;
  } else if (r < lookup->radius_min) {
    r = lookup->radius_min / r;
    coordinate[0] *= r;
    coordinate[1] *= r;
    coordinate[2] *= r;
  }

  success = kd_nearest((struct kdtree *)lookup->kdtree, coordinate, &res);
  if (success != 0) {
    return MYSOFA_INTERNAL_ERROR;
  }
  index = (uintptr_t)res;
  return index;
}

MYSOFA_EXPORT void mysofa_lookup_free(struct MYSOFA_LOOKUP *lookup) {
  if (lookup) {
    kd_free((struct kdtree *)lookup->kdtree);
    free(lookup);
  }
}
