/*

 Copyright 2016 Christian Hoene, Symonics GmbH

 */

#ifndef READER_H_
#define READER_H_

#include "../hrtf/mysofa.h"
#include <stdint.h>
#include <stdio.h>

#define UNUSED(x) (void)(x)

struct READER;
struct DIR;
struct DATAOBJECT;

union RECORD {
  struct TYPE5 {
    uint32_t hash_of_name;
    uint64_t heap_id;
  } type5;
};

struct BTREE {
  uint8_t type, split_percent, merge_percent;
  uint16_t record_size, depth, number_of_records;
  uint32_t node_size;
  uint64_t root_node_address, total_number;

  union RECORD *records;
};

int btreeRead(struct READER *reader, struct BTREE *btree);
void btreeFree(struct BTREE *btree);

struct GCOL {
  uint16_t heap_object_index;
  uint64_t object_size;

  uint64_t address;

  uint64_t value;

  struct GCOL *next;
};

struct FRACTALHEAP {
  uint8_t flags;
  uint16_t heap_id_length, encoded_length, table_width, maximum_heap_size,
      starting_row, current_row;
  uint32_t maximum_size, fitler_mask;
  uint64_t next_huge_object_id, btree_address_of_huge_objects, free_space,
      address_free_space, amount_managed_space, amount_allocated_space,
      offset_managed_space, number_managed_objects, size_huge_objects,
      number_huge_objects, size_tiny_objects, number_tiny_objects,
      starting_block_size, maximum_direct_block_size, address_of_root_block,
      size_of_filtered_block;
  uint8_t *filter_information;
};

int fractalheapRead(struct READER *reader, struct DATAOBJECT *dataobject,
                    struct FRACTALHEAP *fractalheap);
void fractalheapFree(struct FRACTALHEAP *fractalheap);

struct LINKINFO {
  uint8_t flags;
  uint64_t maximum_creation_index, fractal_heap_address, address_btree_index,
      address_btree_order;
};

struct GROUPINFO {
  uint8_t flags;
  uint64_t maximum_compact_value, minimum_dense_value, number_of_entries,
      length_of_entries;
};

struct ATTRIBUTEINFO {
  uint8_t flags;
  uint64_t maximum_creation_index, fractal_heap_address, attribute_name_btree,
      attribute_creation_order_btree;
};

struct DATASPACE {
  uint64_t dimension_size[4], dimension_max_size[4];
  uint8_t dimensionality, flags, type;
};

struct DATATYPE {
  uint8_t class_and_version;
  uint32_t class_bit_field, size;

  union {
    struct {
      uint16_t bit_offset, bit_precision;
    } i;
    struct {
      uint16_t bit_offset, bit_precision;
      uint8_t exponent_location, exponent_size, mantissa_location,
          mantissa_size;
      uint32_t exponent_bias;
    } f;
  } u;

  uint32_t list; /* size of a list in bytes */
};

#define DATAOBJECT_MAX_DIMENSIONALITY 5

struct DATAOBJECT {
  char *name;

  uint64_t address;
  uint8_t flags;

  struct DATATYPE dt;
  struct DATASPACE ds;
  struct LINKINFO li;
  struct GROUPINFO gi;
  struct ATTRIBUTEINFO ai;

  struct BTREE objects_btree;
  struct FRACTALHEAP objects_heap;
  struct BTREE attributes_btree;
  struct FRACTALHEAP attributes_heap;

  int datalayout_chunk[DATAOBJECT_MAX_DIMENSIONALITY];

  struct MYSOFA_ATTRIBUTE *attributes;
  struct DIR *directory;

  void *data;
  int data_len;

  char *string;

  /* list of all current data objects */
  struct DATAOBJECT *all;
};

int dataobjectRead(struct READER *reader, struct DATAOBJECT *dataobject,
                   char *name);
void dataobjectFree(struct READER *reader, struct DATAOBJECT *dataobject);

struct DIR {
  struct DIR *next;

  struct DATAOBJECT dataobject;
};

struct SUPERBLOCK {
  uint8_t size_of_offsets;
  uint8_t size_of_lengths;

  uint64_t base_address, superblock_extension_address, end_of_file_address,
      root_group_object_header_address;

  struct DATAOBJECT dataobject;
};

int superblockRead(struct READER *reader, struct SUPERBLOCK *superblock);
void superblockFree(struct READER *reader, struct SUPERBLOCK *superblock);

int gcolRead(struct READER *reader, uint64_t gcol, int reference,
             uint64_t *dataobject);
void gcolFree(struct GCOL *gcol);

int treeRead(struct READER *reader, struct DATAOBJECT *data);

struct READER {
  FILE *fhd; //< file handler if file is used

  const char *memory; //< memory buffer if memory shall be used
  uint64_t memory_pos;
  uint64_t memory_len;

  struct DATAOBJECT *all;

  struct SUPERBLOCK superblock;

  struct GCOL *gcol;

  int recursive_counter;
};

int validAddress(struct READER *reader, uint64_t address);
uint64_t readValue(struct READER *reader, int size);

int gunzip(int inlen, char *in, int *outlen, char *out);

char *mysofa_strdup(const char *s);
int mysofa_read(struct READER *reader, void *buf, size_t n);
int mysofa_seek(struct READER *reader, long offset, int whence);
long mysofa_tell(struct READER *reader);
int mysofa_getc(struct READER *reader);

#endif /* READER_H_ */
