cmake_minimum_required (VERSION 3.0.0)
project (libnbt)

set(NBT_SOURCES 
    "src/nbt.c" "include/nbt.h"
    "src/error.c" "src/nbterr.h"
    "src/parse.c"
)

include_directories("include/")

find_package(ZLIB)

add_library(libnbt STATIC ${NBT_SOURCES})
target_link_libraries(libnbt ZLIB::ZLIB)

add_subdirectory("example/")