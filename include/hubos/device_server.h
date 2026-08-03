#ifndef HUBOS_DEVICE_SERVER_H
#define HUBOS_DEVICE_SERVER_H

#include "hubos/model.h"

struct hubos_device_server;

typedef struct {
  bool (*set_owner)(void *context, struct hubos_device_server *server, hubos_id_t owner_session_id);
  bool (*release_owner)(void *context, struct hubos_device_server *server);
  bool (*quarantine)(void *context, struct hubos_device_server *server);
  bool (*clear_quarantine)(void *context, struct hubos_device_server *server);
  bool (*reset)(void *context, struct hubos_device_server *server);
  bool (*attach_mmio)(void *context, struct hubos_device_server *server, hubos_id_t owner_session_id);
  bool (*attach_irq)(void *context, struct hubos_device_server *server, hubos_id_t owner_session_id);
  bool (*attach_dma)(void *context, struct hubos_device_server *server, hubos_id_t owner_session_id);
} hubos_device_server_ops_t;

typedef struct hubos_device_server {
  hubos_id_t id;
  hubos_id_t owner_session_id;
  hubos_id_t resource_id;
  const char *name;
  const hubos_device_server_ops_t *ops;
  void *ops_context;
  hubos_id_t mmio_owner_session_id;
  hubos_id_t irq_owner_session_id;
  hubos_id_t dma_owner_session_id;
  bool mmio_attached;
  bool irq_attached;
  bool dma_attached;
  bool quarantined;
  bool running;
} hubos_device_server_t;

void hubos_device_server_init(hubos_device_server_t *server,
                              hubos_id_t id,
                              hubos_id_t owner_session_id,
                              hubos_id_t resource_id,
                              const char *name);

void hubos_device_server_set_ops(hubos_device_server_t *server,
                                 const hubos_device_server_ops_t *ops,
                                 void *ops_context);

static inline bool hubos_device_server_has_owner(const hubos_device_server_t *server) {
  return server != NULL && server->owner_session_id != HUBOS_ID_INVALID;
}

static inline bool hubos_device_server_is_active(const hubos_device_server_t *server) {
  return hubos_device_server_has_owner(server) && server->running && !server->quarantined &&
         (server->mmio_attached || server->irq_attached || server->dma_attached);
}

bool hubos_device_server_set_owner(hubos_device_server_t *server, hubos_id_t owner_session_id);
bool hubos_device_server_release_owner(hubos_device_server_t *server);
bool hubos_device_server_quarantine(hubos_device_server_t *server);
bool hubos_device_server_clear_quarantine(hubos_device_server_t *server);
bool hubos_device_server_reset(hubos_device_server_t *server);
bool hubos_device_server_attach_mmio(hubos_device_server_t *server, hubos_id_t owner_session_id);
bool hubos_device_server_attach_irq(hubos_device_server_t *server, hubos_id_t owner_session_id);
bool hubos_device_server_attach_dma(hubos_device_server_t *server, hubos_id_t owner_session_id);

#endif
