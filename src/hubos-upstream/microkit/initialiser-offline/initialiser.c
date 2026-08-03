// SPDX-License-Identifier: BSD-2-Clause

#include <sel4/sel4.h>
#include <stddef.h>
#include <stdint.h>

seL4_IPCBuffer __sel4_ipc_buffer_obj;
seL4_IPCBuffer *__sel4_ipc_buffer = &__sel4_ipc_buffer_obj;

#define INITIALISER_STACK_SIZE (16 * 1024)
static uint8_t initialiser_stack[INITIALISER_STACK_SIZE] __attribute__((aligned(16), used));

// These symbols are patched by microkit-tool before packaging the image.
// Keep them as machine-word cells so the tool can write values directly.
__attribute__((used, section(".data")))
uintptr_t sel4_capdl_initializer_serialized_spec_data_start = 0;

__attribute__((used, section(".data")))
uintptr_t sel4_capdl_initializer_serialized_spec_data_size = 0;

__attribute__((used, section(".data")))
uintptr_t sel4_capdl_initializer_embedded_frames_data_start = 0;

__attribute__((used, section(".data")))
uintptr_t sel4_capdl_initializer_expected_untypeds_list_num_entries = 0;

__attribute__((used, section(".data")))
uint8_t sel4_capdl_initializer_expected_untypeds_list[4096] = {0};

__attribute__((used, section(".data")))
uintptr_t sel4_capdl_initializer_image_start = 0;

__attribute__((used, section(".data")))
uintptr_t sel4_capdl_initializer_image_end = 0;

static void idle(void) __attribute__((noreturn));
void start_initialiser(void) __attribute__((noreturn));

#define INITIALISER_CSPACE_SCAN_START 1u
#define INITIALISER_CSPACE_SCAN_END   261u

static void start_threads(void)
{
    for (seL4_CPtr slot = INITIALISER_CSPACE_SCAN_START; slot <= INITIALISER_CSPACE_SCAN_END; ++slot) {
        (void)seL4_TCB_Resume(slot);
    }
}

static void idle(void)
{
    seL4_Word badge = 0;
    for (;;) {
        (void)badge;
        seL4_Yield();
    }
}

void _start(void) __attribute__((noreturn));

void _start(void) __attribute__((naked, noreturn));

void _start(void)
{
    __asm__ volatile(
        "lea initialiser_stack(%%rip), %%rsp\n"
        "add %[stack_size], %%rsp\n"
        "and $-16, %%rsp\n"
        "xor %%rbp, %%rbp\n"
        "call start_initialiser\n"
        :
        : [stack_size] "i"(INITIALISER_STACK_SIZE)
        : "memory"
    );
}

void start_initialiser(void)
{
    start_threads();
    idle();
}
