#![no_main]
#![no_std]
#![feature(llvm_asm)]
#![feature(global_asm)]

use core::panic::PanicInfo;

/// This function is called on panic.
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[no_mangle]
pub unsafe fn abort() -> !
{
	llvm_asm!("ebreak");
	loop {}
}

#[no_mangle]
pub extern "C" fn libc_start() {
	unsafe {
		llvm_asm!("ebreak");
	}
}

global_asm!(r#"
.section .text
.globl _start
_start:
	// .option push
	// .option norelax
	lui gp, %hi(__global_pointer$)
	addi gp, gp, %lo(__global_pointer$)
	// .option pop

	add s0, sp, zero

	call libc_start

.globl fastexit
fastexit:
	ebreak
"#);


/// Benchmark 1: Function call overhead
#[no_mangle]
pub extern "C" fn empty_function()
{
}

/// Benchmark 2: Store argument into array, increment index
static mut ARRAY:   [i32; 4096] = [0; 4096];
static mut COUNTER: usize = 0;
#[no_mangle]
pub extern "C" fn test(arg1: i32)
{
	unsafe {
		ARRAY[COUNTER] = arg1;
		COUNTER += 1;
	}
}
