#ifndef HUBOS_DRIVER_LOADER_H
#define HUBOS_DRIVER_LOADER_H

#include "hubos/audit.h"
#include "hubos/driver_registry.h"

#define HUBOS_DRIVER_PACKAGE_HASH_HEX_LENGTH 65

typedef struct {
  const char *manifest;
  const char *binary;
  const char *signature;
  const char *hash;
  const char *version;
  const char *signing_key_id;
  const char *const *dependencies;
  size_t dependency_count;
  unsigned platform_abi_version;
  unsigned minimum_platform_abi_version;
} hubos_driver_package_t;

typedef struct {
  const char *key_id;
  const char *public_key_hex;
} hubos_driver_trusted_key_t;

typedef struct {
  const char *issuer_key_id;
  const char *subject_key_id;
  const char *subject_public_key_hex;
  const char *signature_hex;
} hubos_driver_keyring_update_t;

typedef struct {
  const char *issuer_key_id;
  const char *revoked_key_id;
  const char *signature_hex;
} hubos_driver_keyring_revocation_t;

typedef enum {
  HUBOS_DRIVER_SLOT_UNBOUND = 0,
  HUBOS_DRIVER_SLOT_BOUND,
  HUBOS_DRIVER_SLOT_REBINDING,
  HUBOS_DRIVER_SLOT_QUARANTINED,
} hubos_driver_slot_state_t;

typedef struct {
  hubos_id_t resource_id;
  hubos_id_t driver_id;
  hubos_driver_slot_state_t state;
} hubos_driver_binding_t;

typedef struct {
  char *key_id;
  char *public_key_hex;
} hubos_driver_trusted_key_entry_t;

typedef struct {
  const hubos_driver_registry_t *registry;
  hubos_audit_log_t *audit_log;
  char *root_key_id;
  char *current_key_id;
  hubos_driver_trusted_key_entry_t *trusted_keys;
  size_t trusted_key_count;
  size_t trusted_key_capacity;
  char **revoked_key_ids;
  size_t revoked_key_count;
  size_t revoked_key_capacity;
} hubos_driver_loader_t;

void hubos_driver_loader_init(hubos_driver_loader_t *loader,
                              const hubos_driver_registry_t *registry,
                              hubos_audit_log_t *audit_log,
                              const char *trusted_key_id);
void hubos_driver_loader_destroy(hubos_driver_loader_t *loader);

bool hubos_driver_loader_update_trusted_key(hubos_driver_loader_t *loader,
                                            const hubos_driver_keyring_update_t *update);

bool hubos_driver_loader_revoke_key(hubos_driver_loader_t *loader,
                                    const hubos_driver_keyring_revocation_t *revocation);

bool hubos_driver_loader_is_key_trusted(const hubos_driver_loader_t *loader,
                                        const char *key_id);

bool hubos_driver_loader_is_key_revoked(const hubos_driver_loader_t *loader,
                                        const char *key_id);

size_t hubos_driver_loader_trusted_key_count(const hubos_driver_loader_t *loader);
size_t hubos_driver_loader_revoked_key_count(const hubos_driver_loader_t *loader);

bool hubos_driver_loader_compute_package_hash(const hubos_driver_package_t *package,
                                              char *out_hex,
                                              size_t out_hex_capacity);

bool hubos_driver_loader_validate_package(const hubos_driver_loader_t *loader,
                                          const hubos_driver_package_t *package);

#endif
