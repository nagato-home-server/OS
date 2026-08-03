#include "hubos/hub.h"

void hubos_hub_init(hubos_hub_t *hub,
                    const hubos_resource_registry_t *resource_registry,
                    const hubos_capability_manager_t *capability_manager) {
  if (hub == NULL) {
    return;
  }

  hub->resource_registry = resource_registry;
  hub->capability_manager = capability_manager;
}

bool hubos_hub_resolve(const hubos_hub_t *hub,
                       const char *name,
                       size_t name_len,
                       hubos_service_descriptor_t *out_descriptor) {
  const hubos_resource_t *resource = NULL;

  if (hub == NULL || out_descriptor == NULL) {
    return false;
  }

  resource = hubos_resource_registry_find(hub->resource_registry, name, name_len);
  if (resource == NULL) {
    return false;
  }

  out_descriptor->resource_id = resource->id;
  out_descriptor->name = resource->name;
  out_descriptor->name_len = resource->name_len;
  out_descriptor->resource_state = resource->state;
  out_descriptor->endpoint = resource->name;
  out_descriptor->version = NULL;
  out_descriptor->policy_hints = 0;
  return true;
}

bool hubos_hub_authorize(const hubos_hub_t *hub,
                         hubos_id_t capability_id,
                         hubos_id_t resource_id,
                         unsigned required_rights) {
  if (hub == NULL) {
    return false;
  }

  return hubos_capability_manager_authorize(hub->capability_manager,
                                            capability_id,
                                            resource_id,
                                            required_rights);
}
