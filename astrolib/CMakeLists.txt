cmake_minimum_required(VERSION 3.25.1)
project(astrolib)

find_package(Protobuf REQUIRED)
add_library( astrolib SHARED exception.cpp posix_mmap_file.cpp )
target_include_directories( astrolib PUBLIC ${PROJINCLUDE} ${Protobuf_INCLUDE_DIRS} )
target_link_libraries( astrolib PUBLIC ${Protobuf_LIBRARIES} )
