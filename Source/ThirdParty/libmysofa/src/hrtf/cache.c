/*
 * cache.c
 *
 *  Created on: 08.02.2017
 *      Author: hoene
 */

#include "../hdf/reader.h"
#include "mysofa.h"
#include "mysofa_export.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct MYSOFA_CACHE_ENTRY {
  struct MYSOFA_CACHE_ENTRY *next;
  struct MYSOFA_EASY *easy;
  char *filename;
  float samplerate;
  int count;
} *cache = NULL;

static int compare_filenames(const char *a, const char *b) {
  if (a == NULL && b == NULL)
    return 0;
  if (a == NULL)
    return -1;
  else if (b == NULL)
    return 1;
  return strcmp(a, b);
}

MYSOFA_EXPORT struct MYSOFA_EASY *mysofa_cache_lookup(const char *filename,
                                                      float samplerate) {
  struct MYSOFA_CACHE_ENTRY *p;
  struct MYSOFA_EASY *res = NULL;

  p = cache;

  while (p) {
    if (samplerate == p->samplerate &&
        !compare_filenames(filename, p->filename)) {
      res = p->easy;
      p->count++;
      break;
    }
    p = p->next;
  }

  return res;
}

MYSOFA_EXPORT struct MYSOFA_EASY *mysofa_cache_store(struct MYSOFA_EASY *easy,
                                                     const char *filename,
                                                     float samplerate) {
  struct MYSOFA_CACHE_ENTRY *p;

  assert(easy);

  p = cache;

  while (p) {
    if (samplerate == p->samplerate &&
        !compare_filenames(filename, p->filename)) {
      mysofa_close(easy);
      return p->easy;
    }
    p = p->next;
  }

  p = malloc(sizeof(struct MYSOFA_CACHE_ENTRY));
  if (p == NULL) {
    return NULL;
  }
  p->next = cache;
  p->samplerate = samplerate;
  p->filename = NULL;
  if (filename != NULL) {
    p->filename = mysofa_strdup(filename);
    if (p->filename == NULL) {
      free(p);
      return NULL;
    }
  }
  p->easy = easy;
  p->count = 1;
  cache = p;
  return easy;
}

MYSOFA_EXPORT void mysofa_cache_release(struct MYSOFA_EASY *easy) {
  struct MYSOFA_CACHE_ENTRY **p;
  int count;

  assert(easy);
  assert(cache);

  p = &cache;

  for (count = 0;; count++) {
    if ((*p)->easy == easy)
      break;
    p = &((*p)->next);
    assert(*p);
  }

  if ((*p)->count == 1 && (count > 0 || (*p)->next != NULL)) {
    struct MYSOFA_CACHE_ENTRY *gone = *p;
    free(gone->filename);
    mysofa_close(easy);
    *p = (*p)->next;
    free(gone);
  } else {
    (*p)->count--;
  }
}

MYSOFA_EXPORT void mysofa_cache_release_all() {
  struct MYSOFA_CACHE_ENTRY *p;

  p = cache;
  while (p) {
    struct MYSOFA_CACHE_ENTRY *gone = p;
    p = p->next;
    free(gone->filename);
    free(gone->easy);
    free(gone);
  }
  cache = NULL;
}
