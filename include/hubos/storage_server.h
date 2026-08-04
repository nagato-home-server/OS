#ifndef HUBOS_STORAGE_SERVER_H
#define HUBOS_STORAGE_SERVER_H

#include <string.h>

#include "hubos/hub.h"
#include "hubos/namespace.h"

typedef struct {
  hubos_id_t id;
  hubos_id_t owner_session_id;
  hubos_namespace_handle_t namespace_handle;
} hubos_storage_server_t;

static inline void hubos_storage_server_init(hubos_storage_server_t *server,
                                             hubos_id_t id,
                                             hubos_id_t owner_session_id,
                                             hubos_namespace_handle_t namespace_handle) {
  if (server == NULL) {
    return;
  }

  server->id = id;
  server->owner_session_id = owner_session_id;
  server->namespace_handle = namespace_handle;
}

static inline bool hubos_storage_server_bind_namespace(hubos_storage_server_t *server,
                                                       hubos_namespace_handle_t namespace_handle) {
  if (server == NULL || namespace_handle.kind != HUBOS_NAMESPACE_STORAGE ||
      !hubos_shared_resource_is_active(&namespace_handle.lifecycle)) {
    return false;
  }

  if (server->namespace_handle.owned_by_server &&
      !hubos_namespace_handle_release(&server->namespace_handle)) {
    return false;
  }

  if (!hubos_namespace_handle_bind(&namespace_handle, server->owner_session_id)) {
    return false;
  }

  server->namespace_handle = namespace_handle;
  return true;
}

static inline bool hubos_storage_server_release_namespace(hubos_storage_server_t *server) {
  if (server == NULL || !server->namespace_handle.owned_by_server) {
    return false;
  }

  return hubos_namespace_handle_release(&server->namespace_handle);
}

static inline bool hubos_storage_server_finalize_namespace(hubos_storage_server_t *server) {
  if (server == NULL) {
    return false;
  }

  return hubos_namespace_handle_finalize(&server->namespace_handle);
}

static inline bool hubos_storage_server_describe(const hubos_storage_server_t *server,
                                                 hubos_service_descriptor_t *out_descriptor) {
  const char *name = NULL;

  if (server == NULL || out_descriptor == NULL) {
    return false;
  }

  name = server->namespace_handle.name != NULL ? server->namespace_handle.name : "storage-server";

  out_descriptor->resource_id = server->namespace_handle.id != HUBOS_ID_INVALID ?
                                  server->namespace_handle.id :
                                  server->id;
  out_descriptor->name = name;
  out_descriptor->name_len = strlen(name);
  if (hubos_shared_resource_is_pending_finalization(&server->namespace_handle.lifecycle)) {
    out_descriptor->resource_state = HUBOS_RESOURCE_QUARANTINED;
  } else if (server->namespace_handle.lifecycle.state == HUBOS_SHARED_RESOURCE_RETIRED) {
    out_descriptor->resource_state = HUBOS_RESOURCE_RETIRED;
  } else {
    out_descriptor->resource_state = server->namespace_handle.owned_by_server ?
                                       HUBOS_RESOURCE_READY :
                                       HUBOS_RESOURCE_DISCOVERED;
  }
  out_descriptor->endpoint = name;
  out_descriptor->version = NULL;
  out_descriptor->policy_hints = (unsigned)(server->namespace_handle.owned_by_server ? 1u : 0u);
  return true;
}

#endif
