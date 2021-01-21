cmake_minimum_required(VERSION 3.17)

project(MpvApp)

set(CMAKE_CXX_STANDARD 14)

include_directories("${CMAKE_SOURCE_DIR}/inc")
link_directories("${CMAKE_SOURCE_DIR}/lib")

add_executable(MpvApp main.cpp)
target_link_libraries(MpvApp libmpv -lOpenGL32 -lglfw3 -lglew32)
