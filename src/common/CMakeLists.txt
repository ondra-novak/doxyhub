cmake_minimum_required(VERSION 2.8) 
add_compile_options(-std=c++11 -fPIC)
file(GLOB common_SRC "*.cpp")
add_library (common ${common_SRC})
# target_include_directories (common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

