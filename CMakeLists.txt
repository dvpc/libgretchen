cmake_minimum_required(VERSION 2.6)
project(gretchen)
include(CheckLibraryExists)
include(FindPackageHandleStandardArgs)



add_subdirectory(libs/gretchen)
include_directories(libs/gretchen/include)

add_subdirectory(libs/gretchenbackend)
include_directories(libs/gretchenbackend/include)


add_executable(enc sandbox/enc.c)
add_executable(dec sandbox/dec.c)
target_link_libraries(enc gretchen)
target_link_libraries(dec gretchen)

# the noise.c needs this math linker flag...
# https://stackoverflow.com/questions/16006145/ld-undefined-reference-to-symbol-log2glibc-2-2-5#16006618
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lm")
add_executable(noise sandbox/noise.c) 
target_link_libraries(noise gretchenbackend gretchen) 

add_executable(rec sandbox/rec.c) 
add_executable(play sandbox/play.c) 
target_link_libraries(rec gretchenbackend gretchen)
target_link_libraries(play gretchenbackend gretchen) 
