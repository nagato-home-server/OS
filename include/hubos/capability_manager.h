#ifndef HUBOS_CAPABILITY_MANAGER_H
#define HUBOS_CAPABILITY_MANAGER_H

#include "hubos/model.h"

typedef struct {
  hubos_capability_t *items;
  size_t count;
  size_t capacity;
  hubos_id_t next_id;
} hubos_capability_manager_t;

void hubos_capability_manager_init(hubos_capability_manager_t *manager);
void hubos_capability_manager_destroy(hubos_capability_manager_t *manager);

bool hubos_capability_manager_issue(hubos_capability_manager_t *manager,
                                    hubos_id_t owner_session_id,
                                    hubos_id_t resource_id,
                                    unsigned rights,
                                    bool delegatable,
                                    hubos_id_t *out_capability_id);

bool hubos_capability_manager_copy(hubos_capability_manager_t *manager,
                                   hubos_id_t source_capability_id,
                                   hubos_id_t owner_session_id,
                                   hubos_id_t *out_capability_id);

bool hubos_capability_manager_mint_from(hubos_capability_manager_t *manager,
                                        hubos_id_t source_capability_id,
                                        hubos_id_t owner_session_id,
                                        unsigned rights,
                                        bool delegatable,
                                        hubos_id_t *out_capability_id);

bool hubos_capability_manager_transfer(hubos_capability_manager_t *manager,
                                       hubos_id_t capability_id,
                                       hubos_id_t new_owner_session_id);

bool hubos_capability_manager_revoke(hubos_capability_manager_t *manager,
                                    hubos_id_t capability_id);

size_t hubos_capability_manager_revoke_owned(hubos_capability_manager_t *manager,
                                             hubos_id_t owner_session_id);

const hubos_capability_t *hubos_capability_manager_get(const hubos_capability_manager_t *manager,
                                                       hubos_id_t capability_id);

bool hubos_capability_manager_authorize(const hubos_capability_manager_t *manager,
                                        hubos_id_t capability_id,
                                        hubos_id_t resource_id,
                                        unsigned required_rights);

#endif
