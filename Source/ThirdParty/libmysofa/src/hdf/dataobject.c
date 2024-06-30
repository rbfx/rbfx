/*

 Copyright 2016 Christian Hoene, Symonics GmbH

 */

/* IV.A.1.b. Version 2 Data Object Header Prefix

 00000030  4f 48 44 52 02 2d d3 18  2b 53 d3 18 2b 53 d3 18  |OHDR.-..+S..+S..|
 00000040  2b 53 d3 18 2b 53 f4 01  02 22 00 00 00 00        |+S..+S..."......|
 ....
 00000230  00 00 00 00 00 00 00 00  00 00 00 00 f9 ba 5d c9  |..............].|
 */

#include "reader.h"
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int readOCHK(struct READER *reader, struct DATAOBJECT *dataobject,
                    uint64_t end);

static struct DATAOBJECT *findDataobject(struct READER *reader,
                                         uint64_t address) {
  struct DATAOBJECT *p = reader->all;
  while (p && p->address != address)
    p = p->all;

  return p;
}

/*
 * IV.A.2.a. The NIL Message

 00000090                                 00 9c 01 00 00 00  |................|
 000000a0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
 *
 00000230  00 00 00 00 00 00 00 00  00 00 00 00 f9 ba 5d c9  |..............].|

 */

static int readOHDRHeaderMessageNIL(struct READER *reader, int length) {

  if (mysofa_seek(reader, length, SEEK_CUR) < 0)
    return errno; // LCOV_EXCL_LINE

  return MYSOFA_OK;
}

/*
 * IV.A.2.b. The Dataspace Message

 */

static int readOHDRHeaderMessageDataspace1(struct READER *reader,
                                           struct DATASPACE *ds) {

  int i;

  readValue(reader, 5);

  for (i = 0; i < ds->dimensionality; i++) {
    if (i < 4) {
      ds->dimension_size[i] =
          readValue(reader, reader->superblock.size_of_lengths);
      if (ds->dimension_size[i] > 1000000) {
        mylog("dimension_size is too large\n"); // LCOV_EXCL_LINE
        return MYSOFA_INVALID_FORMAT;           // LCOV_EXCL_LINE
      }
      mylog("   dimension %d %" PRIu64 "\n", i, ds->dimension_size[i]);
    } else
      readValue(reader, reader->superblock.size_of_lengths);
  }

  if (ds->flags & 1) {
    for (i = 0; i < ds->dimensionality; i++) {
      if (i < 4)
        ds->dimension_max_size[i] =
            readValue(reader, reader->superblock.size_of_lengths);
      else
        readValue(reader, reader->superblock.size_of_lengths);
    }
  }

  if (ds->flags & 2) {
    mylog("permutation in OHDR not supported\n"); // LCOV_EXCL_LINE
    return MYSOFA_INVALID_FORMAT;                 // LCOV_EXCL_LINE
  }

  return MYSOFA_OK;
}

static int readOHDRHeaderMessageDataspace2(struct READER *reader,
                                           struct DATASPACE *ds) {

  int i;

  ds->type = (uint8_t)mysofa_getc(reader);

  for (i = 0; i < ds->dimensionality; i++) {
    if (i < 4) {
      ds->dimension_size[i] =
          readValue(reader, reader->superblock.size_of_lengths);
      mylog("   dimension %d %" PRIu64 "\n", i, ds->dimension_size[i]);
    } else
      readValue(reader, reader->superblock.size_of_lengths);
  }

  if (ds->flags & 1) {
    for (i = 0; i < ds->dimensionality; i++) {
      if (i < 4)
        ds->dimension_max_size[i] =
            readValue(reader, reader->superblock.size_of_lengths);
      else
        readValue(reader, reader->superblock.size_of_lengths);
    }
  }

  return MYSOFA_OK;
}

static int readOHDRHeaderMessageDataspace(struct READER *reader,
                                          struct DATASPACE *ds) {

  int version = mysofa_getc(reader);

  ds->dimensionality = (uint8_t)mysofa_getc(reader);
  if (ds->dimensionality > 4) {
    mylog("dimensionality must be lower than 5\n"); // LCOV_EXCL_LINE
    return MYSOFA_INVALID_FORMAT;                   // LCOV_EXCL_LINE
  }

  ds->flags = (uint8_t)mysofa_getc(reader);

  switch (version) {
  case 1:
    return readOHDRHeaderMessageDataspace1(reader, ds);
  case 2:
    return readOHDRHeaderMessageDataspace2(reader, ds);
  default:
    // LCOV_EXCL_START
    mylog("object OHDR dataspace message must have version 1 or 2 but is %X at "
          "%lX\n",
          version, mysofa_tell(reader) - 1);
    return MYSOFA_INVALID_FORMAT;
    // LCOV_EXCL_STOP
  }
}

/*
 * IV.A.2.c. The Link Info Message

 00 03  |+S..+S..."......|
 00000050  0f 00 00 00 00 00 00 00  c9 11 00 00 00 00 00 00  |................|
 00000060  5b 12 00 00 00 00 00 00  81 12 00 00 00 00 00 00  |[...............|
 */

static int readOHDRHeaderMessageLinkInfo(struct READER *reader,
                                         struct LINKINFO *li) {

  if (mysofa_getc(reader) != 0) {
    mylog(
        "object OHDR link info message must have version 0\n"); // LCOV_EXCL_LINE
    return MYSOFA_UNSUPPORTED_FORMAT; // LCOV_EXCL_LINE
  }

  li->flags = (uint8_t)mysofa_getc(reader);

  if (li->flags & 1)
    li->maximum_creation_index = readValue(reader, 8);

  li->fractal_heap_address =
      readValue(reader, reader->superblock.size_of_offsets);
  li->address_btree_index =
      readValue(reader, reader->superblock.size_of_offsets);

  if (li->flags & 2)
    li->address_btree_order =
        readValue(reader, reader->superblock.size_of_offsets);

  return MYSOFA_OK;
}

/*
 * IV.A.2.d. The Datatype Message

 000007c0                       03  14 00 01 00 00|11|21 1f  |..............!.|
 000007d0  00|04 00 00 00|00 00|20  00|17|08|00|17|7f 00 00  |....... ........|
 000007e0  00|05 02 00 01 00 00 03  0a 10 10 00 00 07 00 6d  |...............m|
 000007f0  36 00 00 00 00 00 00 ea  00 00 00 00 00 00 00 15  |6...............|

 */

static int readOHDRHeaderMessageDatatype(struct READER *reader,
                                         struct DATATYPE *dt) {

  int i, j, c, err;
  char *buffer;
  struct DATATYPE dt2;

  dt->class_and_version = (uint8_t)mysofa_getc(reader);
  if ((dt->class_and_version & 0xf0) != 0x10 &&
      (dt->class_and_version & 0xf0) != 0x30) {
    // LCOV_EXCL_START
    mylog("object OHDR datatype message must have version 1 not %d at %lX\n",
          dt->class_and_version >> 4, mysofa_tell(reader) - 1);
    return MYSOFA_UNSUPPORTED_FORMAT;
    // LCOV_EXCL_STOP
  }

  dt->class_bit_field = (uint32_t)readValue(reader, 3);
  dt->size = (uint32_t)readValue(reader, 4);
  if (dt->size > 64)
    return MYSOFA_UNSUPPORTED_FORMAT; // LCOV_EXCL_LINE

  switch (dt->class_and_version & 0xf) {
  case 0: /* int */
    dt->u.i.bit_offset = readValue(reader, 2);
    dt->u.i.bit_precision = readValue(reader, 2);
    mylog("    INT bit %d %d %d %d\n", dt->u.i.bit_offset,
          dt->u.i.bit_precision, dt->class_and_version >> 4, dt->size);
    break;

  case 1: /* float */
    dt->u.f.bit_offset = (uint16_t)readValue(reader, 2);
    dt->u.f.bit_precision = (uint16_t)readValue(reader, 2);
    dt->u.f.exponent_location = (uint8_t)mysofa_getc(reader);
    dt->u.f.exponent_size = (uint8_t)mysofa_getc(reader);
    dt->u.f.mantissa_location = (uint8_t)mysofa_getc(reader);
    dt->u.f.mantissa_size = (uint8_t)mysofa_getc(reader);
    dt->u.f.exponent_bias = (uint32_t)readValue(reader, 4);

    mylog("    FLOAT bit %d %d exponent %d %d MANTISSA %d %d OFFSET %d\n",
          dt->u.f.bit_offset, dt->u.f.bit_precision, dt->u.f.exponent_location,
          dt->u.f.exponent_size, dt->u.f.mantissa_location,
          dt->u.f.mantissa_size, dt->u.f.exponent_bias);

    /* FLOAT bit 0 32 exponent 23 8 MANTISSA 0 23 OFFSET 127
     FLOAT bit 0 64 exponent 52 11 MANTISSA 0 52 OFFSET 1023 */

    if (dt->u.f.bit_offset != 0 || dt->u.f.mantissa_location != 0 ||
        (dt->u.f.bit_precision != 32 && dt->u.f.bit_precision != 64) ||
        (dt->u.f.bit_precision == 32 &&
         (dt->u.f.exponent_location != 23 || dt->u.f.exponent_size != 8 ||
          dt->u.f.mantissa_size != 23 || dt->u.f.exponent_bias != 127)) ||
        (dt->u.f.bit_precision == 64 &&
         (dt->u.f.exponent_location != 52 || dt->u.f.exponent_size != 11 ||
          dt->u.f.mantissa_size != 52 || dt->u.f.exponent_bias != 1023)))
      return MYSOFA_UNSUPPORTED_FORMAT; // LCOV_EXCL_LINE
    break;

  case 3: /* string */
    mylog("    STRING %d %02X\n", dt->size, dt->class_bit_field);
    break;

  case 6:
    mylog("    COMPOUND %d %02X\n", dt->size, dt->class_bit_field);
    switch (dt->class_and_version >> 4) {
    case 3:
      for (i = 0; i < (dt->class_bit_field & 0xffff); i++) {
        int maxsize = 0x1000;
        buffer = malloc(maxsize);
        if (!buffer)
          return MYSOFA_NO_MEMORY;
        for (j = 0; j < maxsize - 1; j++) {
          c = mysofa_getc(reader);
          if (c < 0) {
            free(buffer);
            return MYSOFA_READ_ERROR;
          }
          buffer[j] = c;
          if (c == 0)
            break;
        }
        buffer[j] = 0;

        for (j = 0, c = 0; (dt->size >> (8 * j)) > 0; j++) {
          c |= mysofa_getc(reader) << (8 * j);
        }

        mylog("   COMPOUND %s offset %d\n", buffer, c);

        /* not needed until the data is stored somewhere permanently
         p = realloc(buffer, j);
         if (!p) {
         free(buffer);
         return errno;
         }
         buffer = p;
         */
        free(buffer);

        err = readOHDRHeaderMessageDatatype(reader, &dt2);
        if (err)
          return err; // LCOV_EXCL_LINE
      }
      break;

    case 1:
      for (i = 0; i < (dt->class_bit_field & 0xffff); i++) {
        char name[256];
        int res;
        for (j = 0;; j++) {
          if (j == sizeof(name))
            return MYSOFA_INVALID_FORMAT; // LCOV_EXCL_LINE
          res = mysofa_getc(reader);
          if (res < 0)
            return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE
          name[j] = res;
          if (name[j] == 0)
            break;
        }
        if (mysofa_seek(reader, (7 - j) & 7, SEEK_CUR))
          return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE

        c = readValue(reader, 4);
        int dimension = mysofa_getc(reader);
        if (dimension != 0) {
          mylog("COMPOUND v1 with dimension not supported");
          return MYSOFA_INVALID_FORMAT; // LCOV_EXCL_LINE
        }

        // ignore the following fields
        if (mysofa_seek(reader, 3 + 4 + 4 + 4 * 4, SEEK_CUR))
          return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE

        mylog("  COMPOUND %s %d %d %lX\n", name, c, dimension,
              mysofa_tell(reader));
        err = readOHDRHeaderMessageDatatype(reader, &dt2);
        if (err)
          return err; // LCOV_EXCL_LINE
      }
      break;
    default:
      // LCOV_EXCL_START
      mylog("object OHDR datatype message must have version 1 or 3 not %d\n",
            dt->class_and_version >> 4);
      return MYSOFA_INVALID_FORMAT;
      // LCOV_EXCL_STOP
    }
    break;
  case 7: /* reference */
    mylog("    REFERENCE %d %02X\n", dt->size, dt->class_bit_field);
    break;

  case 9: /* list */
    dt->list = dt->size;
    mylog("  LIST %d\n", dt->size);
    err = readOHDRHeaderMessageDatatype(reader, dt);
    if (err)
      return err; // LCOV_EXCL_LINE
    break;

  default:
    // LCOV_EXCL_START
    mylog("object OHDR datatype message has unknown variable type %d\n",
          dt->class_and_version & 0xf);
    return MYSOFA_UNSUPPORTED_FORMAT;
    // LCOV_EXCL_STOP
  }
  return MYSOFA_OK;
}

/*
 * IV.A.2.f. The Data Storage - Fill Value Message

 000007e0    |05 02 00 01 00 00|03  0a

 */

static int readOHDRHeaderMessageDataFill1or2(struct READER *reader) {

  int spaceAllocationTime = mysofa_getc(reader);
  int fillValueWriteTime = mysofa_getc(reader);
  int fillValueDefined = mysofa_getc(reader);
  if (spaceAllocationTime < 0 || fillValueWriteTime < 0 || fillValueDefined < 0)
    return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE

  if ((spaceAllocationTime & ~1) != 2 || fillValueWriteTime != 2 ||
      (fillValueDefined & ~1) != 0) {
    mylog("spaceAllocationTime %d fillValueWriteTime %d fillValueDefined %d\n",
          spaceAllocationTime, fillValueWriteTime, fillValueDefined);
    return MYSOFA_INVALID_FORMAT; // LCOV_EXCL_LINE
  }
  if (fillValueDefined > 0) {
    uint32_t size = (uint32_t)readValue(reader, 4);
    if (mysofa_seek(reader, size, SEEK_CUR) < 0)
      return errno; // LCOV_EXCL_LINE
  }

  return MYSOFA_OK;
}

static int readOHDRHeaderMessageDataFill3(struct READER *reader) {
  uint8_t flags;
  uint32_t size;

  flags = (uint8_t)mysofa_getc(reader);

  if (flags & (1 << 5)) {
    size = (uint32_t)readValue(reader, 4);
    if (mysofa_seek(reader, size, SEEK_CUR) < 0)
      return errno; // LCOV_EXCL_LINE
  }

  return MYSOFA_OK;
}

static int readOHDRHeaderMessageDataFill(struct READER *reader) {

  int version = mysofa_getc(reader);
  switch (version) {
  case 1:
  case 2:
    return readOHDRHeaderMessageDataFill1or2(reader);
  case 3:
    return readOHDRHeaderMessageDataFill3(reader);
  default:
    // LCOV_EXCL_START
    mylog("object OHDR data storage fill value message must have version 1,2, "
          "or 3 not "
          "%d\n",
          version);
    return MYSOFA_INVALID_FORMAT;
    // LCOV_EXCL_STOP
  }
}

static int readOHDRHeaderMessageDataFillOld(struct READER *reader) {

  uint32_t size;

  size = (uint32_t)readValue(reader, 4);
  if (mysofa_seek(reader, size, SEEK_CUR) < 0)
    return errno; // LCOV_EXCL_LINE

  return MYSOFA_OK;
}

/*
 * IV.A.2.i. The Data Layout Message

 00000ec0                       08  00 00 00 00 00 00 00 00 |......+.........|
 00000ed0  00 9e 47 0b 16 00 01 00  00 02 02 02 00 01 00 01 |..G.............|
 00000ee0  00 08 00 00 00 01 00 01  00 01 00 01 00 00 00 08 |................|
 00000ef0  17 00 01 00 00 03 02 03  01 42 00 00 00 00 00 00 |.........B......|
 00000f00  01 00 00 00 03 00 00 00  08 00 00 00 15 1c 00 04 |................|


 03 02 03  01 42 00 00 00 00 00 00  |.........B......|
 00000f00  01 00 00 00 03 00 00 00  08 00 00 00 15 1c 00 04 |................|
 00000f10  00 00 00 03 03 00 ff ff  ff ff ff ff ff ff ff ff |................|
 00000f20  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff 0c 23 |...............#|
 00000f30  00 00 00 00 03 00 05 00  08 00 04 00 00 54 79 70 |.............Typ|


 */

static int readOHDRHeaderMessageDataLayout(struct READER *reader,
                                           struct DATAOBJECT *data) {

  int i, err;
  unsigned size;

  uint8_t dimensionality, layout_class;
  uint32_t dataset_element_size;
  uint64_t data_address, store, data_size;

  UNUSED(dataset_element_size);
  UNUSED(data_size);

  if (mysofa_getc(reader) != 3) {
    // LCOV_EXCL_START
    mylog("object OHDR message data layout message must have version 3\n");
    return MYSOFA_INVALID_FORMAT;
    // LCOV_EXCL_STOP
  }

  layout_class = (uint8_t)mysofa_getc(reader);
  mylog("data layout %d\n", layout_class);

  switch (layout_class) {
#if 0
	case 0:
	data_size = readValue(reader, 2);
	mysofa_seek(reader, data_size, SEEK_CUR);
	mylog("TODO 0 SIZE %u\n", data_size);
	break;
#endif
  case 1:
    data_address = readValue(reader, reader->superblock.size_of_offsets);
    data_size = readValue(reader, reader->superblock.size_of_lengths);
    mylog("CHUNK Contiguous SIZE %" PRIu64 "\n", data_size);

    if (validAddress(reader, data_address)) {
      store = mysofa_tell(reader);
      if (mysofa_seek(reader, data_address, SEEK_SET) < 0)
        return errno; // LCOV_EXCL_LINE
      if (data->data) {
        free(data->data);
        data->data = NULL;
      }
      if (data_size > 0x10000000)
        return MYSOFA_INVALID_FORMAT;
      data->data_len = data_size;
      data->data = calloc(1, data_size);
      if (!data->data)
        return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE

      err = mysofa_read(reader, data->data, data_size);
      if (err != data_size)
        return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE
      if (mysofa_seek(reader, store, SEEK_SET) < 0)
        return errno; // LCOV_EXCL_LINE
    }
    break;

  case 2:
    dimensionality = (uint8_t)mysofa_getc(reader);
    mylog("dimensionality %d\n", dimensionality);

    if (dimensionality < 1 || dimensionality > DATAOBJECT_MAX_DIMENSIONALITY) {
      mylog("data layout 2: invalid dimensionality %d %lu %lu\n",
            dimensionality, sizeof(data->datalayout_chunk),
            sizeof(data->datalayout_chunk[0]));
      return MYSOFA_INVALID_FORMAT; // LCOV_EXCL_LINE
    }
    data_address = readValue(reader, reader->superblock.size_of_offsets);
    mylog(" CHUNK %" PRIX64 "\n", data_address);
    for (i = 0; i < dimensionality; i++) {
      data->datalayout_chunk[i] = readValue(reader, 4);
      mylog(" %d\n", data->datalayout_chunk[i]);
    }
    /* TODO last entry? error in spec: ?*/

    size = data->datalayout_chunk[dimensionality - 1];
    for (i = 0; i < data->ds.dimensionality; i++)
      size *= data->ds.dimension_size[i];

    if (validAddress(reader, data_address) && dimensionality <= 4) {
      store = mysofa_tell(reader);
      if (mysofa_seek(reader, data_address, SEEK_SET) < 0)
        return errno; // LCOV_EXCL_LINE
      if (!data->data) {
        if (size > 0x10000000)
          return MYSOFA_INVALID_FORMAT; // LCOV_EXCL_LINE
        data->data_len = size;
        data->data = calloc(1, size);
        if (!data->data)
          return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
      }
      err = treeRead(reader, data);
      if (err)
        return err; // LCOV_EXCL_LINE
      if (mysofa_seek(reader, store, SEEK_SET) < 0)
        return errno; // LCOV_EXCL_LINE
    }
    break;

  default:
    // LCOV_EXCL_START
    mylog("object OHDR message data layout message has unknown layout class "
          "%d\n",
          layout_class);
    return MYSOFA_INVALID_FORMAT;
    // LCOV_EXCL_STOP
  }

  return MYSOFA_OK;
}

/*
 * IV.A.2.k. The Group Info Message

 *  00000070  0a 02 00 01 00 00 00 00
 *
 */

static int readOHDRHeaderMessageGroupInfo(struct READER *reader,
                                          struct GROUPINFO *gi) {

  if (mysofa_getc(reader) != 0) {
    // LCOV_EXCL_START
    mylog("object OHDR group info message must have version 0\n");
    return MYSOFA_UNSUPPORTED_FORMAT;
    // LCOV_EXCL_STOP
  }

  gi->flags = (uint8_t)mysofa_getc(reader);

  if (gi->flags & 1) {
    gi->maximum_compact_value = (uint16_t)readValue(reader, 2);
    gi->minimum_dense_value = (uint16_t)readValue(reader, 2);
  }

  if (gi->flags & 2) {
    gi->number_of_entries = (uint16_t)readValue(reader, 2);
    gi->length_of_entries = (uint16_t)readValue(reader, 2);
  }
  return MYSOFA_OK;
}

/*
 * IV.A.2.l. The Data Storage - Filter Pipeline Message
 *
 *  00000070  0a 02 00 01 00 00 00 00
 *
 */

/* type 1

                                                        00  |......G.8.......|
000010a0  00 00 00 00 00 02 00 08  00 01 00 01 00 73 68 75  |.............shu|
000010b0  66 66 6c 65 00 08 00 00  00 00 00 00 00 01 00 08  |ffle............|
000010c0  00 01 00 01 00 64 65 66  6c 61 74 65 00 01 00 00  |.....deflate....|
000010d0  00 00 00 00 00 08 17 00  01 00 00 03 02 03 01 48  |...............H|
*/
static int readOHDRHeaderMessageFilterPipelineV1(struct READER *reader,
                                                 uint8_t filters) {
  int i, j;
  uint16_t filter_identification_value, flags, number_client_data_values,
      namelength;

  if (readValue(reader, 6) != 0) {
    mylog("reserved values not zero\n");
    return MYSOFA_INVALID_FORMAT;
  }

  for (i = 0; i < filters; i++) {
    filter_identification_value = (uint16_t)readValue(reader, 2);
    switch (filter_identification_value) {
    case 1:
    case 2:
      break;
    default:
      // LCOV_EXCL_START
      mylog("object OHDR filter pipeline message contains unsupported filter: "
            "%d %lX\n",
            filter_identification_value, mysofa_tell(reader) - 2);
      return MYSOFA_INVALID_FORMAT;
      // LCOV_EXCL_STOP
    }
    namelength = (uint16_t)readValue(reader, 2);
    flags = (uint16_t)readValue(reader, 2);
    number_client_data_values = (uint16_t)readValue(reader, 2);

    if (namelength > 0)
      if (mysofa_seek(reader, ((namelength - 1) & ~7) + 8, SEEK_CUR) ==
          -1)                     // skip name
        return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE

    mylog("  filter %d namelen %d flags %04X values %d\n",
          filter_identification_value, namelength, flags,
          number_client_data_values);

    if (number_client_data_values > 0x1000)
      return MYSOFA_UNSUPPORTED_FORMAT; // LCOV_EXCL_LINE
    /* no name here */
    for (j = 0; j < number_client_data_values; j++) {
      readValue(reader, 4);
    }
    if ((number_client_data_values & 1) == 1)
      readValue(reader, 4);
  }

  return MYSOFA_OK;
}

static int readOHDRHeaderMessageFilterPipelineV2(struct READER *reader,
                                                 uint8_t filters) {
  int i, j;
  uint16_t filter_identification_value, flags, number_client_data_values;
  uint32_t client_data;
  uint64_t maximum_compact_value, minimum_dense_value, number_of_entries,
      length_of_entries;

  UNUSED(flags);
  UNUSED(client_data);
  UNUSED(maximum_compact_value);
  UNUSED(minimum_dense_value);
  UNUSED(number_of_entries);
  UNUSED(length_of_entries);

  for (i = 0; i < filters; i++) {
    filter_identification_value = (uint16_t)readValue(reader, 2);
    switch (filter_identification_value) {
    case 1:
    case 2:
      break;
    default:
      // LCOV_EXCL_START
      mylog("object OHDR filter pipeline message contains unsupported filter: "
            "%d\n",
            filter_identification_value);
      return MYSOFA_INVALID_FORMAT;
      // LCOV_EXCL_STOP
    }
    mylog("  filter %d\n", filter_identification_value);
    flags = (uint16_t)readValue(reader, 2);
    number_client_data_values = (uint16_t)readValue(reader, 2);
    if (number_client_data_values > 0x1000)
      return MYSOFA_UNSUPPORTED_FORMAT; // LCOV_EXCL_LINE
    /* no name here */
    for (j = 0; j < number_client_data_values; j++) {
      client_data = readValue(reader, 4);
    }
  }

  return MYSOFA_OK;
}

static int readOHDRHeaderMessageFilterPipeline(struct READER *reader) {
  int filterversion, filters;

  filterversion = mysofa_getc(reader);
  filters = mysofa_getc(reader);

  if (filterversion < 0 || filters < 0)
    return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE

  if (filters > 32) {
    // LCOV_EXCL_START
    mylog("object OHDR filter pipeline message has too many filters: %d\n",
          filters);
    return MYSOFA_INVALID_FORMAT;
    // LCOV_EXCL_STOP
  }

  switch (filterversion) {
  case 1:
    return readOHDRHeaderMessageFilterPipelineV1(reader, filters);
  case 2:
    return readOHDRHeaderMessageFilterPipelineV2(reader, filters);
  default:
    // LCOV_EXCL_START
    mylog(
        "object OHDR filter pipeline message must have version 1 or 2 not %d\n",
        filterversion);
    return MYSOFA_INVALID_FORMAT;
    // LCOV_EXCL_STOP
  }
}

int readDataVar(struct READER *reader, struct DATAOBJECT *data,
                struct DATATYPE *dt, struct DATASPACE *ds) {

  char *buffer, number[20];
  uint64_t reference, gcol = 0, dataobject;
  int err;
  struct DATAOBJECT *referenceData;

  if (dt->list) {
    if (dt->list - dt->size == 8) {
      readValue(reader, 4); /* TODO unknown? */
      gcol = readValue(reader, 4);
    } else {
      gcol = readValue(reader, dt->list - dt->size);
    }
    mylog("    GCOL %d %8" PRIX64 " %8lX\n", dt->list - dt->size, gcol,
          mysofa_tell(reader));
    /*	 mysofa_seek(reader, dt->list - dt->size, SEEK_CUR); TODO:
     * TODO: missing part in specification */
  }

  switch (dt->class_and_version & 0xf) {
  case 0:
    mylog("FIXED POINT todo %lX %d\n", mysofa_tell(reader), dt->size);
    if (mysofa_seek(reader, dt->size, SEEK_CUR))
      return errno; // LCOV_EXCL_LINE
    break;

  case 3:
    buffer = malloc(dt->size + 1);
    if (buffer == NULL) {
      return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
    }
    if (mysofa_read(reader, buffer, dt->size) != dt->size) {
      free(buffer);             // LCOV_EXCL_LINE
      return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE
    }
    buffer[dt->size] = 0;
    mylog("STRING %s\n", buffer);
    data->string = buffer;
    break;

    /*
     * 000036e3   67 0e 00 00 00  00 00 00 00 00 00 00 00  |...g............|
     000036f0  00 00 00
     */
  case 6:
    /* TODO unclear spec */
    mylog("COMPONENT todo %lX %d\n", mysofa_tell(reader), dt->size);
    if (mysofa_seek(reader, dt->size, SEEK_CUR))
      return errno; // LCOV_EXCL_LINE
    break;

  case 7:
    readValue(reader, 4); /* TODO unclear reference */
    reference = readValue(reader, dt->size - 4);
    mylog(" REFERENCE size %d %" PRIX64 "\n", dt->size, reference);
    if (!!(err = gcolRead(reader, gcol, reference, &dataobject))) {
      return MYSOFA_OK; /* ignore error. TODO: why?
       return err; */
    }
    referenceData = findDataobject(reader, dataobject);
    if (referenceData)
      buffer = referenceData->name;
    else {
      sprintf(number, "REF%08lX", (long unsigned int)reference);
      buffer = number;
    }
    mylog("    REFERENCE %" PRIX64 " %" PRIX64 " %s\n", reference, dataobject,
          buffer);
    /*		if(!referenceData) { TODO?
     return MYSOFA_UNSUPPORTED_FORMAT;
     } */
    if (data->string) {
      data->string =
          realloc(data->string, strlen(data->string) + strlen(buffer) + 2);
      if (!data->string)
        return MYSOFA_NO_MEMORY;
      strcat(data->string, ",");
      strcat(data->string, buffer);
    } else {
      data->string = mysofa_strdup(buffer);
    }
    break;

  default:
    // LCOV_EXCL_START
    mylog("data reader unknown type %d\n", dt->class_and_version & 0xf);
    return MYSOFA_INTERNAL_ERROR;
    // LCOV_EXCL_STOP
  }
  return MYSOFA_OK;
}

int readDataDim(struct READER *reader, struct DATAOBJECT *da,
                struct DATATYPE *dt, struct DATASPACE *ds, int dim) {
  int i, err;

  if (dim >= sizeof(ds->dimension_size) / sizeof(ds->dimension_size[0]))
    return MYSOFA_UNSUPPORTED_FORMAT; // LCOV_EXCL_LINE

  for (i = 0; i < ds->dimension_size[dim]; i++) {
    if (dim + 1 < ds->dimensionality) {
      if (!!(err = readDataDim(reader, da, dt, ds, dim + 1))) {
        return err; // LCOV_EXCL_LINE
      }
    } else {
      if (!!(err = readDataVar(reader, da, dt, ds))) {
        return err; // LCOV_EXCL_LINE
      }
    }
  }
  return MYSOFA_OK;
}

int readData(struct READER *reader, struct DATAOBJECT *da, struct DATATYPE *dt,
             struct DATASPACE *ds) {
  if (ds->dimensionality == 0) {
    ds->dimension_size[0] = 1;
  }
  return readDataDim(reader, da, dt, ds, 0);
}

/*
 IV.A.2.q. The Object Header Continuation Message

 10 10 00 00 07 00 6d  |...............m|
 000007f0  36 00 00 00 00 00 00 ea  00 00 00 00 00 00 00 15 |6...............|
 */

static int readOHDRHeaderMessageContinue(struct READER *reader,
                                         struct DATAOBJECT *dataobject) {

  int err;
  uint64_t offset, length;
  long store;

  offset = readValue(reader, reader->superblock.size_of_offsets);
  length = readValue(reader, reader->superblock.size_of_lengths);
  if (offset > 0x2000000 || length > 0x10000000)
    return MYSOFA_UNSUPPORTED_FORMAT; // LCOV_EXCL_LINE

  mylog(" continue %08" PRIX64 " %08" PRIX64 "\n", offset, length);
  if (reader->recursive_counter >= 25) {
    mylog("recursive problem");
    return MYSOFA_UNSUPPORTED_FORMAT; // LCOV_EXCL_LINE
  } else
    reader->recursive_counter++;

  store = mysofa_tell(reader);

  if (mysofa_seek(reader, offset, SEEK_SET) < 0)
    return errno; // LCOV_EXCL_LINE

  err = readOCHK(reader, dataobject, offset + length);
  if (err)
    return err; // LCOV_EXCL_LINE

  if (store < 0)
    return MYSOFA_READ_ERROR; // LCOV_EXCL_LINE
  if (mysofa_seek(reader, store, SEEK_SET) < 0)
    return errno; // LCOV_EXCL_LINE

  mylog(" continue back\n");
  return MYSOFA_OK;
}

/*
 IV.A.2.m. The Attribute Message

 */

static int readOHDRHeaderMessageAttribute(struct READER *reader,
                                          struct DATAOBJECT *dataobject) {
  int err;

  uint8_t flags, encoding;
  uint16_t name_size, datatype_size, dataspace_size;
  char *name;
  struct DATAOBJECT d;
  struct MYSOFA_ATTRIBUTE *attr;

  UNUSED(encoding);
  UNUSED(datatype_size);
  UNUSED(dataspace_size);

  memset(&d, 0, sizeof(d));

  int version = mysofa_getc(reader);

  if (version != 1 && version != 3) {
    // LCOV_EXCL_START
    mylog("object OHDR attribute message must have version 1 or 3\n");
    return MYSOFA_INVALID_FORMAT;
    // LCOV_EXCL_STOP
  }

  flags = (uint8_t)mysofa_getc(reader);

  name_size = (uint16_t)readValue(reader, 2);
  datatype_size = (uint16_t)readValue(reader, 2);
  dataspace_size = (uint16_t)readValue(reader, 2);
  if (version == 3)
    encoding = (uint8_t)mysofa_getc(reader);

  if (name_size > 0x1000)
    return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
  name = malloc(name_size + 1);
  if (!name)
    return MYSOFA_NO_MEMORY; // LCOV_EXCL_LINE
  if (mysofa_read(reader, name, name_size) != name_size) {
    free(name);   // LCOV_EXCL_LINE
    return errno; // LCOV_EXCL_LINE
  }
  if (version == 1 && mysofa_seek(reader, (8 - name_size) & 7, SEEK_CUR) != 0) {
    free(name);   // LCOV_EXCL_LINE
    return errno; // LCOV_EXCL_LINE
  }

  name[name_size] = 0;
  mylog("  attribute name %s %d %d %lX\n", name, datatype_size, dataspace_size,
        mysofa_tell(reader));

  if (version == 3 && (flags & 3)) {
    // LCOV_EXCL_START
    mylog("object OHDR attribute message must not have any flags set\n");
    free(name);
    return MYSOFA_INVALID_FORMAT;
    // LCOV_EXCL_STOP
  }
  err = readOHDRHeaderMessageDatatype(reader, &d.dt);
  if (err) {
    // LCOV_EXCL_START
    mylog("object OHDR attribute message read datatype error\n");
    free(name);
    return MYSOFA_INVALID_FORMAT;
    // LCOV_EXCL_STOP
  }
  if (version == 1) {
    if (mysofa_seek(reader, (8 - datatype_size) & 7, SEEK_CUR) < 0) {
      // LCOV_EXCL_START
      free(name);
      return errno;
      // LCOV_EXCL_STOP
    }
  }

  err = readOHDRHeaderMessageDataspace(reader, &d.ds);
  if (err) {
    // LCOV_EXCL_START
    mylog("object OHDR attribute message read dataspace error\n");
    free(name);
    return MYSOFA_INVALID_FORMAT;
    // LCOV_EXCL_STOP
  }
  if (version == 1) {
    if (mysofa_seek(reader, (8 - dataspace_size) & 7, SEEK_CUR) < 0) {
      // LCOV_EXCL_START
      free(name);
      return errno;
      // LCOV_EXCL_STOP
    }
  }
  err = readData(reader, &d, &d.dt, &d.ds);
  if (err) {
    mylog("object OHDR attribute message read data error\n");
    free(name);
    return MYSOFA_INVALID_FORMAT;
  }

  attr = malloc(sizeof(struct MYSOFA_ATTRIBUTE));
  if (!attr) {
    // LCOV_EXCL_START
    free(name);
    return MYSOFA_NO_MEMORY;
    // LCOV_EXCL_STOP
  }
  attr->name = name;
  attr->value = d.string;
  d.string = NULL;
  attr->next = dataobject->attributes;
  dataobject->attributes = attr;

  dataobjectFree(reader, &d);
  return MYSOFA_OK;
}

/*
 * IV.A.2.v. The Attribute Info Message

 00000070                           15 1c 00 04 00 00 00 03 |................|
 00000080  16 00 40 02 00 00 00 00  00 00 d2 02 00 00 00 00 |..@.............|
 00000090  00 00 f8 02 00 00 00 00  00 00

 */

static int readOHDRHeaderMessageAttributeInfo(struct READER *reader,
                                              struct ATTRIBUTEINFO *ai) {

  if (mysofa_getc(reader) != 0) {
    mylog("object OHDR attribute info message must have version 0\n");
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  ai->flags = (uint8_t)mysofa_getc(reader);

  if (ai->flags & 1)
    ai->maximum_creation_index = readValue(reader, 2);

  ai->fractal_heap_address =
      readValue(reader, reader->superblock.size_of_offsets);
  ai->attribute_name_btree =
      readValue(reader, reader->superblock.size_of_offsets);

  if (ai->flags & 2)
    ai->attribute_creation_order_btree =
        readValue(reader, reader->superblock.size_of_offsets);

  return MYSOFA_OK;
}

/**
 * read all OHDR messages
 */
static int readOHDRmessages(struct READER *reader,
                            struct DATAOBJECT *dataobject,
                            uint64_t end_of_messages) {

  int err;
  long end;

  while (mysofa_tell(reader) <
         end_of_messages - 4) { /* final gap may has a size of up to 3 */
    uint8_t header_message_type = (uint8_t)mysofa_getc(reader);
    uint16_t header_message_size = (uint16_t)readValue(reader, 2);
    uint8_t header_message_flags = (uint8_t)mysofa_getc(reader);
    if ((header_message_flags & ~5) != 0) {
      mylog("OHDR unsupported OHDR message flag %02X\n", header_message_flags);
      return MYSOFA_UNSUPPORTED_FORMAT;
    }

    if ((dataobject->flags & (1 << 2)) != 0)
      /* ignore header_creation_order */
      if (mysofa_seek(reader, 2, SEEK_CUR) < 0)
        return errno;

    mylog(" OHDR message type %2d offset %6lX len %4X\n", header_message_type,
          mysofa_tell(reader), header_message_size);

    end = mysofa_tell(reader) + header_message_size;

    switch (header_message_type) {
    case 0: /* NIL Message */
      if (!!(err = readOHDRHeaderMessageNIL(reader, header_message_size)))
        return err;
      break;
    case 1: /* Dataspace Message */
      if (!!(err = readOHDRHeaderMessageDataspace(reader, &dataobject->ds)))
        return err;
      break;
    case 2: /* Link Info Message */
      if (!!(err = readOHDRHeaderMessageLinkInfo(reader, &dataobject->li)))
        return err;
      break;
    case 3: /* Datatype Message */
      if (!!(err = readOHDRHeaderMessageDatatype(reader, &dataobject->dt)))
        return err;
      break;
    case 4: /* Data Fill Message Old */
      if (!!(err = readOHDRHeaderMessageDataFillOld(reader)))
        return err;
      break;
    case 5: /* Data Fill Message */
      if (!!(err = readOHDRHeaderMessageDataFill(reader)))
        return err;
      break;
    case 8: /* Data Layout Message */
      if (!!(err = readOHDRHeaderMessageDataLayout(reader, dataobject)))
        return err;
      break;
    case 10: /* Group Info Message */
      if (!!(err = readOHDRHeaderMessageGroupInfo(reader, &dataobject->gi)))
        return err;
      break;
    case 11: /* Filter Pipeline Message */
      if (!!(err = readOHDRHeaderMessageFilterPipeline(reader)))
        return err;
      break;
    case 12: /* Attribute Message */
      if (!!(err = readOHDRHeaderMessageAttribute(reader, dataobject)))
        return err;
      break;
    case 16: /* Continue Message */
      if (!!(err = readOHDRHeaderMessageContinue(reader, dataobject)))
        return err;
      break;
    case 21: /* Attribute Info Message */
      if (!!(err = readOHDRHeaderMessageAttributeInfo(reader, &dataobject->ai)))
        return err;
      break;
    default:
      mylog("OHDR unknown header message of type %d\n", header_message_type);

      return MYSOFA_UNSUPPORTED_FORMAT;
    }

    if (mysofa_tell(reader) != end) {
      mylog("OHDR message length mismatch by %ld\n", mysofa_tell(reader) - end);
      return MYSOFA_INTERNAL_ERROR;
    }
  }

  if (mysofa_seek(reader, end_of_messages + 4, SEEK_SET) <
      0) /* skip checksum */
    return errno;

  return MYSOFA_OK;
}

static int readOCHK(struct READER *reader, struct DATAOBJECT *dataobject,
                    uint64_t end) {
  int err;
  char buf[5];

  /* read signature */
  if (mysofa_read(reader, buf, 4) != 4 || strncmp(buf, "OCHK", 4)) {
    mylog("cannot read signature of OCHK\n");
    return MYSOFA_INVALID_FORMAT;
  }
  buf[4] = 0;
  mylog("%08" PRIX64 " %.4s\n", (uint64_t)mysofa_tell(reader) - 4, buf);

  err = readOHDRmessages(reader, dataobject, end - 4); /* subtract checksum */
  if (err) {
    return err;
  }

  return MYSOFA_OK;
}

int dataobjectRead(struct READER *reader, struct DATAOBJECT *dataobject,
                   char *name) {
  uint64_t size_of_chunk, end_of_messages;
  int err;
  char buf[5];

  memset(dataobject, 0, sizeof(*dataobject));
  dataobject->address = mysofa_tell(reader);
  dataobject->name = name;

  /* read signature */
  if (mysofa_read(reader, buf, 4) != 4 || strncmp(buf, "OHDR", 4)) {
    mylog("cannot read signature of data object\n");
    return MYSOFA_INVALID_FORMAT;
  }
  buf[4] = 0;
  mylog("%08" PRIX64 " %.4s\n", dataobject->address, buf);

  if (mysofa_getc(reader) != 2) {
    mylog("object OHDR must have version 2\n");
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  dataobject->flags = (uint8_t)mysofa_getc(reader);

  if (dataobject->flags & (1 << 5)) {          /* bit 5 indicated time stamps */
    if (mysofa_seek(reader, 16, SEEK_CUR) < 0) /* skip them */
      return errno;
  }

  if (dataobject->flags & (1 << 4)) { /* bit 4 ? */
    mylog("OHDR: unsupported flags bit 4: %02X\n", dataobject->flags);
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  size_of_chunk = readValue(reader, 1 << (dataobject->flags & 3));
  if (size_of_chunk > 0x1000000)
    return MYSOFA_UNSUPPORTED_FORMAT;

  end_of_messages = mysofa_tell(reader) + size_of_chunk;

  err = readOHDRmessages(reader, dataobject, end_of_messages);

  if (err) {
    return err;
  }

  if (validAddress(reader, dataobject->ai.attribute_name_btree)) {
    /* not needed
         mysofa_seek(reader, dataobject->ai.attribute_name_btree, SEEK_SET);
         btreeRead(reader, &dataobject->attributes);
    */
  }

  /* parse message attribute info */
  if (validAddress(reader, dataobject->ai.fractal_heap_address)) {
    if (mysofa_seek(reader, dataobject->ai.fractal_heap_address, SEEK_SET) < 0)
      return errno;
    err = fractalheapRead(reader, dataobject, &dataobject->attributes_heap);
    if (err)
      return err;
  }

  /* parse message link info */
  if (validAddress(reader, dataobject->li.fractal_heap_address)) {
    mysofa_seek(reader, dataobject->li.fractal_heap_address, SEEK_SET);
    err = fractalheapRead(reader, dataobject, &dataobject->objects_heap);
    if (err)
      return err;
  }

  if (validAddress(reader, dataobject->li.address_btree_index)) {
    /* not needed
       mysofa_seek(reader, dataobject->li.address_btree_index, SEEK_SET);
       btreeRead(reader, &dataobject->objects);
     */
  }

  dataobject->all = reader->all;
  reader->all = dataobject;

  return MYSOFA_OK;
}

void dataobjectFree(struct READER *reader, struct DATAOBJECT *dataobject) {
  struct DATAOBJECT **p;

  btreeFree(&dataobject->attributes_btree);
  fractalheapFree(&dataobject->attributes_heap);
  btreeFree(&dataobject->objects_btree);
  fractalheapFree(&dataobject->objects_heap);

  while (dataobject->attributes) {
    struct MYSOFA_ATTRIBUTE *attr = dataobject->attributes;
    dataobject->attributes = attr->next;
    free(attr->name);
    free(attr->value);
    free(attr);
  }

  while (dataobject->directory) {
    struct DIR *dir = dataobject->directory;
    dataobject->directory = dir->next;
    dataobjectFree(reader, &dir->dataobject);
    free(dir);
  }

  free(dataobject->data);
  free(dataobject->string);
  free(dataobject->name);

  p = &reader->all;
  while (*p) {
    if ((*p) == dataobject) {
      *p = dataobject->all;
      break;
    }
    p = &((*p)->all);
  }
}
