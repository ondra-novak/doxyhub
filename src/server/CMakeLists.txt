cmake_minimum_required(VERSION 2.8) 
add_compile_options(-std=c++14)
file(GLOB doxyhub_SRC "*.cpp" )
file(GLOB doxyhub_HDR "*.h" "*.tcc")

add_executable (doxyhub ${doxyhub_SRC} )
target_link_libraries (doxyhub LINK_PUBLIC  simpleRpcServer simpleServer couchit imtjson ssl crypto pthread)
install(TARGETS doxyhub DESTINATION "bin") 
