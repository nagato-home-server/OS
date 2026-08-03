#include "hubos/resource_registry.h"

#include <stdlib.h>
#include <string.h>

static bool hubos_resource_name_equals(const hubos_resource_t *resource,
                                       const char *name,
                                       size_t name_len) {
  if (resource == NULL || name == NULL) {
    return false;
  }

  return resource->name_len == name_len && memcmp(resource->name, name, name_len) == 0;
}

static hubos_resource_t *hubos_resource_registry_find_mutable(hubos_resource_registry_t *registry,
                                                             const char *name,
                                                             size_t name_len) {
  if (registry == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < registry->count; ++index) {
    if (hubos_resource_name_equals(&registry->items[index], name, name_len)) {
      return &registry->items[index];
    }
  }

  return NULL;
}

static bool hubos_resource_registry_reserve(hubos_resource_registry_t *registry,
                                            size_t desired_capacity) {
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

void hubos_resource_registry_init(hubos_resource_registry_t *registry) {
  if (registry == NULL) {
    return;
  }

  registry->items = NULL;
  registry->count = 0;
  registry->capacity = 0;
  registry->next_id = 1;
}

void hubos_resource_registry_destroy(hubos_resource_registry_t *registry) {
  if (registry == NULL) {
    return;
  }

  for (size_t index = 0; index < registry->count; ++index) {
    if (registry->items[index].name_owned) {
      free((void *)registry->items[index].name);
    }
  }

  free(registry->items);
  registry->items = NULL;
  registry->count = 0;
  registry->capacity = 0;
  registry->next_id = 1;
}

bool hubos_resource_registry_discover(hubos_resource_registry_t *registry,
                                      const char *name,
                                      size_t name_len,
                                      hubos_resource_state_t state,
                                      hubos_id_t *out_resource_id,
                                      bool *out_is_new) {
  hubos_resource_t *existing = NULL;

  if (registry == NULL || name == NULL || name_len == 0) {
    return false;
  }

  existing = hubos_resource_registry_find_mutable(registry, name, name_len);
  if (existing != NULL) {
    existing->provisional = true;
    existing->discovery_count += 1;
    if (hubos_resource_transition_allowed(existing->state, state)) {
      existing->state = state;
    }

    if (out_resource_id != NULL) {
      *out_resource_id = existing->id;
    }
    if (out_is_new != NULL) {
      *out_is_new = false;
    }
    return true;
  }

  if (!hubos_resource_registry_reserve(registry, registry->count + 1)) {
    return false;
  }

  char *owned_name = malloc(name_len + 1);
  if (owned_name == NULL) {
    return false;
  }

  memcpy(owned_name, name, name_len);
  owned_name[name_len] = '\0';

  hubos_resource_t resource;
  hubos_resource_init(&resource, registry->next_id++, owned_name, name_len);
  resource.state = state;
  resource.name_owned = true;
  resource.provisional = true;
  resource.discovery_count = 1;

  registry->items[registry->count++] = resource;

  if (out_resource_id != NULL) {
    *out_resource_id = resource.id;
  }
  if (out_is_new != NULL) {
    *out_is_new = true;
  }
  return true;
}

bool hubos_resource_registry_register(hubos_resource_registry_t *registry,
                                      const char *name,
                                      size_t name_len,
                                      hubos_resource_state_t state,
                                      hubos_id_t *out_resource_id,
                                      bool *out_is_new) {
  hubos_resource_t *existing = NULL;

  if (registry == NULL || name == NULL || name_len == 0) {
    return false;
  }

  existing = hubos_resource_registry_find_mutable(registry, name, name_len);
  if (existing != NULL) {
    if (!hubos_resource_transition_allowed(existing->state, state)) {
      if (out_resource_id != NULL) {
        *out_resource_id = existing->id;
      }
      if (out_is_new != NULL) {
        *out_is_new = false;
      }
      return false;
    }

    existing->state = state;
    existing->provisional = false;

    if (out_resource_id != NULL) {
      *out_resource_id = existing->id;
    }
    if (out_is_new != NULL) {
      *out_is_new = false;
    }
    return true;
  }

  if (!hubos_resource_registry_reserve(registry, registry->count + 1)) {
    return false;
  }

  char *owned_name = malloc(name_len + 1);
  if (owned_name == NULL) {
    return false;
  }

  memcpy(owned_name, name, name_len);
  owned_name[name_len] = '\0';

  hubos_resource_t resource;
  hubos_resource_init(&resource, registry->next_id++, owned_name, name_len);
  resource.state = state;
  resource.name_owned = true;
  resource.provisional = false;
  resource.discovery_count = 0;

  registry->items[registry->count++] = resource;

  if (out_resource_id != NULL) {
    *out_resource_id = resource.id;
  }
  if (out_is_new != NULL) {
    *out_is_new = true;
  }
  return true;
}

const hubos_resource_t *hubos_resource_registry_get(const hubos_resource_registry_t *registry,
                                                    hubos_id_t resource_id) {
  if (registry == NULL || resource_id == HUBOS_ID_INVALID) {
    return NULL;
  }

  for (size_t index = 0; index < registry->count; ++index) {
    if (registry->items[index].id == resource_id) {
      return &registry->items[index];
    }
  }

  return NULL;
}

const hubos_resource_t *hubos_resource_registry_find(const hubos_resource_registry_t *registry,
                                                     const char *name,
                                                     size_t name_len) {
  if (registry == NULL || name == NULL || name_len == 0) {
    return NULL;
  }

  for (size_t index = 0; index < registry->count; ++index) {
    if (hubos_resource_name_equals(&registry->items[index], name, name_len)) {
      return &registry->items[index];
    }
  }

  return NULL;
}

bool hubos_resource_registry_update_state(hubos_resource_registry_t *registry,
                                          hubos_id_t resource_id,
                                          hubos_resource_state_t state) {
  hubos_resource_t *resource = NULL;

  if (registry == NULL || resource_id == HUBOS_ID_INVALID) {
    return false;
  }

  for (size_t index = 0; index < registry->count; ++index) {
    if (registry->items[index].id == resource_id) {
      resource = &registry->items[index];
      break;
    }
  }

  if (resource == NULL) {
    return false;
  }

  if (!hubos_resource_transition_allowed(resource->state, state)) {
    return false;
  }

  resource->state = state;
  return true;
}

bool hubos_resource_registry_quarantine(hubos_resource_registry_t *registry,
                                        hubos_id_t resource_id) {
  return hubos_resource_registry_update_state(registry, resource_id, HUBOS_RESOURCE_QUARANTINED);
}

bool hubos_resource_registry_retire(hubos_resource_registry_t *registry,
                                    hubos_id_t resource_id) {
  return hubos_resource_registry_update_state(registry, resource_id, HUBOS_RESOURCE_RETIRED);
}

size_t hubos_resource_registry_count(const hubos_resource_registry_t *registry) {
  return registry != NULL ? registry->count : 0;
}
