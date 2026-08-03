#ifndef HUBOS_IPC_H
#define HUBOS_IPC_H

#include "hubos/boot.h"

typedef enum {
  HUBOS_IPC_STATUS_OK = 0,
  HUBOS_IPC_STATUS_INVALID_ARGUMENT,
  HUBOS_IPC_STATUS_NOT_FOUND,
  HUBOS_IPC_STATUS_ALREADY_EXISTS,
  HUBOS_IPC_STATUS_DENIED,
  HUBOS_IPC_STATUS_CONFLICT,
} hubos_ipc_status_t;

typedef enum {
  HUBOS_ROOT_TASK_OP_BOOTSTRAP = 0,
  HUBOS_ROOT_TASK_OP_COMPLETE_BOOT_STEP,
  HUBOS_ROOT_TASK_OP_QUERY_BOOT_STEP,
  HUBOS_ROOT_TASK_OP_ADVANCE_CONTROL_PLANE,
} hubos_root_task_operation_t;

typedef struct {
  hubos_boot_step_t step;
} hubos_root_task_boot_step_request_t;

typedef struct {
  hubos_root_task_operation_t operation;
  union {
    hubos_root_task_boot_step_request_t boot_step;
  } payload;
} hubos_root_task_request_t;

typedef struct {
  hubos_ipc_status_t status;
  hubos_boot_step_t boot_step;
  bool bool_result;
} hubos_root_task_response_t;

void hubos_root_task_request_init(hubos_root_task_request_t *request,
                                  hubos_root_task_operation_t operation);

void hubos_root_task_response_init(hubos_root_task_response_t *response);

#endif
