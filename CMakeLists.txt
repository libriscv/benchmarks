cmake_minimum_required(VERSION 3.9)
project(script_bench CXX)

option(DEBUGGING   "Debugging build" OFF)
option(LTO         "Enable interprocedural optimizations" ON)
option(SANITIZE    "Enable sanitizers" OFF)
option(PGO         "Generate PGO binary" OFF)
option(PROFILING   "Profiling mode" OFF)
option(RUST_BINARY "Benchmark with Rust binary" OFF)
option(USE_LUAJIT  "Benchmark using LuaJIT" OFF)

set(OPTIMIZE  "-O2")
set(COMMON    "-Wall -Wextra")
set(CMAKE_CXX_FLAGS "${COMMON} ${OPTIMIZE} -std=c++17")

if (DEBUGGING)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb3")
elseif (MINIMAL)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
endif()

if (SANITIZE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -ggdb3 -fsanitize=address,undefined")
endif()

if (PGO)
	set(PROFPATH $ENV{HOME}/pgo)
	if (PROFILING)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-generate=${PROFPATH}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-use=${PROFPATH}/default.profdata")
	endif()
endif()

add_subdirectory(ext/libriscv/lib libriscv)
# NOTE: We have to enable exec traps to get a realistic picture
# as they are used in the game engine these benchmarks are for.
target_compile_definitions(riscv PUBLIC
	RISCV_EBREAK_MEANS_STOP=1
	RISCV_SYSCALLS_MAX=128)

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
target_include_directories(bench PRIVATE ext/LuaBridge/Source/LuaBridge)
target_include_directories(bench PRIVATE ext/libriscv/emulator/syscalls)
target_link_libraries(bench riscv)
# for some reason this causes -std=gnu++1z
#set_target_properties(bench PROPERTIES CXX_STANDARD 17)
target_compile_definitions(bench PRIVATE
		NATIVE_SYSCALLS_BASE=1
		THREADS_SYSCALL_BASE=21)


if (USE_LUAJIT)
	find_library(LUAJIT luajit-5.1 REQUIRED)
	target_link_libraries(bench ${LUAJIT})
	target_include_directories(bench PRIVATE ${LUAJIT_INCLUDE_DIRS})
	target_compile_definitions(bench PRIVATE LUAJIT=1)
else()
	set (LUAPATH "/home/gonzo/github/lua")

	add_library(lua54 STATIC IMPORTED)
	set_target_properties(lua54 PROPERTIES IMPORTED_LOCATION "${LUAPATH}/liblua.a")
	set_target_properties(lua54 PROPERTIES LINKER_LANGUAGE C)
	target_link_libraries(lua54 INTERFACE dl)
	target_include_directories(lua54 INTERFACE ${LUAPATH})

	target_link_libraries(bench lua54)
endif()


if (LTO)
	set_target_properties(riscv PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_property(TARGET bench PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
	endif()
endif()
if (RUST_BINARY)
	target_compile_definitions(bench PRIVATE RUST_BINARY=1)
endif()
