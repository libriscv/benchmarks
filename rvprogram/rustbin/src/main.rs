#![no_main]
#![no_std]
#![feature(llvm_asm)]
#![feature(global_asm)]
#![feature(naked_functions)]

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
pub unsafe extern "C" fn libc_start() {
	llvm_asm!("ebreak");
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
#[naked]
#[no_mangle]
pub unsafe extern "C" fn empty_function()
{
	llvm_asm!("ebreak");
}

/// Benchmark 2: Store argument into array, increment index
static mut ARRAY:   [i32; 4096] = [0; 4096];
static mut COUNTER: usize = 0;

#[naked]
#[no_mangle]
pub unsafe extern "C" fn test(arg1: i32)
{
	ARRAY[COUNTER] = arg1;
	COUNTER += 1;
	llvm_asm!("ebreak");
}
