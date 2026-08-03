#include "hubos/driver_loader.h"
#include "hubos/sha256.h"

#include <openssl/evp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HUBOS_DRIVER_LOADER_PUBLIC_KEY_BYTES 32
#define HUBOS_DRIVER_LOADER_SIGNATURE_BYTES 64

/*
 * Seeded test root key. The public key is stored here so the loader can verify
 * the bootstrap trust anchor without depending on external state.
 */
static const char HUBOS_DRIVER_LOADER_ROOT_PUBLIC_KEY_HEX[] =
  "79b5562e8fe654f94078b112e8a98ba7901f853ae695bed7e0e3910bad049664";

static char *hubos_driver_loader_strdup(const char *text) {
  size_t length = 0;
  char *copy = NULL;

  if (text == NULL) {
    return NULL;
  }

  length = strlen(text);
  copy = malloc(length + 1);
  if (copy == NULL) {
    return NULL;
  }

  memcpy(copy, text, length + 1);
  return copy;
}

static bool hubos_driver_loader_write_hex_digest(const uint8_t digest[32],
                                                 char *out_hex,
                                                 size_t out_hex_capacity) {
  static const char hex_digits[] = "0123456789abcdef";

  if (digest == NULL || out_hex == NULL || out_hex_capacity < HUBOS_DRIVER_PACKAGE_HASH_HEX_LENGTH) {
    return false;
  }

  for (size_t index = 0; index < 32; ++index) {
    out_hex[index * 2u] = hex_digits[(digest[index] >> 4u) & 0x0fu];
    out_hex[index * 2u + 1u] = hex_digits[digest[index] & 0x0fu];
  }
  out_hex[64] = '\0';
  return true;
}

static bool hubos_driver_loader_hex_decode(const char *hex,
                                           uint8_t *out_bytes,
                                           size_t out_bytes_capacity,
                                           size_t expected_bytes) {
  static const char digits[] = "0123456789abcdef";
  size_t length = 0;

  if (hex == NULL || out_bytes == NULL) {
    return false;
  }

  length = strlen(hex);
  if (length != expected_bytes * 2u || out_bytes_capacity < expected_bytes) {
    return false;
  }

  for (size_t index = 0; index < expected_bytes; ++index) {
    char high = hex[index * 2u];
    char low = hex[index * 2u + 1u];
    const char *high_ptr = strchr(digits, high);
    const char *low_ptr = strchr(digits, low);

    if (high_ptr == NULL || low_ptr == NULL) {
      return false;
    }

    out_bytes[index] = (uint8_t)(((size_t)(high_ptr - digits) << 4u) |
                                 (size_t)(low_ptr - digits));
  }

  return true;
}

static bool hubos_driver_loader_hex_is_valid(const char *hex, size_t expected_bytes) {
  uint8_t scratch[HUBOS_DRIVER_LOADER_SIGNATURE_BYTES];

  if (expected_bytes > sizeof(scratch)) {
    return false;
  }

  return hubos_driver_loader_hex_decode(hex, scratch, sizeof(scratch), expected_bytes);
}

static bool hubos_driver_loader_hash_prefixed_string(hubos_sha256_t *ctx,
                                                     const char *value) {
  uint64_t length = 0;

  if (ctx == NULL || value == NULL) {
    return false;
  }

  length = (uint64_t)strlen(value);
  hubos_sha256_update(ctx, &length, sizeof(length));
  hubos_sha256_update(ctx, value, length);
  return true;
}

static bool hubos_driver_loader_hash_prefixed_size(hubos_sha256_t *ctx, size_t value) {
  uint64_t encoded = (uint64_t)value;

  if (ctx == NULL) {
    return false;
  }

  hubos_sha256_update(ctx, &encoded, sizeof(encoded));
  return true;
}

static bool hubos_driver_loader_verify_signature_hex(const char *public_key_hex,
                                                     const char *message,
                                                     const char *signature_hex) {
  uint8_t public_key_bytes[HUBOS_DRIVER_LOADER_PUBLIC_KEY_BYTES];
  uint8_t signature_bytes[HUBOS_DRIVER_LOADER_SIGNATURE_BYTES];
  EVP_PKEY *key = NULL;
  EVP_MD_CTX *ctx = NULL;
  bool ok = false;
  int result = 0;

  if (public_key_hex == NULL || message == NULL || signature_hex == NULL) {
    return false;
  }

  if (!hubos_driver_loader_hex_decode(public_key_hex,
                                      public_key_bytes,
                                      sizeof(public_key_bytes),
                                      HUBOS_DRIVER_LOADER_PUBLIC_KEY_BYTES) ||
      !hubos_driver_loader_hex_decode(signature_hex,
                                      signature_bytes,
                                      sizeof(signature_bytes),
                                      HUBOS_DRIVER_LOADER_SIGNATURE_BYTES)) {
    return false;
  }

  key = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, NULL, public_key_bytes, sizeof(public_key_bytes));
  if (key == NULL) {
    return false;
  }

  ctx = EVP_MD_CTX_new();
  if (ctx == NULL) {
    EVP_PKEY_free(key);
    return false;
  }

  result = EVP_DigestVerifyInit(ctx, NULL, NULL, NULL, key);
  if (result != 1) {
    goto out;
  }

  result = EVP_DigestVerify(ctx,
                            signature_bytes,
                            sizeof(signature_bytes),
                            (const unsigned char *)message,
                            strlen(message));
  ok = (result == 1);

out:
  EVP_MD_CTX_free(ctx);
  EVP_PKEY_free(key);
  return ok;
}

static bool hubos_driver_loader_signing_message(const char *prefix,
                                                const char *first,
                                                const char *second,
                                                const char *third,
                                                char *out_message,
                                                size_t out_message_capacity) {
  int written = 0;

  if (prefix == NULL || first == NULL || second == NULL || out_message == NULL) {
    return false;
  }

  if (third != NULL) {
    written = snprintf(out_message,
                       out_message_capacity,
                       "%s|%s|%s|%s",
                       prefix,
                       first,
                       second,
                       third);
  } else {
    written = snprintf(out_message, out_message_capacity, "%s|%s|%s", prefix, first, second);
  }

  return written > 0 && (size_t)written < out_message_capacity;
}

static bool hubos_driver_loader_compute_package_signature_message(const hubos_driver_package_t *package,
                                                                  char *out_message,
                                                                  size_t out_message_capacity) {
  if (package == NULL || out_message == NULL) {
    return false;
  }

  return hubos_driver_loader_signing_message("package",
                                             package->signing_key_id,
                                             package->hash,
                                             NULL,
                                             out_message,
                                             out_message_capacity);
}

static bool hubos_driver_loader_compute_key_update_message(const hubos_driver_keyring_update_t *update,
                                                          char *out_message,
                                                          size_t out_message_capacity) {
  if (update == NULL || out_message == NULL) {
    return false;
  }

  return hubos_driver_loader_signing_message("key-update",
                                             update->issuer_key_id,
                                             update->subject_key_id,
                                             update->subject_public_key_hex,
                                             out_message,
                                             out_message_capacity);
}

static bool hubos_driver_loader_compute_key_revocation_message(
  const hubos_driver_keyring_revocation_t *revocation,
  char *out_message,
  size_t out_message_capacity) {
  if (revocation == NULL || out_message == NULL) {
    return false;
  }

  return hubos_driver_loader_signing_message("key-revoke",
                                             revocation->issuer_key_id,
                                             revocation->revoked_key_id,
                                             NULL,
                                             out_message,
                                             out_message_capacity);
}

static bool hubos_driver_loader_reserve_trusted_keys(hubos_driver_loader_t *loader,
                                                     size_t desired_capacity) {
  if (loader == NULL) {
    return false;
  }

  if (loader->trusted_key_capacity >= desired_capacity) {
    return true;
  }

  size_t new_capacity = loader->trusted_key_capacity == 0 ? 4 : loader->trusted_key_capacity;
  while (new_capacity < desired_capacity) {
    new_capacity *= 2;
  }

  void *new_items = realloc(loader->trusted_keys, new_capacity * sizeof(*loader->trusted_keys));
  if (new_items == NULL) {
    return false;
  }

  loader->trusted_keys = new_items;
  loader->trusted_key_capacity = new_capacity;
  return true;
}

static bool hubos_driver_loader_reserve_revoked_keys(hubos_driver_loader_t *loader,
                                                     size_t desired_capacity) {
  if (loader == NULL) {
    return false;
  }

  if (loader->revoked_key_capacity >= desired_capacity) {
    return true;
  }

  size_t new_capacity = loader->revoked_key_capacity == 0 ? 4 : loader->revoked_key_capacity;
  while (new_capacity < desired_capacity) {
    new_capacity *= 2;
  }

  void *new_items = realloc(loader->revoked_key_ids, new_capacity * sizeof(*loader->revoked_key_ids));
  if (new_items == NULL) {
    return false;
  }

  loader->revoked_key_ids = new_items;
  loader->revoked_key_capacity = new_capacity;
  return true;
}

static const hubos_driver_trusted_key_entry_t *hubos_driver_loader_find_trusted_key(
  const hubos_driver_loader_t *loader,
  const char *key_id) {
  if (loader == NULL || key_id == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < loader->trusted_key_count; ++index) {
    const hubos_driver_trusted_key_entry_t *entry = &loader->trusted_keys[index];
    if (entry->key_id != NULL && strcmp(entry->key_id, key_id) == 0) {
      return entry;
    }
  }

  return NULL;
}

static bool hubos_driver_loader_key_revoked(const hubos_driver_loader_t *loader,
                                            const char *key_id) {
  if (loader == NULL || key_id == NULL) {
    return false;
  }

  for (size_t index = 0; index < loader->revoked_key_count; ++index) {
    if (strcmp(loader->revoked_key_ids[index], key_id) == 0) {
      return true;
    }
  }

  return false;
}

static bool hubos_driver_loader_key_trusted(const hubos_driver_loader_t *loader,
                                            const char *key_id) {
  return hubos_driver_loader_find_trusted_key(loader, key_id) != NULL;
}

static bool hubos_driver_loader_add_trusted_key(hubos_driver_loader_t *loader,
                                                const char *key_id,
                                                const char *public_key_hex) {
  hubos_driver_trusted_key_entry_t *entry = NULL;
  const hubos_driver_trusted_key_entry_t *existing = NULL;
  char *key_copy = NULL;
  char *public_key_copy = NULL;

  if (loader == NULL || key_id == NULL || key_id[0] == '\0' || public_key_hex == NULL ||
      public_key_hex[0] == '\0') {
    return false;
  }

  if (!hubos_driver_loader_hex_is_valid(public_key_hex, HUBOS_DRIVER_LOADER_PUBLIC_KEY_BYTES)) {
    return false;
  }

  existing = hubos_driver_loader_find_trusted_key(loader, key_id);
  if (existing != NULL) {
    return strcmp(existing->public_key_hex, public_key_hex) == 0;
  }

  if (!hubos_driver_loader_reserve_trusted_keys(loader, loader->trusted_key_count + 1)) {
    return false;
  }

  key_copy = hubos_driver_loader_strdup(key_id);
  public_key_copy = hubos_driver_loader_strdup(public_key_hex);
  if (key_copy == NULL || public_key_copy == NULL) {
    free(key_copy);
    free(public_key_copy);
    return false;
  }

  entry = &loader->trusted_keys[loader->trusted_key_count++];
  entry->key_id = key_copy;
  entry->public_key_hex = public_key_copy;
  return true;
}

static bool hubos_driver_loader_remove_trusted_key(hubos_driver_loader_t *loader,
                                                  const char *key_id) {
  if (loader == NULL || key_id == NULL) {
    return false;
  }

  for (size_t index = 0; index < loader->trusted_key_count; ++index) {
    hubos_driver_trusted_key_entry_t *entry = &loader->trusted_keys[index];
    if (entry->key_id != NULL && strcmp(entry->key_id, key_id) == 0) {
      free(entry->key_id);
      free(entry->public_key_hex);
      loader->trusted_keys[index] = loader->trusted_keys[loader->trusted_key_count - 1];
      loader->trusted_keys[loader->trusted_key_count - 1].key_id = NULL;
      loader->trusted_keys[loader->trusted_key_count - 1].public_key_hex = NULL;
      --loader->trusted_key_count;
      return true;
    }
  }

  return false;
}

static bool hubos_driver_loader_package_has_required_fields(const hubos_driver_package_t *package) {
  if (package == NULL) {
    return false;
  }

  if (package->manifest == NULL || package->binary == NULL || package->signature == NULL ||
      package->hash == NULL || package->version == NULL || package->signing_key_id == NULL) {
    return false;
  }

  if (package->manifest[0] == '\0' || package->binary[0] == '\0' ||
      package->signature[0] == '\0' || package->hash[0] == '\0' || package->version[0] == '\0' ||
      package->signing_key_id[0] == '\0') {
    return false;
  }

  if (package->dependency_count > 0 && package->dependencies == NULL) {
    return false;
  }

  for (size_t index = 0; index < package->dependency_count; ++index) {
    if (package->dependencies[index] == NULL || package->dependencies[index][0] == '\0') {
      return false;
    }
  }

  return true;
}

static bool hubos_driver_loader_dependency_satisfied(const hubos_driver_loader_t *loader,
                                                     const char *dependency_name) {
  if (loader == NULL || dependency_name == NULL || dependency_name[0] == '\0') {
    return false;
  }

  if (strncmp(dependency_name, "driver:", 7) != 0) {
    return true;
  }

  if (loader->registry == NULL) {
    return false;
  }

  dependency_name += 7;
  for (size_t index = 0; index < loader->registry->count; ++index) {
    const hubos_driver_record_t *record = &loader->registry->items[index];
    if (record->driver_package != NULL && strcmp(record->driver_package, dependency_name) == 0) {
      return true;
    }
  }

  return false;
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
  loader->root_key_id = hubos_driver_loader_strdup(trusted_key_id);
  loader->current_key_id = hubos_driver_loader_strdup(trusted_key_id);
  loader->trusted_keys = NULL;
  loader->trusted_key_count = 0;
  loader->trusted_key_capacity = 0;
  loader->revoked_key_ids = NULL;
  loader->revoked_key_count = 0;
  loader->revoked_key_capacity = 0;
  (void)hubos_driver_loader_add_trusted_key(loader,
                                            trusted_key_id,
                                            HUBOS_DRIVER_LOADER_ROOT_PUBLIC_KEY_HEX);
}

void hubos_driver_loader_destroy(hubos_driver_loader_t *loader) {
  if (loader == NULL) {
    return;
  }

  free(loader->root_key_id);
  free(loader->current_key_id);
  for (size_t index = 0; index < loader->trusted_key_count; ++index) {
    free(loader->trusted_keys[index].key_id);
    free(loader->trusted_keys[index].public_key_hex);
  }
  free(loader->trusted_keys);
  for (size_t index = 0; index < loader->revoked_key_count; ++index) {
    free(loader->revoked_key_ids[index]);
  }
  free(loader->revoked_key_ids);
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
  char message[512];
  char *new_current_key_id = NULL;

  if (loader == NULL || update == NULL || update->issuer_key_id == NULL ||
      update->subject_key_id == NULL || update->subject_public_key_hex == NULL ||
      update->signature_hex == NULL) {
    return false;
  }

  if (loader->current_key_id == NULL || strcmp(loader->current_key_id, update->issuer_key_id) != 0) {
    return false;
  }

  if (!hubos_driver_loader_key_trusted(loader, update->issuer_key_id) ||
      hubos_driver_loader_key_revoked(loader, update->issuer_key_id)) {
    return false;
  }

  if (!hubos_driver_loader_compute_key_update_message(update, message, sizeof(message)) ||
      !hubos_driver_loader_verify_signature_hex(
        hubos_driver_loader_find_trusted_key(loader, update->issuer_key_id)->public_key_hex,
        message,
        update->signature_hex)) {
    return false;
  }

  new_current_key_id = hubos_driver_loader_strdup(update->subject_key_id);
  if (new_current_key_id == NULL) {
    return false;
  }

  if (!hubos_driver_loader_add_trusted_key(loader,
                                          update->subject_key_id,
                                          update->subject_public_key_hex)) {
    free(new_current_key_id);
    return false;
  }

  free(loader->current_key_id);
  loader->current_key_id = new_current_key_id;
  if (loader->audit_log != NULL) {
    (void)hubos_audit_log_record(loader->audit_log,
                                 HUBOS_AUDIT_DRIVER_KEY_ROTATED,
                                 0,
                                 HUBOS_ID_INVALID,
                                 0,
                                 0);
  }
  return true;
}

bool hubos_driver_loader_revoke_key(hubos_driver_loader_t *loader,
                                    const hubos_driver_keyring_revocation_t *revocation) {
  char message[256];
  char *copy = NULL;

  if (loader == NULL || revocation == NULL || revocation->issuer_key_id == NULL ||
      revocation->revoked_key_id == NULL || revocation->signature_hex == NULL) {
    return false;
  }

  if (loader->current_key_id == NULL || strcmp(loader->current_key_id, revocation->issuer_key_id) != 0) {
    return false;
  }

  if (!hubos_driver_loader_key_trusted(loader, revocation->issuer_key_id) ||
      hubos_driver_loader_key_revoked(loader, revocation->issuer_key_id)) {
    return false;
  }

  if (!hubos_driver_loader_compute_key_revocation_message(revocation, message, sizeof(message)) ||
      !hubos_driver_loader_verify_signature_hex(
        hubos_driver_loader_find_trusted_key(loader, revocation->issuer_key_id)->public_key_hex,
        message,
        revocation->signature_hex)) {
    return false;
  }

  if (!hubos_driver_loader_reserve_revoked_keys(loader, loader->revoked_key_count + 1)) {
    return false;
  }

  copy = hubos_driver_loader_strdup(revocation->revoked_key_id);
  if (copy == NULL) {
    return false;
  }

  loader->revoked_key_ids[loader->revoked_key_count++] = copy;
  (void)hubos_driver_loader_remove_trusted_key(loader, revocation->revoked_key_id);
  if (loader->audit_log != NULL) {
    (void)hubos_audit_log_record(loader->audit_log,
                                 HUBOS_AUDIT_DRIVER_KEY_REVOKED,
                                 0,
                                 HUBOS_ID_INVALID,
                                 0,
                                 0);
  }
  return true;
}

bool hubos_driver_loader_is_key_trusted(const hubos_driver_loader_t *loader,
                                        const char *key_id) {
  return hubos_driver_loader_key_trusted(loader, key_id);
}

bool hubos_driver_loader_is_key_revoked(const hubos_driver_loader_t *loader,
                                        const char *key_id) {
  return hubos_driver_loader_key_revoked(loader, key_id);
}

size_t hubos_driver_loader_trusted_key_count(const hubos_driver_loader_t *loader) {
  return loader != NULL ? loader->trusted_key_count : 0;
}

size_t hubos_driver_loader_revoked_key_count(const hubos_driver_loader_t *loader) {
  return loader != NULL ? loader->revoked_key_count : 0;
}

bool hubos_driver_loader_compute_package_hash(const hubos_driver_package_t *package,
                                              char *out_hex,
                                              size_t out_hex_capacity) {
  hubos_sha256_t ctx;
  uint8_t digest[32];

  if (package == NULL || out_hex == NULL || out_hex_capacity < HUBOS_DRIVER_PACKAGE_HASH_HEX_LENGTH) {
    return false;
  }

  hubos_sha256_init(&ctx);
  hubos_driver_loader_hash_prefixed_string(&ctx, package->manifest);
  hubos_driver_loader_hash_prefixed_string(&ctx, package->binary);
  hubos_driver_loader_hash_prefixed_string(&ctx, package->version);
  hubos_driver_loader_hash_prefixed_size(&ctx, (size_t)package->platform_abi_version);
  hubos_driver_loader_hash_prefixed_size(&ctx, (size_t)package->minimum_platform_abi_version);
  hubos_driver_loader_hash_prefixed_size(&ctx, package->dependency_count);

  for (size_t index = 0; index < package->dependency_count; ++index) {
    hubos_driver_loader_hash_prefixed_string(&ctx, package->dependencies[index]);
  }

  hubos_sha256_final(&ctx, digest);
  return hubos_driver_loader_write_hex_digest(digest, out_hex, out_hex_capacity);
}

bool hubos_driver_loader_validate_package(const hubos_driver_loader_t *loader,
                                          const hubos_driver_package_t *package) {
  char computed_hash[HUBOS_DRIVER_PACKAGE_HASH_HEX_LENGTH];
  char message[512];
  const hubos_driver_trusted_key_entry_t *key_entry = NULL;

  if (loader == NULL || !hubos_driver_loader_package_has_required_fields(package)) {
    return false;
  }

  if (!hubos_driver_loader_compute_package_hash(package, computed_hash, sizeof(computed_hash))) {
    return false;
  }

  if (strcmp(package->hash, computed_hash) != 0) {
    return false;
  }

  key_entry = hubos_driver_loader_find_trusted_key(loader, package->signing_key_id);
  if (key_entry == NULL || hubos_driver_loader_key_revoked(loader, package->signing_key_id)) {
    return false;
  }

  if (!hubos_driver_loader_compute_package_signature_message(package,
                                                            message,
                                                            sizeof(message))) {
    return false;
  }

  if (!hubos_driver_loader_hex_is_valid(package->signature, HUBOS_DRIVER_LOADER_SIGNATURE_BYTES)) {
    return false;
  }

  if (!hubos_driver_loader_verify_signature_hex(key_entry->public_key_hex,
                                                message,
                                                package->signature)) {
    return false;
  }

  if (package->minimum_platform_abi_version > 0 &&
      package->platform_abi_version < package->minimum_platform_abi_version) {
    return false;
  }

  for (size_t index = 0; index < package->dependency_count; ++index) {
    if (!hubos_driver_loader_dependency_satisfied(loader, package->dependencies[index])) {
      return false;
    }
  }

  return true;
}
