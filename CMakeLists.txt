cmake_minimum_required(VERSION 3.14)
project(cache_allocation)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB LIB_SOURCE ${PROJECT_SOURCE_DIR}/*.cpp)
message(STATUS "lib source: ${LIB_SOURCE}")
add_library(${PROJECT_NAME} SHARED ${LIB_SOURCE})

find_library(libCacheSim libCacheSim)
find_path(libCacheSimInclude libCacheSim)
message(STATUS "libCacheSim found? ${libCacheSim} ${libCacheSimInclude}")
include_directories(${libCacheSimInclude})
set(HEADERS ${HEADERS} ${libCacheSimInclude})
set(LIBS ${LIBS} ${libCacheSim})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")
find_package(GLib REQUIRED)
include_directories(${GLib_INCLUDE_DIRS})
message(STATUS "glib found? ${GLib_FOUNT}, lib = ${GLib_LIBRARY}, header = ${GLib_INCLUDE_DIRS}")
set(HEADERS ${HEADERS} ${GLib_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GLib_LIBRARY})

find_package(Threads)
message(STATUS "Threads found? ${THREAD_FOUND}")

link_libraries("-lpthread -ldl -lm ${LIBS}")

include(CTest)
enable_testing()
message(STATUS "Building with tests")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)

add_executable(simulator allocation_simulator.cpp)
target_link_libraries(simulator ${PROJECT_NAME} ${LIBS})
