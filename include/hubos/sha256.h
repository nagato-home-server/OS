#ifndef HUBOS_SHA256_H
#define HUBOS_SHA256_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint64_t len;
  uint32_t state[8];
  uint8_t buffer[64];
} hubos_sha256_t;

void hubos_sha256_init(hubos_sha256_t *ctx);
void hubos_sha256_update(hubos_sha256_t *ctx, const void *data, size_t len);
void hubos_sha256_final(hubos_sha256_t *ctx, uint8_t digest[32]);

#endif
