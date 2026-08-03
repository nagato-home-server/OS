#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
out_dir="${1:-$repo_root/microkit-generated}"
source_boundaries="$repo_root/seL4test-manifest/source-boundaries.json"
imported_generated_source="${HUBOS_MICROKIT_GENERATED_SOURCE:-}"

if [ -n "$imported_generated_source" ]; then
  imported_generated_root="$imported_generated_source"
  if [ -d "$imported_generated_root/generated" ]; then
    imported_generated_root="$imported_generated_root/generated"
  fi

  if [ ! -d "$imported_generated_root" ]; then
    echo "Missing imported Microkit generated workspace: $imported_generated_root" >&2
    exit 1
  fi

  rm -rf "$out_dir/generated"
  mkdir -p "$out_dir"
  cp -R "$imported_generated_root" "$out_dir/generated"
  cp "$source_boundaries" "$out_dir/generated/source-boundaries.json"

  printf '%s\n' "Imported Microkit-generated workspace into $out_dir/generated"
  exit 0
fi

mkdir -p "$out_dir/generated"

cat > "$out_dir/generated/manifest.json" <<'EOF'
{
  "name": "hubos-microkit",
  "entrypoints": [
    {"name": "Root Task", "badge": 0, "bootstrap_only": true, "channels": ["endpoint"]},
    {"name": "Resource Registry", "badge": 1, "bootstrap_only": false, "channels": ["endpoint"]},
    {"name": "Capability Manager", "badge": 2, "bootstrap_only": false, "channels": ["endpoint"]},
    {"name": "Session Manager", "badge": 3, "bootstrap_only": false, "channels": ["endpoint"]},
    {"name": "Memory Manager", "badge": 7, "bootstrap_only": false, "channels": ["internal"]},
    {"name": "DMA Manager", "badge": 8, "bootstrap_only": false, "channels": ["notification", "shared-memory"]},
    {"name": "Hub", "badge": 4, "bootstrap_only": false, "channels": ["endpoint"]},
    {"name": "Driver Registry", "badge": 9, "bootstrap_only": false, "channels": ["internal"]},
    {"name": "Driver Loader", "badge": 10, "bootstrap_only": false, "channels": ["notification"]},
    {"name": "Bus Managers", "badge": 11, "bootstrap_only": false, "channels": ["notification"]},
    {"name": "Driver Service", "badge": 5, "bootstrap_only": false, "channels": ["endpoint", "notification"]},
    {"name": "Network Server", "badge": 6, "bootstrap_only": false, "channels": ["endpoint", "notification", "shared-memory"]},
    {"name": "Device Server", "badge": 12, "bootstrap_only": false, "channels": ["endpoint", "notification", "irq", "shared-memory"]},
    {"name": "Storage Server", "badge": 13, "bootstrap_only": false, "channels": ["endpoint", "notification", "shared-memory"]},
    {"name": "Display Server", "badge": 14, "bootstrap_only": false, "channels": ["endpoint", "notification", "shared-memory"]}
  ]
}
EOF

cp "$source_boundaries" "$out_dir/generated/source-boundaries.json"

emit_component() {
  component_dir="$1"
  component_name="$2"
  badge="$3"
  bootstrap_only="$4"
  restartable="$5"
  channels_json="$6"
  channels_label="$7"
  phase="$8"
  dependencies_json="$9"
  dependencies_label="${10}"
  endpoint_published="${11}"
  notification_published="${12}"
  irq_published="${13}"
  shared_memory_published="${14}"
  component_kind="${15}"

  mkdir -p "$out_dir/generated/$component_dir"

  cat > "$out_dir/generated/$component_dir/component.json" <<EOF
{
  "name": "${component_name}",
  "badge": ${badge},
  "phase": "${phase}",
  "bootstrap_only": ${bootstrap_only},
  "restartable": ${restartable},
  "dependencies": [${dependencies_json}],
  "channels": [${channels_json}],
  "endpoint_published": ${endpoint_published},
  "notification_published": ${notification_published},
  "irq_published": ${irq_published},
  "shared_memory_published": ${shared_memory_published}
}
EOF

  cat > "$out_dir/generated/$component_dir/component.h" <<EOF
#ifndef HUBOS_GENERATED_COMPONENT_H
#define HUBOS_GENERATED_COMPONENT_H

#define HUBOS_MICROKIT_COMPONENT_NAME "${component_name}"
#define HUBOS_MICROKIT_COMPONENT_KIND ${component_kind}
#define HUBOS_MICROKIT_COMPONENT_BADGE ${badge}
#define HUBOS_MICROKIT_COMPONENT_BOOTSTRAP_ONLY ${bootstrap_only}
#define HUBOS_MICROKIT_COMPONENT_RESTARTABLE ${restartable}
#define HUBOS_MICROKIT_COMPONENT_PHASE "${phase}"
#define HUBOS_MICROKIT_COMPONENT_ENDPOINT_PUBLISHED ${endpoint_published}
#define HUBOS_MICROKIT_COMPONENT_NOTIFICATION_PUBLISHED ${notification_published}
#define HUBOS_MICROKIT_COMPONENT_IRQ_PUBLISHED ${irq_published}
#define HUBOS_MICROKIT_COMPONENT_SHARED_MEMORY_PUBLISHED ${shared_memory_published}
#define HUBOS_MICROKIT_COMPONENT_TRANSPORT_CALL_LABEL 0
#define HUBOS_MICROKIT_COMPONENT_TRANSPORT_MAX_WORDS 24

#endif
EOF

  cat > "$out_dir/generated/$component_dir/channel-map.h" <<'EOF'
#ifndef HUBOS_GENERATED_CHANNEL_MAP_H
#define HUBOS_GENERATED_CHANNEL_MAP_H

static const microkit_channel hubos_generated_incoming_endpoint_channels[1] = {0};
static const microkit_channel hubos_generated_incoming_notification_channels[1] = {0};

#define HUBOS_GENERATED_INCOMING_ENDPOINT_COUNT 0
#define HUBOS_GENERATED_INCOMING_NOTIFICATION_COUNT 0

#define HUBOS_GENERATED_ENDPOINT_HANDSHAKE_ENABLED 0
#define HUBOS_GENERATED_ENDPOINT_HANDSHAKE_CHANNEL 0
#define HUBOS_GENERATED_ENDPOINT_HANDSHAKE_SERVICE HUBOS_MICROKIT_COMPONENT_ROOT_TASK
#define HUBOS_GENERATED_ENDPOINT_HANDSHAKE_NAME "none"

#define HUBOS_GENERATED_NOTIFY_HANDSHAKE_ENABLED 0
#define HUBOS_GENERATED_NOTIFY_HANDSHAKE_CHANNEL 0
#define HUBOS_GENERATED_NOTIFY_HANDSHAKE_NAME "none"

#endif
EOF

  if [ "$component_dir" = "device-server" ] || [ "$component_dir" = "bus-managers" ]; then
    cat > "$out_dir/generated/$component_dir/main.c" <<EOF
#include <microkit.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "component.h"
#include "channel-map.h"
#include "hubos/microkit_kernel_glue.h"
#include "hubos/microkit_transport.h"
#include "hubos/microkit_runtime.h"
#include "hubos/system.h"
/*
 * Generated Microkit component stub for ${component_name}.
 * Badge: ${badge}
 * Phase: ${phase}
 * Bootstrap-only: ${bootstrap_only}
 * Restartable: ${restartable}
 * Dependencies: ${dependencies_label}
 * Channels: ${channels_label}
 * Endpoint published: ${endpoint_published}
 * Notification published: ${notification_published}
 * IRQ published: ${irq_published}
 * Shared memory published: ${shared_memory_published}
 */
static hubos_system_t hubos_generated_system;
static hubos_microkit_runtime_t hubos_generated_runtime;

static void hubos_generated_bootstrap(void) {
  hubos_system_init(&hubos_generated_system, "root-key");
  hubos_microkit_runtime_init(&hubos_generated_runtime, &hubos_generated_system);
  (void)hubos_microkit_kernel_bootstrap(&hubos_generated_runtime);
}

static bool hubos_generated_supports_endpoint(void) {
  return HUBOS_MICROKIT_COMPONENT_ENDPOINT_PUBLISHED;
}

static bool hubos_generated_supports_notification(void) {
  return HUBOS_MICROKIT_COMPONENT_NOTIFICATION_PUBLISHED;
}

static bool hubos_generated_supports_faults(void) {
  return HUBOS_MICROKIT_COMPONENT_IRQ_PUBLISHED || HUBOS_MICROKIT_COMPONENT_SHARED_MEMORY_PUBLISHED;
}

static bool hubos_generated_matches_endpoint_channel(microkit_channel ch) {
#if HUBOS_GENERATED_INCOMING_ENDPOINT_COUNT > 0
  for (size_t index = 0; index < HUBOS_GENERATED_INCOMING_ENDPOINT_COUNT; ++index) {
    if (hubos_generated_incoming_endpoint_channels[index] == ch) {
      return true;
    }
  }
#endif
  return false;
}

static bool hubos_generated_matches_notification_channel(microkit_channel ch) {
#if HUBOS_GENERATED_INCOMING_NOTIFICATION_COUNT > 0
  for (size_t index = 0; index < HUBOS_GENERATED_INCOMING_NOTIFICATION_COUNT; ++index) {
    if (hubos_generated_incoming_notification_channels[index] == ch) {
      return true;
    }
  }
#endif
  return false;
}

static bool hubos_generated_matches_service(const hubos_microkit_ipc_request_t *request) {
  return request != NULL && request->service == HUBOS_MICROKIT_COMPONENT_KIND;
}

static bool hubos_generated_build_handshake_request(hubos_microkit_ipc_request_t *request) {
  if (request == NULL) {
    return false;
  }

  *request = (hubos_microkit_ipc_request_t){0};
  request->service = HUBOS_GENERATED_ENDPOINT_HANDSHAKE_SERVICE;
  request->operation = 0;

  switch (HUBOS_GENERATED_ENDPOINT_HANDSHAKE_SERVICE) {
  case HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY:
    request->operation = HUBOS_MICROKIT_RESOURCE_OP_DESCRIBE;
    request->payload.resource_describe.resource_id = 1;
    return true;
  case HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER:
    request->operation = HUBOS_MICROKIT_CAPABILITY_OP_AUTHORIZE;
    request->payload.capability_authorize.capability_id = 1;
    request->payload.capability_authorize.resource_id = 1;
    request->payload.capability_authorize.required_rights = 1;
    return true;
  case HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER:
    request->operation = HUBOS_MICROKIT_SESSION_OP_CHILD_COUNT;
    request->payload.session_child_count.session_id = 1;
    return true;
  case HUBOS_MICROKIT_COMPONENT_HUB:
    request->operation = HUBOS_MICROKIT_HUB_OP_RESOLVE;
    request->payload.hub_resolve.name = "service://handshake";
    request->payload.hub_resolve.name_len = 19;
    return true;
  case HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE:
    request->operation = HUBOS_MICROKIT_DRIVER_OP_QUARANTINE;
    request->payload.driver_quarantine.resource_id = 1;
    return true;
  case HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER:
    request->operation = HUBOS_MICROKIT_NETWORK_OP_DESCRIBE;
    return true;
  case HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER:
    request->operation = HUBOS_MICROKIT_DEVICE_OP_DESCRIBE;
    return true;
  case HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER:
    request->operation = HUBOS_MICROKIT_STORAGE_OP_DESCRIBE;
    return true;
  case HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER:
    request->operation = HUBOS_MICROKIT_DISPLAY_OP_DESCRIBE;
    return true;
  default:
    return false;
  }
}

static void hubos_generated_emit_notification_handshake(void) {
  if (!HUBOS_GENERATED_NOTIFY_HANDSHAKE_ENABLED) {
    return;
  }

  microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
  microkit_dbg_puts(": notify->");
  microkit_dbg_puts(HUBOS_GENERATED_NOTIFY_HANDSHAKE_NAME);
  microkit_dbg_puts(" ch=");
  microkit_dbg_put32(HUBOS_GENERATED_NOTIFY_HANDSHAKE_CHANNEL);
  microkit_dbg_puts("\n");
  microkit_notify(HUBOS_GENERATED_NOTIFY_HANDSHAKE_CHANNEL);
}

static void hubos_generated_emit_endpoint_handshake(void) {
  hubos_microkit_ipc_request_t request;
  hubos_microkit_transport_frame_t frame;
  microkit_msginfo request_msginfo;
  microkit_msginfo reply_msginfo;

  if (!HUBOS_GENERATED_ENDPOINT_HANDSHAKE_ENABLED ||
      !hubos_generated_build_handshake_request(&request)) {
    return;
  }

  hubos_microkit_transport_frame_init(&frame);
  if (!hubos_microkit_transport_request_encode(&request, &frame)) {
    microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
    microkit_dbg_puts(": handshake encode failed\n");
    return;
  }

  hubos_microkit_transport_frame_to_mrs(&frame);
  request_msginfo = hubos_microkit_transport_frame_to_msginfo(&frame, HUBOS_MICROKIT_TRANSPORT_LABEL);
  microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
  microkit_dbg_puts(": ppcall->");
  microkit_dbg_puts(HUBOS_GENERATED_ENDPOINT_HANDSHAKE_NAME);
  microkit_dbg_puts(" ch=");
  microkit_dbg_put32(HUBOS_GENERATED_ENDPOINT_HANDSHAKE_CHANNEL);
  microkit_dbg_puts("\n");
  reply_msginfo = microkit_ppcall(HUBOS_GENERATED_ENDPOINT_HANDSHAKE_CHANNEL, request_msginfo);
  microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
  microkit_dbg_puts(": ppreply label=");
  microkit_dbg_put32((seL4_Word)microkit_msginfo_get_label(reply_msginfo));
  microkit_dbg_puts(" count=");
  microkit_dbg_put32((seL4_Word)microkit_msginfo_get_count(reply_msginfo));
  microkit_dbg_puts("\n");
}

static void hubos_generated_trace_message(const char *kind,
                                          microkit_channel ch,
                                          microkit_msginfo msginfo) {
  microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
  microkit_dbg_puts(": ");
  microkit_dbg_puts(kind);
  microkit_dbg_puts(" ch=");
  microkit_dbg_put32(ch);
  microkit_dbg_puts(" label=");
  microkit_dbg_put32((seL4_Word)microkit_msginfo_get_label(msginfo));
  microkit_dbg_puts(" count=");
  microkit_dbg_put32((seL4_Word)microkit_msginfo_get_count(msginfo));
  microkit_dbg_puts("\n");
}

static microkit_msginfo hubos_generated_reject_call(microkit_channel ch,
                                                    microkit_msginfo msginfo) {
  hubos_generated_trace_message("reject", ch, msginfo);
  return microkit_msginfo_new(0, 0);
}

static bool hubos_generated_dispatch_notification(microkit_channel ch) {
  microkit_msginfo msginfo = microkit_msginfo_new(0, 0);

  if (!hubos_generated_supports_notification() || !hubos_generated_matches_notification_channel(ch)) {
    return false;
  }

  hubos_generated_trace_message("notification", ch, msginfo);
  return hubos_microkit_kernel_dispatch_notification(&hubos_generated_runtime,
                                                     HUBOS_MICROKIT_COMPONENT_BADGE);
}

static void hubos_generated_handle_notification(microkit_channel ch) {
  (void)hubos_generated_dispatch_notification(ch);
}

void init(void) {
  hubos_generated_bootstrap();
  microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
  microkit_dbg_puts(": init\n");
  hubos_generated_emit_notification_handshake();
  hubos_generated_emit_endpoint_handshake();
}

void notified(microkit_channel ch) {
  hubos_generated_handle_notification(ch);
}

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo) {
  microkit_msginfo reply_msginfo = microkit_msginfo_new(0, 0);

  if (!hubos_generated_supports_endpoint() || !hubos_generated_matches_endpoint_channel(ch)) {
    return hubos_generated_reject_call(ch, msginfo);
  }

  if (!hubos_microkit_kernel_dispatch_protected(&hubos_generated_runtime,
                                                HUBOS_MICROKIT_COMPONENT_BADGE,
                                                msginfo,
                                                &reply_msginfo)) {
    return hubos_generated_reject_call(ch, msginfo);
  }

  return reply_msginfo;
}

seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo) {
  if (reply_msginfo != NULL) {
    *reply_msginfo = microkit_msginfo_new(0, 0);
  }

  if (hubos_generated_supports_faults()) {
    hubos_generated_trace_message("fault", (microkit_channel)child, msginfo);
  } else {
    hubos_generated_trace_message("fault-unexpected", (microkit_channel)child, msginfo);
  }

  return seL4_False;
}
EOF
  else
    cat > "$out_dir/generated/$component_dir/main.c" <<EOF
#include <microkit.h>
#include <stdbool.h>
#include <stddef.h>
#include "component.h"
#include "channel-map.h"
#include "hubos/microkit_transport.h"

/*
 * Generated Microkit component stub for ${component_name}.
 * Badge: ${badge}
 * Phase: ${phase}
 * Bootstrap-only: ${bootstrap_only}
 * Restartable: ${restartable}
 * Dependencies: ${dependencies_label}
 * Channels: ${channels_label}
 * Endpoint published: ${endpoint_published}
 * Notification published: ${notification_published}
 * IRQ published: ${irq_published}
 * Shared memory published: ${shared_memory_published}
 *
 * This file is intentionally shaped like a Microkit entrypoint so it can be
 * copied into a real Microkit workspace and replaced with SDK-generated
 * output later.
 *
 * The current transport shape is deliberately concrete:
 * - microkit_msginfo label 0 is the protected-call envelope
 * - hubos_microkit_transport_frame_t carries the payload words
 * - protected() round-trips the frame through the transport helpers so the
 *   callback body matches the upstream Microkit event loop that drives the
 *   generated image under QEMU
 */

static void hubos_generated_bootstrap(void) {
  (void)HUBOS_MICROKIT_COMPONENT_NAME;
}

static bool hubos_generated_supports_endpoint(void) {
  return HUBOS_MICROKIT_COMPONENT_ENDPOINT_PUBLISHED;
}

static bool hubos_generated_supports_notification(void) {
  return HUBOS_MICROKIT_COMPONENT_NOTIFICATION_PUBLISHED;
}

static bool hubos_generated_supports_faults(void) {
  return HUBOS_MICROKIT_COMPONENT_IRQ_PUBLISHED || HUBOS_MICROKIT_COMPONENT_SHARED_MEMORY_PUBLISHED;
}

static bool hubos_generated_matches_endpoint_channel(microkit_channel ch) {
#if HUBOS_GENERATED_INCOMING_ENDPOINT_COUNT > 0
  for (size_t index = 0; index < HUBOS_GENERATED_INCOMING_ENDPOINT_COUNT; ++index) {
    if (hubos_generated_incoming_endpoint_channels[index] == ch) {
      return true;
    }
  }
#endif
  return false;
}

static bool hubos_generated_matches_notification_channel(microkit_channel ch) {
#if HUBOS_GENERATED_INCOMING_NOTIFICATION_COUNT > 0
  for (size_t index = 0; index < HUBOS_GENERATED_INCOMING_NOTIFICATION_COUNT; ++index) {
    if (hubos_generated_incoming_notification_channels[index] == ch) {
      return true;
    }
  }
#endif
  return false;
}

static bool hubos_generated_matches_service(const hubos_microkit_ipc_request_t *request) {
  return request != NULL && request->service == HUBOS_MICROKIT_COMPONENT_KIND;
}

static bool hubos_generated_build_handshake_request(hubos_microkit_ipc_request_t *request) {
  if (request == NULL) {
    return false;
  }

  *request = (hubos_microkit_ipc_request_t){0};
  request->service = HUBOS_GENERATED_ENDPOINT_HANDSHAKE_SERVICE;
  request->operation = 0;

  switch (HUBOS_GENERATED_ENDPOINT_HANDSHAKE_SERVICE) {
  case HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY:
    request->operation = HUBOS_MICROKIT_RESOURCE_OP_DESCRIBE;
    request->payload.resource_describe.resource_id = 1;
    return true;
  case HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER:
    request->operation = HUBOS_MICROKIT_CAPABILITY_OP_AUTHORIZE;
    request->payload.capability_authorize.capability_id = 1;
    request->payload.capability_authorize.resource_id = 1;
    request->payload.capability_authorize.required_rights = 1;
    return true;
  case HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER:
    request->operation = HUBOS_MICROKIT_SESSION_OP_CHILD_COUNT;
    request->payload.session_child_count.session_id = 1;
    return true;
  case HUBOS_MICROKIT_COMPONENT_HUB:
    request->operation = HUBOS_MICROKIT_HUB_OP_RESOLVE;
    request->payload.hub_resolve.name = "service://handshake";
    request->payload.hub_resolve.name_len = 19;
    return true;
  case HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE:
    request->operation = HUBOS_MICROKIT_DRIVER_OP_QUARANTINE;
    request->payload.driver_quarantine.resource_id = 1;
    return true;
  case HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER:
    request->operation = HUBOS_MICROKIT_NETWORK_OP_DESCRIBE;
    return true;
  case HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER:
    request->operation = HUBOS_MICROKIT_DEVICE_OP_DESCRIBE;
    return true;
  case HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER:
    request->operation = HUBOS_MICROKIT_STORAGE_OP_DESCRIBE;
    return true;
  case HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER:
    request->operation = HUBOS_MICROKIT_DISPLAY_OP_DESCRIBE;
    return true;
  default:
    return false;
  }
}

typedef hubos_microkit_transport_frame_t hubos_generated_transport_frame_t;

static void hubos_generated_trace_message(const char *kind,
                                          microkit_channel ch,
                                          microkit_msginfo msginfo,
                                          const hubos_generated_transport_frame_t *frame) {
  seL4_Uint16 count = frame != NULL ? frame->count : (seL4_Uint16)microkit_msginfo_get_count(msginfo);

  microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
  microkit_dbg_puts(": ");
  microkit_dbg_puts(kind);
  microkit_dbg_puts(" ch=");
  microkit_dbg_put32(ch);
  microkit_dbg_puts(" label=");
  microkit_dbg_put32((seL4_Word)microkit_msginfo_get_label(msginfo));
  microkit_dbg_puts(" count=");
  microkit_dbg_put32((seL4_Word)count);
  for (seL4_Uint16 index = 0; index < count && index < HUBOS_MICROKIT_COMPONENT_TRANSPORT_MAX_WORDS;
       ++index) {
    microkit_dbg_puts(" mr");
    microkit_dbg_put32((seL4_Word)index);
    microkit_dbg_puts("=");
    microkit_dbg_put32(frame != NULL ? frame->words[index] : microkit_mr_get((seL4_Uint8)index));
  }
  microkit_dbg_puts("\n");
}

static bool hubos_generated_transport_read(microkit_msginfo msginfo,
                                           hubos_generated_transport_frame_t *frame) {
  if (frame == NULL) {
    return false;
  }

  return hubos_microkit_transport_frame_from_msginfo(frame, msginfo);
}

static void hubos_generated_transport_write(const hubos_generated_transport_frame_t *frame) {
  hubos_microkit_transport_frame_to_mrs(frame);
}

static microkit_msginfo hubos_generated_reject_call(microkit_channel ch,
                                                    microkit_msginfo msginfo) {
  hubos_generated_trace_message("reject", ch, msginfo, NULL);
  return microkit_msginfo_new(0, 0);
}

static bool hubos_generated_dispatch_notification(microkit_channel ch) {
  microkit_msginfo msginfo = microkit_msginfo_new(0, 0);
  hubos_generated_transport_frame_t frame;

  if (!hubos_generated_supports_notification() || !hubos_generated_matches_notification_channel(ch)) {
    return false;
  }

  hubos_microkit_transport_frame_init(&frame);
  hubos_generated_trace_message("notification", ch, msginfo, &frame);
  return true;
}

static void hubos_generated_handle_notification(microkit_channel ch) {
  (void)hubos_generated_dispatch_notification(ch);
}

static void hubos_generated_emit_notification_handshake(void) {
  if (!HUBOS_GENERATED_NOTIFY_HANDSHAKE_ENABLED) {
    return;
  }

  microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
  microkit_dbg_puts(": notify->");
  microkit_dbg_puts(HUBOS_GENERATED_NOTIFY_HANDSHAKE_NAME);
  microkit_dbg_puts(" ch=");
  microkit_dbg_put32(HUBOS_GENERATED_NOTIFY_HANDSHAKE_CHANNEL);
  microkit_dbg_puts("\n");
  microkit_notify(HUBOS_GENERATED_NOTIFY_HANDSHAKE_CHANNEL);
}

static void hubos_generated_emit_endpoint_handshake(void) {
  hubos_microkit_ipc_request_t request;
  hubos_generated_transport_frame_t request_frame;
  microkit_msginfo request_msginfo;
  microkit_msginfo reply_msginfo;
  hubos_generated_transport_frame_t reply_frame;
  hubos_microkit_ipc_response_t response;

  if (!HUBOS_GENERATED_ENDPOINT_HANDSHAKE_ENABLED ||
      !hubos_generated_build_handshake_request(&request)) {
    return;
  }

  hubos_microkit_transport_frame_init(&request_frame);
  if (!hubos_microkit_transport_request_encode(&request, &request_frame)) {
    microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
    microkit_dbg_puts(": handshake encode failed\n");
    return;
  }

  hubos_microkit_transport_frame_to_mrs(&request_frame);
  request_msginfo =
    hubos_microkit_transport_frame_to_msginfo(&request_frame, HUBOS_MICROKIT_TRANSPORT_LABEL);
  microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
  microkit_dbg_puts(": ppcall->");
  microkit_dbg_puts(HUBOS_GENERATED_ENDPOINT_HANDSHAKE_NAME);
  microkit_dbg_puts(" ch=");
  microkit_dbg_put32(HUBOS_GENERATED_ENDPOINT_HANDSHAKE_CHANNEL);
  microkit_dbg_puts("\n");
  reply_msginfo = microkit_ppcall(HUBOS_GENERATED_ENDPOINT_HANDSHAKE_CHANNEL, request_msginfo);
  microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
  microkit_dbg_puts(": ppreply label=");
  microkit_dbg_put32((seL4_Word)microkit_msginfo_get_label(reply_msginfo));
  microkit_dbg_puts(" count=");
  microkit_dbg_put32((seL4_Word)microkit_msginfo_get_count(reply_msginfo));
  if (hubos_microkit_transport_frame_from_msginfo(&reply_frame, reply_msginfo) &&
      hubos_microkit_transport_response_decode(&reply_frame, &response)) {
    microkit_dbg_puts(" status=");
    microkit_dbg_put32((seL4_Word)response.status);
  }
  microkit_dbg_puts("\n");
}

static microkit_msginfo hubos_generated_handle_bootstrap_call(microkit_channel ch,
                                                               microkit_msginfo msginfo) {
  (void)ch;
  return hubos_generated_reject_call(ch, msginfo);
}

static microkit_msginfo hubos_generated_handle_service_call(microkit_channel ch,
                                                            microkit_msginfo msginfo) {
  hubos_generated_transport_frame_t request_frame;
  hubos_generated_transport_frame_t response_frame;
  hubos_microkit_ipc_request_t request;
  hubos_microkit_ipc_response_t response;
  microkit_msginfo response_msginfo;

  hubos_microkit_transport_frame_init(&request_frame);
  if (!hubos_generated_transport_read(msginfo, &request_frame)) {
    return hubos_generated_reject_call(ch, msginfo);
  }

  if (!hubos_microkit_transport_request_decode(&request_frame, &request)) {
    return hubos_generated_reject_call(ch, msginfo);
  }

  hubos_generated_trace_message("request", ch, msginfo, &request_frame);

  if (!hubos_generated_matches_service(&request)) {
    return hubos_generated_reject_call(ch, msginfo);
  }

  if (!hubos_microkit_transport_synthesize_response(&request, &response)) {
    return hubos_generated_reject_call(ch, msginfo);
  }

  hubos_microkit_transport_frame_init(&response_frame);
  if (!hubos_microkit_transport_response_encode(&response, &response_frame)) {
    return hubos_generated_reject_call(ch, msginfo);
  }

  response_msginfo =
    hubos_microkit_transport_frame_to_msginfo(&response_frame,
                                              HUBOS_MICROKIT_COMPONENT_TRANSPORT_CALL_LABEL);
  hubos_generated_trace_message("response", ch, response_msginfo, &response_frame);
  hubos_generated_transport_write(&response_frame);
  return response_msginfo;
}

static microkit_msginfo hubos_generated_handle_protected_call(
  microkit_channel ch,
  microkit_msginfo msginfo) {
  if (!hubos_generated_supports_endpoint()) {
    return hubos_generated_reject_call(ch, msginfo);
  }

  if (!hubos_generated_matches_endpoint_channel(ch)) {
    return hubos_generated_reject_call(ch, msginfo);
  }

  switch (microkit_msginfo_get_label(msginfo)) {
  case 0:
    if (HUBOS_MICROKIT_COMPONENT_BOOTSTRAP_ONLY) {
      return hubos_generated_handle_bootstrap_call(ch, msginfo);
    }
    return hubos_generated_handle_service_call(ch, msginfo);
  default:
    return hubos_generated_reject_call(ch, msginfo);
  }
}

void init(void) {
  hubos_generated_bootstrap();
  microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
  microkit_dbg_puts(": entering init\n");
  microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
  microkit_dbg_puts(": init\n");
  hubos_generated_emit_notification_handshake();
  hubos_generated_emit_endpoint_handshake();
  microkit_dbg_puts(HUBOS_MICROKIT_COMPONENT_NAME);
  microkit_dbg_puts(": init complete\n");
}

void notified(microkit_channel ch) {
  hubos_generated_handle_notification(ch);
}

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo) {
  return hubos_generated_handle_protected_call(ch, msginfo);
}

seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo) {
  if (reply_msginfo != NULL) {
    *reply_msginfo = microkit_msginfo_new(0, 0);
  }

  if (hubos_generated_supports_faults()) {
    hubos_generated_trace_message("fault", (microkit_channel)child, msginfo, NULL);
  } else {
    hubos_generated_trace_message("fault-unexpected", (microkit_channel)child, msginfo, NULL);
  }

  return seL4_False;
}
EOF
  fi
}

emit_component "root-task" "Root Task" "0" "true" "false" "\"endpoint\"" "endpoint" "bootstrap" "" "none" "true" "false" "false" "false" "HUBOS_MICROKIT_COMPONENT_ROOT_TASK"
emit_component "resource-registry" "Resource Registry" "1" "false" "true" "\"endpoint\"" "endpoint" "core" "\"HUBOS_MICROKIT_COMPONENT_ROOT_TASK\"" "Root Task" "true" "false" "false" "false" "HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY"
emit_component "capability-manager" "Capability Manager" "2" "false" "true" "\"endpoint\"" "endpoint" "core" "\"HUBOS_MICROKIT_COMPONENT_ROOT_TASK\",\"HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY\"" "Root Task, Resource Registry" "true" "false" "false" "false" "HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER"
emit_component "session-manager" "Session Manager" "3" "false" "true" "\"endpoint\"" "endpoint" "core" "\"HUBOS_MICROKIT_COMPONENT_ROOT_TASK\",\"HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER\"" "Root Task, Capability Manager" "true" "false" "false" "false" "HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER"
emit_component "memory-manager" "Memory Manager" "7" "false" "true" "\"internal\"" "internal" "core" "\"HUBOS_MICROKIT_COMPONENT_ROOT_TASK\",\"HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY\"" "Root Task, Resource Registry" "false" "false" "false" "false" "HUBOS_MICROKIT_COMPONENT_MEMORY_MANAGER"
emit_component "dma-manager" "DMA Manager" "8" "false" "true" "\"notification\",\"shared-memory\"" "notification, shared-memory" "support" "\"HUBOS_MICROKIT_COMPONENT_ROOT_TASK\",\"HUBOS_MICROKIT_COMPONENT_MEMORY_MANAGER\",\"HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY\"" "Root Task, Memory Manager, Resource Registry" "false" "true" "false" "true" "HUBOS_MICROKIT_COMPONENT_DMA_MANAGER"
emit_component "hub" "Hub" "4" "false" "true" "\"endpoint\"" "endpoint" "core" "\"HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY\",\"HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER\",\"HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER\"" "Resource Registry, Capability Manager, Session Manager" "true" "false" "false" "false" "HUBOS_MICROKIT_COMPONENT_HUB"
emit_component "driver-registry" "Driver Registry" "9" "false" "true" "\"internal\"" "internal" "support" "\"HUBOS_MICROKIT_COMPONENT_ROOT_TASK\",\"HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY\"" "Root Task, Resource Registry" "false" "false" "false" "false" "HUBOS_MICROKIT_COMPONENT_DRIVER_REGISTRY"
emit_component "driver-loader" "Driver Loader" "10" "false" "true" "\"notification\"" "notification" "support" "\"HUBOS_MICROKIT_COMPONENT_ROOT_TASK\",\"HUBOS_MICROKIT_COMPONENT_DRIVER_REGISTRY\"" "Root Task, Driver Registry" "false" "true" "false" "false" "HUBOS_MICROKIT_COMPONENT_DRIVER_LOADER"
emit_component "bus-managers" "Bus Managers" "11" "false" "true" "\"endpoint\",\"notification\"" "endpoint, notification" "support" "\"HUBOS_MICROKIT_COMPONENT_ROOT_TASK\",\"HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY\",\"HUBOS_MICROKIT_COMPONENT_DRIVER_REGISTRY\"" "Root Task, Resource Registry, Driver Registry" "true" "true" "false" "false" "HUBOS_MICROKIT_COMPONENT_BUS_MANAGERS"
emit_component "driver-service" "Driver Service" "5" "false" "true" "\"endpoint\",\"notification\"" "endpoint, notification" "support" "\"HUBOS_MICROKIT_COMPONENT_ROOT_TASK\",\"HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER\",\"HUBOS_MICROKIT_COMPONENT_DRIVER_REGISTRY\",\"HUBOS_MICROKIT_COMPONENT_DRIVER_LOADER\"" "Root Task, Capability Manager, Driver Registry, Driver Loader" "true" "true" "false" "false" "HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE"
emit_component "network-server" "Network Server" "6" "false" "true" "\"endpoint\",\"notification\",\"shared-memory\"" "endpoint, notification, shared-memory" "device" "\"HUBOS_MICROKIT_COMPONENT_ROOT_TASK\",\"HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER\",\"HUBOS_MICROKIT_COMPONENT_HUB\",\"HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY\"" "Root Task, Session Manager, Hub, Resource Registry" "true" "true" "false" "true" "HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER"
emit_component "device-server" "Device Server" "12" "false" "true" "\"endpoint\",\"notification\",\"irq\",\"shared-memory\"" "endpoint, notification, irq, shared-memory" "device" "\"HUBOS_MICROKIT_COMPONENT_ROOT_TASK\",\"HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER\",\"HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER\",\"HUBOS_MICROKIT_COMPONENT_DMA_MANAGER\",\"HUBOS_MICROKIT_COMPONENT_BUS_MANAGERS\"" "Root Task, Capability Manager, Session Manager, DMA Manager, Bus Managers" "true" "true" "true" "true" "HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER"
emit_component "storage-server" "Storage Server" "13" "false" "true" "\"endpoint\",\"notification\",\"shared-memory\"" "endpoint, notification, shared-memory" "device" "\"HUBOS_MICROKIT_COMPONENT_ROOT_TASK\",\"HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER\",\"HUBOS_MICROKIT_COMPONENT_HUB\",\"HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY\"" "Root Task, Session Manager, Hub, Resource Registry" "true" "true" "false" "true" "HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER"
emit_component "display-server" "Display Server" "14" "false" "true" "\"endpoint\",\"notification\",\"shared-memory\"" "endpoint, notification, shared-memory" "device" "\"HUBOS_MICROKIT_COMPONENT_ROOT_TASK\",\"HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER\",\"HUBOS_MICROKIT_COMPONENT_HUB\",\"HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY\"" "Root Task, Session Manager, Hub, Resource Registry" "true" "true" "false" "true" "HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER"

  cat > "$out_dir/generated/README.md" <<'EOF'
# Generated Microkit Sources

This directory is generated from the HubOS host-side boot manifest.

Each component directory contains:

- `component.json` with boot, dependency, and channel metadata
- `component.h` with generated callback metadata constants
- a Microkit-shaped `main.c` stub that exposes the protected-call
  message-register envelope used by the QEMU image
- a round-trip transport path that decodes typed service requests and encodes
  typed service responses so the generated image can be exercised under QEMU
  before the generated workspace is replaced by SDK-emitted sources

The stubs expose the Microkit callback surface (`init`, `notified`, and
`protected`) and keep endpoint, notification, IRQ, and shared-memory handler
slots visible so the eventual SDK-generated code has a direct target shape.

The generated workspace also includes `source-boundaries.json`, which records
the native upstream set, optional VM backend, and HubOS-owned wrapper layers.
EOF

printf '%s\n' "Rendered Microkit-generated workspace at $out_dir/generated"
