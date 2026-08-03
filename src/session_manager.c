#include "hubos/session_manager.h"

#include <stdlib.h>

static hubos_session_t *hubos_session_manager_find_mutable(hubos_session_manager_t *manager,
                                                           hubos_id_t session_id) {
  if (manager == NULL || session_id == HUBOS_ID_INVALID) {
    return NULL;
  }

  for (size_t index = 0; index < manager->count; ++index) {
    if (manager->items[index].id == session_id) {
      return &manager->items[index];
    }
  }

  return NULL;
}

static const hubos_session_t *hubos_session_manager_find(const hubos_session_manager_t *manager,
                                                         hubos_id_t session_id) {
  if (manager == NULL || session_id == HUBOS_ID_INVALID) {
    return NULL;
  }

  for (size_t index = 0; index < manager->count; ++index) {
    if (manager->items[index].id == session_id) {
      return &manager->items[index];
    }
  }

  return NULL;
}

static bool hubos_session_manager_reserve(hubos_session_manager_t *manager,
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

static bool hubos_session_manager_descendents_revoke(hubos_session_manager_t *manager,
                                                     hubos_capability_manager_t *capability_manager,
                                                     hubos_id_t session_id) {
  bool success = true;

  for (size_t index = 0; index < manager->count; ++index) {
    if (manager->items[index].parent_id == session_id) {
      if (!hubos_session_manager_descendents_revoke(manager,
                                                    capability_manager,
                                                    manager->items[index].id)) {
        success = false;
      }
      if (capability_manager != NULL) {
        (void)hubos_capability_manager_revoke_owned(capability_manager, manager->items[index].id);
      }
      manager->items[index].state = HUBOS_SESSION_REVOKED;
    }
  }

  return success;
}

void hubos_session_manager_init(hubos_session_manager_t *manager) {
  if (manager == NULL) {
    return;
  }

  manager->items = NULL;
  manager->count = 0;
  manager->capacity = 0;
  manager->next_id = 1;
}

void hubos_session_manager_destroy(hubos_session_manager_t *manager) {
  if (manager == NULL) {
    return;
  }

  free(manager->items);
  manager->items = NULL;
  manager->count = 0;
  manager->capacity = 0;
  manager->next_id = 1;
}

bool hubos_session_manager_create(hubos_session_manager_t *manager,
                                  hubos_id_t owner_id,
                                  hubos_id_t parent_id,
                                  hubos_session_type_t type,
                                  hubos_id_t *out_session_id) {
  hubos_session_t session;
  const hubos_session_t *parent = NULL;
  hubos_id_t session_id = HUBOS_ID_INVALID;

  if (manager == NULL) {
    return false;
  }

  if (parent_id != HUBOS_ID_INVALID) {
    parent = hubos_session_manager_find(manager, parent_id);
    if (parent == NULL || parent->state == HUBOS_SESSION_REVOKED) {
      return false;
    }
  }

  if (!hubos_session_manager_reserve(manager, manager->count + 1)) {
    return false;
  }

  session_id = manager->next_id;
  hubos_session_init(&session, session_id, owner_id, parent_id, type);
  if (parent != NULL) {
    hubos_session_update_context(&session,
                                 parent->namespace_view_version,
                                 parent->policy_context_version);
    hubos_session_update_assets(&session,
                                parent->resource_set_version,
                                parent->lease_version);
  }
  manager->items[manager->count++] = session;
  if (out_session_id != NULL) {
    *out_session_id = session.id;
  }
  manager->next_id = session_id + 1;
  return true;
}

bool hubos_session_manager_refresh_context(hubos_session_manager_t *manager,
                                           hubos_id_t session_id,
                                           hubos_id_t namespace_view_version,
                                           hubos_id_t policy_context_version) {
  hubos_session_t *session = hubos_session_manager_find_mutable(manager, session_id);

  if (session == NULL || session->state == HUBOS_SESSION_REVOKED) {
    return false;
  }

  hubos_session_update_context(session, namespace_view_version, policy_context_version);
  return true;
}

bool hubos_session_manager_refresh_assets(hubos_session_manager_t *manager,
                                          hubos_id_t session_id,
                                          hubos_id_t resource_set_version,
                                          hubos_id_t lease_version) {
  hubos_session_t *session = hubos_session_manager_find_mutable(manager, session_id);

  if (session == NULL || session->state == HUBOS_SESSION_REVOKED) {
    return false;
  }

  hubos_session_update_assets(session, resource_set_version, lease_version);
  return true;
}

const hubos_session_t *hubos_session_manager_get(const hubos_session_manager_t *manager,
                                                 hubos_id_t session_id) {
  return hubos_session_manager_find(manager, session_id);
}

bool hubos_session_manager_set_state(hubos_session_manager_t *manager,
                                     hubos_id_t session_id,
                                     hubos_session_state_t state) {
  hubos_session_t *session = hubos_session_manager_find_mutable(manager, session_id);

  if (session == NULL || !hubos_session_transition_allowed(session->state, state)) {
    return false;
  }

  session->state = state;
  return true;
}

bool hubos_session_manager_is_ancestor(const hubos_session_manager_t *manager,
                                       hubos_id_t ancestor_id,
                                       hubos_id_t session_id) {
  const hubos_session_t *session = hubos_session_manager_find(manager, session_id);
  size_t guard = manager != NULL ? manager->count : 0;

  if (manager == NULL || ancestor_id == HUBOS_ID_INVALID || session == NULL) {
    return false;
  }

  while (session != NULL && guard > 0) {
    if (session->parent_id == ancestor_id) {
      return true;
    }

    if (session->parent_id == HUBOS_ID_INVALID) {
      return false;
    }

    session = hubos_session_manager_find(manager, session->parent_id);
    --guard;
  }

  return false;
}

size_t hubos_session_manager_child_count(const hubos_session_manager_t *manager,
                                         hubos_id_t session_id) {
  size_t count = 0;

  if (manager == NULL || session_id == HUBOS_ID_INVALID) {
    return 0;
  }

  for (size_t index = 0; index < manager->count; ++index) {
    if (manager->items[index].parent_id == session_id) {
      ++count;
    }
  }

  return count;
}

bool hubos_session_manager_revoke_tree(hubos_session_manager_t *manager,
                                       hubos_capability_manager_t *capability_manager,
                                       hubos_id_t session_id) {
  hubos_session_t *session = hubos_session_manager_find_mutable(manager, session_id);

  if (session == NULL) {
    return false;
  }

  if (session->state != HUBOS_SESSION_REVOKED) {
    if (!hubos_session_transition_allowed(session->state, HUBOS_SESSION_REVOKED)) {
      return false;
    }
    session->state = HUBOS_SESSION_REVOKED;
  }

  if (capability_manager != NULL) {
    (void)hubos_capability_manager_revoke_owned(capability_manager, session_id);
  }

  return hubos_session_manager_descendents_revoke(manager, capability_manager, session_id);
}
