/*
 * tools.c
 *
 *  Created on: 13.01.2017
 *      Author: hoene
 */
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif // !_USE_MATH_DEFINES
#include "tools.h"
#include "mysofa.h"
#include "mysofa_export.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *mysofa_strdup(const char *str) {
  size_t size = strlen(str) + 1;
  char *copy = malloc(size);
  if (copy)
    memcpy(copy, str, size);
  return copy;
}

int verifyAttribute(struct MYSOFA_ATTRIBUTE *attr, char *name, char *value) {
  while (attr) {
    if (attr->name && !strcmp(name, attr->name) && attr->value &&
        !strcmp(value, attr->value))
      return 1;
    attr = attr->next;
  }
  return 0;
}

int changeAttribute(struct MYSOFA_ATTRIBUTE *attr, char *name, char *value,
                    char *newvalue) {
  while (attr) {
    if (!strcmp(name, attr->name) &&
        (value == NULL || attr->value == NULL || !strcmp(value, attr->value))) {
      free(attr->value);
      attr->value = mysofa_strdup(newvalue);
      return 1;
    }
    attr = attr->next;
  }
  return 0;
}

MYSOFA_EXPORT
char *mysofa_getAttribute(struct MYSOFA_ATTRIBUTE *attr, char *name) {
  while (attr) {
    if (attr->name && !strcmp(name, attr->name)) {
      return attr->value;
    }
    attr = attr->next;
  }
  return NULL;
}

MYSOFA_EXPORT void mysofa_c2s(float values[3]) {
  float x, y, z, r, theta, phi;
  x = values[0];
  y = values[1];
  z = values[2];
  r = radius(values);

  theta = atan2f(z, sqrtf(x * x + y * y));
  phi = atan2f(y, x);

  values[0] = fmodf(phi * (180 / M_PI) + 360, 360);
  values[1] = theta * (180 / M_PI);
  values[2] = r;
}

MYSOFA_EXPORT void mysofa_s2c(float values[3]) {
  float x, r, theta, phi;
  phi = values[0] * (M_PI / 180);
  theta = values[1] * (M_PI / 180);
  r = values[2];
  x = cosf(theta) * r;
  values[2] = sinf(theta) * r;
  values[0] = cosf(phi) * x;
  values[1] = sinf(phi) * x;
}

void convertCartesianToSpherical(float *values, int elements) {
  int i;

  for (i = 0; i < elements - 2; i += 3) {
    mysofa_c2s(values + i);
  }
}

void convertSphericalToCartesian(float *values, int elements) {

  int i;

  for (i = 0; i < elements - 2; i += 3) {
    mysofa_s2c(values + i);
  }
}

float radius(float *cartesian) {
  return sqrtf(powf(cartesian[0], 2.f) + powf(cartesian[1], 2.f) +
               powf(cartesian[2], 2.f));
}

/*
 * search of the nearest
 */

void nsearch(const void *key, const char *base, size_t num, size_t size,
             int (*cmp)(const void *key, const void *elt), int *lower,
             int *higher) {
  size_t start = 0, end = num;
  int result;

  while (start < end) {
    size_t mid = start + (end - start) / 2;

    result = cmp(key, base + mid * size);
    if (result < 0)
      end = mid;
    else if (result > 0)
      start = mid + 1;
    else {
      *lower = mid;
      *higher = mid;
      return;
    }
  }

  if (start == num) {
    *lower = start - 1;
    *higher = -1;
  } else if (start == 0) {
    *lower = -1;
    *higher = 0;
  } else {
    *lower = start - 1;
    *higher = start;
  }
}

void copyToFloat(float *out, float *in, int size) {
  while (size > 0) {
    *out++ = *in++;
    size--;
  }
}

void copyFromFloat(float *out, float *in, int size) {
  while (size > 0) {
    *out++ = *in++;
    size--;
  }
}

void copyArrayWeighted(float *dst, float *src, int size, float w) {
  while (size > 0) {
    *dst++ = *src++ * w;
    size--;
  }
}

void addArrayWeighted(float *dst, float *src, int size, float w) {
  while (size > 0) {
    *dst++ += *src++ * w;
    size--;
  }
}

void scaleArray(float *dst, int size, float w) {
  while (size > 0) {
    *dst++ *= w;
    size--;
  }
}
float loudness(float *in, int size) {
  float res = 0;
  while (size > 0) {
    res += *in * *in;
    in++;
    size--;
  }
  return res;
}
