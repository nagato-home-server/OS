cmake_minimum_required(VERSION 3.16)

project(hubos_model LANGUAGES C)

find_package(OpenSSL REQUIRED)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(HUBOS_USE_LINUX_VM_NETWORK_BACKEND "Use the optional Linux VM network backend by default" OFF)
set(HUBOS_BOOT_IMAGE "" CACHE FILEPATH "Optional bootable system image to attach to the QEMU boot package")
set(HUBOS_BOOT_IMAGE_BUILD_DIR "" CACHE PATH "Optional build directory to scan for a bootable system image")
set(HUBOS_BOOT_PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/boot-package" CACHE PATH "Output directory for the boot package scaffold")

add_library(hubos_model
  src/model.c
  src/resource_registry.c
  src/capability_manager.c
  src/session_manager.c
  src/hub.c
  src/audit.c
  src/dma_manager.c
  src/driver_registry.c
  src/driver_loader.c
  src/driver_service.c
  src/sha256.c
  src/boot.c
  src/system.c
  src/memory_manager.c
  src/bus_manager.c
  src/device_server.c
  src/network_server.c
  src/runtime_config.c
  src/microkit_graph.c
  src/microkit_boot.c
  src/microkit_endpoint.c
  src/microkit_ipc.c
  src/microkit_kernel_glue.c
  src/microkit_generated.c
  src/microkit_runtime.c
  src/service_endpoints.c
  src/ipc.c
  src/root_task.c
)

if(UNIX AND NOT APPLE)
  target_sources(hubos_model PRIVATE src/linux_usbio_backend.c)
  target_compile_definitions(hubos_model PUBLIC HUBOS_USE_LINUX_USBIO_BACKEND=1)
else()
  target_compile_definitions(hubos_model PUBLIC HUBOS_USE_LINUX_USBIO_BACKEND=0)
endif()

target_include_directories(hubos_model
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(HUBOS_USE_LINUX_VM_NETWORK_BACKEND)
  target_compile_definitions(hubos_model
    PUBLIC
      HUBOS_USE_LINUX_VM_NETWORK_BACKEND=1
  )
else()
  target_compile_definitions(hubos_model
    PUBLIC
      HUBOS_USE_LINUX_VM_NETWORK_BACKEND=0
  )
endif()

add_executable(hubos_model_test
  tests/model_test.c
)

target_link_libraries(hubos_model_test
  PRIVATE
    hubos_model
)

target_link_libraries(hubos_model
  PUBLIC
    OpenSSL::Crypto
)

enable_testing()
add_test(NAME hubos_model_test COMMAND hubos_model_test)

add_custom_target(sel4_integration_verify
  COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify-sel4-integration.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Verifying the HubOS seL4 workspace scaffold and generated Microkit stubs"
)

add_custom_target(qemu_scaffold_verify
  COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify-qemu-scaffold.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Verifying the QEMU bootstrap workspace scaffold"
)

add_custom_target(hubos_boot_package_generate
  COMMAND ${CMAKE_COMMAND} -E env
          HUBOS_BOOT_IMAGE=${HUBOS_BOOT_IMAGE}
          HUBOS_BOOT_IMAGE_BUILD_DIR=${HUBOS_BOOT_IMAGE_BUILD_DIR}
          HUBOS_QEMU_PLATFORM=qemu_virt_riscv64
          HUBOS_QEMU_MEMORY=2G
          sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/render-boot-package.sh
          ${HUBOS_BOOT_PACKAGE_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Rendering the HubOS boot package scaffold"
)

add_custom_target(hubos_boot_package_verify
  COMMAND ${CMAKE_COMMAND} -E env
          HUBOS_BOOT_IMAGE=${HUBOS_BOOT_IMAGE}
          HUBOS_BOOT_IMAGE_BUILD_DIR=${HUBOS_BOOT_IMAGE_BUILD_DIR}
          sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify-boot-package.sh
          ${HUBOS_BOOT_PACKAGE_DIR}
  DEPENDS
    hubos_boot_package_generate
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Verifying the HubOS boot package scaffold"
)

add_custom_target(hubos_manifest_generate
  COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/render-hubos-manifest.sh
          ${CMAKE_CURRENT_SOURCE_DIR}/seL4test-manifest/source-boundaries.json
          ${CMAKE_CURRENT_BINARY_DIR}/hubos.xml
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Rendering the HubOS manifest from source boundaries"
)

add_custom_target(hubos_upstream_verify
  COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify-upstream-assets.sh native
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Verifying the native seL4 upstream asset set"
)

add_custom_target(hubos_upstream_vm_verify
  COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify-upstream-assets.sh vm
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Verifying the optional Linux VM backend upstream asset set"
)

add_custom_target(hubos_runtime_config_verify
  COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify-runtime-config.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Verifying the HubOS App VM runtime selection config"
)

add_custom_target(hubos_source_bundle_verify
  COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify-source-bundles.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Verifying pinned bundled runtime source checkouts"
)

add_custom_target(hubos_full_verify
  DEPENDS
    hubos_upstream_verify
    sel4_integration_verify
    hubos_runtime_config_verify
    hubos_source_bundle_verify
  COMMENT "Verifying the native upstream assets and the seL4 integration scaffold"
)

add_custom_target(hubos_full_vm_verify
  DEPENDS
    hubos_upstream_vm_verify
    sel4_integration_verify
    hubos_runtime_config_verify
    hubos_source_bundle_verify
  COMMENT "Verifying the VM backend upstream assets and the seL4 integration scaffold"
)

add_custom_target(hubos_qemu_bootstrap_verify
  DEPENDS
    qemu_scaffold_verify
    hubos_boot_package_verify
    sel4_integration_verify
  COMMENT "Verifying the QEMU bootstrap workspace scaffold and the seL4 integration scaffold"
)

add_custom_target(hubos_qemu_image_build
  COMMAND ${CMAKE_COMMAND} -E env
          HUBOS_QEMU_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/qemu-workspace/build
          sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-qemu-system-image.sh
          ${CMAKE_CURRENT_SOURCE_DIR}/qemu-workspace
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building the bootable HubOS QEMU image"
)
