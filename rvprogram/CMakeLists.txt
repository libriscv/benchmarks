cmake_minimum_required(VERSION 3.1.0)
project(rvbinary C CXX)

option(MINIMAL     "Minimal build" OFF)
option(DEBUGGING   "Debugging build" OFF)
option(GCSECTIONS  "Garbage collect empty sections" ON)
set(PUBLIC_API "test" CACHE STRING "Symbols to keep during GC")

set(RISCV_ABI "-march=rv32imfd -mabi=ilp32d")
set(WARNINGS  "-Wall -Wextra")
set(COMMON    "-O2 -fstack-protector -nostdlib -nostdinc -fno-builtin")
set(CMAKE_C_FLAGS "${WARNINGS} ${RISCV_ABI} -std=c11 ${COMMON}")
set(CMAKE_CXX_FLAGS "${WARNINGS} ${RISCV_ABI} -std=c++2a ${COMMON}")

if (DEBUGGING)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
elseif (MINIMAL)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
endif()

if (GCSECTIONS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-gc-sections")
	foreach(SYM ${PUBLIC_API})
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=${SYM}")
	endforeach()
endif()


set(SOURCES
	src/rvmain.cpp
)

add_subdirectory(ext)
add_subdirectory(libc)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "") # remove -rdynamic

add_executable(rvbinary ${SOURCES})
target_link_libraries(rvbinary -static libc)
# place ELF into the top binary folder
set_target_properties(rvbinary
	PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
if (NOT DEBUGGING)
	target_link_libraries(rvbinary -Wl,-S)
endif()
