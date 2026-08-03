#include "hubos/ipc.h"

#include <string.h>

void hubos_root_task_request_init(hubos_root_task_request_t *request,
                                  hubos_root_task_operation_t operation) {
  if (request == NULL) {
    return;
  }

  memset(request, 0, sizeof(*request));
  request->operation = operation;
}

void hubos_root_task_response_init(hubos_root_task_response_t *response) {
  if (response == NULL) {
    return;
  }

  memset(response, 0, sizeof(*response));
  response->status = HUBOS_IPC_STATUS_INVALID_ARGUMENT;
  response->boot_step = HUBOS_BOOT_STEP_COUNT;
  response->bool_result = false;
}
