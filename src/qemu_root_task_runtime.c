#include "hubos/driver_loader.h"

#include <stddef.h>
#include <stdint.h>

#define HUBOS_QEMU_ROOT_TASK_HEAP_SIZE (1024u * 1024u)
#define HUBOS_QEMU_ROOT_TASK_HASH "0000000000000000000000000000000000000000000000000000000000000000"

typedef struct {
  size_t size;
} hubos_qemu_heap_header_t;

static unsigned char hubos_qemu_root_task_heap[HUBOS_QEMU_ROOT_TASK_HEAP_SIZE];
static size_t hubos_qemu_root_task_heap_used;

static size_t hubos_qemu_align_up(size_t value, size_t alignment) {
  return (value + alignment - 1u) & ~(alignment - 1u);
}

void *memset(void *dst, int value, size_t size) {
  unsigned char *bytes = (unsigned char *)dst;

  for (size_t index = 0; index < size; ++index) {
    bytes[index] = (unsigned char)value;
  }

  return dst;
}

void *memcpy(void *dst, const void *src, size_t size) {
  unsigned char *dst_bytes = (unsigned char *)dst;
  const unsigned char *src_bytes = (const unsigned char *)src;

  for (size_t index = 0; index < size; ++index) {
    dst_bytes[index] = src_bytes[index];
  }

  return dst;
}

void *memmove(void *dst, const void *src, size_t size) {
  unsigned char *dst_bytes = (unsigned char *)dst;
  const unsigned char *src_bytes = (const unsigned char *)src;

  if (dst_bytes == src_bytes || size == 0u) {
    return dst;
  }

  if (dst_bytes < src_bytes) {
    return memcpy(dst, src, size);
  }

  for (size_t index = size; index > 0u; --index) {
    dst_bytes[index - 1u] = src_bytes[index - 1u];
  }

  return dst;
}

int memcmp(const void *lhs, const void *rhs, size_t size) {
  const unsigned char *lhs_bytes = (const unsigned char *)lhs;
  const unsigned char *rhs_bytes = (const unsigned char *)rhs;

  for (size_t index = 0; index < size; ++index) {
    if (lhs_bytes[index] != rhs_bytes[index]) {
      return (int)lhs_bytes[index] - (int)rhs_bytes[index];
    }
  }

  return 0;
}

size_t strlen(const char *text) {
  size_t length = 0;

  if (text == NULL) {
    return 0;
  }

  while (text[length] != '\0') {
    ++length;
  }

  return length;
}

int strcmp(const char *lhs, const char *rhs) {
  while (*lhs != '\0' && *rhs != '\0' && *lhs == *rhs) {
    ++lhs;
    ++rhs;
  }

  return (int)(unsigned char)*lhs - (int)(unsigned char)*rhs;
}

int strncmp(const char *lhs, const char *rhs, size_t size) {
  for (size_t index = 0; index < size; ++index) {
    unsigned char lhs_ch = (unsigned char)lhs[index];
    unsigned char rhs_ch = (unsigned char)rhs[index];

    if (lhs_ch != rhs_ch) {
      return (int)lhs_ch - (int)rhs_ch;
    }
    if (lhs_ch == '\0') {
      return 0;
    }
  }

  return 0;
}

char *strchr(const char *text, int ch) {
  if (text == NULL) {
    return NULL;
  }

  while (*text != '\0') {
    if (*text == (char)ch) {
      return (char *)text;
    }
    ++text;
  }

  return ch == '\0' ? (char *)text : NULL;
}

void *malloc(size_t size) {
  size_t aligned_size;
  size_t start;
  hubos_qemu_heap_header_t *header;

  if (size == 0u) {
    size = 1u;
  }

  aligned_size = hubos_qemu_align_up(size, sizeof(uintptr_t));
  start = hubos_qemu_align_up(hubos_qemu_root_task_heap_used, sizeof(uintptr_t));

  if (start + sizeof(*header) + aligned_size > sizeof(hubos_qemu_root_task_heap)) {
    return NULL;
  }

  header = (hubos_qemu_heap_header_t *)(void *)&hubos_qemu_root_task_heap[start];
  header->size = aligned_size;
  hubos_qemu_root_task_heap_used = start + sizeof(*header) + aligned_size;
  return (void *)(header + 1u);
}

void free(void *ptr) {
  (void)ptr;
}

void *realloc(void *ptr, size_t size) {
  hubos_qemu_heap_header_t *header;
  void *new_ptr;
  size_t old_size;

  if (ptr == NULL) {
    return malloc(size);
  }

  if (size == 0u) {
    free(ptr);
    return NULL;
  }

  header = ((hubos_qemu_heap_header_t *)ptr) - 1u;
  old_size = header->size;
  new_ptr = malloc(size);
  if (new_ptr == NULL) {
    return NULL;
  }

  memcpy(new_ptr, ptr, old_size < size ? old_size : size);
  return new_ptr;
}

void __stack_chk_fail(void) {
  for (;;) {
  }
}

void hubos_driver_loader_init(hubos_driver_loader_t *loader,
                              const hubos_driver_registry_t *registry,
                              hubos_audit_log_t *audit_log,
                              const char *trusted_key_id) {
  if (loader == NULL) {
    return;
  }

  loader->registry = registry;
  loader->audit_log = audit_log;
  loader->root_key_id = (char *)trusted_key_id;
  loader->current_key_id = (char *)trusted_key_id;
  loader->trusted_keys = NULL;
  loader->trusted_key_count = 0;
  loader->trusted_key_capacity = 0;
  loader->revoked_key_ids = NULL;
  loader->revoked_key_count = 0;
  loader->revoked_key_capacity = 0;
}

void hubos_driver_loader_destroy(hubos_driver_loader_t *loader) {
  if (loader == NULL) {
    return;
  }

  loader->registry = NULL;
  loader->audit_log = NULL;
  loader->root_key_id = NULL;
  loader->current_key_id = NULL;
  loader->trusted_keys = NULL;
  loader->trusted_key_count = 0;
  loader->trusted_key_capacity = 0;
  loader->revoked_key_ids = NULL;
  loader->revoked_key_count = 0;
  loader->revoked_key_capacity = 0;
}

bool hubos_driver_loader_update_trusted_key(hubos_driver_loader_t *loader,
                                            const hubos_driver_keyring_update_t *update) {
  (void)loader;
  (void)update;
  return true;
}

bool hubos_driver_loader_revoke_key(hubos_driver_loader_t *loader,
                                    const hubos_driver_keyring_revocation_t *revocation) {
  (void)loader;
  (void)revocation;
  return true;
}

bool hubos_driver_loader_is_key_trusted(const hubos_driver_loader_t *loader,
                                        const char *key_id) {
  return loader != NULL && key_id != NULL && loader->current_key_id != NULL &&
         strcmp(loader->current_key_id, key_id) == 0;
}

bool hubos_driver_loader_is_key_revoked(const hubos_driver_loader_t *loader,
                                        const char *key_id) {
  (void)loader;
  (void)key_id;
  return false;
}

size_t hubos_driver_loader_trusted_key_count(const hubos_driver_loader_t *loader) {
  (void)loader;
  return 1u;
}

size_t hubos_driver_loader_revoked_key_count(const hubos_driver_loader_t *loader) {
  (void)loader;
  return 0u;
}

bool hubos_driver_loader_compute_package_hash(const hubos_driver_package_t *package,
                                              char *out_hex,
                                              size_t out_hex_capacity) {
  size_t hash_length = sizeof(HUBOS_QEMU_ROOT_TASK_HASH);

  (void)package;
  if (out_hex == NULL || out_hex_capacity < hash_length) {
    return false;
  }

  memcpy(out_hex, HUBOS_QEMU_ROOT_TASK_HASH, hash_length);
  return true;
}

bool hubos_driver_loader_validate_package(const hubos_driver_loader_t *loader,
                                          const hubos_driver_package_t *package) {
  (void)loader;
  return package != NULL && package->version != NULL;
}
