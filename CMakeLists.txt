cmake_minimum_required(VERSION 3.17)
project(porting)
add_executable(porting porting_packages_oop_all.cpp)
target_link_libraries(porting stdc++fs)
