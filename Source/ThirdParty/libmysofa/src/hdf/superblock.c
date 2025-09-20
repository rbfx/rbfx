/*

 Copyright 2016 Christian Hoene, Symonics GmbH

 */

#include "reader.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* read superblock
 00000000  89 48 44 46 0d 0a 1a 0a  02 08 08 00 00 00 00 00  |.HDF............|
 00000010  00 00 00 00 ff ff ff ff  ff ff ff ff bc 62 12 00  |.............b..|
 00000020  00 00 00 00 30 00 00 00  00 00 00 00 27 33 a3 16  |....0.......'3..|
 */

int superblockRead2or3(struct READER *reader, struct SUPERBLOCK *superblock) {
  superblock->size_of_offsets = (uint8_t)mysofa_getc(reader);
  superblock->size_of_lengths = (uint8_t)mysofa_getc(reader);
  if (mysofa_getc(reader) < 0) /* File Consistency Flags */
    return MYSOFA_READ_ERROR;

  if (superblock->size_of_offsets < 2 || superblock->size_of_offsets > 8 ||
      superblock->size_of_lengths < 2 || superblock->size_of_lengths > 8) {
    mylog("size of offsets and length is invalid: %d %d\n",
          superblock->size_of_offsets, superblock->size_of_lengths);
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  superblock->base_address = readValue(reader, superblock->size_of_offsets);
  superblock->superblock_extension_address =
      readValue(reader, superblock->size_of_offsets);
  superblock->end_of_file_address =
      readValue(reader, superblock->size_of_offsets);
  superblock->root_group_object_header_address =
      readValue(reader, superblock->size_of_offsets);

  if (superblock->base_address != 0) {
    mylog("base address is not null\n");
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  if (mysofa_seek(reader, 0L, SEEK_END))
    return errno;

  if (superblock->end_of_file_address != mysofa_tell(reader)) {
    mylog("file size mismatch\n");
    return MYSOFA_INVALID_FORMAT;
  }

  /* end of superblock */

  /* seek to first object */
  if (mysofa_seek(reader, superblock->root_group_object_header_address,
                  SEEK_SET)) {
    mylog("cannot seek to first object at %" PRId64 "\n",
          superblock->root_group_object_header_address);
    return errno;
  }

  return dataobjectRead(reader, &superblock->dataobject, NULL);
}

int superblockRead0or1(struct READER *reader, struct SUPERBLOCK *superblock,
                       int version) {
  if (mysofa_getc(reader) !=
      0) /* Version Number of the File’s Free Space Information */
    return MYSOFA_INVALID_FORMAT;

  if (mysofa_getc(reader) !=
      0) /* Version Number of the Root Group Symbol Table Entry */
    return MYSOFA_INVALID_FORMAT;

  if (mysofa_getc(reader) != 0)
    return MYSOFA_INVALID_FORMAT;

  if (mysofa_getc(reader) !=
      0) /* Version Number of the Shared Header Message Format */
    return MYSOFA_INVALID_FORMAT;

  superblock->size_of_offsets = (uint8_t)mysofa_getc(reader);
  superblock->size_of_lengths = (uint8_t)mysofa_getc(reader);

  if (mysofa_getc(reader) != 0)
    return MYSOFA_INVALID_FORMAT;

  if (superblock->size_of_offsets < 2 || superblock->size_of_offsets > 8 ||
      superblock->size_of_lengths < 2 || superblock->size_of_lengths > 8) {
    mylog("size of offsets and length is invalid: %d %d\n",
          superblock->size_of_offsets, superblock->size_of_lengths);
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  /**
  00000000  89 48 44 46 0d 0a 1a 0a  02 08 08 00 00 00 00 00  |.HDF............|
  00000010  00 00 00 00 ff ff ff ff  ff ff ff ff 72 4d 02 00  |............rM..|
  00000020  00 00 00 00 30 00 00 00  00 00 00 00 e5 a2 a2 a2  |....0...........|

  00000000  89 48 44 46 0d 0a 1a 0a  00 00 00 00 00 08 08 00  |.HDF............|
  00000010  04 00 10 00 01 00 00 00  00 00 00 00 00 00 00 00  |................|
  00000020  ff ff ff ff ff ff ff ff  36 41 9c 00 00 00 00 00  |........6A......|
  00000030  ff ff ff ff ff ff ff ff  00 00 00 00 00 00 00 00  |................|
  00000040  60 00 00 00 00 00 00 00  01 00 00 00 00 00 00 00  |`...............|
  00000050  88 00 00 00 00 00 00 00  a8 02 00 00 00 00 00 00  |................|
  00000060  01 00 1c 00 01 00 00 00  18 00 00 00 00 00 00 00  |................|
  00000070  10 00 10 00 00 00 00 00  20 03 00 00 00 00 00 00  |........ .......|
  00000080  c8 07 00 00 00 00 00 00  54 52 45 45 00 00 02 00  |........TREE....|

  00000000  89 48 44 46 0d 0a 1a 0a  00 00 00 00 00 08 08 00  |.HDF............|
  00000010  04 00 10 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
  00000020  ff ff ff ff ff ff ff ff  a6 e6 11 00 00 00 00 00  |................|
  00000030  ff ff ff ff ff ff ff ff  00 00 00 00 00 00 00 00  |................|
  00000040  60 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |`...............|
  00000050  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
  00000060  4f 48 44 52 02 0d 45 02  02 22 00 00 00 00 00 03  |OHDR..E.."......|
  */

  /*int groupLeafNodeK = */ readValue(reader, 2);
  /*int groupInternalNodeK = */ readValue(reader, 2);

  if (readValue(reader, 4) != 0) { // has been 1 not 0 in Steinberg file
    mylog("File Consistency Flags are not zero\n");
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  if (version == 1) {
    readValue(reader, 2); /* Indexed Storage Internal Node K */
    readValue(reader, 2); /* Reserved (zero) */
  }

  superblock->base_address = readValue(reader, superblock->size_of_offsets);
  if (superblock->base_address != 0) {
    mylog("base address is not null\n");
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  readValue(reader,
            superblock->size_of_offsets); /* Address of File Free space Info */

  superblock->end_of_file_address =
      readValue(reader, superblock->size_of_offsets);

  readValue(reader,
            superblock->size_of_offsets); /* Driver Information Block Address */

  readValue(reader, superblock->size_of_offsets); /* Link Name Offset */

  superblock->root_group_object_header_address = readValue(
      reader, superblock->size_of_offsets); /* Object Header Address */

  uint64_t cache_type = readValue(reader, 4);
  switch (cache_type) {
  case 0:
  case 1:
  case 2:
    break;
  default:
    mylog("cache type must be 0,1, or 2 not %" PRIu64 "\n", cache_type);
    return MYSOFA_UNSUPPORTED_FORMAT;
  }

  if (mysofa_seek(reader, 0L, SEEK_END))
    return errno;

  if (superblock->end_of_file_address != mysofa_tell(reader)) {
    mylog("file size mismatch\n");
  }
  /* end of superblock */

  /* seek to first object */
  if (mysofa_seek(reader, superblock->root_group_object_header_address,
                  SEEK_SET)) {
    mylog("cannot seek to first object at %" PRId64 "\n",
          superblock->root_group_object_header_address);
    return errno;
  }

  return dataobjectRead(reader, &superblock->dataobject, NULL);
}

int superblockRead(struct READER *reader, struct SUPERBLOCK *superblock) {
  char buf[8];
  memset(superblock, 0, sizeof(*superblock));

  /* signature */
  if (mysofa_read(reader, buf, 8) != 8 ||
      strncmp("\211HDF\r\n\032\n", buf, 8)) {
    mylog("file does not have correct signature");
    return MYSOFA_INVALID_FORMAT;
  }

  /* read version of superblock, must be 0,1,2, or 3 */
  int version = mysofa_getc(reader);

  switch (version) {
  case 0:
  case 1:
    return superblockRead0or1(reader, superblock, version);
  case 2:
  case 3:
    return superblockRead2or3(reader, superblock);
  default:
    mylog("superblock must have version 0, 1, 2, or 3 but has %d\n", version);
    return MYSOFA_INVALID_FORMAT;
  }
}

void superblockFree(struct READER *reader, struct SUPERBLOCK *superblock) {
  dataobjectFree(reader, &superblock->dataobject);
}
