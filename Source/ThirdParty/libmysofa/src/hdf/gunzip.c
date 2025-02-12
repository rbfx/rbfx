/*

 Copyright 2016 Christian Hoene, Symonics GmbH

 */

#include "reader.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

int gunzip(int inlen, char *in, int *outlen, char *out) {
  int err;
  z_stream stream;

  memset(&stream, 0, sizeof(stream));
  stream.avail_in = inlen;
  stream.next_in = (unsigned char *)in;
  stream.avail_out = *outlen;
  stream.next_out = (unsigned char *)out;

  err = inflateInit(&stream);
  if (err)
    return err;

  err = inflate(&stream, Z_SYNC_FLUSH);
  *outlen = stream.total_out;
  inflateEnd(&stream);
  if (err && err != Z_STREAM_END) {
    mylog(" gunzip error %d %s\n", err, stream.msg);
    return err;
  }

  return MYSOFA_OK;
}
