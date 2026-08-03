#ifndef HUBOS_SHARED_RESOURCE_H
#define HUBOS_SHARED_RESOURCE_H

#include "hubos/model.h"

typedef enum {
  HUBOS_SHARED_RESOURCE_ACTIVE = 0,
  HUBOS_SHARED_RESOURCE_PENDING_FINALIZATION,
  HUBOS_SHARED_RESOURCE_RETIRED,
} hubos_shared_resource_state_t;

typedef struct {
  hubos_id_t id;
  hubos_id_t owner_session_id;
  size_t refcount;
  hubos_shared_resource_state_t state;
  bool pending_finalization;
} hubos_shared_resource_t;

static inline void hubos_shared_resource_init(hubos_shared_resource_t *resource,
                                              hubos_id_t id,
                                              hubos_id_t owner_session_id,
                                              size_t refcount) {
  if (resource == NULL) {
    return;
  }

  resource->id = id;
  resource->owner_session_id = owner_session_id;
  resource->refcount = refcount;
  resource->state = HUBOS_SHARED_RESOURCE_ACTIVE;
  resource->pending_finalization = false;
}

static inline bool hubos_shared_resource_is_active(const hubos_shared_resource_t *resource) {
  return resource != NULL && resource->state == HUBOS_SHARED_RESOURCE_ACTIVE;
}

static inline bool hubos_shared_resource_is_pending_finalization(
  const hubos_shared_resource_t *resource) {
  return resource != NULL && resource->state == HUBOS_SHARED_RESOURCE_PENDING_FINALIZATION;
}

static inline bool hubos_shared_resource_detach(hubos_shared_resource_t *resource) {
  if (resource == NULL || resource->state == HUBOS_SHARED_RESOURCE_RETIRED ||
      resource->refcount == 0) {
    return false;
  }

  --resource->refcount;
  if (resource->refcount == 0) {
    resource->pending_finalization = true;
    resource->state = HUBOS_SHARED_RESOURCE_PENDING_FINALIZATION;
  }

  return true;
}

static inline bool hubos_shared_resource_acquire(hubos_shared_resource_t *resource) {
  if (resource == NULL || resource->state == HUBOS_SHARED_RESOURCE_RETIRED ||
      resource->pending_finalization) {
    return false;
  }

  ++resource->refcount;
  return true;
}

static inline bool hubos_shared_resource_finalize(hubos_shared_resource_t *resource) {
  if (resource == NULL || resource->state != HUBOS_SHARED_RESOURCE_PENDING_FINALIZATION) {
    return false;
  }

  if (resource->refcount != 0) {
    return false;
  }

  resource->state = HUBOS_SHARED_RESOURCE_RETIRED;
  resource->pending_finalization = false;
  return true;
}

static inline void hubos_shared_resource_abort(hubos_shared_resource_t *resource) {
  if (resource == NULL) {
    return;
  }

  resource->refcount = 0;
  resource->state = HUBOS_SHARED_RESOURCE_RETIRED;
  resource->pending_finalization = false;
}

#endif
