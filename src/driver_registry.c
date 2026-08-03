#include "hubos/driver_registry.h"

#include <stdlib.h>
#include <string.h>

static char *hubos_driver_registry_strdup(const char *text) {
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

static void hubos_driver_record_clear(hubos_driver_record_t *record) {
  if (record == NULL) {
    return;
  }

  free(record->driver_package);
  free(record->version);
  record->driver_package = NULL;
  record->version = NULL;
}

static hubos_driver_record_t *hubos_driver_registry_find_mutable(hubos_driver_registry_t *registry,
                                                                 unsigned vendor_id,
                                                                 unsigned device_id,
                                                                 unsigned class_code) {
  if (registry == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < registry->count; ++index) {
    hubos_driver_record_t *record = &registry->items[index];
    if (record->vendor_id == vendor_id && record->device_id == device_id &&
        record->class_code == class_code) {
      return record;
    }
  }

  return NULL;
}

static const hubos_driver_record_t *hubos_driver_registry_find_record(const hubos_driver_registry_t *registry,
                                                                      unsigned vendor_id,
                                                                      unsigned device_id,
                                                                      unsigned class_code) {
  if (registry == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < registry->count; ++index) {
    const hubos_driver_record_t *record = &registry->items[index];
    if (record->vendor_id == vendor_id && record->device_id == device_id &&
        record->class_code == class_code) {
      return record;
    }
  }

  return NULL;
}

static bool hubos_driver_registry_reserve(hubos_driver_registry_t *registry, size_t desired_capacity) {
  if (registry == NULL) {
    return false;
  }

  if (registry->capacity >= desired_capacity) {
    return true;
  }

  size_t new_capacity = registry->capacity == 0 ? 4 : registry->capacity;
  while (new_capacity < desired_capacity) {
    new_capacity *= 2;
  }

  void *new_items = realloc(registry->items, new_capacity * sizeof(*registry->items));
  if (new_items == NULL) {
    return false;
  }

  registry->items = new_items;
  registry->capacity = new_capacity;
  return true;
}

void hubos_driver_registry_init(hubos_driver_registry_t *registry) {
  if (registry == NULL) {
    return;
  }

  registry->items = NULL;
  registry->count = 0;
  registry->capacity = 0;
  registry->next_id = 1;
}

void hubos_driver_registry_destroy(hubos_driver_registry_t *registry) {
  if (registry == NULL) {
    return;
  }

  for (size_t index = 0; index < registry->count; ++index) {
    hubos_driver_record_clear(&registry->items[index]);
  }

  free(registry->items);
  registry->items = NULL;
  registry->count = 0;
  registry->capacity = 0;
  registry->next_id = 1;
}

bool hubos_driver_registry_register(hubos_driver_registry_t *registry,
                                    unsigned vendor_id,
                                    unsigned device_id,
                                    unsigned class_code,
                                    const char *driver_package,
                                    const char *version,
                                    hubos_id_t *out_driver_id,
                                    bool *out_is_new) {
  hubos_driver_record_t *record = NULL;

  if (registry == NULL || driver_package == NULL || version == NULL) {
    return false;
  }

  record = hubos_driver_registry_find_mutable(registry, vendor_id, device_id, class_code);
  if (record != NULL) {
    char *package_copy = hubos_driver_registry_strdup(driver_package);
    char *version_copy = hubos_driver_registry_strdup(version);

    if (package_copy == NULL || version_copy == NULL) {
      free(package_copy);
      free(version_copy);
      return false;
    }

    hubos_driver_record_clear(record);
    record->driver_package = package_copy;
    record->version = version_copy;

    if (out_driver_id != NULL) {
      *out_driver_id = record->id;
    }
    if (out_is_new != NULL) {
      *out_is_new = false;
    }
    return true;
  }

  if (!hubos_driver_registry_reserve(registry, registry->count + 1)) {
    return false;
  }

  record = &registry->items[registry->count];
  record->id = registry->next_id;
  record->vendor_id = vendor_id;
  record->device_id = device_id;
  record->class_code = class_code;
  record->driver_package = NULL;
  record->version = NULL;
  record->driver_package = hubos_driver_registry_strdup(driver_package);
  record->version = hubos_driver_registry_strdup(version);
  if (record->driver_package == NULL || record->version == NULL) {
    hubos_driver_record_clear(record);
    return false;
  }

  registry->count++;
  registry->next_id++;

  if (out_driver_id != NULL) {
    *out_driver_id = record->id;
  }
  if (out_is_new != NULL) {
    *out_is_new = true;
  }
  return true;
}

const hubos_driver_record_t *hubos_driver_registry_find(const hubos_driver_registry_t *registry,
                                                        unsigned vendor_id,
                                                        unsigned device_id,
                                                        unsigned class_code) {
  return hubos_driver_registry_find_record(registry, vendor_id, device_id, class_code);
}

const hubos_driver_record_t *hubos_driver_registry_get(const hubos_driver_registry_t *registry,
                                                       hubos_id_t driver_id) {
  if (registry == NULL || driver_id == HUBOS_ID_INVALID) {
    return NULL;
  }

  for (size_t index = 0; index < registry->count; ++index) {
    if (registry->items[index].id == driver_id) {
      return &registry->items[index];
    }
  }

  return NULL;
}

size_t hubos_driver_registry_count(const hubos_driver_registry_t *registry) {
  return registry != NULL ? registry->count : 0;
}
