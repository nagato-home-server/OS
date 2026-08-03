#include "hubos/sha256.h"

#include <string.h>

static uint32_t hubos_sha256_rotr(uint32_t value, unsigned shift) {
  return (value >> shift) | (value << (32u - shift));
}

#define HUBOS_SHA256_CH(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define HUBOS_SHA256_MAJ(x, y, z) (((x) & (y)) | ((z) & ((x) | (y))))
#define HUBOS_SHA256_BSIG0(x) \
  (hubos_sha256_rotr((x), 2u) ^ hubos_sha256_rotr((x), 13u) ^ hubos_sha256_rotr((x), 22u))
#define HUBOS_SHA256_BSIG1(x) \
  (hubos_sha256_rotr((x), 6u) ^ hubos_sha256_rotr((x), 11u) ^ hubos_sha256_rotr((x), 25u))
#define HUBOS_SHA256_SSIG0(x) (hubos_sha256_rotr((x), 7u) ^ hubos_sha256_rotr((x), 18u) ^ ((x) >> 3u))
#define HUBOS_SHA256_SSIG1(x) \
  (hubos_sha256_rotr((x), 17u) ^ hubos_sha256_rotr((x), 19u) ^ ((x) >> 10u))

static const uint32_t hubos_sha256_k[64] = {
  0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u,
  0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu,
  0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu,
  0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
  0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
  0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
  0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u,
  0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
  0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u,
  0xc67178f2u,
};

static void hubos_sha256_process_block(hubos_sha256_t *ctx, const uint8_t *block) {
  uint32_t schedule[64];
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
  uint32_t e;
  uint32_t f;
  uint32_t g;
  uint32_t h;

  for (size_t index = 0; index < 16; ++index) {
    schedule[index] = ((uint32_t)block[index * 4u] << 24u) |
                      ((uint32_t)block[index * 4u + 1u] << 16u) |
                      ((uint32_t)block[index * 4u + 2u] << 8u) | (uint32_t)block[index * 4u + 3u];
  }

  for (size_t index = 16; index < 64; ++index) {
    schedule[index] = HUBOS_SHA256_SSIG1(schedule[index - 2u]) + schedule[index - 7u] +
                      HUBOS_SHA256_SSIG0(schedule[index - 15u]) + schedule[index - 16u];
  }

  a = ctx->state[0];
  b = ctx->state[1];
  c = ctx->state[2];
  d = ctx->state[3];
  e = ctx->state[4];
  f = ctx->state[5];
  g = ctx->state[6];
  h = ctx->state[7];

  for (size_t index = 0; index < 64; ++index) {
    uint32_t t1 = h + HUBOS_SHA256_BSIG1(e) + HUBOS_SHA256_CH(e, f, g) + hubos_sha256_k[index] +
                  schedule[index];
    uint32_t t2 = HUBOS_SHA256_BSIG0(a) + HUBOS_SHA256_MAJ(a, b, c);

    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }

  ctx->state[0] += a;
  ctx->state[1] += b;
  ctx->state[2] += c;
  ctx->state[3] += d;
  ctx->state[4] += e;
  ctx->state[5] += f;
  ctx->state[6] += g;
  ctx->state[7] += h;
}

static void hubos_sha256_pad(hubos_sha256_t *ctx) {
  size_t remainder = (size_t)(ctx->len % sizeof(ctx->buffer));
  uint64_t bit_length = ctx->len * 8u;

  ctx->buffer[remainder++] = 0x80u;
  if (remainder > 56u) {
    memset(ctx->buffer + remainder, 0, sizeof(ctx->buffer) - remainder);
    hubos_sha256_process_block(ctx, ctx->buffer);
    remainder = 0;
  }

  memset(ctx->buffer + remainder, 0, 56u - remainder);
  ctx->buffer[56] = (uint8_t)(bit_length >> 56u);
  ctx->buffer[57] = (uint8_t)(bit_length >> 48u);
  ctx->buffer[58] = (uint8_t)(bit_length >> 40u);
  ctx->buffer[59] = (uint8_t)(bit_length >> 32u);
  ctx->buffer[60] = (uint8_t)(bit_length >> 24u);
  ctx->buffer[61] = (uint8_t)(bit_length >> 16u);
  ctx->buffer[62] = (uint8_t)(bit_length >> 8u);
  ctx->buffer[63] = (uint8_t)bit_length;
  hubos_sha256_process_block(ctx, ctx->buffer);
}

void hubos_sha256_init(hubos_sha256_t *ctx) {
  if (ctx == NULL) {
    return;
  }

  ctx->len = 0;
  ctx->state[0] = 0x6a09e667u;
  ctx->state[1] = 0xbb67ae85u;
  ctx->state[2] = 0x3c6ef372u;
  ctx->state[3] = 0xa54ff53au;
  ctx->state[4] = 0x510e527fu;
  ctx->state[5] = 0x9b05688cu;
  ctx->state[6] = 0x1f83d9abu;
  ctx->state[7] = 0x5be0cd19u;
}

void hubos_sha256_update(hubos_sha256_t *ctx, const void *data, size_t len) {
  const uint8_t *bytes = data;
  size_t remainder = 0;

  if (ctx == NULL || data == NULL || len == 0) {
    return;
  }

  remainder = (size_t)(ctx->len % sizeof(ctx->buffer));
  ctx->len += len;
  if (remainder != 0) {
    size_t available = sizeof(ctx->buffer) - remainder;
    if (len < available) {
      memcpy(ctx->buffer + remainder, bytes, len);
      return;
    }
    memcpy(ctx->buffer + remainder, bytes, available);
    hubos_sha256_process_block(ctx, ctx->buffer);
    bytes += available;
    len -= available;
    remainder = 0;
  }

  while (len >= sizeof(ctx->buffer)) {
    hubos_sha256_process_block(ctx, bytes);
    bytes += sizeof(ctx->buffer);
    len -= sizeof(ctx->buffer);
  }

  if (len > 0) {
    memcpy(ctx->buffer + remainder, bytes, len);
  }
}

void hubos_sha256_final(hubos_sha256_t *ctx, uint8_t digest[32]) {
  if (ctx == NULL || digest == NULL) {
    return;
  }

  hubos_sha256_pad(ctx);
  for (size_t index = 0; index < 8; ++index) {
    digest[index * 4u] = (uint8_t)(ctx->state[index] >> 24u);
    digest[index * 4u + 1u] = (uint8_t)(ctx->state[index] >> 16u);
    digest[index * 4u + 2u] = (uint8_t)(ctx->state[index] >> 8u);
    digest[index * 4u + 3u] = (uint8_t)ctx->state[index];
  }
}
