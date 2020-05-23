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

#[repr(C)]
pub struct Value {
	a: i32,
	b: i64
}
#[naked]
#[no_mangle]
pub extern "C" fn test_args(a1: u32, a2: Value, a3: i32, a4: i32, a5: i32, a6: i32, a7: i32, a8: i32) -> i32
{
	if a1 == 0x876633F && a2.a == 222 && a2.b == 666 && a3 == 333
	&& a4 == 444 && a5 == 555 && a6 == 666 && a7 == 777 && a8 == 888 {
		return 666;
	}
	return a2.a;
	return -1;
}

#[naked]
#[no_mangle]
pub extern "C" fn test_maffs(a1: i32, a2: i32) -> i32
{
	let a = a1 + a2;
	let b = a1 - a2;
	let c = a1 * a2;
	let d = a1 / a2;
	return a + b + c + d;
}
