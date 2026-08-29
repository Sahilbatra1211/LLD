cmake_minimum_required(VERSION 3.20)

project(LLD)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(HelloCpp)
add_subdirectory("Design Pattern/Strategy Design Pattern")

# When you add a new pattern, copy Strategy's CMakeLists.txt into that folder,
# then add a line here, for example:
# add_subdirectory("Design Pattern/Observer Design Pattern")
