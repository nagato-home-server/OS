#define _POSIX_C_SOURCE 200809L

#include "hubos/linux_usbio_backend.h"

#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define HUBOS_USBIO_CMD_ENV "HUBOS_USBIO_SYSFS_DIR"
#define HUBOS_USBIO_I2C_ENV "HUBOS_I2C_SYSFS_ROOT"
#define HUBOS_USBIO_SPI_ENV "HUBOS_SPI_SYSFS_ROOT"
#define HUBOS_USBIO_IRQ_ENV "HUBOS_GPIO_USBIO_SYSFS_ROOT"
#define HUBOS_USBIO_DMA_ENV "HUBOS_VIDEO_DEVICE_ROOT"
#define HUBOS_USBIO_ENABLE_ENV "HUBOS_ENABLE_LINUX_USBIO_BACKEND"

typedef struct {
  char usbio_sysfs_dir[PATH_MAX];
  char i2c_sysfs_root[PATH_MAX];
  char spi_sysfs_root[PATH_MAX];
  char irq_sysfs_root[PATH_MAX];
  char dma_root[PATH_MAX];
} hubos_linux_usbio_backend_t;

static bool hubos_linux_path_exists(const char *path) {
  struct stat st;
  return path != NULL && stat(path, &st) == 0;
}

static bool hubos_linux_path_is_dir(const char *path) {
  struct stat st;
  return path != NULL && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool hubos_linux_copy_path(char *dst, size_t dst_size, const char *src) {
  int written;

  if (dst == NULL || dst_size == 0 || src == NULL || src[0] == '\0') {
    return false;
  }

  written = snprintf(dst, dst_size, "%s", src);
  return written >= 0 && (size_t)written < dst_size;
}

static bool hubos_linux_join_path(char *dst, size_t dst_size, const char *left, const char *right) {
  int written;

  if (dst == NULL || dst_size == 0 || left == NULL || right == NULL) {
    return false;
  }

  written = snprintf(dst, dst_size, "%s/%s", left, right);
  return written >= 0 && (size_t)written < dst_size;
}

static bool hubos_linux_file_contains_line(const char *path, const char *expected) {
  FILE *file;
  char buffer[256];
  size_t length;

  if (path == NULL || expected == NULL) {
    return false;
  }

  file = fopen(path, "r");
  if (file == NULL) {
    return false;
  }
  if (fgets(buffer, sizeof(buffer), file) == NULL) {
    fclose(file);
    return false;
  }
  fclose(file);

  length = strcspn(buffer, "\r\n");
  buffer[length] = '\0';
  return strcmp(buffer, expected) == 0;
}

static bool hubos_linux_write_line(const char *path, const char *value) {
  FILE *file;

  if (path == NULL || value == NULL) {
    return false;
  }

  file = fopen(path, "w");
  if (file == NULL) {
    return false;
  }
  if (fputs(value, file) == EOF) {
    fclose(file);
    return false;
  }
  return fclose(file) == 0;
}

static bool hubos_linux_find_usbio_dir(char *out_path, size_t out_path_size) {
  DIR *dir;
  struct dirent *entry;
  const char *configured = getenv(HUBOS_USBIO_CMD_ENV);
  static const char *const default_root = "/sys/bus/usb/drivers/usbio";
  char candidate[PATH_MAX];
  char cmd_path[PATH_MAX];

  if (configured != NULL && configured[0] != '\0') {
    return hubos_linux_copy_path(out_path, out_path_size, configured);
  }

  dir = opendir(default_root);
  if (dir == NULL) {
    return false;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.') {
      continue;
    }
    if (!hubos_linux_join_path(candidate, sizeof(candidate), default_root, entry->d_name) ||
        !hubos_linux_path_is_dir(candidate) ||
        !hubos_linux_join_path(cmd_path, sizeof(cmd_path), candidate, "cmd")) {
      continue;
    }
    if (hubos_linux_path_exists(cmd_path)) {
      closedir(dir);
      return hubos_linux_copy_path(out_path, out_path_size, candidate);
    }
  }

  closedir(dir);
  return false;
}

static bool hubos_linux_find_named_entry(const char *root, const char *needle) {
  DIR *dir;
  struct dirent *entry;
  char candidate[PATH_MAX];
  char name_path[PATH_MAX];

  if (root == NULL || needle == NULL || needle[0] == '\0') {
    return false;
  }

  dir = opendir(root);
  if (dir == NULL) {
    return false;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.') {
      continue;
    }
    if (strcmp(entry->d_name, needle) == 0) {
      closedir(dir);
      return true;
    }
    if (!hubos_linux_join_path(candidate, sizeof(candidate), root, entry->d_name) ||
        !hubos_linux_path_is_dir(candidate) ||
        !hubos_linux_join_path(name_path, sizeof(name_path), candidate, "name")) {
      continue;
    }
    if (hubos_linux_file_contains_line(name_path, needle)) {
      closedir(dir);
      return true;
    }
  }

  closedir(dir);
  return false;
}

static bool hubos_linux_has_any_entry(const char *root) {
  DIR *dir;
  struct dirent *entry;

  if (root == NULL) {
    return false;
  }

  dir = opendir(root);
  if (dir == NULL) {
    return false;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] != '.') {
      closedir(dir);
      return true;
    }
  }

  closedir(dir);
  return false;
}

static const char *hubos_linux_resource_leaf(const char *resource_name, const char *prefix) {
  size_t prefix_len;

  if (resource_name == NULL || prefix == NULL) {
    return NULL;
  }

  prefix_len = strlen(prefix);
  if (strncmp(resource_name, prefix, prefix_len) != 0) {
    return NULL;
  }

  return resource_name + prefix_len;
}

bool hubos_linux_usbio_backend_is_requested(void) {
  const char *enabled = getenv(HUBOS_USBIO_ENABLE_ENV);

  if (enabled != NULL && enabled[0] != '\0' && strcmp(enabled, "0") != 0) {
    return true;
  }

  return getenv(HUBOS_USBIO_CMD_ENV) != NULL ||
         getenv(HUBOS_USBIO_I2C_ENV) != NULL ||
         getenv(HUBOS_USBIO_SPI_ENV) != NULL;
}

static bool hubos_linux_device_set_owner(void *context,
                                         hubos_device_server_t *server,
                                         hubos_id_t owner_session_id) {
  hubos_linux_usbio_backend_t *backend = context;
  char cmd_path[PATH_MAX];

  (void)server;
  (void)owner_session_id;

  if (backend == NULL || backend->usbio_sysfs_dir[0] == '\0') {
    return true;
  }

  return hubos_linux_join_path(cmd_path, sizeof(cmd_path), backend->usbio_sysfs_dir, "cmd") &&
         hubos_linux_path_exists(cmd_path);
}

static bool hubos_linux_device_release_owner(void *context, hubos_device_server_t *server) {
  (void)context;
  (void)server;
  return true;
}

static bool hubos_linux_device_quarantine(void *context, hubos_device_server_t *server) {
  (void)context;
  (void)server;
  return true;
}

static bool hubos_linux_device_clear_quarantine(void *context, hubos_device_server_t *server) {
  hubos_linux_usbio_backend_t *backend = context;
  char version_path[PATH_MAX];

  (void)server;

  if (backend == NULL || backend->usbio_sysfs_dir[0] == '\0') {
    return true;
  }

  return hubos_linux_join_path(version_path, sizeof(version_path), backend->usbio_sysfs_dir, "version") &&
         hubos_linux_path_exists(version_path);
}

static bool hubos_linux_device_reset(void *context, hubos_device_server_t *server) {
  hubos_linux_usbio_backend_t *backend = context;
  char cmd_path[PATH_MAX];

  (void)server;

  if (backend == NULL || backend->usbio_sysfs_dir[0] == '\0') {
    return false;
  }

  return hubos_linux_join_path(cmd_path, sizeof(cmd_path), backend->usbio_sysfs_dir, "cmd") &&
         hubos_linux_write_line(cmd_path, "reset\n");
}

static bool hubos_linux_device_attach_mmio(void *context,
                                           hubos_device_server_t *server,
                                           hubos_id_t owner_session_id) {
  return hubos_linux_device_set_owner(context, server, owner_session_id);
}

static bool hubos_linux_device_attach_irq(void *context,
                                          hubos_device_server_t *server,
                                          hubos_id_t owner_session_id) {
  hubos_linux_usbio_backend_t *backend = context;

  (void)server;
  (void)owner_session_id;

  return backend != NULL && backend->irq_sysfs_root[0] != '\0' &&
         hubos_linux_path_exists(backend->irq_sysfs_root);
}

static bool hubos_linux_device_attach_dma(void *context,
                                          hubos_device_server_t *server,
                                          hubos_id_t owner_session_id) {
  hubos_linux_usbio_backend_t *backend = context;

  (void)server;
  (void)owner_session_id;

  return backend != NULL && backend->dma_root[0] != '\0' &&
         hubos_linux_has_any_entry(backend->dma_root);
}

static bool hubos_linux_bus_discover(void *context,
                                     hubos_bus_manager_t *manager,
                                     const char *resource_name,
                                     size_t resource_name_len,
                                     hubos_resource_state_t state) {
  hubos_linux_usbio_backend_t *backend = context;
  const char *leaf = NULL;
  const char *root = NULL;

  (void)resource_name_len;
  (void)state;

  if (backend == NULL || manager == NULL || resource_name == NULL) {
    return false;
  }

  switch (manager->kind) {
  case HUBOS_BUS_I2C:
    leaf = hubos_linux_resource_leaf(resource_name, "resource://i2c/");
    root = backend->i2c_sysfs_root;
    break;
  case HUBOS_BUS_SPI:
    leaf = hubos_linux_resource_leaf(resource_name, "resource://spi/");
    root = backend->spi_sysfs_root;
    break;
  case HUBOS_BUS_USB:
    leaf = hubos_linux_resource_leaf(resource_name, "resource://usb/");
    root = backend->usbio_sysfs_dir;
    break;
  case HUBOS_BUS_PCIE:
    return true;
  }

  if (root == NULL || root[0] == '\0' || leaf == NULL) {
    return false;
  }

  if (leaf[0] == '\0' || strcmp(leaf, "*") == 0) {
    return hubos_linux_has_any_entry(root);
  }

  return hubos_linux_find_named_entry(root, leaf);
}

static const hubos_device_server_ops_t hubos_linux_device_ops = {
  .set_owner = hubos_linux_device_set_owner,
  .release_owner = hubos_linux_device_release_owner,
  .quarantine = hubos_linux_device_quarantine,
  .clear_quarantine = hubos_linux_device_clear_quarantine,
  .reset = hubos_linux_device_reset,
  .attach_mmio = hubos_linux_device_attach_mmio,
  .attach_irq = hubos_linux_device_attach_irq,
  .attach_dma = hubos_linux_device_attach_dma,
};

static const hubos_bus_manager_ops_t hubos_linux_bus_ops = {
  .discover = hubos_linux_bus_discover,
};

bool hubos_system_enable_linux_usbio_backend(hubos_system_t *system) {
  hubos_linux_usbio_backend_t *backend;
  const char *configured;

  if (system == NULL) {
    return false;
  }

  backend = calloc(1, sizeof(*backend));
  if (backend == NULL) {
    return false;
  }

  configured = getenv(HUBOS_USBIO_I2C_ENV);
  if (!hubos_linux_copy_path(backend->i2c_sysfs_root,
                             sizeof(backend->i2c_sysfs_root),
                             configured != NULL ? configured : "/sys/bus/i2c/devices")) {
    free(backend);
    return false;
  }

  configured = getenv(HUBOS_USBIO_SPI_ENV);
  if (!hubos_linux_copy_path(backend->spi_sysfs_root,
                             sizeof(backend->spi_sysfs_root),
                             configured != NULL ? configured : "/sys/bus/spi/devices")) {
    free(backend);
    return false;
  }

  configured = getenv(HUBOS_USBIO_IRQ_ENV);
  if (!hubos_linux_copy_path(backend->irq_sysfs_root,
                             sizeof(backend->irq_sysfs_root),
                             configured != NULL ? configured : "/sys/bus/platform/drivers/gpio-usbio")) {
    free(backend);
    return false;
  }

  configured = getenv(HUBOS_USBIO_DMA_ENV);
  if (!hubos_linux_copy_path(backend->dma_root,
                             sizeof(backend->dma_root),
                             configured != NULL ? configured : "/dev")) {
    free(backend);
    return false;
  }

  (void)hubos_linux_find_usbio_dir(backend->usbio_sysfs_dir, sizeof(backend->usbio_sysfs_dir));

  hubos_system_set_device_hardware_backend(system, &hubos_linux_device_ops, backend);
  hubos_system_set_bus_hardware_backend(system, HUBOS_BUS_I2C, &hubos_linux_bus_ops, backend);
  hubos_system_set_bus_hardware_backend(system, HUBOS_BUS_SPI, &hubos_linux_bus_ops, backend);
  hubos_system_set_bus_hardware_backend(system, HUBOS_BUS_USB, &hubos_linux_bus_ops, backend);
  return true;
}
