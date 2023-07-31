message(STATUS "Configuring ${CMAKE_CURRENT_LIST_DIR}")

set(coreLib ${LIBS} ${CMAKE_THREAD_LIBS_INIT} ${PROJECT_NAME} -ldl)
message("coreLib: ${coreLib}")
message("HEADERS: ${HEADERS}")

add_executable(test_cache_server test_cache_server.cpp)
target_link_libraries(test_cache_server ${coreLib})

add_test(NAME test_cache_server COMMAND test_cache_server WORKING_DIRECTORY .)