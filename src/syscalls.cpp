#include "syscalls.hpp"
#include <stdexcept>
#include <unistd.h>
using namespace riscv;

template <int W>
void syscall_exit(Machine<W>& machine)
{
	auto* state = machine.template get_userdata<State<W>> ();
	state->exit_code = machine.template sysarg<int> (0);
	machine.stop();
	machine.set_result(state->exit_code);
}

template <int W>
void syscall_write(Machine<W>& machine)
{
	auto [fd, address, len] = machine.template sysargs<int, address_type<W>, address_type<W>> ();
	SYSPRINT("SYSCALL write: addr = 0x%X, len = %zu\n", address, len);
	auto* state = machine.template get_userdata<State<W>> ();
	// we only accept standard pipes, for now :)
	if (fd >= 0 && fd < 3) {
		const size_t len_g = std::min((size_t) 1024u, (size_t) len);
		machine.memory.memview(address, len_g,
			[state] (auto* data, size_t len) {
				state->output.append((char*) data, len);
#ifdef RISCV_DEBUG
				(void) write(0, data, len);
#endif
			});
		machine.set_result(len_g);
		return;
	}
	machine.set_result(-EBADF);
}

template <int W>
void syscall_ebreak(riscv::Machine<W>& machine)
{
	printf("\n>>> EBREAK at %#X\n", machine.cpu.pc());
#ifdef RISCV_DEBUG
	machine.print_and_pause();
#else
	throw std::runtime_error("Unhandled EBREAK instruction");
#endif
}

template <int W>
inline void setup_minimal_syscalls(State<W>& state, Machine<W>& machine)
{
	machine.set_userdata(&state);
#ifdef RISCV_DEBUG
	machine.install_syscall_handler(SYSCALL_EBREAK, syscall_ebreak<W>);
#else
	machine.install_syscall_handler(SYSCALL_EBREAK, syscall_exit<W>);
#endif
	machine.install_syscall_handler(64, syscall_write<W>);
	machine.install_syscall_handler(93, syscall_exit<W>);
}

/* le sigh */
template void setup_minimal_syscalls<4>(State<4>&, Machine<4>&);
template void setup_minimal_syscalls<8>(State<8>&, Machine<8>&);
