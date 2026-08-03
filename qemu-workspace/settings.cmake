#
# Copyright 2026 HubOS
#
# SPDX-License-Identifier: BSD-2-Clause
#

# QEMU bootstrap settings for the HubOS seL4 workspace.
# This selects the x86-64 generic platform because it matches the current
# seL4 target and keeps the Linux VM path on the same host architecture.

set(PLATFORM "x86_64_generic" CACHE STRING "" FORCE)
set(SIMULATION ON CACHE BOOL "" FORCE)
set(RELEASE OFF CACHE BOOL "" FORCE)
set(VERIFICATION ON CACHE BOOL "" FORCE)
set(MCS ON CACHE BOOL "" FORCE)
set(DOMAINS OFF CACHE BOOL "" FORCE)
set(SMP OFF CACHE BOOL "" FORCE)
set(Sel4testAllowSettingsOverride OFF CACHE BOOL "" FORCE)
set(QEMU_MEMORY "2G" CACHE STRING "" FORCE)
