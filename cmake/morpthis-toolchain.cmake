# CMake toolchain for the Morpthis device.
#
#   cmake -B build \
#     -DCMAKE_TOOLCHAIN_FILE=path/to/morpthis-sdk/cmake/morpthis-toolchain.cmake \
#     -DMORPTHIS_TOOLCHAIN_PREFIX=mipsel-rockbox-linux-gnu
#
# The default is the prefix rockboxdev.sh installs. app.mk is the primary build
# interface; this file exists for projects that already use CMake.

include_guard(GLOBAL)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR mips)

set(MORPTHIS_TOOLCHAIN_PREFIX "mipsel-rockbox-linux-gnu" CACHE STRING
	"Cross toolchain prefix")
set(MORPTHIS_SYSROOT "" CACHE PATH "Optional sysroot")

find_program(CMAKE_C_COMPILER NAMES ${MORPTHIS_TOOLCHAIN_PREFIX}-gcc REQUIRED)
find_program(CMAKE_CXX_COMPILER NAMES ${MORPTHIS_TOOLCHAIN_PREFIX}-g++ REQUIRED)
find_program(CMAKE_AR NAMES ${MORPTHIS_TOOLCHAIN_PREFIX}-ar REQUIRED)
find_program(CMAKE_RANLIB NAMES ${MORPTHIS_TOOLCHAIN_PREFIX}-ranlib REQUIRED)
find_program(CMAKE_STRIP NAMES ${MORPTHIS_TOOLCHAIN_PREFIX}-strip)

if(MORPTHIS_SYSROOT)
	set(CMAKE_SYSROOT "${MORPTHIS_SYSROOT}")
endif()

# -march=mips32r2 because the X1600's core is MIPS32r2 and the toolchain
# otherwise defaults to MIPS I for o32.
# -mno-madd4 keeps every float multiply and add separately rounded.
# -D_FILE_OFFSET_BITS=64 keeps readdir()/stat() working on exFAT inodes wider
# than 32 bits.
set(CMAKE_C_FLAGS_INIT
	"-O2 -g -march=mips32r2 -mno-madd4 -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS_INIT
	"-O2 -g -march=mips32r2 -mno-madd4 -D_FILE_OFFSET_BITS=64")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-static-libgcc")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-static-libgcc")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
