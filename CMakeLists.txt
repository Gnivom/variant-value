cmake_minimum_required (VERSION 3.8)

set (CMAKE_CXX_STANDARD 17)

include_directories ("code/include")

add_executable (variant-value "code/main.cpp")

#target_compile_options (variant-value PRIVATE /d1reportTime)