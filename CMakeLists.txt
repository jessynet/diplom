cmake_minimum_required(VERSION 3.17)
project(porting)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(OS_SOURCES
        windows.h windows.cpp
    )
elseif(UNIX)
    set(OS_SOURCES
        unix.h unix.cpp
        freebsd.h freebsd.cpp
        linux.h linux.cpp
        opensuse.h opensuse.cpp
        ubuntu.h ubuntu.cpp
    )
else()
    message(FATAL_ERROR "Unsupported OS ${CMAKE_SYSTEM_NAME}")
endif()

add_executable(pp
    main.cpp
    os.h os.cpp
    ${OS_SOURCES}
)

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10)
        target_link_libraries(pp c++fs)
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(pp stdc++fs)
else()
    message(FATAL_ERROR "Unsupported C++ filesystem library")
endif()

find_package(nlohmann_json REQUIRED)
target_link_libraries(pp nlohmann_json)
