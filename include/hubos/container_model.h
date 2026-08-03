#ifndef HUBOS_CONTAINER_MODEL_H
#define HUBOS_CONTAINER_MODEL_H

#include "hubos/model.h"
#include "hubos/namespace.h"

typedef struct {
  hubos_id_t id;
  hubos_id_t session_id;
  hubos_namespace_handle_t network_namespace;
  hubos_namespace_handle_t storage_namespace;
  hubos_namespace_handle_t display_namespace;
} hubos_container_t;

static inline void hubos_container_init(hubos_container_t *container,
                                        hubos_id_t id,
                                        hubos_id_t session_id,
                                        hubos_namespace_handle_t network_namespace,
                                        hubos_namespace_handle_t storage_namespace,
                                        hubos_namespace_handle_t display_namespace) {
  if (container == NULL) {
    return;
  }

  container->id = id;
  container->session_id = session_id;
  container->network_namespace = network_namespace;
  container->storage_namespace = storage_namespace;
  container->display_namespace = display_namespace;
}

#endif
