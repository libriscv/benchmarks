cmake_minimum_required(VERSION 3.1.0)
project(script_bench CXX)

option(MINIMAL     "Minimal build" ON)
option(DEBUGGING   "Debugging build" OFF)

set(MARCH     "-march=native")
set(COMMON    "-O2 -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${COMMON} ${MARCH} -std=c++17")

if (DEBUGGING)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
elseif (MINIMAL)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
endif()

add_subdirectory(ext/libriscv/lib libriscv)

set(SOURCES
	src/main.cpp
	src/luascript.cpp
	src/syscalls.cpp
	src/test1.cpp
)

add_executable(bench ${SOURCES})
target_link_libraries(bench riscv lua5.3)
target_include_directories(bench PRIVATE ext/LuaBridge/Source/LuaBridge)
set_target_properties(bench PROPERTIES CXX_STANDARD 17)
