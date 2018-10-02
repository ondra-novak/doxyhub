cmake_minimum_required(VERSION 2.8) 
add_compile_options(-std=c++11 -fPIC)
file(GLOB zwebpak_SRC "*.cpp")
file(GLOB zwebpak_HDR "*.h" "*.tcc")
add_library (zwebpak ${zwebpak_SRC})
# target_include_directories (zwebpak PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

