cmake_minimum_required(VERSION 3.1.0)
project(script_bench CXX)

option(DEBUGGING   "Debugging build" OFF)

set(OPTIMIZE  "-Ofast -march=native -DNDEBUG=1")
set(COMMON    "-Wall -Wextra")
set(CMAKE_CXX_FLAGS "${COMMON} ${OPTIMIZE} -std=c++17")

if (DEBUGGING)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
elseif (MINIMAL)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
endif()

add_subdirectory(ext/libriscv/lib libriscv)

set(SOURCES
	src/main.cpp
	src/luascript.cpp
	src/selftest.cpp
	src/syscalls.cpp
	src/test1.cpp
	ext/libriscv/emulator/src/native_libc.cpp
)

add_executable(bench ${SOURCES})
target_link_libraries(bench riscv lua5.3)
target_include_directories(bench PRIVATE ext/LuaBridge/Source/LuaBridge)
# for some reason this causes -std=gnu++1z
#set_target_properties(bench PROPERTIES CXX_STANDARD 17)
