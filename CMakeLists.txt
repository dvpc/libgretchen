cmake_minimum_required(VERSION 2.6)
project(gretchen)
include(CheckLibraryExists)
include(FindPackageHandleStandardArgs)

# Guess the platform
# http://cmake.3232098.n2.nabble.com/How-to-check-the-operating-system-is-MacOSX-td7581659.html
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(WINDOWS TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LINUX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()


if(LINUX)
    # https://stackoverflow.com/questions/16006145/ld-undefined-reference-to-symbol-log2glibc-2-2-5#16006618
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lm")
endif()

# use local header files
include_directories(libs/external/)


add_subdirectory(libs/gretchen)
include_directories(libs/gretchen/include)

add_subdirectory(libs/gretchenbackend)
include_directories(libs/gretchenbackend/include)



add_executable(gret sandbox/gret.c)
target_link_libraries(gret gretchenbackend gretchen)




add_executable(enc sandbox/enc.c)
target_link_libraries(enc gretchen)
add_executable(dec sandbox/dec.c)
target_link_libraries(dec gretchen)

add_executable(noise sandbox/noise.c) 
target_link_libraries(noise gretchenbackend gretchen m) 

add_executable(rec sandbox/rec2.c) 
target_link_libraries(rec gretchenbackend gretchen)
add_executable(play sandbox/play2.c) 
target_link_libraries(play gretchenbackend gretchen) 


add_executable(testrecdev libs/gretchenbackend/testestrecdevice.c)
target_link_libraries(testrecdev gretchenbackend gretchen)

