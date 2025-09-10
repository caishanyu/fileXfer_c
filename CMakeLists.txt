cmake_minimum_required(VERSION 3.11.4)
project(fileXfer_c)

include_directories(include)

# clients
add_executable(clients source/clients.c)

# server
add_executable(server source/server.c)
target_link_libraries(server pthread)