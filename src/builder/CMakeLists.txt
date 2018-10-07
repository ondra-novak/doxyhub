cmake_minimum_required(VERSION 2.8) 
add_compile_options(-std=c++14)
add_compile_options(-Wall)
file(GLOB docbuilder_SRC "*.cpp" )
file(GLOB docbuilder_HDR "*.h" "*.tcc")

add_executable (docbuilder ${docbuilder_SRC} )
target_link_libraries (docbuilder LINK_PUBLIC zwebpak simpleServer couchit imtjson ssl z crypto pthread)
install(TARGETS docbuilder DESTINATION "bin") 
