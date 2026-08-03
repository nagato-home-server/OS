#ifndef HUBOS_NETWORK_SERVER_H
#define HUBOS_NETWORK_SERVER_H

#include "hubos/hub.h"
#include "hubos/namespace.h"

typedef enum {
  HUBOS_NETWORK_BACKEND_LWIP = 0,
  HUBOS_NETWORK_BACKEND_NETWORKMANAGER,
} hubos_network_backend_kind_t;

typedef struct {
  char *destination;
  size_t destination_len;
  hubos_id_t nic_resource_id;
  unsigned metric;
  bool active;
} hubos_network_route_t;

typedef struct {
  unsigned port;
  hubos_id_t nic_resource_id;
  hubos_id_t session_id;
  bool bound;
} hubos_network_port_binding_t;

typedef struct {
  hubos_id_t id;
  hubos_id_t owner_session_id;
  hubos_namespace_handle_t namespace_handle;
  bool namespace_bound;
  bool routing_enabled;
  bool firewall_enabled;
  bool failover_enabled;
  hubos_id_t selected_nic_resource_id;
  hubos_id_t default_route_nic_resource_id;
  hubos_id_t preferred_nic_resource_id;
  hubos_network_backend_kind_t backend_kind;
  hubos_network_route_t *routes;
  size_t route_count;
  size_t route_capacity;
  hubos_network_port_binding_t *port_bindings;
  size_t port_binding_count;
  size_t port_binding_capacity;
} hubos_network_server_t;

void hubos_network_server_init(hubos_network_server_t *server,
                               hubos_id_t id,
                               hubos_id_t owner_session_id);

void hubos_network_server_destroy(hubos_network_server_t *server);

bool hubos_network_server_set_backend(hubos_network_server_t *server,
                                      hubos_network_backend_kind_t backend_kind);
hubos_network_backend_kind_t hubos_network_server_backend(const hubos_network_server_t *server);

bool hubos_network_server_bind_namespace(hubos_network_server_t *server,
                                         hubos_namespace_handle_t namespace_handle);
bool hubos_network_server_release_namespace(hubos_network_server_t *server);
bool hubos_network_server_finalize_namespace(hubos_network_server_t *server);

bool hubos_network_server_set_policy(hubos_network_server_t *server,
                                     bool routing_enabled,
                                     bool firewall_enabled);

bool hubos_network_server_set_default_route(hubos_network_server_t *server,
                                            hubos_id_t nic_resource_id);

bool hubos_network_server_add_route(hubos_network_server_t *server,
                                    const char *destination,
                                    hubos_id_t nic_resource_id,
                                    unsigned metric);

bool hubos_network_server_select_nic(hubos_network_server_t *server,
                                     const char *destination,
                                     size_t destination_len,
                                     hubos_id_t *out_nic_resource_id);

bool hubos_network_server_bind_port(hubos_network_server_t *server,
                                    unsigned port,
                                    hubos_id_t nic_resource_id,
                                    hubos_id_t session_id);

bool hubos_network_server_set_failover_policy(hubos_network_server_t *server,
                                              bool failover_enabled,
                                              hubos_id_t preferred_nic_resource_id);

bool hubos_network_server_describe(const hubos_network_server_t *server,
                                   hubos_service_descriptor_t *out_descriptor);

bool hubos_network_server_can_relay(const hubos_network_server_t *server);

#endif
