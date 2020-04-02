#include "syscalls.hpp"
#include <unistd.h>
using namespace riscv;
static constexpr uint32_t G_SHMEM_BASE = 0x70000000;

struct iovec32 {
    uint32_t iov_base;
    int32_t  iov_len;
};

template <int W>
long State<W>::syscall_exit(Machine<W>& machine)
{
	this->exit_code = machine.template sysarg<int> (0);
	machine.stop();
	return this->exit_code;
}

template <int W>
long State<W>::syscall_write(Machine<W>& machine)
{
	const int  fd      = machine.template sysarg<int>(0);
	const auto address = machine.template sysarg<address_type<W>>(1);
	const size_t len   = machine.template sysarg<address_type<W>>(2);
	SYSPRINT("SYSCALL write: addr = 0x%X, len = %zu\n", address, len);
	// we only accept standard pipes, for now :)
	if (fd >= 0 && fd < 3) {
		char buffer[1024];
		const size_t len_g = std::min(sizeof(buffer), len);
		machine.memory.memcpy_out(buffer, address, len_g);
		output.append(buffer, len_g);
#ifdef RISCV_DEBUG
		return write(fd, buffer, len);
#else
		return len_g;
#endif
	}
	return -EBADF;
}

template <int W>
long State<W>::syscall_writev(Machine<W>& machine)
{
	const int  fd     = machine.template sysarg<int>(0);
	const auto iov_g  = machine.template sysarg<uint32_t>(1);
	const auto count  = machine.template sysarg<int>(2);
	if constexpr (false) {
		printf("SYSCALL writev called, iov = %#X  cnt = %d\n", iov_g, count);
	}
	if (count < 0 || count > 256) return -EINVAL;
	// we only accept standard pipes, for now :)
	if (fd >= 0 && fd < 3) {
        const size_t size = sizeof(iovec32) * count;

        std::vector<iovec32> vec(count);
        machine.memory.memcpy_out(vec.data(), iov_g, size);

        int res = 0;
        for (const auto& iov : vec)
        {
			char buffer[1024];
            auto src_g = (uint32_t) iov.iov_base;
            auto len_g = std::min(sizeof(buffer), (size_t) iov.iov_len);
            machine.memory.memcpy_out(buffer, src_g, len_g);
			output += std::string(buffer, len_g);
#ifdef RISCV_DEBUG
			res += write(fd, buffer, len_g);
#else
			res += len_g;
#endif
        }
        return res;
	}
	return -EBADF;
}

template <int W>
long syscall_close(riscv::Machine<W>& machine)
{
	const int fd = machine.template sysarg<int>(0);
	if constexpr (verbose_syscalls) {
		printf("SYSCALL close called, fd = %d\n", fd);
	}
	if (fd <= 2) {
		return 0;
	}
	printf(">>> Close %d\n", fd);
	return -1;
}

template <int W>
long syscall_ebreak(riscv::Machine<W>& machine)
{
	printf("\n>>> EBREAK at %#X\n", machine.cpu.pc());
#ifdef RISCV_DEBUG
	machine.print_and_pause();
#else
	throw std::runtime_error("Unhandled EBREAK instruction");
#endif
	return 0;
}

template <int W>
inline void setup_minimal_syscalls(State<W>& state, Machine<W>& machine)
{
#ifdef RISCV_DEBUG
	machine.install_syscall_handler(SYSCALL_EBREAK, syscall_ebreak<W>);
#else
	machine.install_syscall_handler(SYSCALL_EBREAK, {&state, &State<W>::syscall_exit});
#endif
	machine.install_syscall_handler(64, {&state, &State<W>::syscall_write});
	machine.install_syscall_handler(93, {&state, &State<W>::syscall_exit});
}

/* le sigh */
template void setup_minimal_syscalls<4>(State<4>&, Machine<4>&);
