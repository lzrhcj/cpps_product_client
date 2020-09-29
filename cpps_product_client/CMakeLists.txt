cmake_minimum_required(VERSION 3.1)
project(cpps_product_client)

set (CMAKE_CXX_STANDARD 14)

find_package(Threads)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} /usr/include/mysql)

add_executable(cpps_product_client main.cpp database_function.cpp ua_function.cpp open62541.c)

LINK_DIRECTORIES(/usr/lib/x86_64-linux-gnu/)
target_link_libraries(cpps_product_client ${CMAKE_THREAD_LIBS_INIT} mysqlclient)
