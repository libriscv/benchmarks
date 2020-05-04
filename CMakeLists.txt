cmake_minimum_required(VERSION 3.9)
project(script_bench CXX)

option(DEBUGGING   "Debugging build" OFF)
option(LTO         "Enable interprocedural optimizations" ON)
option(SANITIZE    "Enable sanitizers" OFF)

set(OPTIMIZE  "-Ofast -march=native")
set(COMMON    "-Wall -Wextra")
set(CMAKE_CXX_FLAGS "${COMMON} ${OPTIMIZE} -std=c++17")

if (DEBUGGING)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
elseif (MINIMAL)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
endif()

if (SANITIZE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -march=native -ggdb3 -fsanitize=address,undefined")
endif()

add_subdirectory(ext/libriscv/lib libriscv)
target_compile_definitions(riscv PUBLIC RISCV_EXEC_TRAPS_ENABLED=1
										RISCV_THROW_UNIMPLEMENTED=1
										RISCV_EBREAK_MEANS_STOP=1)

set(SOURCES
	src/main.cpp
	src/native.cpp
	src/luascript.cpp
	src/selftest.cpp
	src/syscalls.cpp
	src/test1.cpp
	ext/libriscv/emulator/syscalls/src/native_libc.cpp
	ext/libriscv/emulator/syscalls/src/native_threads.cpp
)

add_executable(bench ${SOURCES})
target_link_libraries(bench riscv luajit-5.1) #lua5.3)
target_include_directories(bench PRIVATE ext/LuaBridge/Source/LuaBridge)
target_include_directories(bench PRIVATE ext/libriscv/emulator/syscalls)
# for some reason this causes -std=gnu++1z
#set_target_properties(bench PROPERTIES CXX_STANDARD 17)

if (LTO)
	set_target_properties(riscv PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_property(TARGET bench PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
	endif()
endif()
