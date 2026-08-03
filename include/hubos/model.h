#ifndef HUBOS_MODEL_H
#define HUBOS_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t hubos_id_t;

#define HUBOS_ID_INVALID ((hubos_id_t)0)

typedef enum {
  HUBOS_RESOURCE_DISCOVERED = 0,
  HUBOS_RESOURCE_CLASSIFIED,
  HUBOS_RESOURCE_BOUND,
  HUBOS_RESOURCE_READY,
  HUBOS_RESOURCE_FAILED,
  HUBOS_RESOURCE_QUARANTINED,
  HUBOS_RESOURCE_RETIRED,
} hubos_resource_state_t;

typedef enum {
  HUBOS_SESSION_PERMANENT = 0,
  HUBOS_SESSION_PERSISTENT,
  HUBOS_SESSION_EPHEMERAL,
  HUBOS_SESSION_TRANSACTIONAL,
} hubos_session_type_t;

typedef enum {
  HUBOS_SESSION_CREATED = 0,
  HUBOS_SESSION_ACTIVE,
  HUBOS_SESSION_DRAINING,
  HUBOS_SESSION_REVOKED,
  HUBOS_SESSION_RETIRED,
} hubos_session_state_t;

typedef enum {
  HUBOS_DMA_UNMAPPED = 0,
  HUBOS_DMA_MAPPING,
  HUBOS_DMA_ACTIVE,
  HUBOS_DMA_QUIESCING,
  HUBOS_DMA_REVOKED,
  HUBOS_DMA_ABORTED,
} hubos_dma_state_t;

enum {
  HUBOS_CAP_RIGHT_COPY = 1u << 0,
  HUBOS_CAP_RIGHT_MINT = 1u << 1,
  HUBOS_CAP_RIGHT_REVOKE = 1u << 2,
  HUBOS_CAP_RIGHT_TRANSFER = 1u << 3,
  HUBOS_CAP_RIGHT_INSPECT = 1u << 4,
};

typedef struct {
  hubos_id_t id;
  const char *name;
  size_t name_len;
  hubos_resource_state_t state;
  bool name_owned;
  bool provisional;
  unsigned discovery_count;
} hubos_resource_t;

typedef struct {
  hubos_id_t id;
  hubos_id_t owner_session_id;
  hubos_id_t resource_id;
  unsigned rights;
  bool delegatable;
  bool revoked;
} hubos_capability_t;

typedef struct {
  hubos_id_t id;
  hubos_id_t owner_id;
  hubos_id_t parent_id;
  hubos_session_type_t type;
  hubos_session_state_t state;
  hubos_id_t namespace_view_version;
  hubos_id_t policy_context_version;
  hubos_id_t resource_set_version;
  hubos_id_t lease_version;
} hubos_session_t;

typedef struct {
  hubos_id_t resource_id;
  hubos_dma_state_t state;
  bool queue_empty;
  bool outstanding_complete;
  bool interrupts_drained;
} hubos_dma_mapping_t;

void hubos_resource_init(hubos_resource_t *resource,
                         hubos_id_t id,
                         const char *name,
                         size_t name_len);

void hubos_capability_init(hubos_capability_t *capability,
                           hubos_id_t id,
                           hubos_id_t owner_session_id,
                           hubos_id_t resource_id,
                           unsigned rights,
                           bool delegatable);

void hubos_session_init(hubos_session_t *session,
                        hubos_id_t id,
                        hubos_id_t owner_id,
                        hubos_id_t parent_id,
                        hubos_session_type_t type);

void hubos_session_update_context(hubos_session_t *session,
                                  hubos_id_t namespace_view_version,
                                  hubos_id_t policy_context_version);

void hubos_session_update_assets(hubos_session_t *session,
                                 hubos_id_t resource_set_version,
                                 hubos_id_t lease_version);

bool hubos_resource_transition_allowed(hubos_resource_state_t from,
                                       hubos_resource_state_t to);

bool hubos_session_transition_allowed(hubos_session_state_t from,
                                      hubos_session_state_t to);

bool hubos_dma_transition_allowed(hubos_dma_state_t from,
                                  hubos_dma_state_t to);

bool hubos_capability_can_delegate(const hubos_capability_t *capability);
bool hubos_capability_is_active(const hubos_capability_t *capability);
bool hubos_rights_contains(unsigned available, unsigned required);

const char *hubos_resource_state_name(hubos_resource_state_t state);
const char *hubos_session_state_name(hubos_session_state_t state);
const char *hubos_dma_state_name(hubos_dma_state_t state);

#endif
