#ifndef HUBOS_NAMESPACE_H
#define HUBOS_NAMESPACE_H

#include "hubos/model.h"
#include "hubos/shared_resource.h"

typedef enum {
  HUBOS_NAMESPACE_NETWORK = 0,
  HUBOS_NAMESPACE_STORAGE,
  HUBOS_NAMESPACE_DISPLAY,
} hubos_namespace_kind_t;

typedef struct {
  hubos_id_t id;
  hubos_namespace_kind_t kind;
  const char *name;
  bool owned_by_server;
  hubos_shared_resource_t lifecycle;
} hubos_namespace_handle_t;

static inline void hubos_namespace_handle_init(hubos_namespace_handle_t *handle,
                                               hubos_id_t id,
                                               hubos_namespace_kind_t kind,
                                               const char *name,
                                               bool owned_by_server) {
  if (handle == NULL) {
    return;
  }

  handle->id = id;
  handle->kind = kind;
  handle->name = name;
  handle->owned_by_server = owned_by_server;
  hubos_shared_resource_init(&handle->lifecycle, id, HUBOS_ID_INVALID, 1);
}

static inline void hubos_namespace_handle_set_owner_session(hubos_namespace_handle_t *handle,
                                                            hubos_id_t owner_session_id) {
  if (handle == NULL) {
    return;
  }

  handle->lifecycle.owner_session_id = owner_session_id;
}

static inline bool hubos_namespace_handle_bind(hubos_namespace_handle_t *handle,
                                               hubos_id_t owner_session_id) {
  if (handle == NULL || !hubos_shared_resource_is_active(&handle->lifecycle)) {
    return false;
  }

  hubos_namespace_handle_set_owner_session(handle, owner_session_id);
  handle->owned_by_server = true;
  return true;
}

static inline bool hubos_namespace_handle_release(hubos_namespace_handle_t *handle) {
  if (handle == NULL) {
    return false;
  }

  if (!hubos_shared_resource_detach(&handle->lifecycle)) {
    return false;
  }

  handle->owned_by_server = false;
  return true;
}

static inline bool hubos_namespace_handle_finalize(hubos_namespace_handle_t *handle) {
  if (handle == NULL) {
    return false;
  }

  if (!hubos_shared_resource_finalize(&handle->lifecycle)) {
    return false;
  }

  handle->owned_by_server = false;
  return true;
}

static inline void hubos_namespace_handle_abort(hubos_namespace_handle_t *handle) {
  if (handle == NULL) {
    return;
  }

  hubos_shared_resource_abort(&handle->lifecycle);
  handle->owned_by_server = false;
}

#endif
