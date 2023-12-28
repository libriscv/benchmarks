cmake_minimum_required(VERSION 3.9.4)
cmake_policy(SET CMP0069 NEW)
project(script_bench CXX)

option(GPROF       "Instrumented profiling" OFF)
option(LTO         "Enable interprocedural optimizations" ON)
option(SANITIZE    "Enable sanitizers" OFF)
option(PGO         "Generate PGO binary" OFF)
option(PROFILING   "Profiling mode" OFF)
option(RUST_BINARY "Benchmark with Rust binary" OFF)
option(USE_LUAJIT  "Benchmark using LuaJIT" ON)

set(OPTIMIZE  "-O2 -gdwarf-4")
set(COMMON    "-Wall -Wextra")
set(CMAKE_CXX_FLAGS "${COMMON} ${OPTIMIZE} -std=c++17")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
elseif (MINIMAL)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
endif()

if (GPROF)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()
if (SANITIZE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
endif()

if (LTO)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT supported OUTPUT error)
	if (supported)
		message(STATUS "IPO / LTO enabled")
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
	else()
		message(STATUS "IPO / LTO not supported: <${error}>")
	endif()
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -fuse-ld=lld")
endif()

if (PGO)
	set(PROFPATH $ENV{HOME}/pgo)
	if (PROFILING)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-generate=${PROFPATH}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-use=${PROFPATH}/default.profdata")
	endif()
endif()

option(RISCV_BINARY_TRANSLATION "" OFF)
option(RISCV_EXPERIMENTAL "" ON)
option(RISCV_EXT_C "" OFF)
add_subdirectory(ext/libriscv/lib libriscv)

set(SOURCES
	src/main.cpp
	src/native.cpp
	src/luascript.cpp
	src/selftest.cpp
	src/syscalls.cpp
	src/test1.cpp
)


add_executable(bench ${SOURCES})
target_include_directories(bench PRIVATE ext/LuaBridge/Source)
target_include_directories(bench PRIVATE ext/libriscv/emulator/syscalls)
target_link_libraries(bench riscv)
# for some reason this causes -std=gnu++1z
#set_target_properties(bench PROPERTIES CXX_STANDARD 17)

if (USE_LUAJIT)
	find_library(LUAJIT luajit-5.1 REQUIRED)
	target_link_libraries(bench ${LUAJIT})
	target_include_directories(bench PRIVATE ${LUAJIT_INCLUDE_DIRS})
	target_compile_definitions(bench PRIVATE LUAJIT=1)
else()
	find_library(LUA lua5.3 REQUIRED)
	target_link_libraries(bench ${LUA})
	target_include_directories(bench PRIVATE ${LUA_INCLUDE_DIRS})
endif()


if (LTO)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
	endif()
endif()
if (RUST_BINARY)
	target_compile_definitions(bench PRIVATE RUST_BINARY=1)
endif()
