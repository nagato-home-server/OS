#include "hubos/model.h"

void hubos_resource_init(hubos_resource_t *resource,
                         hubos_id_t id,
                         const char *name,
                         size_t name_len) {
  if (resource == NULL) {
    return;
  }

  resource->id = id;
  resource->name = name;
  resource->name_len = name_len;
  resource->state = HUBOS_RESOURCE_DISCOVERED;
  resource->name_owned = false;
  resource->provisional = false;
  resource->discovery_count = 0;
}

void hubos_capability_init(hubos_capability_t *capability,
                           hubos_id_t id,
                           hubos_id_t owner_session_id,
                           hubos_id_t resource_id,
                           unsigned rights,
                           bool delegatable) {
  if (capability == NULL) {
    return;
  }

  capability->id = id;
  capability->owner_session_id = owner_session_id;
  capability->resource_id = resource_id;
  capability->rights = rights;
  capability->delegatable = delegatable;
  capability->revoked = false;
}

void hubos_session_init(hubos_session_t *session,
                        hubos_id_t id,
                        hubos_id_t owner_id,
                        hubos_id_t parent_id,
                        hubos_session_type_t type) {
  if (session == NULL) {
    return;
  }

  session->id = id;
  session->owner_id = owner_id;
  session->parent_id = parent_id;
  session->type = type;
  session->state = HUBOS_SESSION_CREATED;
  session->namespace_view_version = HUBOS_ID_INVALID;
  session->policy_context_version = HUBOS_ID_INVALID;
  session->resource_set_version = HUBOS_ID_INVALID;
  session->lease_version = HUBOS_ID_INVALID;
}

void hubos_session_update_context(hubos_session_t *session,
                                  hubos_id_t namespace_view_version,
                                  hubos_id_t policy_context_version) {
  if (session == NULL) {
    return;
  }

  session->namespace_view_version = namespace_view_version;
  session->policy_context_version = policy_context_version;
}

void hubos_session_update_assets(hubos_session_t *session,
                                 hubos_id_t resource_set_version,
                                 hubos_id_t lease_version) {
  if (session == NULL) {
    return;
  }

  session->resource_set_version = resource_set_version;
  session->lease_version = lease_version;
}

static bool hubos_state_is_terminal_resource(hubos_resource_state_t state) {
  return state == HUBOS_RESOURCE_FAILED || state == HUBOS_RESOURCE_QUARANTINED ||
         state == HUBOS_RESOURCE_RETIRED;
}

static bool hubos_state_is_terminal_session(hubos_session_state_t state) {
  return state == HUBOS_SESSION_REVOKED || state == HUBOS_SESSION_RETIRED;
}

static bool hubos_state_is_terminal_dma(hubos_dma_state_t state) {
  return state == HUBOS_DMA_REVOKED || state == HUBOS_DMA_ABORTED;
}

bool hubos_resource_transition_allowed(hubos_resource_state_t from,
                                       hubos_resource_state_t to) {
  if (from == to) {
    return true;
  }

  if (hubos_state_is_terminal_resource(from)) {
    return false;
  }

  switch (from) {
  case HUBOS_RESOURCE_DISCOVERED:
    return to == HUBOS_RESOURCE_CLASSIFIED || hubos_state_is_terminal_resource(to);
  case HUBOS_RESOURCE_CLASSIFIED:
    return to == HUBOS_RESOURCE_BOUND || hubos_state_is_terminal_resource(to);
  case HUBOS_RESOURCE_BOUND:
    return to == HUBOS_RESOURCE_READY || hubos_state_is_terminal_resource(to);
  case HUBOS_RESOURCE_READY:
    return to == HUBOS_RESOURCE_RETIRED || hubos_state_is_terminal_resource(to);
  case HUBOS_RESOURCE_FAILED:
  case HUBOS_RESOURCE_QUARANTINED:
  case HUBOS_RESOURCE_RETIRED:
    return false;
  }

  return false;
}

bool hubos_session_transition_allowed(hubos_session_state_t from,
                                      hubos_session_state_t to) {
  if (from == to) {
    return true;
  }

  if (hubos_state_is_terminal_session(from)) {
    return false;
  }

  switch (from) {
  case HUBOS_SESSION_CREATED:
    return to == HUBOS_SESSION_ACTIVE || to == HUBOS_SESSION_REVOKED;
  case HUBOS_SESSION_ACTIVE:
    return to == HUBOS_SESSION_DRAINING || to == HUBOS_SESSION_REVOKED;
  case HUBOS_SESSION_DRAINING:
    return to == HUBOS_SESSION_REVOKED;
  case HUBOS_SESSION_REVOKED:
  case HUBOS_SESSION_RETIRED:
    return false;
  }

  return false;
}

bool hubos_dma_transition_allowed(hubos_dma_state_t from,
                                  hubos_dma_state_t to) {
  if (from == to) {
    return true;
  }

  if (hubos_state_is_terminal_dma(from)) {
    return false;
  }

  switch (from) {
  case HUBOS_DMA_UNMAPPED:
    return to == HUBOS_DMA_MAPPING;
  case HUBOS_DMA_MAPPING:
    return to == HUBOS_DMA_ACTIVE;
  case HUBOS_DMA_ACTIVE:
    return to == HUBOS_DMA_QUIESCING || to == HUBOS_DMA_ABORTED;
  case HUBOS_DMA_QUIESCING:
    return to == HUBOS_DMA_REVOKED || to == HUBOS_DMA_ABORTED;
  case HUBOS_DMA_REVOKED:
  case HUBOS_DMA_ABORTED:
    return false;
  }

  return false;
}

bool hubos_capability_can_delegate(const hubos_capability_t *capability) {
  return capability != NULL && capability->delegatable;
}

bool hubos_capability_is_active(const hubos_capability_t *capability) {
  return capability != NULL && !capability->revoked;
}

bool hubos_rights_contains(unsigned available, unsigned required) {
  return (available & required) == required;
}

const char *hubos_resource_state_name(hubos_resource_state_t state) {
  switch (state) {
  case HUBOS_RESOURCE_DISCOVERED:
    return "DISCOVERED";
  case HUBOS_RESOURCE_CLASSIFIED:
    return "CLASSIFIED";
  case HUBOS_RESOURCE_BOUND:
    return "BOUND";
  case HUBOS_RESOURCE_READY:
    return "READY";
  case HUBOS_RESOURCE_FAILED:
    return "FAILED";
  case HUBOS_RESOURCE_QUARANTINED:
    return "QUARANTINED";
  case HUBOS_RESOURCE_RETIRED:
    return "RETIRED";
  }

  return "UNKNOWN";
}

const char *hubos_session_state_name(hubos_session_state_t state) {
  switch (state) {
  case HUBOS_SESSION_CREATED:
    return "CREATED";
  case HUBOS_SESSION_ACTIVE:
    return "ACTIVE";
  case HUBOS_SESSION_DRAINING:
    return "DRAINING";
  case HUBOS_SESSION_REVOKED:
    return "REVOKED";
  case HUBOS_SESSION_RETIRED:
    return "RETIRED";
  }

  return "UNKNOWN";
}

const char *hubos_dma_state_name(hubos_dma_state_t state) {
  switch (state) {
  case HUBOS_DMA_UNMAPPED:
    return "UNMAPPED";
  case HUBOS_DMA_MAPPING:
    return "MAPPING";
  case HUBOS_DMA_ACTIVE:
    return "ACTIVE";
  case HUBOS_DMA_QUIESCING:
    return "QUIESCING";
  case HUBOS_DMA_REVOKED:
    return "REVOKED";
  case HUBOS_DMA_ABORTED:
    return "ABORTED";
  }

  return "UNKNOWN";
}
