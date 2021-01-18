cmake_minimum_required(VERSION 3.17)

project(MpvApp)

set(CMAKE_CXX_STANDARD 14)

add_library(libmpv SHARED IMPORTED) # or STATIC instead of SHARED
set_target_properties(libmpv PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libmpv.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/inc"
        )

add_executable(MpvApp main.cpp)
target_link_libraries(MpvApp libmpv)
