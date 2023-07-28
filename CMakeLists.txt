cmake_minimum_required(VERSION 3.25.1)

project(Astrolabe)
set(PROJROOT ${PROJECT_SOURCE_DIR})
set(PROTODIR ${PROJROOT}/proto)
set(PROJINCLUDE ${PROJROOT}/include)

include_directories(${PROJINCLUDE})

add_subdirectory(astrolib)
add_subdirectory(mapindexer)

include_directories(${Protobuf_INCLUDE_DIRS})
#find_package(libastrolabe CONFIG REQUIRED  )

add_executable(astrolabe source/main.cpp)
target_include_directories(astrolabe PRIVATE include)
#target_link_libraries(astrolabe ${Protobuf_LIBRARIES})

target_link_libraries(astrolabe PRIVATE astrolib)
