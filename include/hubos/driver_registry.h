#ifndef HUBOS_DRIVER_REGISTRY_H
#define HUBOS_DRIVER_REGISTRY_H

#include "hubos/model.h"

typedef struct {
  hubos_id_t id;
  unsigned vendor_id;
  unsigned device_id;
  unsigned class_code;
  char *driver_package;
  char *version;
} hubos_driver_record_t;

typedef struct {
  hubos_driver_record_t *items;
  size_t count;
  size_t capacity;
  hubos_id_t next_id;
} hubos_driver_registry_t;

void hubos_driver_registry_init(hubos_driver_registry_t *registry);
void hubos_driver_registry_destroy(hubos_driver_registry_t *registry);

bool hubos_driver_registry_register(hubos_driver_registry_t *registry,
                                    unsigned vendor_id,
                                    unsigned device_id,
                                    unsigned class_code,
                                    const char *driver_package,
                                    const char *version,
                                    hubos_id_t *out_driver_id,
                                    bool *out_is_new);

const hubos_driver_record_t *hubos_driver_registry_find(const hubos_driver_registry_t *registry,
                                                        unsigned vendor_id,
                                                        unsigned device_id,
                                                        unsigned class_code);

const hubos_driver_record_t *hubos_driver_registry_get(const hubos_driver_registry_t *registry,
                                                       hubos_id_t driver_id);
size_t hubos_driver_registry_count(const hubos_driver_registry_t *registry);

#endif
