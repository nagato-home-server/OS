#ifndef HUBOS_HUB_H
#define HUBOS_HUB_H

#include "hubos/capability_manager.h"
#include "hubos/resource_registry.h"

typedef struct {
  const hubos_resource_registry_t *resource_registry;
  const hubos_capability_manager_t *capability_manager;
} hubos_hub_t;

typedef struct {
  hubos_id_t resource_id;
  const char *name;
  size_t name_len;
  hubos_resource_state_t resource_state;
  const char *endpoint;
  const char *version;
  unsigned policy_hints;
} hubos_service_descriptor_t;

void hubos_hub_init(hubos_hub_t *hub,
                    const hubos_resource_registry_t *resource_registry,
                    const hubos_capability_manager_t *capability_manager);

bool hubos_hub_resolve(const hubos_hub_t *hub,
                       const char *name,
                       size_t name_len,
                       hubos_service_descriptor_t *out_descriptor);

bool hubos_hub_authorize(const hubos_hub_t *hub,
                         hubos_id_t capability_id,
                         hubos_id_t resource_id,
                         unsigned required_rights);

#endif
