#include "syscalls.hpp"
#include <stdexcept>
#include <unistd.h>
using namespace riscv;

template <int W>
void syscall_exit(Machine<W>& machine)
{
	machine.stop();
}

template <int W>
void syscall_write(Machine<W>& machine)
{
	auto [fd, view] = machine.template sysargs<int, std::string_view> ();
	// we only accept standard pipes, for now :)
	if (fd >= 0 && fd < 3) {
#ifdef RISCV_DEBUG
		(void) write(0, view.begin(), view.size());
#endif
		machine.set_result(view.size());
		return;
	}
	machine.set_result(-EBADF);
}

template <int W>
void syscall_ebreak(Machine<W>& machine)
{
	printf("\n>>> EBREAK at %#X\n", machine.cpu.pc());
#ifdef RISCV_DEBUG
	machine.print_and_pause();
#else
	throw std::runtime_error("Unhandled EBREAK instruction");
#endif
}

template <int W>
inline void setup_minimal_syscalls(Machine<W>& machine)
{
#ifdef RISCV_DEBUG
	machine.install_syscall_handler(SYSCALL_EBREAK, syscall_ebreak<W>);
#else
	machine.install_syscall_handler(SYSCALL_EBREAK, syscall_exit<W>);
#endif
	machine.install_syscall_handler(64, syscall_write<W>);
	machine.install_syscall_handler(93, syscall_exit<W>);
}

/* le sigh */
#ifdef RISCV_32I
template void setup_minimal_syscalls<4>(Machine<4>&);
#endif
#ifdef RISCV_64I
template void setup_minimal_syscalls<8>(Machine<8>&);
#endif
