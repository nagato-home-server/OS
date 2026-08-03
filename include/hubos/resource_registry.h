#ifndef HUBOS_RESOURCE_REGISTRY_H
#define HUBOS_RESOURCE_REGISTRY_H

#include "hubos/model.h"

typedef struct {
  hubos_resource_t *items;
  size_t count;
  size_t capacity;
  hubos_id_t next_id;
} hubos_resource_registry_t;

void hubos_resource_registry_init(hubos_resource_registry_t *registry);
void hubos_resource_registry_destroy(hubos_resource_registry_t *registry);

bool hubos_resource_registry_discover(hubos_resource_registry_t *registry,
                                      const char *name,
                                      size_t name_len,
                                      hubos_resource_state_t state,
                                      hubos_id_t *out_resource_id,
                                      bool *out_is_new);

bool hubos_resource_registry_register(hubos_resource_registry_t *registry,
                                      const char *name,
                                      size_t name_len,
                                      hubos_resource_state_t state,
                                      hubos_id_t *out_resource_id,
                                      bool *out_is_new);

const hubos_resource_t *hubos_resource_registry_get(const hubos_resource_registry_t *registry,
                                                    hubos_id_t resource_id);

const hubos_resource_t *hubos_resource_registry_find(const hubos_resource_registry_t *registry,
                                                     const char *name,
                                                     size_t name_len);

bool hubos_resource_registry_update_state(hubos_resource_registry_t *registry,
                                          hubos_id_t resource_id,
                                          hubos_resource_state_t state);

bool hubos_resource_registry_quarantine(hubos_resource_registry_t *registry,
                                        hubos_id_t resource_id);

bool hubos_resource_registry_retire(hubos_resource_registry_t *registry,
                                    hubos_id_t resource_id);

size_t hubos_resource_registry_count(const hubos_resource_registry_t *registry);

#endif
