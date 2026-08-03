#include "hubos/device_server.h"

static bool hubos_device_server_call_owner_op(
  const hubos_device_server_ops_t *ops,
  bool (*callback)(void *context, hubos_device_server_t *server, hubos_id_t owner_session_id),
  void *context,
  hubos_device_server_t *server,
  hubos_id_t owner_session_id) {
  (void)ops;

  if (callback == NULL) {
    return true;
  }

  return callback(context, server, owner_session_id);
}

static bool hubos_device_server_call_simple_op(
  const hubos_device_server_ops_t *ops,
  bool (*callback)(void *context, hubos_device_server_t *server),
  void *context,
  hubos_device_server_t *server) {
  (void)ops;

  if (callback == NULL) {
    return true;
  }

  return callback(context, server);
}

void hubos_device_server_init(hubos_device_server_t *server,
                              hubos_id_t id,
                              hubos_id_t owner_session_id,
                              hubos_id_t resource_id,
                              const char *name) {
  if (server == NULL) {
    return;
  }

  server->id = id;
  server->owner_session_id = owner_session_id;
  server->resource_id = resource_id;
  server->name = name;
  server->ops = NULL;
  server->ops_context = NULL;
  server->mmio_owner_session_id = HUBOS_ID_INVALID;
  server->irq_owner_session_id = HUBOS_ID_INVALID;
  server->dma_owner_session_id = HUBOS_ID_INVALID;
  server->mmio_attached = owner_session_id != HUBOS_ID_INVALID;
  server->irq_attached = owner_session_id != HUBOS_ID_INVALID;
  server->dma_attached = owner_session_id != HUBOS_ID_INVALID;
  server->quarantined = false;
  server->running = owner_session_id != HUBOS_ID_INVALID;
}

void hubos_device_server_set_ops(hubos_device_server_t *server,
                                 const hubos_device_server_ops_t *ops,
                                 void *ops_context) {
  if (server == NULL) {
    return;
  }

  server->ops = ops;
  server->ops_context = ops_context;
}

bool hubos_device_server_set_owner(hubos_device_server_t *server, hubos_id_t owner_session_id) {
  const hubos_device_server_ops_t *ops = NULL;

  if (server == NULL || owner_session_id == HUBOS_ID_INVALID) {
    return false;
  }

  ops = server->ops;
  if (!hubos_device_server_call_owner_op(ops, ops != NULL ? ops->set_owner : NULL,
                                         server->ops_context, server, owner_session_id)) {
    return false;
  }

  server->owner_session_id = owner_session_id;
  server->mmio_owner_session_id = owner_session_id;
  server->irq_owner_session_id = owner_session_id;
  server->dma_owner_session_id = owner_session_id;
  server->mmio_attached = true;
  server->irq_attached = true;
  server->dma_attached = true;
  server->running = true;
  server->quarantined = false;
  return true;
}

bool hubos_device_server_release_owner(hubos_device_server_t *server) {
  const hubos_device_server_ops_t *ops = NULL;

  if (server == NULL || !hubos_device_server_has_owner(server)) {
    return false;
  }

  ops = server->ops;
  if (!hubos_device_server_call_simple_op(ops, ops != NULL ? ops->release_owner : NULL,
                                          server->ops_context, server)) {
    return false;
  }

  server->owner_session_id = HUBOS_ID_INVALID;
  server->mmio_owner_session_id = HUBOS_ID_INVALID;
  server->irq_owner_session_id = HUBOS_ID_INVALID;
  server->dma_owner_session_id = HUBOS_ID_INVALID;
  server->mmio_attached = false;
  server->irq_attached = false;
  server->dma_attached = false;
  server->quarantined = false;
  server->running = false;
  return true;
}

bool hubos_device_server_quarantine(hubos_device_server_t *server) {
  const hubos_device_server_ops_t *ops = NULL;

  if (server == NULL || !hubos_device_server_has_owner(server)) {
    return false;
  }

  ops = server->ops;
  if (!hubos_device_server_call_simple_op(ops, ops != NULL ? ops->quarantine : NULL,
                                          server->ops_context, server)) {
    return false;
  }

  server->quarantined = true;
  server->running = false;
  return true;
}

bool hubos_device_server_clear_quarantine(hubos_device_server_t *server) {
  const hubos_device_server_ops_t *ops = NULL;

  if (server == NULL || !hubos_device_server_has_owner(server)) {
    return false;
  }

  ops = server->ops;
  if (!hubos_device_server_call_simple_op(ops, ops != NULL ? ops->clear_quarantine : NULL,
                                          server->ops_context, server)) {
    return false;
  }

  server->quarantined = false;
  server->running = server->mmio_attached || server->irq_attached || server->dma_attached;
  return true;
}

bool hubos_device_server_reset(hubos_device_server_t *server) {
  const hubos_device_server_ops_t *ops = NULL;

  if (server == NULL || !hubos_device_server_has_owner(server)) {
    return false;
  }

  ops = server->ops;
  if (!hubos_device_server_call_simple_op(ops, ops != NULL ? ops->reset : NULL,
                                          server->ops_context, server)) {
    return false;
  }

  server->mmio_owner_session_id = server->owner_session_id;
  server->irq_owner_session_id = server->owner_session_id;
  server->dma_owner_session_id = server->owner_session_id;
  server->mmio_attached = true;
  server->irq_attached = true;
  server->dma_attached = true;
  server->quarantined = false;
  server->running = true;
  return true;
}

bool hubos_device_server_attach_mmio(hubos_device_server_t *server, hubos_id_t owner_session_id) {
  const hubos_device_server_ops_t *ops = NULL;

  if (server == NULL || owner_session_id == HUBOS_ID_INVALID ||
      server->owner_session_id != owner_session_id) {
    return false;
  }

  ops = server->ops;
  if (!hubos_device_server_call_owner_op(ops, ops != NULL ? ops->attach_mmio : NULL,
                                         server->ops_context, server, owner_session_id)) {
    return false;
  }

  server->mmio_owner_session_id = owner_session_id;
  server->mmio_attached = true;
  server->running = !server->quarantined;
  return true;
}

bool hubos_device_server_attach_irq(hubos_device_server_t *server, hubos_id_t owner_session_id) {
  const hubos_device_server_ops_t *ops = NULL;

  if (server == NULL || owner_session_id == HUBOS_ID_INVALID ||
      server->owner_session_id != owner_session_id) {
    return false;
  }

  ops = server->ops;
  if (!hubos_device_server_call_owner_op(ops, ops != NULL ? ops->attach_irq : NULL,
                                         server->ops_context, server, owner_session_id)) {
    return false;
  }

  server->irq_owner_session_id = owner_session_id;
  server->irq_attached = true;
  server->running = !server->quarantined;
  return true;
}

bool hubos_device_server_attach_dma(hubos_device_server_t *server, hubos_id_t owner_session_id) {
  const hubos_device_server_ops_t *ops = NULL;

  if (server == NULL || owner_session_id == HUBOS_ID_INVALID ||
      server->owner_session_id != owner_session_id) {
    return false;
  }

  ops = server->ops;
  if (!hubos_device_server_call_owner_op(ops, ops != NULL ? ops->attach_dma : NULL,
                                         server->ops_context, server, owner_session_id)) {
    return false;
  }

  server->dma_owner_session_id = owner_session_id;
  server->dma_attached = true;
  server->running = !server->quarantined;
  return true;
}
