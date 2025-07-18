cmake_minimum_required(VERSION 3.1.0)
project(rvbinary C CXX)

option(MINIMAL     "Minimal build" OFF)
option(DEBUGGING   "Debugging build" OFF)
option(GCSECTIONS  "Garbage collect empty sections" OFF)
option(LIBC_USE_STDLIB "" ON)
option(LIBC_WRAP_NATIVE "" ON)

set(WARNINGS  "-Wall -Wextra")
set(COMMON    "-ggdb3 -O3")

if (GCC_TRIPLE STREQUAL "riscv32-unknown-elf")
	set(RISCV_ABI "-march=rv32g_zba_zbb_zbc_zbs -mabi=ilp32d")
else()
	set(RISCV_ABI "-march=rv64gc_zba_zbb_zbc_zbs -mabi=lp64d")
endif()
set(COMMON    "${COMMON} -fno-builtin-memcpy -fno-builtin-memset -fno-builtin-memmove -fno-builtin-memcmp")
if (NOT LIBC_USE_STDLIB)
	set(COMMON    "${COMMON} -fno-builtin")
endif()

# we need this libgcc for 64-bit divisions
set(LIBGCC ${LIBRARY_DIR}/libgcc.a)

set(CMAKE_C_FLAGS "${WARNINGS} ${COMMON} -std=c11 ${RISCV_ABI}")
set(CMAKE_CXX_FLAGS "${WARNINGS} ${COMMON} -std=c++17 ${RISCV_ABI} -fexceptions -frtti -fno-threadsafe-statics")

if (DEBUGGING)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
elseif (MINIMAL)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
endif()

if (GCSECTIONS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-gc-sections")
endif()

set(SOURCES
	src/rvmain.cpp
)

# this fixes a bug in CMake where it randomly removes system includes
unset(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
unset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
# this fixes a bug where CMake adds -rdynamic to literally everything
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

add_subdirectory(ext)
add_subdirectory(libc)
target_compile_definitions(libc PUBLIC
	NATIVE_SYSCALLS_BASE=1
	THREAD_SYSCALLS_BASE=21
)

add_executable(rvbinary ${SOURCES})
target_include_directories(rvbinary PRIVATE ${CMAKE_SOURCE_DIR}/../src/include)
target_link_libraries(rvbinary -static -Wl,--whole-archive libc tinyprintf -Wl,--no-whole-archive ${LIBGCC})
target_link_libraries(rvbinary
	-Wl,--wrap=memcpy,--wrap=memset,--wrap=memmove,--wrap=memcmp
	-Wl,--wrap=strlen,--wrap=strcmp,--wrap=strncmp
	-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free
)
# place ELF into the top binary folder
set_target_properties(rvbinary
	PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
