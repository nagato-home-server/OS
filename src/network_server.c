#include "hubos/network_server.h"

#include <stdlib.h>
#include <string.h>

#ifndef HUBOS_USE_LINUX_VM_NETWORK_BACKEND
#define HUBOS_USE_LINUX_VM_NETWORK_BACKEND 0
#endif

#if HUBOS_USE_LINUX_VM_NETWORK_BACKEND
#define HUBOS_NETWORK_SERVER_DEFAULT_BACKEND HUBOS_NETWORK_BACKEND_NETWORKMANAGER
#else
#define HUBOS_NETWORK_SERVER_DEFAULT_BACKEND HUBOS_NETWORK_BACKEND_LWIP
#endif

static char *hubos_network_server_strdup(const char *text) {
  size_t length = 0;
  char *copy = NULL;

  if (text == NULL) {
    return NULL;
  }

  length = strlen(text);
  copy = malloc(length + 1);
  if (copy == NULL) {
    return NULL;
  }

  memcpy(copy, text, length + 1);
  return copy;
}

static void hubos_network_server_clear_routes(hubos_network_server_t *server) {
  if (server == NULL) {
    return;
  }

  for (size_t index = 0; index < server->route_count; ++index) {
    free(server->routes[index].destination);
  }
  free(server->routes);
  server->routes = NULL;
  server->route_count = 0;
  server->route_capacity = 0;
}

static void hubos_network_server_clear_port_bindings(hubos_network_server_t *server) {
  if (server == NULL) {
    return;
  }

  free(server->port_bindings);
  server->port_bindings = NULL;
  server->port_binding_count = 0;
  server->port_binding_capacity = 0;
}

static bool hubos_network_server_reserve_routes(hubos_network_server_t *server,
                                                size_t desired_capacity) {
  if (server == NULL) {
    return false;
  }

  if (server->route_capacity >= desired_capacity) {
    return true;
  }

  size_t new_capacity = server->route_capacity == 0 ? 4 : server->route_capacity;
  while (new_capacity < desired_capacity) {
    new_capacity *= 2;
  }

  void *new_items = realloc(server->routes, new_capacity * sizeof(*server->routes));
  if (new_items == NULL) {
    return false;
  }

  server->routes = new_items;
  server->route_capacity = new_capacity;
  return true;
}

static bool hubos_network_server_reserve_port_bindings(hubos_network_server_t *server,
                                                       size_t desired_capacity) {
  if (server == NULL) {
    return false;
  }

  if (server->port_binding_capacity >= desired_capacity) {
    return true;
  }

  size_t new_capacity = server->port_binding_capacity == 0 ? 4 : server->port_binding_capacity;
  while (new_capacity < desired_capacity) {
    new_capacity *= 2;
  }

  void *new_items = realloc(server->port_bindings, new_capacity * sizeof(*server->port_bindings));
  if (new_items == NULL) {
    return false;
  }

  server->port_bindings = new_items;
  server->port_binding_capacity = new_capacity;
  return true;
}

static hubos_network_route_t *hubos_network_server_find_route_mutable(
  hubos_network_server_t *server,
  const char *destination,
  size_t destination_len,
  hubos_id_t nic_resource_id) {
  if (server == NULL || destination == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < server->route_count; ++index) {
    hubos_network_route_t *route = &server->routes[index];
    if (route->active && route->destination_len == destination_len &&
        route->nic_resource_id == nic_resource_id &&
        memcmp(route->destination, destination, destination_len) == 0) {
      return route;
    }
  }

  return NULL;
}

static const hubos_network_route_t *hubos_network_server_find_best_route(
  const hubos_network_server_t *server,
  const char *destination,
  size_t destination_len) {
  const hubos_network_route_t *best_route = NULL;

  if (server == NULL || destination == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < server->route_count; ++index) {
    const hubos_network_route_t *route = &server->routes[index];
    if (!route->active || route->destination_len != destination_len ||
        memcmp(route->destination, destination, destination_len) != 0) {
      continue;
    }

    if (best_route == NULL || route->metric < best_route->metric) {
      best_route = route;
    }
  }

  return best_route;
}

static hubos_network_port_binding_t *hubos_network_server_find_port_binding_mutable(
  hubos_network_server_t *server,
  unsigned port) {
  if (server == NULL) {
    return NULL;
  }

  for (size_t index = 0; index < server->port_binding_count; ++index) {
    if (server->port_bindings[index].bound && server->port_bindings[index].port == port) {
      return &server->port_bindings[index];
    }
  }

  return NULL;
}

void hubos_network_server_init(hubos_network_server_t *server,
                               hubos_id_t id,
                               hubos_id_t owner_session_id) {
  if (server == NULL) {
    return;
  }

  server->id = id;
  server->owner_session_id = owner_session_id;
  hubos_namespace_handle_init(&server->namespace_handle,
                              HUBOS_ID_INVALID,
                              HUBOS_NAMESPACE_NETWORK,
                              NULL,
                              true);
  server->namespace_bound = false;
  server->routing_enabled = false;
  server->firewall_enabled = true;
  server->failover_enabled = false;
  server->selected_nic_resource_id = HUBOS_ID_INVALID;
  server->default_route_nic_resource_id = HUBOS_ID_INVALID;
  server->preferred_nic_resource_id = HUBOS_ID_INVALID;
  server->backend_kind = HUBOS_NETWORK_SERVER_DEFAULT_BACKEND;
  server->routes = NULL;
  server->route_count = 0;
  server->route_capacity = 0;
  server->port_bindings = NULL;
  server->port_binding_count = 0;
  server->port_binding_capacity = 0;
}

void hubos_network_server_destroy(hubos_network_server_t *server) {
  if (server == NULL) {
    return;
  }

  hubos_network_server_clear_routes(server);
  hubos_network_server_clear_port_bindings(server);
  server->id = HUBOS_ID_INVALID;
  server->owner_session_id = HUBOS_ID_INVALID;
  hubos_namespace_handle_init(&server->namespace_handle,
                              HUBOS_ID_INVALID,
                              HUBOS_NAMESPACE_NETWORK,
                              NULL,
                              false);
  server->namespace_bound = false;
  server->routing_enabled = false;
  server->firewall_enabled = false;
  server->failover_enabled = false;
  server->selected_nic_resource_id = HUBOS_ID_INVALID;
  server->default_route_nic_resource_id = HUBOS_ID_INVALID;
  server->preferred_nic_resource_id = HUBOS_ID_INVALID;
  server->backend_kind = HUBOS_NETWORK_SERVER_DEFAULT_BACKEND;
}

bool hubos_network_server_set_backend(hubos_network_server_t *server,
                                      hubos_network_backend_kind_t backend_kind) {
  if (server == NULL) {
    return false;
  }

  server->backend_kind = backend_kind;
  return true;
}

hubos_network_backend_kind_t hubos_network_server_backend(const hubos_network_server_t *server) {
  if (server == NULL) {
    return HUBOS_NETWORK_SERVER_DEFAULT_BACKEND;
  }

  return server->backend_kind;
}

bool hubos_network_server_bind_namespace(hubos_network_server_t *server,
                                         hubos_namespace_handle_t namespace_handle) {
  if (server == NULL || namespace_handle.kind != HUBOS_NAMESPACE_NETWORK ||
      !hubos_shared_resource_is_active(&namespace_handle.lifecycle)) {
    return false;
  }

  if (server->namespace_bound &&
      !hubos_namespace_handle_release(&server->namespace_handle)) {
    return false;
  }

  if (!hubos_namespace_handle_bind(&namespace_handle, server->owner_session_id)) {
    return false;
  }

  server->namespace_handle = namespace_handle;
  server->namespace_bound = true;
  return true;
}

bool hubos_network_server_release_namespace(hubos_network_server_t *server) {
  if (server == NULL || !server->namespace_bound) {
    return false;
  }

  if (!hubos_namespace_handle_release(&server->namespace_handle)) {
    return false;
  }

  server->namespace_bound = false;
  return true;
}

bool hubos_network_server_finalize_namespace(hubos_network_server_t *server) {
  if (server == NULL) {
    return false;
  }

  return hubos_namespace_handle_finalize(&server->namespace_handle);
}

bool hubos_network_server_set_policy(hubos_network_server_t *server,
                                     bool routing_enabled,
                                     bool firewall_enabled) {
  if (server == NULL) {
    return false;
  }

  server->routing_enabled = routing_enabled;
  server->firewall_enabled = firewall_enabled;
  if (!routing_enabled) {
    server->selected_nic_resource_id = HUBOS_ID_INVALID;
  }
  return true;
}

bool hubos_network_server_set_default_route(hubos_network_server_t *server,
                                            hubos_id_t nic_resource_id) {
  if (server == NULL || nic_resource_id == HUBOS_ID_INVALID) {
    return false;
  }

  server->default_route_nic_resource_id = nic_resource_id;
  return true;
}

bool hubos_network_server_add_route(hubos_network_server_t *server,
                                    const char *destination,
                                    hubos_id_t nic_resource_id,
                                    unsigned metric) {
  hubos_network_route_t *route = NULL;
  char *destination_copy = NULL;

  if (server == NULL || destination == NULL || destination[0] == '\0' ||
      nic_resource_id == HUBOS_ID_INVALID) {
    return false;
  }

  route = hubos_network_server_find_route_mutable(server,
                                                  destination,
                                                  strlen(destination),
                                                  nic_resource_id);
  if (route != NULL) {
    route->nic_resource_id = nic_resource_id;
    route->metric = metric;
    return true;
  }

  if (!hubos_network_server_reserve_routes(server, server->route_count + 1)) {
    return false;
  }

  destination_copy = hubos_network_server_strdup(destination);
  if (destination_copy == NULL) {
    return false;
  }

  route = &server->routes[server->route_count++];
  route->destination = destination_copy;
  route->destination_len = strlen(destination);
  route->nic_resource_id = nic_resource_id;
  route->metric = metric;
  route->active = true;
  return true;
}

bool hubos_network_server_select_nic(hubos_network_server_t *server,
                                     const char *destination,
                                     size_t destination_len,
                                     hubos_id_t *out_nic_resource_id) {
  const hubos_network_route_t *route = NULL;

  if (server == NULL || out_nic_resource_id == NULL || !server->routing_enabled) {
    return false;
  }

  route = hubos_network_server_find_best_route(server, destination, destination_len);
  if (route != NULL) {
    *out_nic_resource_id = route->nic_resource_id;
    server->selected_nic_resource_id = route->nic_resource_id;
    return true;
  }

  if (server->preferred_nic_resource_id != HUBOS_ID_INVALID && server->failover_enabled) {
    *out_nic_resource_id = server->preferred_nic_resource_id;
    server->selected_nic_resource_id = server->preferred_nic_resource_id;
    return true;
  }

  if (server->default_route_nic_resource_id != HUBOS_ID_INVALID) {
    *out_nic_resource_id = server->default_route_nic_resource_id;
    server->selected_nic_resource_id = server->default_route_nic_resource_id;
    return true;
  }

  return false;
}

bool hubos_network_server_bind_port(hubos_network_server_t *server,
                                    unsigned port,
                                    hubos_id_t nic_resource_id,
                                    hubos_id_t session_id) {
  hubos_network_port_binding_t *binding = NULL;

  if (server == NULL || port == 0 || nic_resource_id == HUBOS_ID_INVALID ||
      session_id == HUBOS_ID_INVALID) {
    return false;
  }

  binding = hubos_network_server_find_port_binding_mutable(server, port);
  if (binding == NULL) {
    if (!hubos_network_server_reserve_port_bindings(server, server->port_binding_count + 1)) {
      return false;
    }
    binding = &server->port_bindings[server->port_binding_count++];
    binding->port = port;
  }

  binding->nic_resource_id = nic_resource_id;
  binding->session_id = session_id;
  binding->bound = true;
  return true;
}

bool hubos_network_server_set_failover_policy(hubos_network_server_t *server,
                                              bool failover_enabled,
                                              hubos_id_t preferred_nic_resource_id) {
  if (server == NULL) {
    return false;
  }

  server->failover_enabled = failover_enabled;
  server->preferred_nic_resource_id = preferred_nic_resource_id;
  return true;
}

bool hubos_network_server_describe(const hubos_network_server_t *server,
                                   hubos_service_descriptor_t *out_descriptor) {
  const char *name = NULL;

  if (server == NULL || out_descriptor == NULL) {
    return false;
  }

  name = server->namespace_handle.name != NULL ? server->namespace_handle.name :
         (server->backend_kind == HUBOS_NETWORK_BACKEND_NETWORKMANAGER ?
            "networkmanager" :
            "network-server");

  out_descriptor->resource_id = server->namespace_bound ? server->namespace_handle.id : server->id;
  out_descriptor->name = name;
  out_descriptor->name_len = strlen(name);
  out_descriptor->resource_state = server->namespace_bound ? HUBOS_RESOURCE_READY :
                                   HUBOS_RESOURCE_DISCOVERED;
  out_descriptor->endpoint = name;
  out_descriptor->version = NULL;
  out_descriptor->policy_hints = (unsigned)(server->namespace_bound ? 1u : 0u) |
                                 (unsigned)(server->routing_enabled ? 2u : 0u) |
                                 (unsigned)(server->firewall_enabled ? 4u : 0u) |
                                 (unsigned)(server->failover_enabled ? 8u : 0u) |
                                 (unsigned)(server->default_route_nic_resource_id != HUBOS_ID_INVALID ? 16u : 0u) |
                                 (unsigned)(server->selected_nic_resource_id != HUBOS_ID_INVALID ? 32u : 0u) |
                                 (unsigned)(server->backend_kind == HUBOS_NETWORK_BACKEND_NETWORKMANAGER ? 64u : 0u);
  return true;
}

bool hubos_network_server_can_relay(const hubos_network_server_t *server) {
  if (server == NULL) {
    return false;
  }

  if (server->backend_kind != HUBOS_NETWORK_BACKEND_NETWORKMANAGER) {
    return false;
  }

  return server->namespace_bound && server->routing_enabled &&
         server->default_route_nic_resource_id != HUBOS_ID_INVALID;
}
