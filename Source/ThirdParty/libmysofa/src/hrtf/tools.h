/*
 * tools.h
 *
 *  Created on: 13.01.2017
 *      Author: hoene
 */

#ifndef SRC_TOOLS_H_
#define SRC_TOOLS_H_

#include "mysofa.h"
#include <math.h>
#include <stdlib.h>

int changeAttribute(struct MYSOFA_ATTRIBUTE *attr, char *name, char *value,
                    char *newvalue);
int verifyAttribute(struct MYSOFA_ATTRIBUTE *attr, char *name, char *value);
char *getAttribute(struct MYSOFA_ATTRIBUTE *attr, char *name);

void convertCartesianToSpherical(float *values, int elements);
void convertSphericalToCartesian(float *values, int elements);

#define fequals(a, b) (fabs(a - b) < 0.00001)

float radius(float *cartesian);

#define distance(cartesian1, cartesian2)                                       \
  (sqrtf(powf((cartesian1)[0] - (cartesian2)[0], 2.f) +                        \
         powf((cartesian1)[1] - (cartesian2)[1], 2.f) +                        \
         powf((cartesian1)[2] - (cartesian2)[2], 2.f)))

void copyToFloat(float *out, float *in, int size);
void copyFromFloat(float *out, float *in, int size);

void copyArrayWeighted(float *dst, float *src, int size, float w);
void addArrayWeighted(float *dst, float *src, int size, float w);
void scaleArray(float *dst, int size, float w);
float loudness(float *in, int size);

void nsearch(const void *key, const char *base, size_t num, size_t size,
             int (*cmp)(const void *key, const void *elt), int *lower,
             int *higher);

#endif /* SRC_TOOLS_H_ */
