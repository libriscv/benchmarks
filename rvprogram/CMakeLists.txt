cmake_minimum_required(VERSION 3.1.0)
project(rvbinary C CXX)

option(MINIMAL     "Minimal build" OFF)
option(DEBUGGING   "Debugging build" OFF)
option(GCSECTIONS  "Garbage collect empty sections" OFF)
set(VERSION_FILE   "symbols.map" CACHE STRING "Retained symbols file")

set(WARNINGS  "-Wall -Wextra")
set(COMMON    "-O3 -nostdlib -fno-builtin")

if (GCC_TRIPLE STREQUAL "riscv32-unknown-elf")
	set(RISCV_ABI "-march=rv32gc -mabi=ilp32d")
else()
	set(RISCV_ABI "-march=rv64gc -mabi=lp64d")
endif()
set(COMMON    "${COMMON} -flto=4")

# we need this libgcc for 64-bit divisions
set(LIBGCC ${LIBRARY_DIR}/libgcc.a)

set(CMAKE_C_FLAGS "${WARNINGS} ${COMMON} -std=c11 ${RISCV_ABI}")
set(CMAKE_CXX_FLAGS "${WARNINGS} ${COMMON} -std=c++17 ${RISCV_ABI} -fno-exceptions -fno-rtti -fno-threadsafe-statics")

if (DEBUGGING)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb3")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb3")
elseif (MINIMAL)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
endif()

if (GCSECTIONS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-gc-sections")
	file(STRINGS "${CMAKE_SOURCE_DIR}/${VERSION_FILE}" SYMBOLS)
	foreach(SYMBOL ${SYMBOLS})
		if (NOT ${SYMBOL} STREQUAL "")
			message(STATUS "Symbol retained: ${SYMBOL}")
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=${SYMBOL}")
		endif()
	endforeach()
endif()

set(SOURCES
	src/rvmain.cpp
)

# this fixes a bug in CMake where it randomly removes system includes
unset(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
unset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
# this fixes a bug where CMake adds -rdynamic to literally everything
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
# place ELF into the top binary folder
set_target_properties(rvbinary
	PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if (NOT DEBUGGING)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--retain-symbols-file=${CMAKE_SOURCE_DIR}/${VERSION_FILE}")
	set_target_properties(rvbinary PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/${VERSION_FILE})
endif()
