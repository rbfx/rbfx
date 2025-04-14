/*

 Copyright 2016 Christian Hoene, Symonics GmbH

 */

#include "reader.h"
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LENGTH (0x100)

static int log2i(int a) { return round(log2(a)); }

static int directblockRead(struct READER *reader, struct DATAOBJECT *dataobject,
                           struct FRACTALHEAP *fractalheap) {

  char buf[5], *name, *value;
  int size, offset_size, length_size, err, len;
  uint8_t typeandversion;
  uint64_t unknown1, unknown2, unknown3, unknown4, heap_header_address,
      block_offset, block_size, offset, length;
  long store;
  struct DIR *dir;
  struct MYSOFA_ATTRIBUTE *attr;

  UNUSED(offset);
  UNUSED(block_size);
  UNUSED(block_offset);

  if (reader->recursive_counter >= 20) {
    mylog("recursive problem");
    return MYSOFA_INVALID_FORMAT;
  } else
    reader->recursive_counter++;

  /* read signature */
  if (mysofa_read(reader, buf, 4) != 4 || strncmp(buf, "FHDB", 4)) {
    // LCOV_EXCL_START
    mylog("cannot read signature of fractal heap indirect block\n");
    return MYSOFA_INVALID_FORMAT;
    // LCOV_EXCL_STOP
  }
  buf[4] = 0;
  mylog("%08" PRIX64 " %.4s stack %d\n", (uint64_t)mysofa_tell(reader) - 4, buf,
        reader->recursive_counter);

  if (mysofa_getc(reader) != 0) {
    mylog("object FHDB must have version 0\n"); // LCOV_EXCL_LINE
    return MYSOFA_UNSUPPORTED_FORMAT;           // LCOV_EXCL_LINE
  }

  /* ignore heap_header_address */
  if (mysofa_seek(reader, reader->superblock.size_of_offsets, SEEK_CUR) < 0)
    return errno; // LCOV_EXCL_LINE

  size = (fractalheap->maximum_heap_size + 7) / 8;
  block_offset = readValue(reader, size);

  if (fractalheap->flags & 2)
    if (mysofa_seek(reader, 4, SEEK_CUR))
      return errno; // LCOV_EXCL_LINE

  offset_size = ceilf(log2f(fractalheap->maximum_heap_size) / 8);
  if (fractalheap->maximum_direct_block_size < fractalheap->maximum_size)
    length_size = ceilf(log2f(fractalheap->maximum_direct_block_size) / 8);
  else
    length_size = ceilf(log2f(fractalheap->maximum_size) / 8);

  mylog(" %d %" PRIu64 " %d\n", size, block_offset, offset_size);

  /*
   00003e00  00 46 48 44 42 00 40 02  00 00 00 00 00 00 00 00
   |.FHDB.@.........|
   00003e10  00 00 00 83 8d ac f6 03  00 0c 00 08 00 04 00 00
   |................|
   00003e20  43 6f 6e 76 65 6e 74 69  6f 6e 73 00 13 00 00 00
   |Conventions.....|
   00003e30  04 00 00 00 02 00 00 00  53 4f 46 41 03 00 08 00
   |........SOFA....|
   00003e40  08 00 04 00 00 56 65 72  73 69 6f 6e 00 13 00 00
   |.....Version....|
   00003e50  00 03 00 00 00 02 00 00  00 30 2e 36 03 00 10 00
   |.........0.6....|
   00003e60  08 00 04 00 00 53 4f 46  41 43 6f 6e 76 65 6e 74
   |.....SOFAConvent|
   00003e70  69 6f 6e 73 00 13 00 00  00 13 00 00 00 02 00 00
   |ions............|
   00003e80  00 53 69 6d 70 6c 65 46  72 65 65 46 69 65 6c 64
   |.SimpleFreeField|
   00003e90  48 52 49 52 03 00 17 00  08 00 04 00 00 53 4f 46
   |HRIR.........SOF|
   00003ea0  41 43 6f 6e 76 65 6e 74  69 6f 6e 73 56 65 72 73
   |AConventionsVers|
   00003eb0  69 6f 6e 00 13 00 00 00  03 00 00 00 02 00 00 00
   |ion.............|
   *

  00002730  00 00 00 00 00 00 00 46  48 44 42 00 97 02 00 00  |.......FHDB.....|
  00002740  00 00 00 00 00 00 00 00  00 99 b9 5c d8


   */
  do {
    typeandversion = (uint8_t)mysofa_getc(reader);
    offset = readValue(reader, offset_size);
    length = readValue(reader, length_size);
    if (offset > 0x10000000 || length > 0x10000000)
      return MYSOFA_UNSUPPORTED_FORMAT; // LCOV_EXCL_LINE

    mylog(" %d %4" PRIX64 " %" PRIX64 " %08lX\n", typeandversion, offset,
          length, mysofa_tell(reader) - 1 - offset_size - length_size);

    /* TODO: for the following part, the specification is incomplete */
    if (typeandversion == 3) {
      /*
       * this seems to be a name and value pair
       */

      if (readValue(reader, 5) != 0x0000040008) {
        mylog("FHDB type 3 unsupported values"); // LCOV_EXCL_LINE
        return MYSOFA_UNSUPPORTED_FORMAT;        // LCOV_EXCL_LINE
      }

      if (!(name = malloc(length + 1)))
        return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
      if (mysofa_read(reader, name, length) != length) {
        free(name);               // LCOV_EXCL_LINE
        return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE
      }
      name[length] = 0;

      if (readValue(reader, 4) != 0x00000013) {
        mylog("FHDB type 3 unsupported values"); // LCOV_EXCL_LINE
        free(name);                              // LCOV_EXCL_LINE
        return MYSOFA_UNSUPPORTED_FORMAT;        // LCOV_EXCL_LINE
      }

      len = (int)readValue(reader, 2);
      if (len > 0x1000 || len < 0) {
        free(name);                       // LCOV_EXCL_LINE
        return MYSOFA_UNSUPPORTED_FORMAT; // LCOV_EXCL_LINE
      }

      /* TODO: Get definition of this field */
      unknown1 = readValue(reader, 6);
      if (unknown1 == 0x000000020200)
        value = NULL;
      else if (unknown1 == 0x000000020000) {
        if (!(value = malloc(len + 1))) {
          free(name);              // LCOV_EXCL_LINE
          return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
        }
        if (mysofa_read(reader, value, len) != len) {
          free(value);              // LCOV_EXCL_LINE
          free(name);               // LCOV_EXCL_LINE
          return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE
        }
        value[len] = 0;
      } else if (unknown1 == 0x20000020000) {
        if (!(value = malloc(5))) {
          free(name);              // LCOV_EXCL_LINE
          return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
        }
        strcpy(value, "");
      } else {
        mylog("FHDB type 3 unsupported values: %12" PRIX64 "\n", unknown1);
        free(name);
        /* TODO:			return MYSOFA_UNSUPPORTED_FORMAT; */
        return MYSOFA_OK;
      }
      mylog(" %s = %s\n", name, value);

      attr = malloc(sizeof(struct MYSOFA_ATTRIBUTE));
      if (attr == NULL) {
        free(value);             // LCOV_EXCL_LINE
        free(name);              // LCOV_EXCL_LINE
        return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
      }

      attr->name = name;
      attr->value = value;
      attr->next = dataobject->attributes;
      dataobject->attributes = attr;
    } else if (typeandversion == 1) {

      /* TODO: Get definition of this field */
      unknown2 = readValue(reader, 4);
      switch (unknown2) {
      case 0:

        /* TODO: Get definition of this field */
        unknown3 = readValue(reader, 2);
        if (unknown3 != 0x0000)
          return MYSOFA_INVALID_FORMAT;

        len = mysofa_getc(reader);
        if (len < 0)
          return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE
        if (len > MAX_NAME_LENGTH)
          return MYSOFA_INVALID_FORMAT; // LCOV_EXCL_LINE

        if (!(name = malloc(len + 1)))
          return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
        if (mysofa_read(reader, name, len) != len) {
          free(name);               // LCOV_EXCL_LINE
          return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE
        }
        name[len] = 0;

        heap_header_address =
            readValue(reader, reader->superblock.size_of_offsets);

        mylog("fractal head type 1 length %4" PRIX64 " name %s address %" PRIX64
              "\n",
              length, name, heap_header_address);

        dir = malloc(sizeof(struct DIR));
        if (!dir) {
          free(name);              // LCOV_EXCL_LINE
          return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
        }
        memset(dir, 0, sizeof(*dir));

        dir->next = dataobject->directory;
        dataobject->directory = dir;

        store = mysofa_tell(reader);
        if (mysofa_seek(reader, heap_header_address, SEEK_SET)) {
          free(name);   // LCOV_EXCL_LINE
          return errno; // LCOV_EXCL_LINE
        }

        err = dataobjectRead(reader, &dir->dataobject, name);
        if (err) {
          return err; // LCOV_EXCL_LINE
        }

        if (store < 0) {
          return errno; // LCOV_EXCL_LINE
        }
        if (mysofa_seek(reader, store, SEEK_SET) < 0)
          return errno; // LCOV_EXCL_LINE
        break;
      case 0x00080008:
      /*
                                                    01 00 0e
  |...........\....|
  00002750  00 08 00 08 00 5f 4e 43  50 72 6f 70 65 72 74 69
  |....._NCProperti|
  00002760  65 73 00 00 00 13 00 00  00 37 00 00 00 01 00 00
  |es.......7......|
  00002770  00 00 00 00 00 76 65 72  73 69 6f 6e 3d 31 7c 6e
  |.....version=1|n|
  00002780  65 74 63 64 66 6c 69 62  76 65 72 73 69 6f 6e 3d
  |etcdflibversion=|
  00002790  34 2e 36 2e 31 7c 68 64  66 35 6c 69 62 76 65 72
  |4.6.1|hdf5libver|
  000027a0  73 69 6f 6e 3d 31 2e 31  30 2e 34 00 01 00 0c 00
  |sion=1.10.4.....|
  000027b0  08 00 08 00 43 6f 6e 76  65 6e 74 69 6f 6e 73 00
  |....Conventions.|
  000027c0  00 00 00 00 13 00 00 00  04 00 00 00 01 00 00 00
  |................|
  000027d0  00 00 00 00 53 4f 46 41  01 00 08 00 08 00 08 00
  |....SOFA........|
  000027e0  56 65 72 73 69 6f 6e 00  13 00 00 00 03 00 00 00
  |Version.........|
  000027f0  01 00 00 00 00 00 00 00 31 2e 30 01 00 10 00 08
  |........1.0.....|
  00002800  00 08 00 53 4f 46 41 43  6f 6e 76 65 6e 74 69 6f
  |...SOFAConventio|
  00002810  6e 73 00 13 00 00 00 13  00 00 00 01 00 00 00 00
  |ns..............|
  00002820  00 00 00 53 69 6d 70 6c  65 46 72 65 65 46 69 65
  |...SimpleFreeFie|
  */
      case 0x00040008:
        /*
        00004610           08 00 04 00 41  75 74 68 6f 72 43 6f 6e
        |.......AuthorCon|
        00004620  74 61 63 74 00 00 00 13  00 00 00 01 00 00 00 02
        |tact............|
        00004630  00 00 02 00 00 00 00 01  00 08 00 08 00 04 00 43
        |...............C|
        00004640  6f 6d 6d 65 6e 74 00 13  00 00 00 01 00 00 00 02
        |omment..........|
        00004650  00 00 02 00 00 00 00 01  00 09 00 08 00 08 00 44
        |...............D|
        00004660  61 74 61 54 79 70 65 00  00 00 00 00 00 00 00 13
        |ataType.........|
        00004670  00 00 00 03 00 00 00 01  00 00 00 00 00 00 00 46
        |...............F|
        00004680  49 52 01 00 08 00 08 00  08 00 48 69 73 74 6f 72
        |IR........Histor|
        00004690  79 00 13 00 00 00 34 00  00 00 01 00 00 00 00 00
        |y.....4.........|
        000046a0  00 00 43 6f 6e 76 65 72  74 65 64 20 66 72 6f 6d
        |..Converted from|
        000046b0  20 74 68 65 20 4d 49 54  20 66 6f 72 6d 61 74 0a
        | the MIT format.|
        000046c0  55 70 67 72 61 64 65 64  20 66 72 6f 6d 20 53 4f
        |Upgraded from SO|
        000046d0  46 41 20 30 2e 36 01 00  08 00 08 00 08 00 4c 69
        |FA 0.6........Li|
        000046e0  63 65 6e 73 65 00 13 00  00 00 32 00 00 00 01 00
        |cense.....2.....|
        000046f0  00 00 00 00 00 00 4e 6f  20 6c 69 63 65 6e 73 65
        |......No license|
        */

        if (!(name = malloc(MAX_NAME_LENGTH)))
          return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
        len = -1;
        for (int i = 0; i < MAX_NAME_LENGTH; i++) {
          int c = mysofa_getc(reader);
          if (c < 0 || i == MAX_NAME_LENGTH - 1) {
            free(name);               // LCOV_EXCL_LINE
            return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE
          }
          name[i] = c;
          if (len < 0 && c == 0)
            len = i;
          if (c == 0x13)
            break;
        }
        name = realloc(name, len + 1);
        if (!name)
          return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
        mylog("name %d %s\n", len, name);

        if (readValue(reader, 3) != 0x000000) {
          mylog("FHDB type 3 unsupported values: 3bytes"); // LCOV_EXCL_LINE
          free(name);                                      // LCOV_EXCL_LINE
          return MYSOFA_UNSUPPORTED_FORMAT;                // LCOV_EXCL_LINE
        }

        len = (int)readValue(reader, 4);
        if (len > 0x1000 || len < 0) {
          mylog("FHDB type 3 unsupported values: len "); // LCOV_EXCL_LINE
          free(name);                                    // LCOV_EXCL_LINE
          return MYSOFA_UNSUPPORTED_FORMAT;              // LCOV_EXCL_LINE
        }

        /* TODO: Get definition of this field */
        unknown4 = (int)readValue(reader, 8);
        if (unknown4 != 0x00000001 && unknown4 != 0x02000002) {
          mylog("FHDB type 3 unsupported values: unknown4 %" PRIX64 "\n",
                unknown4);                  // LCOV_EXCL_LINE
          free(name);                       // LCOV_EXCL_LINE
          return MYSOFA_UNSUPPORTED_FORMAT; // LCOV_EXCL_LINE
        }
        if (unknown4 == 0x02000002)
          len = 0;

        if (!(value = malloc(len + 1))) {
          free(name);              // LCOV_EXCL_LINE
          return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
        }
        if (mysofa_read(reader, value, len) != len) {
          free(value);              // LCOV_EXCL_LINE
          free(name);               // LCOV_EXCL_LINE
          return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE
        }
        value[len] = 0;

        mylog(" %s = %s\n", name, value);

        attr = malloc(sizeof(struct MYSOFA_ATTRIBUTE));
        if (attr == NULL) {
          free(value);             // LCOV_EXCL_LINE
          free(name);              // LCOV_EXCL_LINE
          return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
        }

        attr->name = name;
        attr->value = value;
        attr->next = dataobject->attributes;
        dataobject->attributes = attr;
        break;

        // LCOV_EXCL_START
      default:
        mylog("FHDB type 1 unsupported values %08" PRIX64 " %" PRIX64 "\n",
              unknown2, (uint64_t)mysofa_tell(reader) - 4);
        return MYSOFA_UNSUPPORTED_FORMAT;
        // LCOV_EXCL_STOP
      }
    } else if (typeandversion != 0) {
      /* TODO is must be avoided somehow */
      mylog("fractal head unknown type %d\n", typeandversion);
      /*			return MYSOFA_UNSUPPORTED_FORMAT; */
      return MYSOFA_OK;
    }

  } while (typeandversion != 0);

  reader->recursive_counter--;
  return MYSOFA_OK;
}

/*  III.G. Disk Format: Level 1G - Fractal Heap
 * indirect block
 */

static int indirectblockRead(struct READER *reader,
                             struct DATAOBJECT *dataobject,
                             struct FRACTALHEAP *fractalheap,
                             uint64_t iblock_size) {
  int size, nrows, max_dblock_rows, k, n, err;
  uint32_t filter_mask;
  uint64_t heap_header_address, block_offset,
      child_direct_block = 0, size_filtered, child_indirect_block;
  long store;

  char buf[5];

  UNUSED(size_filtered);
  UNUSED(heap_header_address);
  UNUSED(filter_mask);

  /* read signature */
  if (mysofa_read(reader, buf, 4) != 4 || strncmp(buf, "FHIB", 4)) {
    mylog("cannot read signature of fractal heap indirect block\n");
    return MYSOFA_INVALID_FORMAT;
  }
  buf[4] = 0;
  mylog("%08" PRIX64 " %.4s\n", (uint64_t)mysofa_tell(reader) - 4, buf);

  if (mysofa_getc(reader) != 0) {
    mylog("object FHIB must have version 0\n");
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  /* ignore it */
  heap_header_address = readValue(reader, reader->superblock.size_of_offsets);

  size = (fractalheap->maximum_heap_size + 7) / 8;
  block_offset = readValue(reader, size);

  if (block_offset) {
    mylog("FHIB block offset is not 0\n");
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  /*	 The number of rows of blocks, nrows, in an indirect block of size
   * iblock_size is given by the following expression: */
  nrows = (log2i(iblock_size) - log2i(fractalheap->starting_block_size)) + 1;

  /* The maximum number of rows of direct blocks, max_dblock_rows, in any
   * indirect block of a fractal heap is given by the following expression: */
  max_dblock_rows = (log2i(fractalheap->maximum_direct_block_size) -
                     log2i(fractalheap->starting_block_size)) +
                    2;

  /* Using the computed values for nrows and max_dblock_rows, along with the
   * Width of the doubling table, the number of direct and indirect block
   * entries (K and N in the indirect block description, below) in an indirect
   * block can be computed: */
  if (nrows < max_dblock_rows)
    k = nrows * fractalheap->table_width;
  else
    k = max_dblock_rows * fractalheap->table_width;

  /* If nrows is less than or equal to max_dblock_rows, N is 0. Otherwise, N is
   * simply computed: */
  n = k - (max_dblock_rows * fractalheap->table_width);

  while (k > 0) {
    child_direct_block = readValue(reader, reader->superblock.size_of_offsets);
    if (fractalheap->encoded_length > 0) {
      size_filtered = readValue(reader, reader->superblock.size_of_lengths);
      filter_mask = readValue(reader, 4);
    }
    mylog(">> %d %" PRIX64 " %d\n", k, child_direct_block, size);
    if (validAddress(reader, child_direct_block)) {
      store = mysofa_tell(reader);
      if (mysofa_seek(reader, child_direct_block, SEEK_SET) < 0)
        return errno;
      err = directblockRead(reader, dataobject, fractalheap);
      if (err)
        return err;
      if (store < 0)
        return MYSOFA_READ_ERROR;
      if (mysofa_seek(reader, store, SEEK_SET) < 0)
        return errno;
    }

    k--;
  }

  while (n > 0) {
    child_indirect_block =
        readValue(reader, reader->superblock.size_of_offsets);

    if (validAddress(reader, child_direct_block)) {
      store = mysofa_tell(reader);
      if (mysofa_seek(reader, child_indirect_block, SEEK_SET) < 0)
        return errno;
      err = indirectblockRead(reader, dataobject, fractalheap, iblock_size * 2);
      if (err)
        return err;
      if (store < 0)
        return MYSOFA_READ_ERROR;
      if (mysofa_seek(reader, store, SEEK_SET) < 0)
        return errno;
    }

    n--;
  }

  return MYSOFA_OK;
}

/*  III.G. Disk Format: Level 1G - Fractal Heap

 00000240  46 52 48 50 00 08 00 00  00 02 00 10 00 00 00 00  |FRHP............|
 00000250  00 00 00 00 00 00 ff ff  ff ff ff ff ff ff a3 0b  |................|
 00000260  00 00 00 00 00 00 1e 03  00 00 00 00 00 00 00 10  |................|
 00000270  00 00 00 00 00 00 00 08  00 00 00 00 00 00 00 08  |................|
 00000280  00 00 00 00 00 00 16 00  00 00 00 00 00 00 00 00  |................|
 00000290  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
 000002a0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 04 00  |................|
 000002b0  00 04 00 00 00 00 00 00  00 00 01 00 00 00 00 00  |................|
 000002c0  28 00 01 00 29 32 00 00  00 00 00 00 01 00 60 49  |(...)2........`I|
 000002d0  32 1d 42 54 48 44 00 08  00 02 00 00 11 00 00 00  |2.BTHD..........|

 */

int fractalheapRead(struct READER *reader, struct DATAOBJECT *dataobject,
                    struct FRACTALHEAP *fractalheap) {
  int err;
  char buf[5];

  /* read signature */
  if (mysofa_read(reader, buf, 4) != 4 || strncmp(buf, "FRHP", 4)) {
    mylog("cannot read signature of fractal heap\n");
    return MYSOFA_UNSUPPORTED_FORMAT;
  }
  buf[4] = 0;
  mylog("%" PRIX64 " %.4s\n", (uint64_t)mysofa_tell(reader) - 4, buf);

  if (mysofa_getc(reader) != 0) {
    mylog("object fractal heap must have version 0\n");
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  fractalheap->heap_id_length = (uint16_t)readValue(reader, 2);
  fractalheap->encoded_length = (uint16_t)readValue(reader, 2);
  if (fractalheap->encoded_length > 0x8000)
    return MYSOFA_UNSUPPORTED_FORMAT;
  fractalheap->flags = (uint8_t)mysofa_getc(reader);
  fractalheap->maximum_size = (uint32_t)readValue(reader, 4);

  fractalheap->next_huge_object_id =
      readValue(reader, reader->superblock.size_of_lengths);
  fractalheap->btree_address_of_huge_objects =
      readValue(reader, reader->superblock.size_of_offsets);
  fractalheap->free_space =
      readValue(reader, reader->superblock.size_of_lengths);
  fractalheap->address_free_space =
      readValue(reader, reader->superblock.size_of_offsets);
  fractalheap->amount_managed_space =
      readValue(reader, reader->superblock.size_of_lengths);
  fractalheap->amount_allocated_space =
      readValue(reader, reader->superblock.size_of_lengths);
  fractalheap->offset_managed_space =
      readValue(reader, reader->superblock.size_of_lengths);
  fractalheap->number_managed_objects =
      readValue(reader, reader->superblock.size_of_lengths);
  fractalheap->size_huge_objects =
      readValue(reader, reader->superblock.size_of_lengths);
  fractalheap->number_huge_objects =
      readValue(reader, reader->superblock.size_of_lengths);
  fractalheap->size_tiny_objects =
      readValue(reader, reader->superblock.size_of_lengths);
  fractalheap->number_tiny_objects =
      readValue(reader, reader->superblock.size_of_lengths);

  fractalheap->table_width = (uint16_t)readValue(reader, 2);

  fractalheap->starting_block_size =
      readValue(reader, reader->superblock.size_of_lengths);
  fractalheap->maximum_direct_block_size =
      readValue(reader, reader->superblock.size_of_lengths);

  fractalheap->maximum_heap_size = (uint16_t)readValue(reader, 2);
  fractalheap->starting_row = (uint16_t)readValue(reader, 2);

  fractalheap->address_of_root_block =
      readValue(reader, reader->superblock.size_of_offsets);

  fractalheap->current_row = (uint16_t)readValue(reader, 2);

  if (fractalheap->encoded_length > 0) {

    fractalheap->size_of_filtered_block =
        readValue(reader, reader->superblock.size_of_lengths);
    fractalheap->fitler_mask = (uint32_t)readValue(reader, 4);

    fractalheap->filter_information = malloc(fractalheap->encoded_length);
    if (!fractalheap->filter_information)
      return MYSOFA_NO_MEMORY;

    if (mysofa_read(reader, fractalheap->filter_information,
                    fractalheap->encoded_length) !=
        fractalheap->encoded_length) {
      return MYSOFA_READ_ERROR;
    }
  }

  if (mysofa_seek(reader, 4, SEEK_CUR) < 0) { /* skip checksum */
    return MYSOFA_READ_ERROR;
  }

  if (fractalheap->number_huge_objects) {
    mylog("cannot handle huge objects\n");
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  if (fractalheap->number_tiny_objects) {
    mylog("cannot handle tiny objects\n");
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  if (validAddress(reader, fractalheap->address_of_root_block)) {

    if (mysofa_seek(reader, fractalheap->address_of_root_block, SEEK_SET) < 0)
      return errno;
    if (fractalheap->current_row)
      err = indirectblockRead(reader, dataobject, fractalheap,
                              fractalheap->starting_block_size);
    else {
      err = directblockRead(reader, dataobject, fractalheap);
    }
    if (err)
      return err;
  }

  return MYSOFA_OK;
}

void fractalheapFree(struct FRACTALHEAP *fractalheap) {
  free(fractalheap->filter_information);
}
