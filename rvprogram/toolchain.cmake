SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_CROSSCOMPILING 1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

message(STATUS ${CMAKE_SOURCE_DIR}/libc/override)

if (NOT GCC_TRIPLE)
	set(GCC_TRIPLE "riscv64-unknown-elf")
endif()
set(GCC_VERSION 10.1.0)
set(COMPILER_DIR $ENV{HOME}/riscv/${GCC_TRIPLE})
set(LIBRARY_DIR $ENV{HOME}/riscv/lib/gcc/${GCC_TRIPLE}/${GCC_VERSION} CACHE STRING "GCC libraries")

include_directories(SYSTEM
	${CMAKE_SOURCE_DIR}/libc/override
	${COMPILER_DIR}/include/c++/${GCC_VERSION}
	${COMPILER_DIR}/include/c++/${GCC_VERSION}/${GCC_TRIPLE}
	${COMPILER_DIR}/include
	${LIBRARY_DIR}/include-fixed
	${LIBRARY_DIR}/include
)
