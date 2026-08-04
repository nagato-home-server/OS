#define _POSIX_C_SOURCE 200809L

#include "hubos/audit.h"
#include "hubos/app_vm_runtime.h"
#include "hubos/app_model.h"
#include "hubos/bus_manager.h"
#include "hubos/container_model.h"
#include "hubos/device_server.h"
#include "hubos/boot.h"
#include "hubos/capability_manager.h"
#include "hubos/dma_manager.h"
#include "hubos/driver_loader.h"
#include "hubos/driver_service.h"
#include "hubos/driver_registry.h"
#include "hubos/display_server.h"
#include "hubos/hub.h"
#include "hubos/memory_manager.h"
#include "hubos/network_server.h"
#include "hubos/ipc.h"
#include "hubos/linux_usbio_backend.h"
#include "hubos/linux_vm_layout.h"
#include "hubos/sha256.h"
#include "hubos/namespace.h"
#include "hubos/shared_resource.h"
#include "hubos/service_endpoints.h"
#include "hubos/root_task.h"
#include "hubos/runtime_config.h"
#include "hubos/microkit_transport.h"
#include "hubos/microkit_generated.h"
#include "hubos/microkit_kernel_glue.h"
#include "hubos/resource_registry.h"
#include "hubos/sha256.h"
#include "hubos/storage_server.h"
#include "hubos/system.h"
#include "hubos/session_manager.h"
#include "hubos/vm_server.h"
#include "hubos/vm_model.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define TEST_ED25519_SIGNATURE_HEX_LENGTH 129

static const char *const test_linux_dev_source_bundles[] = {
  "linux-kernel",
  "buildroot",
};

static const hubos_app_vm_runtime_artifact_hash_t test_linux_dev_artifact_hashes[] = {
  { "kernel.elf", "2602c46fd19da5ac16986db9fd8d90227f310b1e5d04118d03873c5a11ba3d68" },
  { "initramfs.cpio.gz", "bcbd23fd825ef20aa5116085e21223614017513459938fbc4ba203396fe358e3" },
  { "rootfs.img", "d19aac3c1ff4e04231c3d86394c8b11c26fb359e7a59e9ecfd0402ac032cbb2d" },
};

static const char *const test_mini_bsd_source_bundles[] = {
  "freebsd-src",
  "freebsd-ports",
};

static const hubos_app_vm_runtime_artifact_hash_t test_mini_bsd_artifact_hashes[] = {
  { "kernel.elf", "e04e21b3c900625ad85ebad1b4e64484f11321d377064c1a7d43e158e055e316" },
  { "rootfs.img", "f641054e7467e27ff1a3565bf908f95e43eec70c28fab61d9dee266d0ecb9cb6" },
};

static const hubos_app_vm_runtime_profile_t test_runtime_profiles[] = {
  {
    "linux-dev",
    HUBOS_APP_VM_GUEST_CLASS_FULL,
    HUBOS_APP_VM_OS_FAMILY_LINUX,
    "General-purpose Linux guest for broad app compatibility.",
    "1.0.0",
    "guest-managed",
    "src/runtime-images/linux-dev/1.0.0",
    test_linux_dev_source_bundles,
    sizeof(test_linux_dev_source_bundles) / sizeof(test_linux_dev_source_bundles[0]),
    test_linux_dev_artifact_hashes,
    sizeof(test_linux_dev_artifact_hashes) / sizeof(test_linux_dev_artifact_hashes[0]),
    {
      "src/runtime-images/linux-dev/1.0.0/kernel.elf",
      "src/runtime-images/linux-dev/1.0.0/initramfs.cpio.gz",
      "src/runtime-images/linux-dev/1.0.0/rootfs.img",
      NULL,
      "console=ttyS0 root=/dev/vda rw",
    },
    { 1024, 2, true, true, false },
  },
  {
    "mini-bsd-service",
    HUBOS_APP_VM_GUEST_CLASS_RUNTIME,
    HUBOS_APP_VM_OS_FAMILY_BSD,
    "Smaller BSD-style runtime for service-focused app VMs.",
    "1.0.0",
    "guest-managed",
    "src/runtime-images/mini-bsd-service/1.0.0",
    test_mini_bsd_source_bundles,
    sizeof(test_mini_bsd_source_bundles) / sizeof(test_mini_bsd_source_bundles[0]),
    test_mini_bsd_artifact_hashes,
    sizeof(test_mini_bsd_artifact_hashes) / sizeof(test_mini_bsd_artifact_hashes[0]),
    {
      "src/runtime-images/mini-bsd-service/1.0.0/kernel.elf",
      NULL,
      "src/runtime-images/mini-bsd-service/1.0.0/rootfs.img",
      NULL,
      "console=ttyS0",
    },
    { 256, 1, true, false, false },
  },
};

typedef struct {
  unsigned set_owner_calls;
  unsigned release_owner_calls;
  unsigned quarantine_calls;
  unsigned clear_quarantine_calls;
  unsigned reset_calls;
  unsigned attach_mmio_calls;
  unsigned attach_irq_calls;
  unsigned attach_dma_calls;
  bool fail_reset;
} test_device_backend_t;

typedef struct {
  unsigned discover_calls;
  bool fail_discover;
} test_bus_backend_t;

static bool test_device_backend_set_owner(void *context,
                                          hubos_device_server_t *server,
                                          hubos_id_t owner_session_id) {
  test_device_backend_t *backend = context;

  assert(server != NULL);
  assert(owner_session_id != HUBOS_ID_INVALID);
  backend->set_owner_calls++;
  return true;
}

static bool test_device_backend_release_owner(void *context, hubos_device_server_t *server) {
  test_device_backend_t *backend = context;

  assert(server != NULL);
  backend->release_owner_calls++;
  return true;
}

static bool test_device_backend_quarantine(void *context, hubos_device_server_t *server) {
  test_device_backend_t *backend = context;

  assert(server != NULL);
  backend->quarantine_calls++;
  return true;
}

static bool test_device_backend_clear_quarantine(void *context, hubos_device_server_t *server) {
  test_device_backend_t *backend = context;

  assert(server != NULL);
  backend->clear_quarantine_calls++;
  return true;
}

static bool test_device_backend_reset(void *context, hubos_device_server_t *server) {
  test_device_backend_t *backend = context;

  assert(server != NULL);
  backend->reset_calls++;
  return !backend->fail_reset;
}

static bool test_device_backend_attach_mmio(void *context,
                                            hubos_device_server_t *server,
                                            hubos_id_t owner_session_id) {
  test_device_backend_t *backend = context;

  assert(server != NULL);
  assert(owner_session_id != HUBOS_ID_INVALID);
  backend->attach_mmio_calls++;
  return true;
}

static bool test_device_backend_attach_irq(void *context,
                                           hubos_device_server_t *server,
                                           hubos_id_t owner_session_id) {
  test_device_backend_t *backend = context;

  assert(server != NULL);
  assert(owner_session_id != HUBOS_ID_INVALID);
  backend->attach_irq_calls++;
  return true;
}

static bool test_device_backend_attach_dma(void *context,
                                           hubos_device_server_t *server,
                                           hubos_id_t owner_session_id) {
  test_device_backend_t *backend = context;

  assert(server != NULL);
  assert(owner_session_id != HUBOS_ID_INVALID);
  backend->attach_dma_calls++;
  return true;
}

static const hubos_device_server_ops_t test_device_backend_ops = {
  .set_owner = test_device_backend_set_owner,
  .release_owner = test_device_backend_release_owner,
  .quarantine = test_device_backend_quarantine,
  .clear_quarantine = test_device_backend_clear_quarantine,
  .reset = test_device_backend_reset,
  .attach_mmio = test_device_backend_attach_mmio,
  .attach_irq = test_device_backend_attach_irq,
  .attach_dma = test_device_backend_attach_dma,
};

static bool test_bus_backend_discover(void *context,
                                      hubos_bus_manager_t *manager,
                                      const char *resource_name,
                                      size_t resource_name_len,
                                      hubos_resource_state_t state) {
  test_bus_backend_t *backend = context;

  assert(manager != NULL);
  assert(resource_name != NULL);
  assert(resource_name_len != 0);
  assert(state == HUBOS_RESOURCE_DISCOVERED || state == HUBOS_RESOURCE_READY);
  backend->discover_calls++;
  return !backend->fail_discover;
}

static const hubos_bus_manager_ops_t test_bus_backend_ops = {
  .discover = test_bus_backend_discover,
};

static const hubos_audit_event_t *audit_find_event_type(const hubos_audit_log_t *log,
                                                        hubos_audit_event_type_t type) {
  size_t i;

  if (log == NULL) {
    return NULL;
  }

  for (i = 0; i < hubos_audit_log_count(log); ++i) {
    const hubos_audit_event_t *event = hubos_audit_log_get(log, i);
    if (event != NULL && event->type == type) {
      return event;
    }
  }

  return NULL;
}

static bool test_hex_encode(const uint8_t *bytes, size_t length, char *out_hex, size_t out_capacity) {
  static const char hex_digits[] = "0123456789abcdef";

  if (bytes == NULL || out_hex == NULL || out_capacity < (length * 2u) + 1u) {
    return false;
  }

  for (size_t index = 0; index < length; ++index) {
    out_hex[index * 2u] = hex_digits[(bytes[index] >> 4u) & 0x0fu];
    out_hex[index * 2u + 1u] = hex_digits[bytes[index] & 0x0fu];
  }
  out_hex[length * 2u] = '\0';
  return true;
}

static bool test_ed25519_keypair_from_seed(const uint8_t seed[32],
                                           uint8_t out_public_key[32],
                                           EVP_PKEY **out_private_key) {
  size_t public_length = 32;
  EVP_PKEY *private_key = NULL;

  if (seed == NULL || out_public_key == NULL || out_private_key == NULL) {
    return false;
  }

  private_key = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, seed, 32);
  if (private_key == NULL) {
    return false;
  }

  if (EVP_PKEY_get_raw_public_key(private_key, out_public_key, &public_length) != 1 ||
      public_length != 32) {
    EVP_PKEY_free(private_key);
    return false;
  }

  *out_private_key = private_key;
  return true;
}

static bool test_ed25519_sign_message_hex(EVP_PKEY *private_key,
                                          const char *message,
                                          char *out_signature_hex,
                                          size_t out_signature_capacity) {
  EVP_MD_CTX *ctx = NULL;
  uint8_t signature[64];
  size_t signature_length = sizeof(signature);

  if (private_key == NULL || message == NULL || out_signature_hex == NULL) {
    return false;
  }

  ctx = EVP_MD_CTX_new();
  if (ctx == NULL) {
    return false;
  }

  if (EVP_DigestSignInit(ctx, NULL, NULL, NULL, private_key) != 1) {
    EVP_MD_CTX_free(ctx);
    return false;
  }

  if (EVP_DigestSign(ctx, signature, &signature_length, (const unsigned char *)message, strlen(message)) != 1 ||
      signature_length != sizeof(signature)) {
    EVP_MD_CTX_free(ctx);
    return false;
  }

  EVP_MD_CTX_free(ctx);
  return test_hex_encode(signature, signature_length, out_signature_hex, out_signature_capacity);
}

static bool test_make_package_signature(EVP_PKEY *private_key,
                                        const hubos_driver_package_t *package,
                                        char *out_signature_hex,
                                        size_t out_signature_capacity) {
  char message[512];
  int written = 0;

  if (package == NULL) {
    return false;
  }

  written = snprintf(message,
                     sizeof(message),
                     "package|%s|%s",
                     package->signing_key_id,
                     package->hash);
  if (written < 0 || (size_t)written >= sizeof(message)) {
    return false;
  }

  return test_ed25519_sign_message_hex(private_key, message, out_signature_hex, out_signature_capacity);
}

static bool test_make_directory(const char *path) {
  if (mkdir(path, 0700) == 0) {
    return true;
  }

  return errno == EEXIST;
}

static bool test_write_file(const char *path, const char *value) {
  FILE *file = fopen(path, "w");

  if (file == NULL) {
    return false;
  }
  if (fputs(value, file) == EOF) {
    fclose(file);
    return false;
  }
  return fclose(file) == 0;
}

static void test_linux_usbio_backend(void) {
  char template[] = "/tmp/hubos-linux-backend-XXXXXX";
  char *root = mkdtemp(template);
  char usbio_dir[256];
  char i2c_root[256];
  char spi_root[256];
  char irq_root[256];
  char dma_root[256];
  char path[256];
  char device_dir[256];
  hubos_system_t system;
  hubos_id_t resource_id = HUBOS_ID_INVALID;
  hubos_service_descriptor_t descriptor;

  assert(root != NULL);
  assert(snprintf(usbio_dir, sizeof(usbio_dir), "%s/usbio0", root) < (int)sizeof(usbio_dir));
  assert(snprintf(i2c_root, sizeof(i2c_root), "%s/i2c", root) < (int)sizeof(i2c_root));
  assert(snprintf(spi_root, sizeof(spi_root), "%s/spi", root) < (int)sizeof(spi_root));
  assert(snprintf(irq_root, sizeof(irq_root), "%s/gpio-usbio", root) < (int)sizeof(irq_root));
  assert(snprintf(dma_root, sizeof(dma_root), "%s/video", root) < (int)sizeof(dma_root));

  assert(test_make_directory(usbio_dir));
  assert(test_make_directory(i2c_root));
  assert(test_make_directory(spi_root));
  assert(test_make_directory(irq_root));
  assert(test_make_directory(dma_root));
  assert(snprintf(path, sizeof(path), "%s/gpiochip0", irq_root) < (int)sizeof(path));
  assert(test_write_file(path, ""));

  assert(snprintf(path, sizeof(path), "%s/cmd", usbio_dir) < (int)sizeof(path));
  assert(test_write_file(path, ""));
  assert(snprintf(path, sizeof(path), "%s/version", usbio_dir) < (int)sizeof(path));
  assert(test_write_file(path, "1.2.3\n"));

  assert(snprintf(device_dir, sizeof(device_dir), "%s/1-0036", i2c_root) < (int)sizeof(device_dir));
  assert(test_make_directory(device_dir));
  assert(snprintf(path, sizeof(path), "%s/name", device_dir) < (int)sizeof(path));
  assert(test_write_file(path, "ov08x40\n"));

  assert(snprintf(device_dir, sizeof(device_dir), "%s/spi0.0", spi_root) < (int)sizeof(device_dir));
  assert(test_make_directory(device_dir));
  assert(snprintf(path, sizeof(path), "%s/name", device_dir) < (int)sizeof(path));
  assert(test_write_file(path, "usbio-spi\n"));

  assert(snprintf(path, sizeof(path), "%s/video0", dma_root) < (int)sizeof(path));
  assert(test_write_file(path, ""));

  assert(setenv("HUBOS_ENABLE_LINUX_USBIO_BACKEND", "1", 1) == 0);
  assert(setenv("HUBOS_USBIO_SYSFS_DIR", usbio_dir, 1) == 0);
  assert(setenv("HUBOS_I2C_SYSFS_ROOT", i2c_root, 1) == 0);
  assert(setenv("HUBOS_SPI_SYSFS_ROOT", spi_root, 1) == 0);
  assert(setenv("HUBOS_GPIO_USBIO_SYSFS_ROOT", irq_root, 1) == 0);
  assert(setenv("HUBOS_VIDEO_DEVICE_ROOT", dma_root, 1) == 0);
  assert(hubos_linux_usbio_backend_is_requested());

  hubos_system_init(&system, "root-key");
  assert(hubos_system_bus_discover(&system,
                                   HUBOS_BUS_I2C,
                                   "resource://i2c/ov08x40",
                                   strlen("resource://i2c/ov08x40"),
                                   HUBOS_RESOURCE_DISCOVERED,
                                   &resource_id));
  assert(resource_id != HUBOS_ID_INVALID);
  assert(hubos_system_set_device_owner(&system, 77));
  assert(hubos_system_attach_device_mmio(&system, 77));
  assert(hubos_system_attach_device_irq(&system, 77));
  assert(hubos_system_attach_device_dma(&system, 77));
  assert(hubos_system_reset_device(&system));
  assert(hubos_system_describe_device(&system, &descriptor));
  assert((descriptor.policy_hints & 0x7u) == 0x7u);
  hubos_system_destroy(&system);

  assert(unsetenv("HUBOS_ENABLE_LINUX_USBIO_BACKEND") == 0);
  assert(unsetenv("HUBOS_USBIO_SYSFS_DIR") == 0);
  assert(unsetenv("HUBOS_I2C_SYSFS_ROOT") == 0);
  assert(unsetenv("HUBOS_SPI_SYSFS_ROOT") == 0);
  assert(unsetenv("HUBOS_GPIO_USBIO_SYSFS_ROOT") == 0);
  assert(unsetenv("HUBOS_VIDEO_DEVICE_ROOT") == 0);
}

static bool test_make_key_update_signature(EVP_PKEY *private_key,
                                           const hubos_driver_keyring_update_t *update,
                                           char *out_signature_hex,
                                           size_t out_signature_capacity) {
  char message[512];
  int written = 0;

  if (update == NULL) {
    return false;
  }

  written = snprintf(message,
                     sizeof(message),
                     "key-update|%s|%s|%s",
                     update->issuer_key_id,
                     update->subject_key_id,
                     update->subject_public_key_hex);
  if (written < 0 || (size_t)written >= sizeof(message)) {
    return false;
  }

  return test_ed25519_sign_message_hex(private_key, message, out_signature_hex, out_signature_capacity);
}

static bool test_make_key_revocation_signature(EVP_PKEY *private_key,
                                               const hubos_driver_keyring_revocation_t *revocation,
                                               char *out_signature_hex,
                                               size_t out_signature_capacity) {
  char message[256];
  int written = 0;

  if (revocation == NULL) {
    return false;
  }

  written = snprintf(message,
                     sizeof(message),
                     "key-revoke|%s|%s",
                     revocation->issuer_key_id,
                     revocation->revoked_key_id);
  if (written < 0 || (size_t)written >= sizeof(message)) {
    return false;
  }

  return test_ed25519_sign_message_hex(private_key, message, out_signature_hex, out_signature_capacity);
}

static const uint8_t test_root_seed[32] = {
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
  0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
  0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
};

static const uint8_t test_next_seed[32] = {
  0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
  0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
  0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
  0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
};

static void test_sha256_helper(void) {
  hubos_sha256_t ctx;
  uint8_t digest[32];
  static const char expected[] =
    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
  static const char input[] = "abc";
  char digest_hex[HUBOS_DRIVER_PACKAGE_HASH_HEX_LENGTH];

  hubos_sha256_init(&ctx);
  hubos_sha256_update(&ctx, input, strlen(input));
  hubos_sha256_final(&ctx, digest);
  for (size_t index = 0; index < sizeof(digest); ++index) {
    static const char hex_digits[] = "0123456789abcdef";
    digest_hex[index * 2u] = hex_digits[(digest[index] >> 4u) & 0x0fu];
    digest_hex[index * 2u + 1u] = hex_digits[digest[index] & 0x0fu];
  }
  digest_hex[64] = '\0';

  assert(strcmp(digest_hex, expected) == 0);
}

static void test_resource_model(void) {
  hubos_resource_t resource;
  hubos_resource_init(&resource, 1, "resource://network/nic0", strlen("resource://network/nic0"));

  assert(resource.id == 1);
  assert(resource.state == HUBOS_RESOURCE_DISCOVERED);
  assert(hubos_resource_transition_allowed(HUBOS_RESOURCE_DISCOVERED,
                                           HUBOS_RESOURCE_CLASSIFIED));
  assert(hubos_resource_transition_allowed(HUBOS_RESOURCE_CLASSIFIED,
                                           HUBOS_RESOURCE_BOUND));
  assert(hubos_resource_transition_allowed(HUBOS_RESOURCE_BOUND,
                                           HUBOS_RESOURCE_READY));
  assert(hubos_resource_transition_allowed(HUBOS_RESOURCE_READY,
                                           HUBOS_RESOURCE_RETIRED));
  assert(!hubos_resource_transition_allowed(HUBOS_RESOURCE_READY,
                                            HUBOS_RESOURCE_DISCOVERED));
}

static void test_resource_registry(void) {
  hubos_resource_registry_t registry;
  hubos_id_t resource_id = HUBOS_ID_INVALID;
  bool is_new = false;

  hubos_resource_registry_init(&registry);

  assert(hubos_resource_registry_register(&registry,
                                          "resource://pci/0000:01:00.0",
                                          strlen("resource://pci/0000:01:00.0"),
                                          HUBOS_RESOURCE_DISCOVERED,
                                          &resource_id,
                                          &is_new));
  assert(is_new);
  assert(resource_id == 1);
  assert(hubos_resource_registry_count(&registry) == 1);

  assert(hubos_resource_registry_register(&registry,
                                          "resource://pci/0000:01:00.0",
                                          strlen("resource://pci/0000:01:00.0"),
                                          HUBOS_RESOURCE_CLASSIFIED,
                                          &resource_id,
                                          &is_new));
  assert(!is_new);
  assert(resource_id == 1);
  assert(hubos_resource_registry_count(&registry) == 1);

  const hubos_resource_t *resource = hubos_resource_registry_get(&registry, resource_id);
  assert(resource != NULL);
  assert(resource->state == HUBOS_RESOURCE_CLASSIFIED);
  assert(hubos_resource_registry_find(&registry,
                                      "resource://pci/0000:01:00.0",
                                      strlen("resource://pci/0000:01:00.0")) != NULL);

  hubos_resource_registry_destroy(&registry);
}

static void test_session_model(void) {
  hubos_session_manager_t manager;
  hubos_id_t root_id = HUBOS_ID_INVALID;
  hubos_id_t child_id = HUBOS_ID_INVALID;
  hubos_id_t grandchild_id = HUBOS_ID_INVALID;

  hubos_session_manager_init(&manager);

  assert(hubos_session_manager_create(&manager,
                                      100,
                                      HUBOS_ID_INVALID,
                                      HUBOS_SESSION_PERSISTENT,
                                      &root_id));
  assert(hubos_session_manager_set_state(&manager, root_id, HUBOS_SESSION_ACTIVE));
  assert(hubos_session_manager_refresh_assets(&manager, root_id, 501, 601));

  assert(hubos_session_manager_create(&manager,
                                      101,
                                      root_id,
                                      HUBOS_SESSION_EPHEMERAL,
                                      &child_id));
  assert(hubos_session_manager_get(&manager, child_id)->resource_set_version == 501);
  assert(hubos_session_manager_get(&manager, child_id)->lease_version == 601);
  assert(hubos_session_manager_create(&manager,
                                      102,
                                      child_id,
                                      HUBOS_SESSION_TRANSACTIONAL,
                                      &grandchild_id));

  assert(hubos_session_manager_set_state(&manager, child_id, HUBOS_SESSION_ACTIVE));
  assert(hubos_session_manager_set_state(&manager, grandchild_id, HUBOS_SESSION_ACTIVE));

  assert(hubos_session_manager_is_ancestor(&manager, root_id, grandchild_id));
  assert(hubos_session_manager_child_count(&manager, root_id) == 1);
  assert(hubos_session_manager_child_count(&manager, child_id) == 1);

  assert(hubos_session_manager_revoke_tree(&manager, NULL, child_id));
  assert(hubos_session_manager_get(&manager, child_id)->state == HUBOS_SESSION_REVOKED);
  assert(hubos_session_manager_get(&manager, grandchild_id)->state == HUBOS_SESSION_REVOKED);
  assert(hubos_session_manager_get(&manager, root_id)->state == HUBOS_SESSION_ACTIVE);
  assert(hubos_session_manager_refresh_assets(&manager, root_id, 701, 801));
  assert(hubos_session_manager_get(&manager, child_id)->resource_set_version == 501);
  assert(hubos_session_manager_get(&manager, child_id)->lease_version == 601);

  hubos_session_manager_destroy(&manager);
}

static void test_session_capability_cascade(void) {
  hubos_session_manager_t sessions;
  hubos_capability_manager_t capabilities;
  hubos_id_t root_session_id = HUBOS_ID_INVALID;
  hubos_id_t child_session_id = HUBOS_ID_INVALID;
  hubos_id_t grandchild_session_id = HUBOS_ID_INVALID;
  hubos_id_t root_cap_id = HUBOS_ID_INVALID;
  hubos_id_t child_cap_id = HUBOS_ID_INVALID;
  hubos_id_t grandchild_cap_id = HUBOS_ID_INVALID;

  hubos_session_manager_init(&sessions);
  hubos_capability_manager_init(&capabilities);

  assert(hubos_session_manager_create(&sessions,
                                      200,
                                      HUBOS_ID_INVALID,
                                      HUBOS_SESSION_PERSISTENT,
                                      &root_session_id));
  assert(hubos_session_manager_set_state(&sessions, root_session_id, HUBOS_SESSION_ACTIVE));

  assert(hubos_session_manager_create(&sessions,
                                      201,
                                      root_session_id,
                                      HUBOS_SESSION_EPHEMERAL,
                                      &child_session_id));
  assert(hubos_session_manager_create(&sessions,
                                      202,
                                      child_session_id,
                                      HUBOS_SESSION_TRANSACTIONAL,
                                      &grandchild_session_id));

  assert(hubos_capability_manager_issue(&capabilities,
                                        root_session_id,
                                        1,
                                        HUBOS_CAP_RIGHT_INSPECT,
                                        true,
                                        &root_cap_id));
  assert(hubos_capability_manager_issue(&capabilities,
                                        child_session_id,
                                        1,
                                        HUBOS_CAP_RIGHT_COPY,
                                        true,
                                        &child_cap_id));
  assert(hubos_capability_manager_issue(&capabilities,
                                        grandchild_session_id,
                                        1,
                                        HUBOS_CAP_RIGHT_TRANSFER,
                                        false,
                                        &grandchild_cap_id));

  assert(hubos_session_manager_revoke_tree(&sessions, &capabilities, child_session_id));

  assert(hubos_session_manager_get(&sessions, child_session_id)->state == HUBOS_SESSION_REVOKED);
  assert(hubos_session_manager_get(&sessions, grandchild_session_id)->state ==
         HUBOS_SESSION_REVOKED);
  assert(hubos_session_manager_get(&sessions, root_session_id)->state == HUBOS_SESSION_ACTIVE);

  assert(hubos_capability_is_active(hubos_capability_manager_get(&capabilities, root_cap_id)));
  assert(!hubos_capability_is_active(hubos_capability_manager_get(&capabilities, child_cap_id)));
  assert(!hubos_capability_is_active(
    hubos_capability_manager_get(&capabilities, grandchild_cap_id)));

  hubos_capability_manager_destroy(&capabilities);
  hubos_session_manager_destroy(&sessions);
}

static void test_capability_model(void) {
  hubos_capability_manager_t manager;
  hubos_id_t root_cap_id = HUBOS_ID_INVALID;
  hubos_id_t child_cap_id = HUBOS_ID_INVALID;

  hubos_capability_manager_init(&manager);

  assert(hubos_capability_manager_issue(&manager,
                                        10,
                                        1,
                                        HUBOS_CAP_RIGHT_COPY | HUBOS_CAP_RIGHT_MINT |
                                          HUBOS_CAP_RIGHT_TRANSFER | HUBOS_CAP_RIGHT_INSPECT,
                                        true,
                                        &root_cap_id));
  assert(root_cap_id == 1);

  assert(hubos_capability_manager_mint_from(&manager,
                                            root_cap_id,
                                            20,
                                            HUBOS_CAP_RIGHT_COPY | HUBOS_CAP_RIGHT_INSPECT,
                                            true,
                                            &child_cap_id));
  assert(child_cap_id == 2);
  assert(hubos_capability_manager_get(&manager, child_cap_id)->owner_session_id == 20);

  assert(hubos_capability_manager_transfer(&manager, child_cap_id, 30));
  assert(hubos_capability_manager_get(&manager, child_cap_id)->owner_session_id == 30);

  assert(hubos_capability_manager_authorize(&manager,
                                            child_cap_id,
                                            1,
                                            HUBOS_CAP_RIGHT_COPY));
  assert(!hubos_capability_manager_authorize(&manager,
                                             child_cap_id,
                                             2,
                                             HUBOS_CAP_RIGHT_COPY));

  assert(hubos_capability_manager_revoke(&manager, child_cap_id));
  assert(!hubos_capability_manager_transfer(&manager, child_cap_id, 40));
  assert(!hubos_capability_manager_authorize(&manager,
                                            child_cap_id,
                                            1,
                                            HUBOS_CAP_RIGHT_COPY));

  hubos_capability_manager_destroy(&manager);
}

static void test_dma_model(void) {
  assert(hubos_dma_transition_allowed(HUBOS_DMA_UNMAPPED, HUBOS_DMA_MAPPING));
  assert(hubos_dma_transition_allowed(HUBOS_DMA_MAPPING, HUBOS_DMA_ACTIVE));
  assert(hubos_dma_transition_allowed(HUBOS_DMA_ACTIVE, HUBOS_DMA_QUIESCING));
  assert(hubos_dma_transition_allowed(HUBOS_DMA_QUIESCING, HUBOS_DMA_REVOKED));
  assert(hubos_dma_transition_allowed(HUBOS_DMA_ACTIVE, HUBOS_DMA_ABORTED));
  assert(!hubos_dma_transition_allowed(HUBOS_DMA_UNMAPPED, HUBOS_DMA_ABORTED));
  assert(!hubos_dma_transition_allowed(HUBOS_DMA_MAPPING, HUBOS_DMA_ABORTED));
  assert(!hubos_dma_transition_allowed(HUBOS_DMA_REVOKED, HUBOS_DMA_ACTIVE));
}

static void test_memory_manager(void) {
  hubos_memory_manager_t manager;
  hubos_id_t frame_id = HUBOS_ID_INVALID;
  hubos_id_t hugepage_id = HUBOS_ID_INVALID;

  hubos_memory_manager_init(&manager);

  assert(hubos_memory_manager_allocate_frame(&manager, 4096, 0, &frame_id));
  assert(hubos_memory_manager_allocate_hugepage(&manager, 2 * 1024 * 1024, 1, &hugepage_id));
  assert(hubos_memory_manager_count(&manager) == 2);
  assert(hubos_memory_manager_get(&manager, frame_id)->kind == HUBOS_MEMORY_FRAME);
  assert(hubos_memory_manager_share(&manager, frame_id));
  assert(hubos_memory_manager_get(&manager, frame_id)->kind == HUBOS_MEMORY_SHARED);
  assert(hubos_memory_manager_reclaim(&manager, frame_id));
  assert(!hubos_memory_manager_get(&manager, frame_id)->in_use);

  hubos_memory_manager_destroy(&manager);
}

static void test_shared_resource_model(void) {
  hubos_shared_resource_t resource;

  hubos_shared_resource_init(&resource, 1, 100, 1);
  assert(hubos_shared_resource_is_active(&resource));
  assert(resource.refcount == 1);

  assert(hubos_shared_resource_detach(&resource));
  assert(hubos_shared_resource_is_pending_finalization(&resource));
  assert(resource.refcount == 0);
  assert(!hubos_shared_resource_acquire(&resource));
  assert(hubos_shared_resource_finalize(&resource));
  assert(!hubos_shared_resource_is_active(&resource));
  assert(resource.state == HUBOS_SHARED_RESOURCE_RETIRED);
}

static void test_bus_manager(void) {
  hubos_resource_registry_t registry;
  hubos_audit_log_t audit;
  hubos_bus_manager_t bus;
  hubos_id_t resource_id = HUBOS_ID_INVALID;
  bool is_new = false;

  hubos_resource_registry_init(&registry);
  hubos_audit_log_init(&audit);
  hubos_bus_manager_init(&bus, HUBOS_BUS_PCIE, "pcie0", &registry, &audit);

  assert(hubos_bus_manager_discover(&bus,
                                    "resource://pci/0000:01:00.0",
                                    strlen("resource://pci/0000:01:00.0"),
                                    HUBOS_RESOURCE_DISCOVERED,
                                    &resource_id));
  assert(resource_id == 1);
  assert(hubos_resource_registry_get(&registry, resource_id)->provisional);
  assert(hubos_resource_registry_register(&registry,
                                          "resource://pci/0000:01:00.0",
                                          strlen("resource://pci/0000:01:00.0"),
                                          HUBOS_RESOURCE_CLASSIFIED,
                                          &resource_id,
                                          &is_new));
  assert(!is_new);
  assert(!hubos_resource_registry_get(&registry, resource_id)->provisional);
  assert(hubos_audit_log_count(&audit) >= 1);
  assert(hubos_audit_log_get(&audit, 0)->type == HUBOS_AUDIT_RESOURCE_DISCOVERED);

  hubos_audit_log_destroy(&audit);
  hubos_resource_registry_destroy(&registry);
}

static void test_audit_log(void) {
  hubos_audit_log_t log;

  hubos_audit_log_init(&log);
  assert(hubos_audit_log_count(&log) == 0);
  assert(hubos_audit_log_record(&log,
                                HUBOS_AUDIT_RESOURCE_REGISTERED,
                                0,
                                1,
                                0,
                                0));
  assert(hubos_audit_log_count(&log) == 1);
  assert(hubos_audit_log_get(&log, 0)->sequence == 1);
  assert(hubos_audit_log_get(&log, 0)->resource_id == 1);
  hubos_audit_log_destroy(&log);
}

static void test_boot_sequence(void) {
  hubos_boot_state_t state;
  hubos_audit_log_t log;

  hubos_boot_state_init(&state);
  hubos_audit_log_init(&log);

  assert(!hubos_boot_state_complete_step(&state, &log, HUBOS_BOOT_HUB));
  assert(hubos_boot_state_complete_step(&state, &log, HUBOS_BOOT_FIRMWARE));
  assert(hubos_boot_state_complete_step(&state, &log, HUBOS_BOOT_SEL4));
  assert(hubos_boot_state_complete_step(&state, &log, HUBOS_BOOT_ROOT_TASK));
  assert(!hubos_boot_state_complete_step(&state, &log, HUBOS_BOOT_DMA_MANAGER));
  assert(hubos_boot_state_complete_step(&state, &log, HUBOS_BOOT_RESOURCE_REGISTRY));
  assert(hubos_boot_state_complete_step(&state, &log, HUBOS_BOOT_SESSION_MANAGER));
  assert(hubos_boot_state_complete_step(&state, &log, HUBOS_BOOT_CAPABILITY_MANAGER));
  assert(hubos_boot_state_complete_step(&state, &log, HUBOS_BOOT_MEMORY_MANAGER));
  assert(hubos_boot_state_complete_step(&state, &log, HUBOS_BOOT_DMA_MANAGER));
  assert(hubos_boot_state_complete_step(&state, &log, HUBOS_BOOT_HUB));
  assert(hubos_boot_state_is_complete(&state, HUBOS_BOOT_HUB));
  assert(hubos_audit_log_count(&log) >= 7);

  hubos_audit_log_destroy(&log);
}

static void test_boot_capability_set(void) {
  hubos_boot_capability_set_t set;

  hubos_boot_capability_set_init(&set);
  assert(!hubos_boot_capability_set_validate_minimal(&set));

  assert(hubos_boot_capability_set_grant(&set, HUBOS_BOOT_CAP_FIRMWARE));
  assert(hubos_boot_capability_set_grant(&set, HUBOS_BOOT_CAP_SEL4));
  assert(hubos_boot_capability_set_grant(&set, HUBOS_BOOT_CAP_ROOT_TASK));
  assert(hubos_boot_capability_set_grant(&set, HUBOS_BOOT_CAP_RESOURCE_REGISTRY));
  assert(hubos_boot_capability_set_grant(&set, HUBOS_BOOT_CAP_CAPABILITY_MANAGER));
  assert(hubos_boot_capability_set_grant(&set, HUBOS_BOOT_CAP_MEMORY_MANAGER));
  assert(hubos_boot_capability_set_grant(&set, HUBOS_BOOT_CAP_DMA_MANAGER));
  assert(hubos_boot_capability_set_grant(&set, HUBOS_BOOT_CAP_HUB));
  assert(hubos_boot_capability_set_count(&set) == HUBOS_BOOT_CAP_COUNT);
  assert(hubos_boot_capability_set_validate_minimal(&set));
  assert(hubos_boot_capability_set_has(&set, HUBOS_BOOT_CAP_DMA_MANAGER));
}

static void test_dma_manager(void) {
  hubos_dma_manager_t manager;
  const hubos_dma_mapping_t *mapping = NULL;

  hubos_dma_manager_init(&manager);

  assert(!hubos_dma_manager_abort(&manager, 3));
  assert(hubos_dma_manager_map(&manager, 1));
  mapping = hubos_dma_manager_get(&manager, 1);
  assert(mapping != NULL);
  assert(mapping->state == HUBOS_DMA_ACTIVE);

  assert(hubos_dma_manager_begin_revoke(&manager, 1));
  assert(hubos_dma_manager_mark_queue_empty(&manager, 1));
  assert(hubos_dma_manager_mark_outstanding_complete(&manager, 1));
  assert(hubos_dma_manager_mark_interrupts_drained(&manager, 1));
  assert(hubos_dma_manager_finalize_revoke(&manager, 1));
  assert(hubos_dma_manager_get(&manager, 1)->state == HUBOS_DMA_REVOKED);

  assert(hubos_dma_manager_map(&manager, 2));
  assert(hubos_dma_manager_begin_revoke(&manager, 2));
  assert(hubos_dma_manager_abort(&manager, 2));
  assert(hubos_dma_manager_get(&manager, 2)->state == HUBOS_DMA_ABORTED);

  hubos_dma_manager_destroy(&manager);
}

static void test_driver_registry_loader(void) {
  hubos_audit_log_t audit;
  const char *const dependencies[] = { "libsel4" };
  hubos_driver_package_t package = {
    .manifest = "manifest",
    .binary = "binary",
    .signature = "signature",
    .hash = "hash",
    .version = "1.0.0",
    .signing_key_id = "root-key",
    .dependencies = dependencies,
    .dependency_count = 1,
    .platform_abi_version = 2,
    .minimum_platform_abi_version = 1,
  };
  char package_hash[HUBOS_DRIVER_PACKAGE_HASH_HEX_LENGTH];
  char package_signature[TEST_ED25519_SIGNATURE_HEX_LENGTH];
  char next_package_signature[TEST_ED25519_SIGNATURE_HEX_LENGTH];
  char update_signature[TEST_ED25519_SIGNATURE_HEX_LENGTH];
  char revoke_signature[TEST_ED25519_SIGNATURE_HEX_LENGTH];
  char root_public_key_hex[65];
  char next_public_key_hex[65];
  uint8_t root_public_key[32];
  uint8_t next_public_key[32];
  EVP_PKEY *root_private_key = NULL;
  EVP_PKEY *next_private_key = NULL;
  hubos_driver_keyring_update_t update = {
    .issuer_key_id = "root-key",
    .subject_key_id = "next-key",
    .subject_public_key_hex = next_public_key_hex,
    .signature_hex = update_signature,
  };
  hubos_driver_keyring_revocation_t revocation = {
    .issuer_key_id = "next-key",
    .revoked_key_id = "root-key",
    .signature_hex = revoke_signature,
  };
  hubos_audit_log_init(&audit);
  hubos_driver_loader_t loader;

  assert(test_ed25519_keypair_from_seed(test_root_seed, root_public_key, &root_private_key));
  assert(test_ed25519_keypair_from_seed(test_next_seed, next_public_key, &next_private_key));
  assert(test_hex_encode(root_public_key, sizeof(root_public_key), root_public_key_hex, sizeof(root_public_key_hex)));
  assert(test_hex_encode(next_public_key, sizeof(next_public_key), next_public_key_hex, sizeof(next_public_key_hex)));

  hubos_driver_loader_init(&loader, NULL, &audit, "root-key");
  assert(hubos_driver_loader_compute_package_hash(&package, package_hash, sizeof(package_hash)));
  package.hash = package_hash;
  assert(test_make_package_signature(root_private_key, &package, package_signature, sizeof(package_signature)));
  package.hash = package_hash;
  package.signature = package_signature;
  assert(hubos_driver_loader_validate_package(&loader, &package));
  assert(test_make_key_update_signature(root_private_key, &update, update_signature, sizeof(update_signature)));
  assert(hubos_driver_loader_update_trusted_key(&loader, &update));
  assert(audit_find_event_type(&audit, HUBOS_AUDIT_DRIVER_KEY_ROTATED) != NULL);
  package.signing_key_id = "next-key";
  assert(hubos_driver_loader_compute_package_hash(&package, package_hash, sizeof(package_hash)));
  package.hash = package_hash;
  assert(test_make_package_signature(next_private_key, &package, next_package_signature, sizeof(next_package_signature)));
  package.signature = next_package_signature;
  assert(hubos_driver_loader_validate_package(&loader, &package));
  package.signing_key_id = "root-key";
  assert(hubos_driver_loader_compute_package_hash(&package, package_hash, sizeof(package_hash)));
  package.hash = package_hash;
  assert(test_make_package_signature(root_private_key, &package, package_signature, sizeof(package_signature)));
  package.signature = package_signature;
  assert(hubos_driver_loader_validate_package(&loader, &package));
  assert(test_make_key_revocation_signature(next_private_key, &revocation, revoke_signature, sizeof(revoke_signature)));
  assert(hubos_driver_loader_revoke_key(&loader, &revocation));
  assert(audit_find_event_type(&audit, HUBOS_AUDIT_DRIVER_KEY_REVOKED) != NULL);
  package.signing_key_id = "root-key";
  assert(hubos_driver_loader_compute_package_hash(&package, package_hash, sizeof(package_hash)));
  package.hash = package_hash;
  assert(test_make_package_signature(root_private_key, &package, package_signature, sizeof(package_signature)));
  package.signature = package_signature;
  assert(!hubos_driver_loader_validate_package(&loader, &package));

  hubos_driver_loader_destroy(&loader);
  hubos_audit_log_destroy(&audit);
  EVP_PKEY_free(root_private_key);
  EVP_PKEY_free(next_private_key);
}

static void test_driver_service(void) {
  hubos_driver_registry_t registry;
  hubos_driver_loader_t loader;
  hubos_driver_service_t service;
  hubos_driver_service_endpoint_t endpoint;
  hubos_audit_log_t audit;
  hubos_id_t driver_id = HUBOS_ID_INVALID;
  bool is_new = false;
  const char *const dependencies[] = { "libsel4" };
  const hubos_driver_binding_t *binding = NULL;
  hubos_driver_package_t package = {
    .manifest = "manifest",
    .binary = "binary",
    .signature = "signature",
    .hash = "hash",
    .version = "1.0.0",
    .signing_key_id = "root-key",
    .dependencies = dependencies,
    .dependency_count = 1,
    .platform_abi_version = 2,
    .minimum_platform_abi_version = 1,
  };
  hubos_driver_package_t rotated_package = package;
  char package_hash[HUBOS_DRIVER_PACKAGE_HASH_HEX_LENGTH];
  char package_signature[TEST_ED25519_SIGNATURE_HEX_LENGTH];
  char rotated_hash[HUBOS_DRIVER_PACKAGE_HASH_HEX_LENGTH];
  char rotated_signature[TEST_ED25519_SIGNATURE_HEX_LENGTH];
  char update_signature[TEST_ED25519_SIGNATURE_HEX_LENGTH];
  char revoke_signature[TEST_ED25519_SIGNATURE_HEX_LENGTH];
  char root_public_key_hex[65];
  char next_public_key_hex[65];
  uint8_t root_public_key[32];
  uint8_t next_public_key[32];
  EVP_PKEY *root_private_key = NULL;
  EVP_PKEY *next_private_key = NULL;
  hubos_driver_keyring_update_t update = {
    .issuer_key_id = "root-key",
    .subject_key_id = "next-key",
    .subject_public_key_hex = next_public_key_hex,
    .signature_hex = update_signature,
  };
  hubos_driver_keyring_revocation_t revocation = {
    .issuer_key_id = "next-key",
    .revoked_key_id = "root-key",
    .signature_hex = revoke_signature,
  };

  hubos_driver_registry_init(&registry);
  hubos_audit_log_init(&audit);
  assert(test_ed25519_keypair_from_seed(test_root_seed, root_public_key, &root_private_key));
  assert(test_ed25519_keypair_from_seed(test_next_seed, next_public_key, &next_private_key));
  assert(test_hex_encode(root_public_key,
                         sizeof(root_public_key),
                         root_public_key_hex,
                         sizeof(root_public_key_hex)));
  assert(test_hex_encode(next_public_key, sizeof(next_public_key), next_public_key_hex, sizeof(next_public_key_hex)));

  assert(hubos_driver_registry_register(&registry,
                                        1,
                                        2,
                                        3,
                                        "nic-driver",
                                        "1.0.0",
                                        &driver_id,
                                        &is_new));
  assert(is_new);
  assert(driver_id == 1);

  hubos_driver_loader_init(&loader, &registry, &audit, "root-key");
  hubos_driver_service_init(&service, &registry, &loader, &audit);
  hubos_driver_service_endpoint_init(&endpoint, &service);
  assert(hubos_driver_loader_is_key_trusted(&loader, "root-key"));
  assert(hubos_driver_loader_trusted_key_count(&loader) == 1);

  assert(hubos_driver_loader_compute_package_hash(&package, package_hash, sizeof(package_hash)));
  package.hash = package_hash;
  assert(test_make_package_signature(root_private_key, &package, package_signature, sizeof(package_signature)));
  package.signature = package_signature;
  assert(hubos_driver_service_endpoint_bind(&endpoint, 10, driver_id, &package));
  binding = hubos_driver_service_endpoint_get(&endpoint, 10);
  assert(binding != NULL);
  assert(binding->state == HUBOS_DRIVER_SLOT_BOUND);

  assert(hubos_audit_log_count(&audit) >= 1);
  assert(test_make_key_update_signature(root_private_key, &update, update_signature, sizeof(update_signature)));
  assert(hubos_driver_loader_update_trusted_key(&loader, &update));
  assert(hubos_driver_loader_is_key_trusted(&loader, "next-key"));
  assert(hubos_driver_loader_trusted_key_count(&loader) == 2);
  assert(audit_find_event_type(&audit, HUBOS_AUDIT_DRIVER_KEY_ROTATED) != NULL);
  rotated_package.signing_key_id = "next-key";
  assert(hubos_driver_loader_compute_package_hash(&rotated_package,
                                                  rotated_hash,
                                                  sizeof(rotated_hash)));
  rotated_package.hash = rotated_hash;
  assert(test_make_package_signature(next_private_key,
                                      &rotated_package,
                                      rotated_signature,
                                      sizeof(rotated_signature)));
  rotated_package.signature = rotated_signature;
  assert(hubos_driver_service_endpoint_prepare_rebind(&endpoint, 10, driver_id, &rotated_package));
  assert(hubos_driver_service_endpoint_get(&endpoint, 10)->state ==
         HUBOS_DRIVER_SLOT_REBINDING);
  assert(hubos_audit_log_count(&audit) >= 2);
  assert(audit_find_event_type(&audit, HUBOS_AUDIT_DRIVER_REBIND_PREPARED) != NULL);
  assert(hubos_driver_service_endpoint_complete_rebind(&endpoint, 10, driver_id, &rotated_package));
  assert(hubos_driver_service_endpoint_get(&endpoint, 10)->state == HUBOS_DRIVER_SLOT_BOUND);

  assert(test_make_key_revocation_signature(next_private_key, &revocation, revoke_signature, sizeof(revoke_signature)));
  assert(hubos_driver_loader_revoke_key(&loader, &revocation));
  assert(hubos_driver_loader_is_key_revoked(&loader, "root-key"));
  assert(hubos_driver_loader_revoked_key_count(&loader) == 1);
  assert(audit_find_event_type(&audit, HUBOS_AUDIT_DRIVER_KEY_REVOKED) != NULL);
  package.signing_key_id = "root-key";
  assert(hubos_driver_loader_compute_package_hash(&package, package_hash, sizeof(package_hash)));
  package.hash = package_hash;
  assert(test_make_package_signature(root_private_key, &package, package_signature, sizeof(package_signature)));
  package.signature = package_signature;
  assert(!hubos_driver_service_endpoint_bind(&endpoint, 11, driver_id, &package));
  assert(hubos_driver_service_endpoint_unbind(&endpoint, 10));
  assert(hubos_driver_service_endpoint_get(&endpoint, 10)->state == HUBOS_DRIVER_SLOT_UNBOUND);

  hubos_driver_service_destroy(&service);
  hubos_driver_loader_destroy(&loader);
  hubos_audit_log_destroy(&audit);
  hubos_driver_registry_destroy(&registry);
  EVP_PKEY_free(root_private_key);
  EVP_PKEY_free(next_private_key);
}

static void test_network_server(void) {
  hubos_network_server_t server;
  hubos_network_server_endpoint_t endpoint;
  hubos_namespace_handle_t namespace_handle;
  hubos_service_descriptor_t descriptor;
  hubos_id_t selected_nic = HUBOS_ID_INVALID;

  hubos_network_server_init(&server, 77, 88);
#if HUBOS_USE_LINUX_VM_NETWORK_BACKEND
  assert(hubos_network_server_backend(&server) == HUBOS_NETWORK_BACKEND_NETWORKMANAGER);
#else
  assert(hubos_network_server_backend(&server) == HUBOS_NETWORK_BACKEND_LWIP);
#endif
  assert(hubos_network_server_set_backend(&server, HUBOS_NETWORK_BACKEND_NETWORKMANAGER));
  assert(hubos_network_server_backend(&server) == HUBOS_NETWORK_BACKEND_NETWORKMANAGER);
  hubos_network_server_endpoint_init(&endpoint, &server);
  hubos_namespace_handle_init(&namespace_handle,
                              10,
                              HUBOS_NAMESPACE_NETWORK,
                              "network",
                              false);

  assert(hubos_network_server_endpoint_bind_namespace(&endpoint, namespace_handle));
  assert(server.namespace_handle.lifecycle.refcount == 1);
  assert(server.namespace_handle.lifecycle.state == HUBOS_SHARED_RESOURCE_ACTIVE);
  assert(server.namespace_handle.lifecycle.owner_session_id == 88);
  assert(server.namespace_handle.owned_by_server);
  assert(hubos_network_server_endpoint_set_policy(&endpoint, true, true));
  assert(hubos_network_server_endpoint_add_route(&endpoint, "10.0.0.0/24", 20, 10));
  assert(hubos_network_server_endpoint_set_default_route(&endpoint, 30));
  assert(hubos_network_server_endpoint_set_failover_policy(&endpoint, true, 40));
  assert(hubos_network_server_endpoint_select_nic(&endpoint,
                                                   "10.0.0.0/24",
                                                   strlen("10.0.0.0/24"),
                                                   &selected_nic));
  assert(selected_nic == 20);
  assert(hubos_network_server_endpoint_select_nic(&endpoint, "192.168.0.1", strlen("192.168.0.1"), &selected_nic));
  assert(selected_nic == 40);
  assert(hubos_network_server_endpoint_bind_port(&endpoint, 443, 20, 99));
  assert(hubos_network_server_can_relay(&server));
  assert(hubos_network_server_endpoint_describe(&endpoint, &descriptor));
  assert(descriptor.resource_id == 10);
  assert(descriptor.resource_state == HUBOS_RESOURCE_READY);
  assert(hubos_network_server_set_policy(&server, false, true));
  assert(!hubos_network_server_can_relay(&server));
  assert(hubos_network_server_release_namespace(&server));
  assert(!server.namespace_bound);
  assert(server.namespace_handle.lifecycle.state == HUBOS_SHARED_RESOURCE_PENDING_FINALIZATION);
  assert(hubos_network_server_endpoint_describe(&endpoint, &descriptor));
  assert(descriptor.resource_id == 77);
  assert(descriptor.resource_state == HUBOS_RESOURCE_DISCOVERED);
  assert(hubos_network_server_finalize_namespace(&server));
  assert(server.namespace_handle.lifecycle.state == HUBOS_SHARED_RESOURCE_RETIRED);

  hubos_network_server_destroy(&server);
}

static void test_hub_model(void) {
  hubos_resource_registry_t resources;
  hubos_capability_manager_t capabilities;
  hubos_hub_t hub;
  hubos_service_descriptor_t descriptor;
  hubos_id_t resource_id = HUBOS_ID_INVALID;
  hubos_id_t capability_id = HUBOS_ID_INVALID;
  bool is_new = false;

  hubos_resource_registry_init(&resources);
  hubos_capability_manager_init(&capabilities);

  assert(hubos_resource_registry_register(&resources,
                                          "network.nic0",
                                          strlen("network.nic0"),
                                          HUBOS_RESOURCE_READY,
                                          &resource_id,
                                          &is_new));
  assert(is_new);

  assert(hubos_capability_manager_issue(&capabilities,
                                        100,
                                        resource_id,
                                        HUBOS_CAP_RIGHT_INSPECT,
                                        true,
                                        &capability_id));

  hubos_hub_init(&hub, &resources, &capabilities);

  assert(hubos_hub_resolve(&hub, "network.nic0", strlen("network.nic0"), &descriptor));
  assert(descriptor.resource_id == resource_id);
  assert(descriptor.resource_state == HUBOS_RESOURCE_READY);

  assert(hubos_hub_authorize(&hub,
                             capability_id,
                             resource_id,
                             HUBOS_CAP_RIGHT_INSPECT));
  assert(!hubos_hub_authorize(&hub,
                              capability_id,
                              resource_id,
                              HUBOS_CAP_RIGHT_COPY));

  hubos_capability_manager_destroy(&capabilities);
  hubos_resource_registry_destroy(&resources);
}

static void test_system_model(void) {
  hubos_system_t system;
  hubos_id_t resource_id = HUBOS_ID_INVALID;
  hubos_id_t driver_id = HUBOS_ID_INVALID;
  hubos_id_t session_id = HUBOS_ID_INVALID;
  hubos_id_t capability_id = HUBOS_ID_INVALID;
  hubos_id_t memory_id = HUBOS_ID_INVALID;
  hubos_service_descriptor_t descriptor;
  bool is_new = false;
  char package_hash[HUBOS_DRIVER_PACKAGE_HASH_HEX_LENGTH];
  char package_signature[TEST_ED25519_SIGNATURE_HEX_LENGTH];
  uint8_t root_public_key[32];
  char root_public_key_hex[65];
  EVP_PKEY *root_private_key = NULL;
  hubos_driver_package_t package = {
    .manifest = "manifest",
    .binary = "binary",
    .signature = "signature",
    .hash = "hash",
    .version = "1.0.0",
    .signing_key_id = "root-key",
    .dependencies = NULL,
    .dependency_count = 0,
  };

  assert(test_ed25519_keypair_from_seed(test_root_seed, root_public_key, &root_private_key));
  assert(test_hex_encode(root_public_key,
                         sizeof(root_public_key),
                         root_public_key_hex,
                         sizeof(root_public_key_hex)));
  hubos_system_init(&system, "root-key");

  assert(hubos_boot_state_complete_step(&system.boot_state, &system.audit_log, HUBOS_BOOT_FIRMWARE));
  assert(hubos_boot_state_complete_step(&system.boot_state, &system.audit_log, HUBOS_BOOT_SEL4));
  assert(hubos_boot_state_complete_step(&system.boot_state, &system.audit_log, HUBOS_BOOT_ROOT_TASK));
  assert(hubos_boot_state_complete_step(&system.boot_state,
                                        &system.audit_log,
                                        HUBOS_BOOT_RESOURCE_REGISTRY));
  assert(hubos_boot_state_complete_step(&system.boot_state,
                                        &system.audit_log,
                                        HUBOS_BOOT_SESSION_MANAGER));
  assert(hubos_boot_state_complete_step(&system.boot_state,
                                        &system.audit_log,
                                        HUBOS_BOOT_CAPABILITY_MANAGER));
  assert(hubos_boot_state_complete_step(&system.boot_state,
                                        &system.audit_log,
                                        HUBOS_BOOT_MEMORY_MANAGER));
  assert(hubos_boot_state_complete_step(&system.boot_state, &system.audit_log, HUBOS_BOOT_DMA_MANAGER));
  assert(hubos_boot_state_complete_step(&system.boot_state, &system.audit_log, HUBOS_BOOT_HUB));
  assert(hubos_system_boot_capabilities_validate(&system));
  assert(hubos_boot_capability_set_count(hubos_system_boot_capabilities(&system)) ==
         HUBOS_BOOT_CAP_COUNT);

  assert(hubos_resource_registry_endpoint_register(&system.resource_registry_endpoint,
                                                   "network.nic0",
                                                   strlen("network.nic0"),
                                                   HUBOS_RESOURCE_READY,
                                                   &resource_id,
                                                   &is_new));
  assert(is_new);
  assert(hubos_system_register_driver(&system,
                                      1,
                                      2,
                                      3,
                                      "nic-driver",
                                      "1.0.0",
                                      &driver_id,
                                      &is_new));
  assert(hubos_session_manager_endpoint_create(&system.session_manager_endpoint,
                                               100,
                                               HUBOS_ID_INVALID,
                                               HUBOS_SESSION_PERSISTENT,
                                               &session_id));
  assert(hubos_capability_manager_endpoint_issue(&system.capability_manager_endpoint,
                                                 session_id,
                                                 resource_id,
                                                 HUBOS_CAP_RIGHT_INSPECT,
                                                 true,
                                                 &capability_id));
  assert(hubos_hub_endpoint_resolve(&system.hub_endpoint,
                                    "network.nic0",
                                    strlen("network.nic0"),
                                    &descriptor));
  assert(descriptor.resource_id == resource_id);
  assert(hubos_capability_manager_endpoint_authorize(&system.capability_manager_endpoint,
                                                     capability_id,
                                                     resource_id,
                                                     HUBOS_CAP_RIGHT_INSPECT));
  assert(hubos_system_allocate_frame(&system, 4096, 0, &memory_id));
  assert(hubos_system_share_memory(&system, memory_id));
  assert(hubos_system_reclaim_memory(&system, memory_id));
  assert(hubos_system_bus_discover(&system,
                                   HUBOS_BUS_PCIE,
                                   "resource://pci/0000:01:00.1",
                                   strlen("resource://pci/0000:01:00.1"),
                                   HUBOS_RESOURCE_DISCOVERED,
                                   &resource_id));
  assert(hubos_system_map_dma(&system, resource_id));
  assert(hubos_driver_loader_compute_package_hash(&package,
                                                  package_hash,
                                                  sizeof(package_hash)));
  package.hash = package_hash;
  assert(test_make_package_signature(root_private_key,
                                     &package,
                                     package_signature,
                                     sizeof(package_signature)));
  package.signature = package_signature;
  assert(hubos_driver_service_endpoint_bind(&system.driver_service_endpoint,
                                            resource_id,
                                            driver_id,
                                            &package));
  assert(hubos_audit_log_count(&system.audit_log) > 0);

  hubos_system_destroy(&system);
  hubos_system_init(&system, "root-key");
  assert(!hubos_system_boot_step_is_complete(&system, HUBOS_BOOT_HUB));
  assert(hubos_system_boot_capabilities_validate(&system));
  hubos_system_destroy(&system);
  EVP_PKEY_free(root_private_key);
}

static void test_root_task_ipc(void) {
  hubos_system_t system;
  hubos_root_task_t root_task;
  hubos_root_task_request_t request;
  hubos_root_task_response_t response;

  hubos_system_init(&system, "root-key");
  hubos_root_task_init(&root_task, &system);

  hubos_root_task_request_init(&request, HUBOS_ROOT_TASK_OP_BOOTSTRAP);
  assert(hubos_root_task_dispatch(&root_task, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(hubos_root_task_is_dormant(&root_task));
  assert(root_task.hub_resource_id != HUBOS_ID_INVALID);
  assert(root_task.resource_registry_resource_id != HUBOS_ID_INVALID);
  assert(root_task.root_session_id != HUBOS_ID_INVALID);
  assert(root_task.root_capability_id != HUBOS_ID_INVALID);
  assert(root_task.driver_registry_resource_id != HUBOS_ID_INVALID);
  assert(root_task.bootstrap_driver_id != HUBOS_ID_INVALID);
  assert(!hubos_root_task_dispatch(&root_task, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_DENIED);

  hubos_root_task_request_init(&request, HUBOS_ROOT_TASK_OP_COMPLETE_BOOT_STEP);
  request.payload.boot_step.step = HUBOS_BOOT_FIRMWARE;
  assert(hubos_root_task_dispatch(&root_task, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.boot_step == HUBOS_BOOT_FIRMWARE);
  assert(hubos_system_boot_step_is_complete(&system, HUBOS_BOOT_FIRMWARE));

  hubos_root_task_request_init(&request, HUBOS_ROOT_TASK_OP_QUERY_BOOT_STEP);
  request.payload.boot_step.step = HUBOS_BOOT_FIRMWARE;
  assert(hubos_root_task_dispatch(&root_task, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.boot_step == HUBOS_BOOT_FIRMWARE);
  assert(response.bool_result);

  hubos_root_task_request_init(&request, HUBOS_ROOT_TASK_OP_ADVANCE_CONTROL_PLANE);
  assert(hubos_root_task_dispatch(&root_task, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.boot_step == HUBOS_BOOT_RESOURCE_REGISTRY);
  assert(response.bool_result);
  assert(hubos_system_boot_step_is_complete(&system, HUBOS_BOOT_RESOURCE_REGISTRY));

  hubos_system_destroy(&system);
}

static void test_microkit_entrypoints(void) {
  hubos_system_t system;
  hubos_root_task_request_t root_request;
  hubos_root_task_response_t root_response;
  hubos_microkit_ipc_request_t service_request;
  hubos_microkit_ipc_response_t service_response;
  hubos_microkit_ipc_response_t protected_response;
  hubos_microkit_transport_frame_t service_frame;
  hubos_microkit_transport_frame_t reply_frame;
  microkit_msginfo service_msginfo;
  microkit_msginfo reply_msginfo;

  (void)protected_response;
  (void)reply_frame;

  hubos_system_init(&system, "root-key");
  hubos_microkit_generated_init(&system);
  assert(hubos_microkit_generated_validate());

  hubos_root_task_request_init(&root_request, HUBOS_ROOT_TASK_OP_BOOTSTRAP);
  assert(hubos_microkit_generated_dispatch_root_task_entrypoint(0, &root_request, &root_response));
  assert(root_response.status == HUBOS_IPC_STATUS_OK);
  assert(hubos_microkit_generated_state()->runtime.root_task.bootstrapped);
  assert(hubos_microkit_generated_state()->runtime.root_task.root_session_id != HUBOS_ID_INVALID);
  assert(!hubos_microkit_generated_is_bootstrapped());

  hubos_root_task_request_init(&root_request, HUBOS_ROOT_TASK_OP_COMPLETE_BOOT_STEP);
  root_request.payload.boot_step.step = HUBOS_BOOT_FIRMWARE;
  assert(hubos_microkit_generated_dispatch_root_task_entrypoint(0, &root_request, &root_response));
  assert(root_response.status == HUBOS_IPC_STATUS_OK);
  assert(root_response.boot_step == HUBOS_BOOT_FIRMWARE);
  assert(hubos_system_boot_step_is_complete(&system, HUBOS_BOOT_FIRMWARE));

  hubos_root_task_request_init(&root_request, HUBOS_ROOT_TASK_OP_QUERY_BOOT_STEP);
  root_request.payload.boot_step.step = HUBOS_BOOT_FIRMWARE;
  assert(hubos_microkit_generated_dispatch_root_task_entrypoint(0, &root_request, &root_response));
  assert(root_response.status == HUBOS_IPC_STATUS_OK);
  assert(root_response.bool_result);

  hubos_root_task_request_init(&root_request, HUBOS_ROOT_TASK_OP_ADVANCE_CONTROL_PLANE);
  assert(hubos_microkit_generated_dispatch_root_task_entrypoint(0, &root_request, &root_response));
  assert(root_response.status == HUBOS_IPC_STATUS_OK);
  assert(root_response.boot_step == HUBOS_BOOT_RESOURCE_REGISTRY);
  assert(root_response.bool_result);
  assert(hubos_system_boot_step_is_complete(&system, HUBOS_BOOT_RESOURCE_REGISTRY));

  hubos_microkit_generated_reset();
  hubos_system_destroy(&system);

  hubos_system_init(&system, "root-key");
  hubos_microkit_generated_init(&system);
  assert(hubos_microkit_generated_validate());
  assert(hubos_microkit_generated_bootstrap_entrypoint());
  assert(hubos_microkit_generated_is_bootstrapped());

  hubos_microkit_ipc_request_init(&service_request,
                                  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
                                  HUBOS_MICROKIT_RESOURCE_OP_REGISTER);
  service_request.payload.resource_register.name = "resource://pci/0000:01:00.0";
  service_request.payload.resource_register.name_len = strlen("resource://pci/0000:01:00.0");
  service_request.payload.resource_register.state = HUBOS_RESOURCE_DISCOVERED;

  hubos_microkit_transport_frame_init(&service_frame);
  assert(hubos_microkit_transport_request_encode(&service_request, &service_frame));
  hubos_microkit_transport_frame_to_mrs(&service_frame);
  service_msginfo = hubos_microkit_transport_frame_to_msginfo(&service_frame,
                                                              HUBOS_MICROKIT_TRANSPORT_LABEL);
  reply_msginfo = microkit_msginfo_new(99, 99);
  assert(!hubos_microkit_generated_dispatch_protected(1,
                                                     microkit_msginfo_new(1, service_frame.count),
                                                     &reply_msginfo));
  assert(microkit_msginfo_get_label(reply_msginfo) == 0);
  assert(microkit_msginfo_get_count(reply_msginfo) == 0);
  reply_msginfo = microkit_msginfo_new(99, 99);
  assert(!hubos_microkit_generated_dispatch_protected(7, service_msginfo, &reply_msginfo));
  assert(microkit_msginfo_get_label(reply_msginfo) == 0);
  assert(microkit_msginfo_get_count(reply_msginfo) == 0);

  assert(hubos_microkit_generated_dispatch_entrypoint(1,
                                                      HUBOS_MICROKIT_ENTRYPOINT_SERVICE,
                                                      &service_request,
                                                      &service_response));
  assert(service_response.status == HUBOS_IPC_STATUS_OK);
  assert(service_response.resource_id != HUBOS_ID_INVALID);

  hubos_root_task_request_init(&root_request, HUBOS_ROOT_TASK_OP_COMPLETE_BOOT_STEP);
  root_request.payload.boot_step.step = HUBOS_BOOT_FIRMWARE;
  assert(hubos_microkit_generated_dispatch_root_task_entrypoint(0, &root_request, &root_response));
  assert(root_response.status == HUBOS_IPC_STATUS_OK);
  assert(root_response.boot_step == HUBOS_BOOT_FIRMWARE);
  assert(hubos_system_boot_step_is_complete(&system, HUBOS_BOOT_FIRMWARE));

  assert(hubos_microkit_generated_dispatch_notification(10));
  assert(!hubos_microkit_generated_dispatch_notification(1));
  assert(hubos_microkit_generated_dispatch_notification_entrypoint(10));
  assert(!hubos_microkit_generated_dispatch_notification_entrypoint(1));

  hubos_microkit_generated_reset();
  hubos_system_destroy(&system);
}

static void test_microkit_kernel_glue_entrypoints(void) {
  hubos_system_t system;
  hubos_microkit_runtime_t runtime;
  hubos_microkit_runtime_t service_runtime;
  hubos_root_task_request_t root_request;
  hubos_root_task_response_t root_response;
  hubos_microkit_ipc_request_t service_request;
  hubos_microkit_ipc_response_t service_response;
  hubos_microkit_ipc_response_t decoded_response;
  hubos_microkit_transport_frame_t request_frame;
  hubos_microkit_transport_frame_t reply_frame;
  microkit_msginfo request_msginfo;
  microkit_msginfo reply_msginfo;
  hubos_microkit_ipc_request_t decoded_request;

  hubos_system_init(&system, "root-key");
  hubos_microkit_runtime_init(&runtime, &system);
  hubos_root_task_request_init(&root_request, HUBOS_ROOT_TASK_OP_BOOTSTRAP);
  assert(hubos_microkit_kernel_dispatch_root_task(&runtime,
                                                  0,
                                                  &root_request,
                                                  &root_response));
  assert(root_response.status == HUBOS_IPC_STATUS_OK);
  assert(runtime.root_task.bootstrapped);
  assert(!hubos_microkit_runtime_is_bootstrapped(&runtime));

  hubos_root_task_request_init(&root_request, HUBOS_ROOT_TASK_OP_COMPLETE_BOOT_STEP);
  root_request.payload.boot_step.step = HUBOS_BOOT_FIRMWARE;
  assert(hubos_microkit_kernel_dispatch_root_task(&runtime,
                                                  0,
                                                  &root_request,
                                                  &root_response));
  assert(root_response.status == HUBOS_IPC_STATUS_OK);
  assert(root_response.boot_step == HUBOS_BOOT_FIRMWARE);
  assert(hubos_system_boot_step_is_complete(&system, HUBOS_BOOT_FIRMWARE));

  hubos_root_task_request_init(&root_request, HUBOS_ROOT_TASK_OP_ADVANCE_CONTROL_PLANE);
  assert(hubos_microkit_kernel_dispatch_root_task(&runtime,
                                                  0,
                                                  &root_request,
                                                  &root_response));
  assert(root_response.status == HUBOS_IPC_STATUS_OK);
  assert(root_response.boot_step == HUBOS_BOOT_RESOURCE_REGISTRY);
  assert(root_response.bool_result);

  hubos_microkit_runtime_destroy(&runtime);

  hubos_microkit_runtime_init(&service_runtime, &system);
  assert(hubos_microkit_kernel_bootstrap(&service_runtime));
  assert(hubos_microkit_runtime_is_bootstrapped(&service_runtime));
  hubos_microkit_ipc_request_init(&service_request,
                                  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
                                  HUBOS_MICROKIT_RESOURCE_OP_REGISTER);
  service_request.payload.resource_register.name = "resource://pci/0000:02:00.0";
  service_request.payload.resource_register.name_len = strlen("resource://pci/0000:02:00.0");
  service_request.payload.resource_register.state = HUBOS_RESOURCE_DISCOVERED;

  assert(hubos_microkit_kernel_dispatch_notification(&service_runtime, 10));
  assert(!hubos_microkit_kernel_dispatch_notification(&service_runtime, 1));
  assert(hubos_microkit_kernel_dispatch_entrypoint(&service_runtime,
                                                   10,
                                                   HUBOS_MICROKIT_ENTRYPOINT_NOTIFICATION,
                                                   NULL,
                                                   NULL));
  assert(!hubos_microkit_kernel_dispatch_entrypoint(&service_runtime,
                                                    1,
                                                    HUBOS_MICROKIT_ENTRYPOINT_NOTIFICATION,
                                                    NULL,
                                                    NULL));
  hubos_microkit_transport_frame_init(&request_frame);
  assert(hubos_microkit_transport_request_encode(&service_request, &request_frame));
  hubos_microkit_transport_frame_to_mrs(&request_frame);
  request_msginfo = hubos_microkit_transport_frame_to_msginfo(
    &request_frame,
    HUBOS_MICROKIT_TRANSPORT_LABEL);
  assert(hubos_microkit_transport_frame_from_msginfo(&reply_frame, request_msginfo));
  assert(hubos_microkit_transport_request_decode(&reply_frame, &decoded_request));
  assert(decoded_request.service == HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY);
  assert(decoded_request.operation == HUBOS_MICROKIT_RESOURCE_OP_REGISTER);
  assert(decoded_request.payload.resource_register.name ==
         service_request.payload.resource_register.name);
  assert(decoded_request.payload.resource_register.name_len ==
         service_request.payload.resource_register.name_len);
  assert(decoded_request.payload.resource_register.state ==
         service_request.payload.resource_register.state);
  reply_msginfo = microkit_msginfo_new(99, 99);
  assert(!hubos_microkit_kernel_dispatch_protected(&service_runtime,
                                                   1,
                                                   microkit_msginfo_new(1, request_frame.count),
                                                   &reply_msginfo));
  assert(microkit_msginfo_get_label(reply_msginfo) == 0);
  assert(microkit_msginfo_get_count(reply_msginfo) == 0);
  reply_msginfo = microkit_msginfo_new(99, 99);
  assert(!hubos_microkit_kernel_dispatch_protected(&service_runtime,
                                                   7,
                                                   request_msginfo,
                                                   &reply_msginfo));
  assert(microkit_msginfo_get_label(reply_msginfo) == 0);
  assert(microkit_msginfo_get_count(reply_msginfo) == 0);
  assert(hubos_microkit_kernel_dispatch_service(&service_runtime,
                                                1,
                                                &service_request,
                                                &service_response));
  assert(service_response.status == HUBOS_IPC_STATUS_OK);
  assert(service_response.resource_id != HUBOS_ID_INVALID);
  assert(service_response.is_new);
  hubos_microkit_transport_frame_init(&reply_frame);
  assert(hubos_microkit_transport_response_encode(&service_response, &reply_frame));
  hubos_microkit_transport_frame_to_mrs(&reply_frame);
  reply_msginfo = hubos_microkit_transport_frame_to_msginfo(&reply_frame,
                                                            HUBOS_MICROKIT_TRANSPORT_LABEL);
  assert(microkit_msginfo_get_label(reply_msginfo) == HUBOS_MICROKIT_TRANSPORT_LABEL);
  assert(microkit_msginfo_get_count(reply_msginfo) == 16);
  assert(hubos_microkit_transport_frame_from_msginfo(&request_frame, reply_msginfo));
  assert(hubos_microkit_transport_response_decode(&request_frame, &decoded_response));
  assert(decoded_response.status == HUBOS_IPC_STATUS_OK);
  assert(decoded_response.resource_id != HUBOS_ID_INVALID);
  assert(decoded_response.is_new);
  assert(decoded_response.descriptor.resource_id == HUBOS_ID_INVALID);
  assert(decoded_response.descriptor.name == NULL);
  assert(decoded_response.descriptor.name_len == 0);
  hubos_microkit_runtime_destroy(&service_runtime);
  hubos_system_destroy(&system);
}

static void test_microkit_endpoint_badge_coverage(void) {
  hubos_system_t system;
  const hubos_microkit_boot_manifest_t *manifest;
  const hubos_microkit_endpoint_binding_t *binding;

  hubos_system_init(&system, "root-key");
  manifest = hubos_system_microkit_boot_manifest(&system);
  assert(manifest != NULL);
  assert(hubos_microkit_boot_manifest_publishable_endpoint_count(manifest) == 11);
  assert(hubos_system_microkit_boot_manifest_validate(&system));

  binding = hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 0);
  assert(binding != NULL);
  assert(binding->component == HUBOS_MICROKIT_COMPONENT_ROOT_TASK);
  assert(binding->exposed);

  binding = hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 1);
  assert(binding != NULL);
  assert(binding->component == HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY);
  assert(binding->exposed);

  binding = hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 2);
  assert(binding != NULL);
  assert(binding->component == HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER);
  assert(binding->exposed);

  binding = hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 3);
  assert(binding != NULL);
  assert(binding->component == HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER);
  assert(binding->exposed);

  binding = hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 4);
  assert(binding != NULL);
  assert(binding->component == HUBOS_MICROKIT_COMPONENT_HUB);
  assert(binding->exposed);

  binding = hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 5);
  assert(binding != NULL);
  assert(binding->component == HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE);
  assert(binding->exposed);

  binding = hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 6);
  assert(binding != NULL);
  assert(binding->component == HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER);
  assert(binding->exposed);

  binding = hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 12);
  assert(binding != NULL);
  assert(binding->component == HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER);
  assert(binding->exposed);

  binding = hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 13);
  assert(binding != NULL);
  assert(binding->component == HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER);
  assert(binding->exposed);

  binding = hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 14);
  assert(binding != NULL);
  assert(binding->component == HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER);
  assert(binding->exposed);

  binding = hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 15);
  assert(binding != NULL);
  assert(binding->component == HUBOS_MICROKIT_COMPONENT_VM_SERVER);
  assert(binding->exposed);

  assert(hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 7) == NULL);
  assert(hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 8) == NULL);
  assert(hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 9) == NULL);
  assert(hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 10) == NULL);
  assert(hubos_microkit_ipc_layout_get_by_badge(&system.microkit_ipc_layout, 11) == NULL);

  hubos_system_destroy(&system);
}

static void test_microkit_service_dispatch_routes(void) {
  hubos_system_t system;
  hubos_root_task_request_t root_request;
  hubos_root_task_response_t root_response;
  hubos_microkit_ipc_request_t request;
  hubos_microkit_ipc_response_t response;
  hubos_namespace_handle_t namespace_handle;
  hubos_driver_package_t package = {
    .manifest = "manifest",
    .binary = "binary",
    .signature = NULL,
    .hash = NULL,
    .version = "1.0.0",
    .signing_key_id = "root-key",
    .dependencies = NULL,
    .dependency_count = 0,
    .platform_abi_version = 0,
    .minimum_platform_abi_version = 0,
  };
  hubos_id_t resource_id = HUBOS_ID_INVALID;
  hubos_id_t session_id = HUBOS_ID_INVALID;
  hubos_id_t capability_id = HUBOS_ID_INVALID;
  hubos_id_t driver_id = HUBOS_ID_INVALID;
  bool is_new = false;
  char package_hash[HUBOS_DRIVER_PACKAGE_HASH_HEX_LENGTH];
  char package_signature[TEST_ED25519_SIGNATURE_HEX_LENGTH];
  uint8_t root_public_key[32];
  char root_public_key_hex[65];
  EVP_PKEY *root_private_key = NULL;

  assert(test_ed25519_keypair_from_seed(test_root_seed, root_public_key, &root_private_key));
  assert(test_hex_encode(root_public_key,
                         sizeof(root_public_key),
                         root_public_key_hex,
                         sizeof(root_public_key_hex)));
  hubos_system_init(&system, "root-key");
  hubos_microkit_generated_init(&system);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
                                  HUBOS_MICROKIT_RESOURCE_OP_REGISTER);
  request.payload.resource_register.name = "resource://pci/0000:01:00.0";
  request.payload.resource_register.name_len = strlen("resource://pci/0000:01:00.0");
  request.payload.resource_register.state = HUBOS_RESOURCE_DISCOVERED;
  assert(!hubos_microkit_generated_dispatch_service_entrypoint(1, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_INVALID_ARGUMENT);

  hubos_root_task_request_init(&root_request, HUBOS_ROOT_TASK_OP_BOOTSTRAP);
  assert(hubos_microkit_generated_dispatch_root_task_entrypoint(0, &root_request, &root_response));
  assert(root_response.status == HUBOS_IPC_STATUS_OK);
  assert(hubos_microkit_generated_state()->runtime.root_task.bootstrapped);
  assert(!hubos_microkit_generated_is_bootstrapped());
  hubos_microkit_generated_reset();
  hubos_system_destroy(&system);

  hubos_system_init(&system, "root-key");
  hubos_microkit_generated_init(&system);
  assert(hubos_microkit_generated_bootstrap_entrypoint());
  assert(hubos_microkit_generated_is_bootstrapped());

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
                                  HUBOS_MICROKIT_RESOURCE_OP_REGISTER);
  request.payload.resource_register.name = "resource://pci/0000:01:00.0";
  request.payload.resource_register.name_len = strlen("resource://pci/0000:01:00.0");
  request.payload.resource_register.state = HUBOS_RESOURCE_DISCOVERED;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(1, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.resource_id != HUBOS_ID_INVALID);
  resource_id = response.resource_id;

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER,
                                  HUBOS_MICROKIT_SESSION_OP_CREATE);
  request.payload.session_create.owner_id = 100;
  request.payload.session_create.parent_id = HUBOS_ID_INVALID;
  request.payload.session_create.type = HUBOS_SESSION_PERSISTENT;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(3, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.session_id != HUBOS_ID_INVALID);
  session_id = response.session_id;

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER,
                                  HUBOS_MICROKIT_CAPABILITY_OP_ISSUE);
  request.payload.capability_issue.owner_session_id = session_id;
  request.payload.capability_issue.resource_id = resource_id;
  request.payload.capability_issue.rights = HUBOS_CAP_RIGHT_INSPECT | HUBOS_CAP_RIGHT_COPY;
  request.payload.capability_issue.delegatable = true;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(2, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.capability_id != HUBOS_ID_INVALID);
  capability_id = response.capability_id;

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER,
                                  HUBOS_MICROKIT_CAPABILITY_OP_AUTHORIZE);
  request.payload.capability_authorize.capability_id = capability_id;
  request.payload.capability_authorize.resource_id = resource_id;
  request.payload.capability_authorize.required_rights = HUBOS_CAP_RIGHT_INSPECT;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(2, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.bool_result);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request, HUBOS_MICROKIT_COMPONENT_HUB, HUBOS_MICROKIT_HUB_OP_RESOLVE);
  request.payload.hub_resolve.name = "resource://pci/0000:01:00.0";
  request.payload.hub_resolve.name_len = strlen("resource://pci/0000:01:00.0");
  assert(hubos_microkit_generated_dispatch_service_entrypoint(4, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.descriptor.resource_id == resource_id);

  assert(hubos_system_register_driver(&system,
                                      1,
                                      2,
                                      3,
                                      "nic-driver",
                                      "1.0.0",
                                      &driver_id,
                                      &is_new));
  assert(is_new);
  assert(driver_id != HUBOS_ID_INVALID);

  assert(hubos_driver_loader_compute_package_hash(&package,
                                                  package_hash,
                                                  sizeof(package_hash)));
  package.hash = package_hash;
  assert(test_make_package_signature(root_private_key,
                                     &package,
                                     package_signature,
                                     sizeof(package_signature)));
  package.signature = package_signature;

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE,
                                  HUBOS_MICROKIT_DRIVER_OP_BIND);
  request.payload.driver_bind.resource_id = resource_id;
  request.payload.driver_bind.driver_id = driver_id;
  request.payload.driver_bind.package = package;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(5, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.driver_id == driver_id);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE,
                                  HUBOS_MICROKIT_DRIVER_OP_UNBIND);
  request.payload.driver_unbind.resource_id = resource_id;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(5, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);

  hubos_namespace_handle_init(&namespace_handle,
                              10,
                              HUBOS_NAMESPACE_NETWORK,
                              "network",
                              false);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER,
                                  HUBOS_MICROKIT_NETWORK_OP_BIND_NAMESPACE);
  request.payload.network_bind_namespace.namespace_handle = namespace_handle;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(6, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER,
                                  HUBOS_MICROKIT_NETWORK_OP_SET_POLICY);
  request.payload.network_set_policy.routing_enabled = true;
  request.payload.network_set_policy.firewall_enabled = true;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(6, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER,
                                  HUBOS_MICROKIT_NETWORK_OP_ADD_ROUTE);
  request.payload.network_add_route.destination = "10.0.0.0/24";
  request.payload.network_add_route.destination_len = strlen("10.0.0.0/24");
  request.payload.network_add_route.nic_resource_id = resource_id;
  request.payload.network_add_route.metric = 10;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(6, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER,
                                  HUBOS_MICROKIT_NETWORK_OP_SET_DEFAULT_ROUTE);
  request.payload.network_set_default_route.nic_resource_id = resource_id;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(6, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER,
                                  HUBOS_MICROKIT_NETWORK_OP_SELECT_NIC);
  request.payload.network_select_nic.destination = "192.168.0.1";
  request.payload.network_select_nic.destination_len = strlen("192.168.0.1");
  assert(hubos_microkit_generated_dispatch_service_entrypoint(6, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.resource_id == resource_id);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER,
                                  HUBOS_MICROKIT_NETWORK_OP_DESCRIBE);
  assert(hubos_microkit_generated_dispatch_service_entrypoint(6, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.descriptor.resource_id == namespace_handle.id);

  hubos_namespace_handle_init(&namespace_handle,
                              11,
                              HUBOS_NAMESPACE_STORAGE,
                              "storage",
                              false);
  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER,
                                  HUBOS_MICROKIT_STORAGE_OP_BIND_NAMESPACE);
  request.payload.storage_bind_namespace.namespace_handle = namespace_handle;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(13, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.resource_id == namespace_handle.id);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER,
                                  HUBOS_MICROKIT_STORAGE_OP_DESCRIBE);
  assert(hubos_microkit_generated_dispatch_service_entrypoint(13, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.descriptor.resource_id == namespace_handle.id);

  hubos_namespace_handle_init(&namespace_handle,
                              12,
                              HUBOS_NAMESPACE_DISPLAY,
                              "display",
                              false);
  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER,
                                  HUBOS_MICROKIT_DISPLAY_OP_BIND_NAMESPACE);
  request.payload.display_bind_namespace.namespace_handle = namespace_handle;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(14, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.resource_id == namespace_handle.id);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER,
                                  HUBOS_MICROKIT_DISPLAY_OP_DESCRIBE);
  assert(hubos_microkit_generated_dispatch_service_entrypoint(14, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.descriptor.resource_id == namespace_handle.id);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER,
                                  HUBOS_MICROKIT_DEVICE_OP_SET_OWNER);
  request.payload.device_owner.owner_session_id = 77;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(12, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.resource_id == 77);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER,
                                  HUBOS_MICROKIT_DEVICE_OP_ATTACH_MMIO);
  request.payload.device_owner.owner_session_id = 77;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(12, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER,
                                  HUBOS_MICROKIT_DEVICE_OP_RESET);
  assert(hubos_microkit_generated_dispatch_service_entrypoint(12, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.resource_id == system.device_server.resource_id);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_SET_GUEST_MEMORY);
  request.payload.vm_guest_memory.guest_memory_id = 200;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.resource_id == 200);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_SET_VCPU_COUNT);
  request.payload.vm_vcpu_count.vcpu_count = 4;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.count == 4);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_NET);
  request.payload.vm_session.session_id = 201;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.session_id == 201);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_SELECT_RUNTIME_PROFILE);
  request.payload.vm_select_runtime_profile.runtime_profile_id = "linux-dev";
  request.payload.vm_select_runtime_profile.runtime_profile_id_len = strlen("linux-dev");
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(strcmp(response.descriptor.version,
                request.payload.vm_select_runtime_profile.runtime_profile_id) == 0);
  assert(response.count == 2);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_SET_ARTIFACTS);
  request.payload.vm_set_artifacts.artifacts = (hubos_linux_vm_artifacts_t){
    .kernel_image = "build/vm/linux/Image",
    .initramfs_image = "build/vm/linux/initramfs.cpio.gz",
    .rootfs_image = "build/vm/linux/rootfs.ext4",
    .device_tree_blob = "build/vm/linux/guest.dtb",
    .kernel_cmdline = "console=ttyS0 root=/dev/vda rw",
  };
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_SET_RESTART_POLICY);
  request.payload.vm_set_restart_policy.policy = HUBOS_VM_RESTART_AUTO;
  request.payload.vm_set_restart_policy.max_restart_attempts = 1;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.count == 1);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_START);
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.resource_id == system.vm_server.id);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_DESCRIBE);
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.descriptor.resource_state == HUBOS_RESOURCE_CLASSIFIED);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_COMPLETE_BOOT);
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_DESCRIBE);
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.descriptor.resource_id == system.vm_server.id);
  assert(response.descriptor.resource_state == HUBOS_RESOURCE_READY);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_START);
  assert(!hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));

  hubos_system_stop_vm(&system);
  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_START);
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_FAIL);
  request.payload.vm_fail.failure_code = 17;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.bool_result);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_DESCRIBE);
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.descriptor.resource_state == HUBOS_RESOURCE_CLASSIFIED);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_FAIL);
  request.payload.vm_fail.failure_code = 18;
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(!response.bool_result);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_DESCRIBE);
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.descriptor.resource_state == HUBOS_RESOURCE_FAILED);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_STOP);
  assert(hubos_microkit_generated_dispatch_service_entrypoint(15, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_OK);
  assert(response.resource_id == system.vm_server.id);

  hubos_microkit_ipc_response_init(&response);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
                                  HUBOS_MICROKIT_RESOURCE_OP_REGISTER);
  request.payload.resource_register.name = "resource://pci/0000:01:00.1";
  request.payload.resource_register.name_len = strlen("resource://pci/0000:01:00.1");
  request.payload.resource_register.state = HUBOS_RESOURCE_DISCOVERED;
  assert(!hubos_microkit_generated_dispatch_service_entrypoint(0, &request, &response));
  assert(response.status == HUBOS_IPC_STATUS_INVALID_ARGUMENT);

  hubos_microkit_generated_reset();
  EVP_PKEY_free(root_private_key);
  hubos_system_destroy(&system);
}

static void test_microkit_transport_codec(void) {
  hubos_microkit_ipc_request_t request;
  hubos_microkit_ipc_request_t decoded_request;
  hubos_microkit_transport_frame_t frame;
  hubos_microkit_ipc_response_t response;
  hubos_microkit_ipc_response_t decoded_response;
  const char *dependencies[] = {"dep://alpha", "dep://beta"};
  const char *destination = "10.10.0.0/16";
  const char *manifest = "manifest";
  const char *binary = "binary";
  const char *signature = "signature";
  const char *hash = "hash";
  const char *version = "1.2.3";
  const char *signing_key = "root-key";
  hubos_namespace_handle_t namespace_handle;
  hubos_driver_package_t package = {
    .manifest = manifest,
    .binary = binary,
    .signature = signature,
    .hash = hash,
    .version = version,
    .signing_key_id = signing_key,
    .dependencies = dependencies,
    .dependency_count = 2,
    .platform_abi_version = 7,
    .minimum_platform_abi_version = 3,
  };

  hubos_namespace_handle_init(&namespace_handle, 42, HUBOS_NAMESPACE_NETWORK, "network", true);
  hubos_namespace_handle_set_owner_session(&namespace_handle, 99);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER,
                                  HUBOS_MICROKIT_NETWORK_OP_BIND_NAMESPACE);
  request.payload.network_bind_namespace.namespace_handle = namespace_handle;
  hubos_microkit_transport_frame_init(&frame);
  assert(hubos_microkit_transport_request_encode(&request, &frame));
  assert(frame.count == hubos_microkit_transport_request_word_count(&request));
  assert(hubos_microkit_transport_request_decode(&frame, &decoded_request));
  assert(decoded_request.service == request.service);
  assert(decoded_request.operation == request.operation);
  assert(decoded_request.payload.network_bind_namespace.namespace_handle.id ==
         namespace_handle.id);
  assert(decoded_request.payload.network_bind_namespace.namespace_handle.kind ==
         namespace_handle.kind);
  assert(decoded_request.payload.network_bind_namespace.namespace_handle.name ==
         namespace_handle.name);
  assert(decoded_request.payload.network_bind_namespace.namespace_handle.owned_by_server ==
         namespace_handle.owned_by_server);
  assert(decoded_request.payload.network_bind_namespace.namespace_handle.lifecycle.owner_session_id ==
         namespace_handle.lifecycle.owner_session_id);
  assert(decoded_request.payload.network_bind_namespace.namespace_handle.lifecycle.refcount ==
         namespace_handle.lifecycle.refcount);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE,
                                  HUBOS_MICROKIT_DRIVER_OP_BIND);
  request.payload.driver_bind.resource_id = 17;
  request.payload.driver_bind.driver_id = 23;
  request.payload.driver_bind.package = package;
  hubos_microkit_transport_frame_init(&frame);
  assert(hubos_microkit_transport_request_encode(&request, &frame));
  assert(frame.count == hubos_microkit_transport_request_word_count(&request));
  assert(hubos_microkit_transport_request_decode(&frame, &decoded_request));
  assert(decoded_request.payload.driver_bind.resource_id == request.payload.driver_bind.resource_id);
  assert(decoded_request.payload.driver_bind.driver_id == request.payload.driver_bind.driver_id);
  assert(decoded_request.payload.driver_bind.package.manifest == manifest);
  assert(decoded_request.payload.driver_bind.package.binary == binary);
  assert(decoded_request.payload.driver_bind.package.signature == signature);
  assert(decoded_request.payload.driver_bind.package.hash == hash);
  assert(decoded_request.payload.driver_bind.package.version == version);
  assert(decoded_request.payload.driver_bind.package.signing_key_id == signing_key);
  assert(decoded_request.payload.driver_bind.package.dependencies == dependencies);
  assert(decoded_request.payload.driver_bind.package.dependency_count == 2);
  assert(decoded_request.payload.driver_bind.package.platform_abi_version == 7);
  assert(decoded_request.payload.driver_bind.package.minimum_platform_abi_version == 3);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER,
                                  HUBOS_MICROKIT_NETWORK_OP_ADD_ROUTE);
  request.payload.network_add_route.destination = destination;
  request.payload.network_add_route.destination_len = strlen(destination);
  request.payload.network_add_route.nic_resource_id = 88;
  request.payload.network_add_route.metric = 4;
  hubos_microkit_transport_frame_init(&frame);
  assert(hubos_microkit_transport_request_encode(&request, &frame));
  assert(frame.count == hubos_microkit_transport_request_word_count(&request));
  assert(hubos_microkit_transport_request_decode(&frame, &decoded_request));
  assert(decoded_request.payload.network_add_route.destination == destination);
  assert(decoded_request.payload.network_add_route.destination_len == strlen(destination));
  assert(decoded_request.payload.network_add_route.nic_resource_id == 88);
  assert(decoded_request.payload.network_add_route.metric == 4);

  hubos_namespace_handle_init(&namespace_handle, 43, HUBOS_NAMESPACE_STORAGE, "storage", true);
  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER,
                                  HUBOS_MICROKIT_STORAGE_OP_BIND_NAMESPACE);
  request.payload.storage_bind_namespace.namespace_handle = namespace_handle;
  hubos_microkit_transport_frame_init(&frame);
  assert(hubos_microkit_transport_request_encode(&request, &frame));
  assert(frame.count == hubos_microkit_transport_request_word_count(&request));
  assert(hubos_microkit_transport_request_decode(&frame, &decoded_request));
  assert(decoded_request.payload.storage_bind_namespace.namespace_handle.id == namespace_handle.id);
  assert(decoded_request.payload.storage_bind_namespace.namespace_handle.kind ==
         namespace_handle.kind);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER,
                                  HUBOS_MICROKIT_DISPLAY_OP_DESCRIBE);
  hubos_microkit_transport_frame_init(&frame);
  assert(hubos_microkit_transport_request_encode(&request, &frame));
  assert(frame.count == hubos_microkit_transport_request_word_count(&request));
  assert(hubos_microkit_transport_request_decode(&frame, &decoded_request));
  assert(decoded_request.service == HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER);
  assert(decoded_request.operation == HUBOS_MICROKIT_DISPLAY_OP_DESCRIBE);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER,
                                  HUBOS_MICROKIT_DEVICE_OP_ATTACH_IRQ);
  request.payload.device_owner.owner_session_id = 88;
  hubos_microkit_transport_frame_init(&frame);
  assert(hubos_microkit_transport_request_encode(&request, &frame));
  assert(frame.count == hubos_microkit_transport_request_word_count(&request));
  assert(hubos_microkit_transport_request_decode(&frame, &decoded_request));
  assert(decoded_request.service == HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER);
  assert(decoded_request.operation == HUBOS_MICROKIT_DEVICE_OP_ATTACH_IRQ);
  assert(decoded_request.payload.device_owner.owner_session_id == 88);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_SELECT_RUNTIME_PROFILE);
  request.payload.vm_select_runtime_profile.runtime_profile_id = "mini-bsd-service";
  request.payload.vm_select_runtime_profile.runtime_profile_id_len =
    strlen("mini-bsd-service");
  hubos_microkit_transport_frame_init(&frame);
  assert(hubos_microkit_transport_request_encode(&request, &frame));
  assert(frame.count == hubos_microkit_transport_request_word_count(&request));
  assert(hubos_microkit_transport_request_decode(&frame, &decoded_request));
  assert(decoded_request.service == HUBOS_MICROKIT_COMPONENT_VM_SERVER);
  assert(decoded_request.operation == HUBOS_MICROKIT_VM_OP_SELECT_RUNTIME_PROFILE);
  assert(decoded_request.payload.vm_select_runtime_profile.runtime_profile_id ==
         request.payload.vm_select_runtime_profile.runtime_profile_id);
  assert(decoded_request.payload.vm_select_runtime_profile.runtime_profile_id_len ==
         strlen("mini-bsd-service"));

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_SET_ARTIFACTS);
  request.payload.vm_set_artifacts.artifacts = (hubos_linux_vm_artifacts_t){
    .kernel_image = "kernel.elf",
    .initramfs_image = "initramfs.cpio",
    .rootfs_image = "rootfs.ext4",
    .device_tree_blob = "guest.dtb",
    .kernel_cmdline = "console=ttyS0",
  };
  hubos_microkit_transport_frame_init(&frame);
  assert(hubos_microkit_transport_request_encode(&request, &frame));
  assert(frame.count == hubos_microkit_transport_request_word_count(&request));
  assert(hubos_microkit_transport_request_decode(&frame, &decoded_request));
  assert(decoded_request.service == HUBOS_MICROKIT_COMPONENT_VM_SERVER);
  assert(decoded_request.operation == HUBOS_MICROKIT_VM_OP_SET_ARTIFACTS);
  assert(decoded_request.payload.vm_set_artifacts.artifacts.kernel_image ==
         request.payload.vm_set_artifacts.artifacts.kernel_image);
  assert(decoded_request.payload.vm_set_artifacts.artifacts.kernel_cmdline ==
         request.payload.vm_set_artifacts.artifacts.kernel_cmdline);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_SET_RESTART_POLICY);
  request.payload.vm_set_restart_policy.policy = HUBOS_VM_RESTART_AUTO;
  request.payload.vm_set_restart_policy.max_restart_attempts = 3;
  hubos_microkit_transport_frame_init(&frame);
  assert(hubos_microkit_transport_request_encode(&request, &frame));
  assert(frame.count == hubos_microkit_transport_request_word_count(&request));
  assert(hubos_microkit_transport_request_decode(&frame, &decoded_request));
  assert(decoded_request.payload.vm_set_restart_policy.policy == HUBOS_VM_RESTART_AUTO);
  assert(decoded_request.payload.vm_set_restart_policy.max_restart_attempts == 3);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_START);
  hubos_microkit_transport_frame_init(&frame);
  assert(hubos_microkit_transport_request_encode(&request, &frame));
  assert(frame.count == hubos_microkit_transport_request_word_count(&request));
  assert(hubos_microkit_transport_request_decode(&frame, &decoded_request));
  assert(decoded_request.operation == HUBOS_MICROKIT_VM_OP_START);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_FAIL);
  request.payload.vm_fail.failure_code = 9;
  hubos_microkit_transport_frame_init(&frame);
  assert(hubos_microkit_transport_request_encode(&request, &frame));
  assert(frame.count == hubos_microkit_transport_request_word_count(&request));
  assert(hubos_microkit_transport_request_decode(&frame, &decoded_request));
  assert(decoded_request.operation == HUBOS_MICROKIT_VM_OP_FAIL);
  assert(decoded_request.payload.vm_fail.failure_code == 9);

  hubos_microkit_ipc_response_init(&response);
  response.status = HUBOS_IPC_STATUS_OK;
  response.resource_id = 11;
  response.capability_id = 12;
  response.session_id = 13;
  response.driver_id = 14;
  response.count = 15;
  response.is_new = true;
  response.bool_result = false;
  response.descriptor.resource_id = 16;
  response.descriptor.name = "descriptor";
  response.descriptor.name_len = strlen("descriptor");
  response.descriptor.resource_state = HUBOS_RESOURCE_READY;
  response.descriptor.endpoint = "endpoint://alpha";
  response.descriptor.version = "v1";
  response.descriptor.policy_hints = 31;

  hubos_microkit_transport_frame_init(&frame);
  assert(hubos_microkit_transport_response_encode(&response, &frame));
  assert(frame.count == hubos_microkit_transport_response_word_count(&response));
  assert(hubos_microkit_transport_response_decode(&frame, &decoded_response));
  assert(decoded_response.status == response.status);
  assert(decoded_response.resource_id == response.resource_id);
  assert(decoded_response.capability_id == response.capability_id);
  assert(decoded_response.session_id == response.session_id);
  assert(decoded_response.driver_id == response.driver_id);
  assert(decoded_response.count == response.count);
  assert(decoded_response.is_new == response.is_new);
  assert(decoded_response.bool_result == response.bool_result);
  assert(decoded_response.descriptor.resource_id == response.descriptor.resource_id);
  assert(decoded_response.descriptor.name == response.descriptor.name);
  assert(decoded_response.descriptor.name_len == response.descriptor.name_len);
  assert(decoded_response.descriptor.resource_state == response.descriptor.resource_state);
  assert(decoded_response.descriptor.endpoint == response.descriptor.endpoint);
  assert(decoded_response.descriptor.version == response.descriptor.version);
  assert(decoded_response.descriptor.policy_hints == response.descriptor.policy_hints);

  hubos_microkit_ipc_response_t synthesized;

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY,
                                  HUBOS_MICROKIT_RESOURCE_OP_REGISTER);
  request.payload.resource_register.name = "resource://pci/0000:01:00.0";
  request.payload.resource_register.name_len = strlen(request.payload.resource_register.name);
  request.payload.resource_register.state = HUBOS_RESOURCE_CLASSIFIED;
  assert(hubos_microkit_transport_synthesize_response(&request, &synthesized));
  assert(synthesized.status == HUBOS_IPC_STATUS_OK);
  assert(synthesized.resource_id == request.payload.resource_register.name_len);
  assert(synthesized.is_new);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER,
                                  HUBOS_MICROKIT_SESSION_OP_CREATE);
  request.payload.session_create.owner_id = 77;
  request.payload.session_create.parent_id = HUBOS_ID_INVALID;
  request.payload.session_create.type = HUBOS_SESSION_EPHEMERAL;
  assert(hubos_microkit_transport_synthesize_response(&request, &synthesized));
  assert(synthesized.session_id == 78);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER,
                                  HUBOS_MICROKIT_NETWORK_OP_BIND_NAMESPACE);
  request.payload.network_bind_namespace.namespace_handle = namespace_handle;
  assert(hubos_microkit_transport_synthesize_response(&request, &synthesized));
  assert(synthesized.resource_id == namespace_handle.id);
  assert(synthesized.descriptor.endpoint == namespace_handle.name);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER,
                                  HUBOS_MICROKIT_STORAGE_OP_BIND_NAMESPACE);
  request.payload.storage_bind_namespace.namespace_handle = namespace_handle;
  assert(hubos_microkit_transport_synthesize_response(&request, &synthesized));
  assert(synthesized.resource_id == namespace_handle.id);
  assert(synthesized.descriptor.endpoint == namespace_handle.name);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER,
                                  HUBOS_MICROKIT_DEVICE_OP_DESCRIBE);
  assert(hubos_microkit_transport_synthesize_response(&request, &synthesized));
  assert(synthesized.descriptor.resource_id == HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_BLK);
  request.payload.vm_session.session_id = 300;
  assert(hubos_microkit_transport_synthesize_response(&request, &synthesized));
  assert(synthesized.session_id == 300);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_SELECT_RUNTIME_PROFILE);
  request.payload.vm_select_runtime_profile.runtime_profile_id = "linux-dev";
  request.payload.vm_select_runtime_profile.runtime_profile_id_len = strlen("linux-dev");
  assert(hubos_microkit_transport_synthesize_response(&request, &synthesized));
  assert(synthesized.descriptor.version == request.payload.vm_select_runtime_profile.runtime_profile_id);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_START);
  assert(hubos_microkit_transport_synthesize_response(&request, &synthesized));
  assert(synthesized.resource_id == HUBOS_MICROKIT_COMPONENT_VM_SERVER);
  assert(synthesized.descriptor.resource_state == HUBOS_RESOURCE_CLASSIFIED);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_COMPLETE_BOOT);
  assert(hubos_microkit_transport_synthesize_response(&request, &synthesized));
  assert(synthesized.descriptor.resource_state == HUBOS_RESOURCE_READY);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_VM_SERVER,
                                  HUBOS_MICROKIT_VM_OP_FAIL);
  request.payload.vm_fail.failure_code = 11;
  assert(hubos_microkit_transport_synthesize_response(&request, &synthesized));
  assert(synthesized.descriptor.resource_state == HUBOS_RESOURCE_FAILED);

  hubos_microkit_ipc_request_init(&request,
                                  HUBOS_MICROKIT_COMPONENT_HUB,
                                  HUBOS_MICROKIT_HUB_OP_RESOLVE);
  request.payload.hub_resolve.name = "resource://pci/0000:01:00.0";
  request.payload.hub_resolve.name_len = strlen(request.payload.hub_resolve.name);
  assert(hubos_microkit_transport_synthesize_response(&request, &synthesized));
  assert(synthesized.resource_id == request.payload.hub_resolve.name_len);
  assert(synthesized.descriptor.version != NULL);
}

static void test_microkit_transport_msginfo_bridge(void) {
  hubos_microkit_transport_frame_t frame;
  microkit_msginfo msginfo;

  microkit_mr_set(0, 101);
  microkit_mr_set(1, 202);
  microkit_mr_set(2, 303);
  microkit_mr_set(3, 404);

  msginfo = microkit_msginfo_new(7, 4);
  assert(hubos_microkit_transport_frame_from_msginfo(&frame, msginfo));
  assert(frame.count == 4);
  assert(frame.words[0] == 101);
  assert(frame.words[1] == 202);
  assert(frame.words[2] == 303);
  assert(frame.words[3] == 404);

  frame.words[0] = 11;
  frame.words[1] = 22;
  frame.words[2] = 33;
  frame.words[3] = 44;
  frame.count = 4;
  hubos_microkit_transport_frame_to_mrs(&frame);
  assert(microkit_mr_get(0) == 11);
  assert(microkit_mr_get(1) == 22);
  assert(microkit_mr_get(2) == 33);
  assert(microkit_mr_get(3) == 44);

  msginfo = hubos_microkit_transport_frame_to_msginfo(&frame, 99);
  assert(microkit_msginfo_get_label(msginfo) == 99);
  assert(microkit_msginfo_get_count(msginfo) == 4);
}

static void test_app_container_vm_models(void) {
  hubos_system_t control_system;
  hubos_resource_envelope_t envelope;
  hubos_app_t app;
  hubos_namespace_handle_t network_namespace;
  hubos_namespace_handle_t storage_namespace;
  hubos_namespace_handle_t display_namespace;
  hubos_container_t container;
  hubos_vm_t vm;
  hubos_vm_server_t vm_server;
  hubos_linux_vm_artifacts_t linux_artifacts;
  hubos_linux_vm_layout_t linux_layout;
  hubos_device_server_t server;
  hubos_storage_server_t storage_server;
  hubos_display_server_t display_server;
  hubos_device_server_endpoint_t device_endpoint;
  hubos_storage_server_endpoint_t storage_endpoint;
  hubos_display_server_endpoint_t display_endpoint;
  hubos_service_descriptor_t device_descriptor;
  hubos_service_descriptor_t storage_descriptor;
  hubos_service_descriptor_t display_descriptor;
  const char *installed_profiles[] = { "linux-dev", "mini-bsd-service" };
  hubos_app_vm_runtime_assignment_t runtime_assignments[1];
  hubos_app_vm_runtime_selection_t runtime_selection;
  test_device_backend_t device_backend = {0};
  test_bus_backend_t bus_backend = {0};

  hubos_resource_envelope_init(&envelope, 1024, 2, 4, 100, 0, 3, 2);
  hubos_app_init(&app, 1, HUBOS_APP_LARGE, envelope);
  assert(app.kind == HUBOS_APP_LARGE);
  assert(app.envelope.memory == 1024);
  runtime_assignments[0].app_id = app.id;
  runtime_assignments[0].runtime_profile_id = "mini-bsd-service";
  runtime_selection.profiles = test_runtime_profiles;
  runtime_selection.profile_count = sizeof(test_runtime_profiles) / sizeof(test_runtime_profiles[0]);
  runtime_selection.installed_profile_ids = installed_profiles;
  runtime_selection.installed_profile_count = sizeof(installed_profiles) / sizeof(installed_profiles[0]);
  runtime_selection.default_profile_id = "linux-dev";
  runtime_selection.app_assignments = runtime_assignments;
  runtime_selection.app_assignment_count = 1;
  hubos_system_init(&control_system, "root-key");
  assert(control_system.vm_server.runtime_profile == hubos_runtime_config_default_profile());
  assert(strcmp(control_system.vm_server.runtime_profile->id, "linux-dev") == 0);
  assert(control_system.vm_server.vm.vcpu_count == 2);

  hubos_namespace_handle_init(&network_namespace, 10, HUBOS_NAMESPACE_NETWORK, "network", false);
  hubos_namespace_handle_init(&storage_namespace, 11, HUBOS_NAMESPACE_STORAGE, "storage", false);
  hubos_namespace_handle_init(&display_namespace, 12, HUBOS_NAMESPACE_DISPLAY, "display", false);
  assert(network_namespace.lifecycle.refcount == 1);
  assert(storage_namespace.lifecycle.state == HUBOS_SHARED_RESOURCE_ACTIVE);
  assert(display_namespace.lifecycle.state == HUBOS_SHARED_RESOURCE_ACTIVE);
  hubos_container_init(&container,
                        20,
                        21,
                        network_namespace,
                        storage_namespace,
                        display_namespace);
  assert(container.network_namespace.kind == HUBOS_NAMESPACE_NETWORK);

  hubos_vm_init(&vm, 30, 31, 32, 4, 40, 41, 42);
  assert(vm.vcpu_count == 4);
  assert(vm.guest_memory_id == 32);
  hubos_linux_vm_artifacts_init(&linux_artifacts,
                                "build/vm/linux/Image",
                                "build/vm/linux/initramfs.cpio.gz",
                                "build/vm/linux/rootfs.ext4",
                                "build/vm/linux/guest.dtb",
                                "console=ttyS0 earlycon=serial root=/dev/vda rw");
  hubos_linux_vm_layout_init(&linux_layout, "libvmm", vm, linux_artifacts);
  assert(linux_layout.vm.virtio_net_session_id == 40);
  assert(linux_layout.vm.virtio_blk_session_id == 41);
  assert(linux_layout.vm.vgpu_session_id == 42);
  assert(hubos_linux_vm_layout_uses_virtio_net(&linux_layout));
  assert(hubos_linux_vm_layout_uses_virtio_blk(&linux_layout));
  assert(hubos_linux_vm_layout_uses_vgpu(&linux_layout));
  assert(strcmp(linux_layout.artifacts.rootfs_image, "build/vm/linux/rootfs.ext4") == 0);
  assert(hubos_app_vm_runtime_catalog_find(test_runtime_profiles,
                                           sizeof(test_runtime_profiles) / sizeof(test_runtime_profiles[0]),
                                           "linux-dev") == &test_runtime_profiles[0]);
  assert(hubos_app_vm_runtime_profile_validate(&test_runtime_profiles[0]));
  assert(hubos_app_vm_runtime_uses_source_bundle(&test_runtime_profiles[0], "linux-kernel"));
  assert(!hubos_app_vm_runtime_uses_source_bundle(&test_runtime_profiles[0], "freebsd-src"));
  assert(hubos_app_vm_runtime_find_artifact_hash(&test_runtime_profiles[0], "kernel.elf") != NULL);
  assert(strcmp(hubos_app_vm_runtime_find_artifact_hash(&test_runtime_profiles[0], "kernel.elf")->sha256_hex,
                "2602c46fd19da5ac16986db9fd8d90227f310b1e5d04118d03873c5a11ba3d68") == 0);
  {
    size_t runtime_profile_count = 0;
    const hubos_app_vm_runtime_profile_t *runtime_profiles =
      hubos_runtime_config_profiles(&runtime_profile_count);
    assert(runtime_profiles != NULL);
    assert(runtime_profile_count == 3);
    assert(hubos_runtime_config_default_selection() != NULL);
    assert(hubos_runtime_config_default_profile() != NULL);
    assert(strcmp(hubos_runtime_config_default_profile()->id, "linux-dev") == 0);
    assert(hubos_app_vm_runtime_profile_validate(hubos_runtime_config_default_profile()));
    assert(strcmp(hubos_runtime_config_default_profile()->bundle_path,
                  "src/runtime-images/linux-dev/1.0.0") == 0);
    assert(hubos_app_vm_runtime_uses_source_bundle(hubos_runtime_config_default_profile(),
                                                   "buildroot"));
    assert(hubos_app_vm_runtime_find_artifact_hash(hubos_runtime_config_default_profile(),
                                                   "rootfs.img") != NULL);
  }
  assert(hubos_app_vm_runtime_selection_is_installed(&runtime_selection, "linux-dev"));
  assert(hubos_app_vm_runtime_selection_profile_id_for_app(&runtime_selection, app.id) ==
         runtime_assignments[0].runtime_profile_id);
  assert(hubos_app_vm_runtime_selection_resolve(&runtime_selection, app.id) == &test_runtime_profiles[1]);
  hubos_vm_server_init(&vm_server, "libvmm", 30, 31, vm, linux_artifacts);
  assert(hubos_vm_server_select_runtime_profile(&vm_server, &test_runtime_profiles[0]));
  assert(vm_server.runtime_profile == &test_runtime_profiles[0]);
  assert(vm_server.default_memory_mb == 1024);
  assert(vm_server.vm.vcpu_count == 2);
  assert(vm_server.vm.vgpu_session_id == HUBOS_ID_INVALID);
  assert(hubos_vm_server_matches_runtime_profile(&vm_server));
  assert(hubos_vm_server_uses_virtio_net(&vm_server));
  assert(hubos_vm_server_uses_virtio_blk(&vm_server));
  assert(!hubos_vm_server_uses_vgpu(&vm_server));
  assert(hubos_vm_server_set_vcpu_count(&vm_server, 6));
  assert(vm_server.vm.vcpu_count == 6);
  assert(hubos_vm_server_attach_guest_memory(&vm_server, 99));
  assert(vm_server.vm.guest_memory_id == 99);
  assert(hubos_vm_server_attach_virtio_net_session(&vm_server, 100));
  assert(hubos_vm_server_attach_virtio_blk_session(&vm_server, 101));
  assert(hubos_vm_server_attach_vgpu_session(&vm_server, 102));
  assert(vm_server.vm.virtio_net_session_id == 100);
  assert(vm_server.vm.virtio_blk_session_id == 101);
  assert(vm_server.vm.vgpu_session_id == 102);
  assert(hubos_vm_server_is_configured(&vm_server));
  assert(hubos_vm_server_set_restart_policy(&vm_server, HUBOS_VM_RESTART_AUTO, 2));
  assert(hubos_vm_server_start(&vm_server));
  assert(vm_server.state == HUBOS_VM_BOOTING);
  assert(!hubos_vm_server_start(&vm_server));
  assert(hubos_vm_server_complete_boot(&vm_server));
  assert(vm_server.state == HUBOS_VM_RUNNING);
  assert(!hubos_vm_server_console_relay_available(&vm_server));
  assert(!hubos_vm_server_attach_console(&vm_server));
  hubos_vm_server_set_console_relay(&vm_server, true, "uart16550");
  assert(hubos_vm_server_console_relay_available(&vm_server));
  assert(hubos_vm_server_attach_console(&vm_server));
  assert(vm_server.console_attached);
  assert(hubos_vm_server_console_write(&vm_server, "help", 0));
  assert(vm_server.console_tx_bytes == 4);
  assert(hubos_vm_server_detach_console(&vm_server));
  assert(!vm_server.console_attached);
  assert(hubos_vm_server_describe(&vm_server, &display_descriptor));
  assert(display_descriptor.resource_id == 30);
  assert(strcmp(display_descriptor.name, "libvmm") == 0);
  assert(display_descriptor.version == test_runtime_profiles[0].id);
  assert(display_descriptor.resource_state == HUBOS_RESOURCE_READY);
  assert((display_descriptor.policy_hints & 1024u) != 0);
  assert((display_descriptor.policy_hints & 4u) != 0);
  assert((display_descriptor.policy_hints & 8u) != 0);
  assert((display_descriptor.policy_hints & 16u) != 0);
  assert((display_descriptor.policy_hints & 16384u) != 0);
  assert((display_descriptor.policy_hints & 32768u) == 0);
  assert(hubos_vm_server_stop(&vm_server));
  assert(vm_server.state == HUBOS_VM_STOPPED);
  assert(hubos_vm_server_start(&vm_server));
  {
    bool restarting = false;
    assert(hubos_vm_server_fail(&vm_server, 77, &restarting));
    assert(restarting);
  }
  assert(vm_server.state == HUBOS_VM_BOOTING);
  assert(vm_server.restart_attempts == 1);
  assert(hubos_vm_server_fail(&vm_server, 88, NULL));
  assert(vm_server.state == HUBOS_VM_BOOTING);
  assert(vm_server.restart_attempts == 2);
  assert(hubos_vm_server_fail(&vm_server, 99, NULL));
  assert(vm_server.state == HUBOS_VM_FAILED);
  assert(vm_server.last_failure_code == 99);
  assert(hubos_vm_server_describe(&vm_server, &display_descriptor));
  assert(display_descriptor.resource_state == HUBOS_RESOURCE_FAILED);
  assert((display_descriptor.policy_hints & 2048u) != 0);
  assert((display_descriptor.policy_hints & 8192u) != 0);
  assert(hubos_vm_server_stop(&vm_server));
  assert(vm_server.restart_attempts == 0);
  assert(hubos_system_set_vm_guest_memory(&control_system, 111));
  assert(hubos_system_select_vm_runtime_for_app(&control_system, &runtime_selection, app.id));
  assert(control_system.vm_server.runtime_profile == &test_runtime_profiles[1]);
  assert(control_system.vm_server.vm.vcpu_count == 1);
  assert(control_system.vm_server.vm.virtio_blk_session_id == HUBOS_ID_INVALID);
  assert(hubos_system_set_vm_vcpu_count(&control_system, 2));
  assert(hubos_system_attach_vm_virtio_net(&control_system, 112));
  assert(hubos_system_attach_vm_virtio_blk(&control_system, 113));
  assert(hubos_system_attach_vm_vgpu(&control_system, 114));
  assert(hubos_system_set_vm_artifacts(&control_system, linux_artifacts));
  assert(hubos_system_set_vm_restart_policy(&control_system, HUBOS_VM_RESTART_AUTO, 1));
  assert(hubos_system_start_vm(&control_system));
  assert(hubos_system_describe_vm(&control_system, &display_descriptor));
  assert(display_descriptor.resource_state == HUBOS_RESOURCE_CLASSIFIED);
  assert(hubos_system_complete_vm_boot(&control_system));
  assert(hubos_system_describe_vm(&control_system, &display_descriptor));
  assert(display_descriptor.resource_id == 5);
  assert(display_descriptor.version == test_runtime_profiles[1].id);
  assert(display_descriptor.resource_state == HUBOS_RESOURCE_READY);
  assert(!hubos_system_vm_console_relay_available(&control_system));
  assert(!hubos_system_attach_vm_console(&control_system));
  hubos_system_set_vm_console_relay(&control_system, true, "uart16550");
  assert(hubos_system_vm_console_relay_available(&control_system));
  assert(hubos_system_attach_vm_console(&control_system));
  assert(control_system.vm_server.console_attached);
  assert(hubos_system_write_vm_console(&control_system, "uname", 0));
  assert(control_system.vm_server.console_tx_bytes == 5);
  assert(hubos_system_detach_vm_console(&control_system));
  assert(!control_system.vm_server.console_attached);
  assert(hubos_system_start_vm(&control_system) == false);
  assert(hubos_system_stop_vm(&control_system));
  assert(hubos_system_describe_vm(&control_system, &display_descriptor));
  assert(display_descriptor.resource_state == HUBOS_RESOURCE_BOUND);
  assert(hubos_system_start_vm(&control_system));
  assert(hubos_system_fail_vm(&control_system, 5));
  assert(hubos_system_describe_vm(&control_system, &display_descriptor));
  assert(display_descriptor.resource_state == HUBOS_RESOURCE_CLASSIFIED);
  assert(hubos_system_fail_vm(&control_system, 6));
  assert(hubos_system_describe_vm(&control_system, &display_descriptor));
  assert(display_descriptor.resource_state == HUBOS_RESOURCE_FAILED);
  assert(hubos_system_stop_vm(&control_system));
  assert(audit_find_event_type(&control_system.audit_log,
                               HUBOS_AUDIT_VM_RESTART_POLICY_CHANGED) != NULL);
  assert(audit_find_event_type(&control_system.audit_log, HUBOS_AUDIT_VM_START_REQUESTED) != NULL);
  assert(audit_find_event_type(&control_system.audit_log, HUBOS_AUDIT_VM_BOOT_COMPLETED) != NULL);
  assert(audit_find_event_type(&control_system.audit_log, HUBOS_AUDIT_VM_FAILED) != NULL);
  assert(audit_find_event_type(&control_system.audit_log,
                               HUBOS_AUDIT_VM_RESTART_SCHEDULED) != NULL);
  assert(audit_find_event_type(&control_system.audit_log, HUBOS_AUDIT_VM_STOPPED) != NULL);

  hubos_device_server_init(&server, 50, 51, 52, "nic-server");
  hubos_device_server_set_ops(&server, &test_device_backend_ops, &device_backend);
  assert(server.resource_id == 52);
  hubos_device_server_endpoint_init(&device_endpoint, &server);
  assert(hubos_device_server_endpoint_is_active(&device_endpoint));
  assert(hubos_device_server_endpoint_set_owner(&device_endpoint, 51));
  assert(hubos_device_server_endpoint_is_active(&device_endpoint));
  assert(hubos_device_server_endpoint_attach_mmio(&device_endpoint, 51));
  assert(hubos_device_server_endpoint_attach_irq(&device_endpoint, 51));
  assert(hubos_device_server_endpoint_attach_dma(&device_endpoint, 51));
  assert(server.mmio_owner_session_id == 51);
  assert(server.irq_owner_session_id == 51);
  assert(server.dma_owner_session_id == 51);
  assert(server.mmio_attached);
  assert(server.irq_attached);
  assert(server.dma_attached);
  assert(!hubos_device_server_endpoint_attach_mmio(&device_endpoint, 52));
  assert(!hubos_device_server_endpoint_attach_irq(&device_endpoint, 52));
  assert(!hubos_device_server_endpoint_attach_dma(&device_endpoint, 52));
  assert(hubos_device_server_endpoint_quarantine(&device_endpoint));
  assert(server.quarantined);
  assert(!server.running);
  assert(hubos_device_server_endpoint_clear_quarantine(&device_endpoint));
  assert(!server.quarantined);
  assert(server.running);
  assert(hubos_device_server_endpoint_reset(&device_endpoint));
  assert(server.mmio_owner_session_id == 51);
  assert(server.irq_owner_session_id == 51);
  assert(server.dma_owner_session_id == 51);
  assert(server.mmio_attached);
  assert(server.irq_attached);
  assert(server.dma_attached);
  assert(hubos_device_server_endpoint_release_owner(&device_endpoint));
  assert(!hubos_device_server_endpoint_is_active(&device_endpoint));
  assert(server.owner_session_id == HUBOS_ID_INVALID);
  assert(!server.mmio_attached);
  assert(!server.irq_attached);
  assert(!server.dma_attached);
  assert(!server.running);
  assert(!hubos_device_server_endpoint_quarantine(&device_endpoint));
  assert(!hubos_device_server_endpoint_clear_quarantine(&device_endpoint));
  assert(!hubos_device_server_endpoint_reset(&device_endpoint));
  assert(!hubos_device_server_endpoint_release_owner(&device_endpoint));
  assert(device_backend.set_owner_calls == 1);
  assert(device_backend.release_owner_calls == 1);
  assert(device_backend.quarantine_calls == 1);
  assert(device_backend.clear_quarantine_calls == 1);
  assert(device_backend.reset_calls == 1);
  assert(device_backend.attach_mmio_calls == 1);
  assert(device_backend.attach_irq_calls == 1);
  assert(device_backend.attach_dma_calls == 1);

  device_backend.fail_reset = true;
  hubos_device_server_init(&server, 50, 51, 52, "nic-server");
  hubos_device_server_set_ops(&server, &test_device_backend_ops, &device_backend);
  assert(!hubos_device_server_reset(&server));
  assert(server.running);
  device_backend.fail_reset = false;

  hubos_storage_server_init(&storage_server, 60, 61, storage_namespace);
  hubos_storage_server_endpoint_init(&storage_endpoint, &storage_server);
  assert(hubos_storage_server_endpoint_bind_namespace(&storage_endpoint, storage_namespace));
  assert(hubos_storage_server_bind_namespace(&storage_server, storage_namespace));
  assert(storage_server.namespace_handle.lifecycle.owner_session_id == 61);
  assert(storage_server.namespace_handle.owned_by_server);
  assert(hubos_storage_server_endpoint_describe(&storage_endpoint, &storage_descriptor));
  assert(storage_descriptor.resource_state == HUBOS_RESOURCE_READY);
  assert(storage_descriptor.resource_id == storage_namespace.id);
  assert(hubos_storage_server_endpoint_release_namespace(&storage_endpoint));
  assert(storage_server.namespace_handle.lifecycle.state ==
         HUBOS_SHARED_RESOURCE_PENDING_FINALIZATION);
  assert(hubos_storage_server_endpoint_describe(&storage_endpoint, &storage_descriptor));
  assert(storage_descriptor.resource_state == HUBOS_RESOURCE_QUARANTINED);
  assert(hubos_storage_server_endpoint_finalize_namespace(&storage_endpoint));
  assert(storage_server.namespace_handle.lifecycle.state == HUBOS_SHARED_RESOURCE_RETIRED);
  assert(hubos_storage_server_endpoint_describe(&storage_endpoint, &storage_descriptor));
  assert(storage_descriptor.resource_state == HUBOS_RESOURCE_RETIRED);

  hubos_display_server_init(&display_server, 70, 71, display_namespace);
  hubos_display_server_endpoint_init(&display_endpoint, &display_server);
  assert(hubos_display_server_endpoint_bind_namespace(&display_endpoint, display_namespace));
  assert(hubos_display_server_bind_namespace(&display_server, display_namespace));
  assert(display_server.namespace_handle.lifecycle.owner_session_id == 71);
  assert(display_server.namespace_handle.owned_by_server);
  assert(hubos_display_server_endpoint_describe(&display_endpoint, &display_descriptor));
  assert(display_descriptor.resource_state == HUBOS_RESOURCE_READY);
  assert(display_descriptor.resource_id == display_namespace.id);
  assert(hubos_display_server_endpoint_release_namespace(&display_endpoint));
  assert(display_server.namespace_handle.lifecycle.state ==
         HUBOS_SHARED_RESOURCE_PENDING_FINALIZATION);
  assert(hubos_display_server_endpoint_describe(&display_endpoint, &display_descriptor));
  assert(display_descriptor.resource_state == HUBOS_RESOURCE_QUARANTINED);
  assert(hubos_display_server_endpoint_finalize_namespace(&display_endpoint));
  assert(display_server.namespace_handle.lifecycle.state == HUBOS_SHARED_RESOURCE_RETIRED);
  assert(hubos_display_server_endpoint_describe(&display_endpoint, &display_descriptor));
  assert(display_descriptor.resource_state == HUBOS_RESOURCE_RETIRED);

  assert(hubos_system_bind_storage_namespace(&control_system, storage_namespace));
  assert(hubos_system_describe_storage_server(&control_system, &storage_descriptor));
  assert(storage_descriptor.resource_id == storage_namespace.id);
  assert(hubos_system_release_storage_namespace(&control_system));
  assert(hubos_system_finalize_storage_namespace(&control_system));

  assert(hubos_system_bind_display_namespace(&control_system, display_namespace));
  assert(hubos_system_describe_display_server(&control_system, &display_descriptor));
  assert(display_descriptor.resource_id == display_namespace.id);
  assert(hubos_system_release_display_namespace(&control_system));
  assert(hubos_system_finalize_display_namespace(&control_system));

  hubos_system_set_device_hardware_backend(&control_system, &test_device_backend_ops, &device_backend);
  hubos_system_set_bus_hardware_backend(&control_system, HUBOS_BUS_I2C, &test_bus_backend_ops, &bus_backend);
  assert(hubos_system_set_device_owner(&control_system, 61));
  assert(hubos_system_bus_discover(&control_system,
                                   HUBOS_BUS_I2C,
                                   "resource://i2c/ov5640",
                                   strlen("resource://i2c/ov5640"),
                                   HUBOS_RESOURCE_DISCOVERED,
                                   NULL));
  assert(bus_backend.discover_calls == 1);
  assert(hubos_system_attach_device_mmio(&control_system, 61));
  assert(hubos_system_attach_device_irq(&control_system, 61));
  assert(hubos_system_attach_device_dma(&control_system, 61));
  assert(hubos_system_describe_device(&control_system, &device_descriptor));
  assert(device_descriptor.resource_state == HUBOS_RESOURCE_READY);
  assert((device_descriptor.policy_hints & 0x7u) == 0x7u);
  assert(hubos_system_quarantine_device(&control_system));
  assert(hubos_system_describe_device(&control_system, &device_descriptor));
  assert(device_descriptor.resource_state == HUBOS_RESOURCE_QUARANTINED);
  assert(hubos_system_clear_device_quarantine(&control_system));
  assert(hubos_system_describe_device(&control_system, &device_descriptor));
  assert(device_descriptor.resource_state == HUBOS_RESOURCE_READY);
  assert(hubos_system_reset_device(&control_system));
  assert(hubos_system_release_device_owner(&control_system));
  assert(device_backend.set_owner_calls == 2);
  assert(device_backend.release_owner_calls == 2);
  assert(device_backend.quarantine_calls == 2);
  assert(device_backend.clear_quarantine_calls == 2);
  assert(device_backend.reset_calls == 3);
  assert(device_backend.attach_mmio_calls == 2);
  assert(device_backend.attach_irq_calls == 2);
  assert(device_backend.attach_dma_calls == 2);

  hubos_system_destroy(&control_system);
}

int main(void) {
  test_resource_model();
  test_resource_registry();
  test_session_model();
  test_session_capability_cascade();
  test_capability_model();
  test_dma_model();
  test_sha256_helper();
  test_boot_capability_set();
  test_audit_log();
  test_boot_sequence();
  test_memory_manager();
  test_shared_resource_model();
  test_bus_manager();
  test_dma_manager();
  test_driver_registry_loader();
  test_driver_service();
  test_network_server();
  test_hub_model();
  test_system_model();
  test_root_task_ipc();
  test_microkit_entrypoints();
  test_microkit_kernel_glue_entrypoints();
  test_microkit_endpoint_badge_coverage();
  test_microkit_service_dispatch_routes();
  test_microkit_transport_codec();
  test_microkit_transport_msginfo_bridge();
  test_linux_usbio_backend();
  test_app_container_vm_models();
  return 0;
}
