cmake_minimum_required(VERSION 3.1.0)
project(rvbinary C CXX)

option(MINIMAL     "Minimal build" OFF)
option(DEBUGGING   "Debugging build" OFF)
option(GCSECTIONS  "Garbage collect empty sections" ON)
set(VERSION_FILE   "symbols.map" CACHE STRING "Retained symbols file")

set(WARNINGS  "-Wall -Wextra")
set(COMMON    "-O2 -fsingle-precision-constant -nostdlib -nostdinc -fno-builtin")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(RISCV_ABI "-target riscv32 -mabi=ilp32d -march=rv32imfd")
	#set(COMMON    "${COMMON} -flto")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
else()
	set(RISCV_ABI "-march=rv32imfd -mabi=ilp32d")
	set(COMMON    "${COMMON} -flto=4")
endif()
# we need this libgcc for 64-bit divisions
set(LIBGCC ${LIBRARY_DIR}/rv32imfd/ilp32d/libgcc.a)

set(CMAKE_C_FLAGS "${WARNINGS} ${COMMON} -std=c11 ${RISCV_ABI}")
set(CMAKE_CXX_FLAGS "${WARNINGS} ${COMMON} -std=c++2a ${RISCV_ABI} -fno-exceptions -fno-rtti")

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
	THREAD_SYSCALLS_BASE=10
)

add_executable(rvbinary ${SOURCES})
target_include_directories(rvbinary PRIVATE ${CMAKE_SOURCE_DIR}/../src/include)
target_link_libraries(rvbinary -static -Wl,--whole-archive libc -Wl,--no-whole-archive ${LIBGCC})
# place ELF into the top binary folder
set_target_properties(rvbinary
	PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if (NOT DEBUGGING)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--retain-symbols-file=${CMAKE_SOURCE_DIR}/${VERSION_FILE}")
	set_target_properties(rvbinary PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/${VERSION_FILE})
endif()
