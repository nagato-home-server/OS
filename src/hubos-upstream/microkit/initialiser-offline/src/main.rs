#![no_std]
#![no_main]

use core::hint::spin_loop;
use core::panic::PanicInfo;

// These symbols are patched by microkit-tool before packaging the image.
// Keep them as plain machine-word cells so the tool can write values directly.
#[used]
#[unsafe(no_mangle)]
#[unsafe(link_section = ".data")]
pub static mut sel4_capdl_initializer_serialized_spec_data_start: usize = 0;

#[used]
#[unsafe(no_mangle)]
#[unsafe(link_section = ".data")]
pub static mut sel4_capdl_initializer_serialized_spec_data_size: usize = 0;

#[used]
#[unsafe(no_mangle)]
#[unsafe(link_section = ".data")]
pub static mut sel4_capdl_initializer_embedded_frames_data_start: usize = 0;

#[used]
#[unsafe(no_mangle)]
#[unsafe(link_section = ".data")]
pub static mut sel4_capdl_initializer_image_start: usize = 0;

#[used]
#[unsafe(no_mangle)]
#[unsafe(link_section = ".data")]
pub static mut sel4_capdl_initializer_image_end: usize = 0;

#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    idle()
}

#[unsafe(no_mangle)]
pub extern "C" fn main() -> ! {
    idle()
}

fn idle() -> ! {
    loop {
        spin_loop();
    }
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    idle()
}
