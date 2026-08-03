#ifndef HUBOS_SESSION_MANAGER_H
#define HUBOS_SESSION_MANAGER_H

#include "hubos/model.h"
#include "hubos/capability_manager.h"

typedef struct {
  hubos_session_t *items;
  size_t count;
  size_t capacity;
  hubos_id_t next_id;
} hubos_session_manager_t;

void hubos_session_manager_init(hubos_session_manager_t *manager);
void hubos_session_manager_destroy(hubos_session_manager_t *manager);

bool hubos_session_manager_create(hubos_session_manager_t *manager,
                                  hubos_id_t owner_id,
                                  hubos_id_t parent_id,
                                  hubos_session_type_t type,
                                  hubos_id_t *out_session_id);

bool hubos_session_manager_refresh_context(hubos_session_manager_t *manager,
                                           hubos_id_t session_id,
                                           hubos_id_t namespace_view_version,
                                           hubos_id_t policy_context_version);

bool hubos_session_manager_refresh_assets(hubos_session_manager_t *manager,
                                          hubos_id_t session_id,
                                          hubos_id_t resource_set_version,
                                          hubos_id_t lease_version);

const hubos_session_t *hubos_session_manager_get(const hubos_session_manager_t *manager,
                                                 hubos_id_t session_id);

bool hubos_session_manager_set_state(hubos_session_manager_t *manager,
                                     hubos_id_t session_id,
                                     hubos_session_state_t state);

bool hubos_session_manager_is_ancestor(const hubos_session_manager_t *manager,
                                       hubos_id_t ancestor_id,
                                       hubos_id_t session_id);

size_t hubos_session_manager_child_count(const hubos_session_manager_t *manager,
                                         hubos_id_t session_id);

bool hubos_session_manager_revoke_tree(hubos_session_manager_t *manager,
                                       hubos_capability_manager_t *capability_manager,
                                       hubos_id_t session_id);

#endif
