#ifndef HUBOS_HOST_MICROKIT_H
#define HUBOS_HOST_MICROKIT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t seL4_Word;
typedef uint8_t seL4_Uint8;
typedef uint16_t seL4_Uint16;
typedef uint32_t seL4_Uint32;
typedef int seL4_Bool;
typedef uintptr_t seL4_CPtr;
typedef uint64_t seL4_Error;

typedef unsigned int microkit_channel;
typedef unsigned int microkit_child;
typedef unsigned int microkit_ioport;

typedef struct {
  seL4_Word label;
  seL4_Uint16 count;
} seL4_MessageInfo_t;

typedef seL4_MessageInfo_t microkit_msginfo;

#ifndef seL4_True
#define seL4_True 1
#endif

#ifndef seL4_False
#define seL4_False 0
#endif

static seL4_Word microkit_stub_message_registers[64];

static inline microkit_msginfo microkit_msginfo_new(seL4_Word label, seL4_Uint16 count) {
  microkit_msginfo msginfo;

  msginfo.label = label;
  msginfo.count = count;
  return msginfo;
}

static inline seL4_Word microkit_msginfo_get_label(microkit_msginfo msginfo) {
  return msginfo.label;
}

static inline seL4_Word microkit_msginfo_get_count(microkit_msginfo msginfo) {
  return msginfo.count;
}

static inline void microkit_mr_set(seL4_Uint8 mr, seL4_Word value) {
  if (mr < 64) {
    microkit_stub_message_registers[mr] = value;
  }
}

static inline seL4_Word microkit_mr_get(seL4_Uint8 mr) {
  if (mr < 64) {
    return microkit_stub_message_registers[mr];
  }

  return 0;
}

static inline void microkit_dbg_puts(const char *s) {
  (void)s;
}

static inline void microkit_dbg_putc(int c) {
  (void)c;
}

static inline void microkit_dbg_put32(seL4_Word value) {
  (void)value;
}

static inline void microkit_dbg_put8(seL4_Uint8 value) {
  (void)value;
}

#endif
