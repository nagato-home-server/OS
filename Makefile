CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Iinclude
NATIVE_SRCS := src/linux_usbio_backend.c
NATIVE_CFLAGS := -DHUBOS_USE_LINUX_USBIO_BACKEND=1
LDLIBS ?= -lcrypto
BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/hubos-model-test

.PHONY: all test clean

all: test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): src/model.c src/resource_registry.c src/capability_manager.c src/session_manager.c src/hub.c src/audit.c src/dma_manager.c src/driver_registry.c src/driver_loader.c src/driver_service.c src/boot.c src/system.c src/memory_manager.c src/bus_manager.c src/device_server.c src/network_server.c src/runtime_config.c src/microkit_graph.c src/microkit_boot.c src/microkit_endpoint.c src/microkit_ipc.c src/microkit_generated.c src/microkit_runtime.c src/microkit_kernel_glue.c src/sha256.c src/service_endpoints.c src/ipc.c src/root_task.c $(NATIVE_SRCS) tests/model_test.c include/hubos/model.h include/hubos/audit.h include/hubos/bus_manager.h include/hubos/dma_manager.h include/hubos/driver_registry.h include/hubos/driver_loader.h include/hubos/driver_service.h include/hubos/network_server.h include/hubos/microkit_graph.h include/hubos/microkit_boot.h include/hubos/microkit_endpoint.h include/hubos/microkit_ipc.h include/hubos/microkit_runtime.h include/hubos/microkit_generated.h include/hubos/service_endpoints.h include/hubos/ipc.h include/hubos/root_task.h include/hubos/boot.h include/hubos/memory_manager.h include/hubos/system.h include/hubos/resource_registry.h include/hubos/capability_manager.h include/hubos/session_manager.h include/hubos/hub.h include/hubos/linux_usbio_backend.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(NATIVE_CFLAGS) src/model.c src/resource_registry.c src/capability_manager.c src/session_manager.c src/hub.c src/audit.c src/dma_manager.c src/driver_registry.c src/driver_loader.c src/driver_service.c src/boot.c src/system.c src/memory_manager.c src/bus_manager.c src/device_server.c src/network_server.c src/runtime_config.c src/microkit_graph.c src/microkit_boot.c src/microkit_endpoint.c src/microkit_ipc.c src/microkit_generated.c src/microkit_runtime.c src/microkit_kernel_glue.c src/sha256.c src/service_endpoints.c src/ipc.c src/root_task.c $(NATIVE_SRCS) tests/model_test.c -o $(TARGET) $(LDLIBS)

test: $(TARGET)
	$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
