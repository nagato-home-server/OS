#include "hubos/capability_manager.h"

#include <stdlib.h>

static hubos_capability_t *hubos_capability_manager_find_mutable(hubos_capability_manager_t *manager,
                                                                 hubos_id_t capability_id) {
  if (manager == NULL || capability_id == HUBOS_ID_INVALID) {
    return NULL;
  }

  for (size_t index = 0; index < manager->count; ++index) {
    if (manager->items[index].id == capability_id) {
      return &manager->items[index];
    }
  }

  return NULL;
}

static const hubos_capability_t *hubos_capability_manager_find(const hubos_capability_manager_t *manager,
                                                               hubos_id_t capability_id) {
  if (manager == NULL || capability_id == HUBOS_ID_INVALID) {
    return NULL;
  }

  for (size_t index = 0; index < manager->count; ++index) {
    if (manager->items[index].id == capability_id) {
      return &manager->items[index];
    }
  }

  return NULL;
}

static bool hubos_capability_manager_reserve(hubos_capability_manager_t *manager,
                                             size_t desired_capacity) {
  if (manager == NULL) {
    return false;
  }

  if (manager->capacity >= desired_capacity) {
    return true;
  }

  size_t new_capacity = manager->capacity == 0 ? 4 : manager->capacity;
  while (new_capacity < desired_capacity) {
    new_capacity *= 2;
  }

  void *new_items = realloc(manager->items, new_capacity * sizeof(*manager->items));
  if (new_items == NULL) {
    return false;
  }

  manager->items = new_items;
  manager->capacity = new_capacity;
  return true;
}

static bool hubos_capability_manager_append(hubos_capability_manager_t *manager,
                                            hubos_capability_t capability,
                                            hubos_id_t *out_capability_id) {
  if (!hubos_capability_manager_reserve(manager, manager->count + 1)) {
    return false;
  }

  manager->items[manager->count++] = capability;
  if (out_capability_id != NULL) {
    *out_capability_id = capability.id;
  }
  return true;
}

static bool hubos_capability_manager_can_mint_from(const hubos_capability_t *source,
                                                   unsigned rights,
                                                   bool delegatable) {
  if (!hubos_capability_is_active(source)) {
    return false;
  }

  if (!hubos_capability_can_delegate(source)) {
    return false;
  }

  if (!hubos_rights_contains(source->rights, HUBOS_CAP_RIGHT_MINT)) {
    return false;
  }

  if (!hubos_rights_contains(source->rights, rights)) {
    return false;
  }

  if (delegatable && !source->delegatable) {
    return false;
  }

  return true;
}

void hubos_capability_manager_init(hubos_capability_manager_t *manager) {
  if (manager == NULL) {
    return;
  }

  manager->items = NULL;
  manager->count = 0;
  manager->capacity = 0;
  manager->next_id = 1;
}

void hubos_capability_manager_destroy(hubos_capability_manager_t *manager) {
  if (manager == NULL) {
    return;
  }

  free(manager->items);
  manager->items = NULL;
  manager->count = 0;
  manager->capacity = 0;
  manager->next_id = 1;
}

bool hubos_capability_manager_issue(hubos_capability_manager_t *manager,
                                    hubos_id_t owner_session_id,
                                    hubos_id_t resource_id,
                                    unsigned rights,
                                    bool delegatable,
                                    hubos_id_t *out_capability_id) {
  hubos_id_t capability_id = HUBOS_ID_INVALID;

  if (manager == NULL || resource_id == HUBOS_ID_INVALID) {
    return false;
  }

  capability_id = manager->next_id;
  hubos_capability_t capability;
  hubos_capability_init(&capability,
                        capability_id,
                        owner_session_id,
                        resource_id,
                        rights,
                        delegatable);
  if (!hubos_capability_manager_append(manager, capability, out_capability_id)) {
    return false;
  }

  manager->next_id = capability_id + 1;
  return true;
}

bool hubos_capability_manager_copy(hubos_capability_manager_t *manager,
                                   hubos_id_t source_capability_id,
                                   hubos_id_t owner_session_id,
                                   hubos_id_t *out_capability_id) {
  const hubos_capability_t *source = NULL;

  if (manager == NULL) {
    return false;
  }

  source = hubos_capability_manager_find(manager, source_capability_id);
  if (!hubos_capability_is_active(source) ||
      !hubos_rights_contains(source->rights, HUBOS_CAP_RIGHT_COPY)) {
    return false;
  }

  hubos_id_t capability_id = manager->next_id;
  hubos_capability_t capability;
  hubos_capability_init(&capability,
                        capability_id,
                        owner_session_id,
                        source->resource_id,
                        source->rights,
                        source->delegatable);
  if (!hubos_capability_manager_append(manager, capability, out_capability_id)) {
    return false;
  }

  manager->next_id = capability_id + 1;
  return true;
}

bool hubos_capability_manager_mint_from(hubos_capability_manager_t *manager,
                                        hubos_id_t source_capability_id,
                                        hubos_id_t owner_session_id,
                                        unsigned rights,
                                        bool delegatable,
                                        hubos_id_t *out_capability_id) {
  const hubos_capability_t *source = NULL;

  if (manager == NULL) {
    return false;
  }

  source = hubos_capability_manager_find(manager, source_capability_id);
  if (!hubos_capability_manager_can_mint_from(source, rights, delegatable)) {
    return false;
  }

  hubos_id_t capability_id = manager->next_id;
  hubos_capability_t capability;
  hubos_capability_init(&capability,
                        capability_id,
                        owner_session_id,
                        source->resource_id,
                        rights,
                        delegatable);
  if (!hubos_capability_manager_append(manager, capability, out_capability_id)) {
    return false;
  }

  manager->next_id = capability_id + 1;
  return true;
}

bool hubos_capability_manager_transfer(hubos_capability_manager_t *manager,
                                       hubos_id_t capability_id,
                                       hubos_id_t new_owner_session_id) {
  hubos_capability_t *capability = hubos_capability_manager_find_mutable(manager, capability_id);

  if (!hubos_capability_is_active(capability) || !hubos_capability_can_delegate(capability)) {
    return false;
  }

  capability->owner_session_id = new_owner_session_id;
  return true;
}

bool hubos_capability_manager_revoke(hubos_capability_manager_t *manager,
                                    hubos_id_t capability_id) {
  hubos_capability_t *capability = hubos_capability_manager_find_mutable(manager, capability_id);

  if (!hubos_capability_is_active(capability)) {
    return false;
  }

  capability->revoked = true;
  return true;
}

size_t hubos_capability_manager_revoke_owned(hubos_capability_manager_t *manager,
                                             hubos_id_t owner_session_id) {
  size_t revoked_count = 0;

  if (manager == NULL || owner_session_id == HUBOS_ID_INVALID) {
    return 0;
  }

  for (size_t index = 0; index < manager->count; ++index) {
    hubos_capability_t *capability = &manager->items[index];

    if (capability->owner_session_id != owner_session_id || capability->revoked) {
      continue;
    }

    capability->revoked = true;
    ++revoked_count;
  }

  return revoked_count;
}

const hubos_capability_t *hubos_capability_manager_get(const hubos_capability_manager_t *manager,
                                                       hubos_id_t capability_id) {
  return hubos_capability_manager_find(manager, capability_id);
}

bool hubos_capability_manager_authorize(const hubos_capability_manager_t *manager,
                                        hubos_id_t capability_id,
                                        hubos_id_t resource_id,
                                        unsigned required_rights) {
  const hubos_capability_t *capability = hubos_capability_manager_find(manager, capability_id);

  if (!hubos_capability_is_active(capability)) {
    return false;
  }

  if (capability->resource_id != resource_id) {
    return false;
  }

  return hubos_rights_contains(capability->rights, required_rights);
}
