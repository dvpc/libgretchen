message(STATUS "processing gretchen")

# FIXME this is a dynamic ref
# so the output still depends on dynamically linking this...
# ...
# liquid sdr dependency
#find_path(liquid_include NAMES liquid/liquid.h)
#find_library(liquid_library NAMES liquid)
#find_package_handle_standard_args(Liquid DEFAULT_MSG liquid_library liquid_include)
#mark_as_advanced(liquid_include liquid_library)
#if(LIQUID_FOUND)
    #add_library(liquid STATIC IMPORTED)
    #set_target_properties(liquid PROPERTIES IMPORTED_LOCATION "${liquid_library}" INTERFACE_INCLUDE_DIRECTORIES "${liquid_include}")
#else()
    #unset(LIQUID_FOUND CACHE)
    #message(FATAL_ERROR "

#could not find libliquid.
#you can get libliquid from http://liquidsdr.org

#")
#endif()

ADD_LIBRARY(fec STATIC IMPORTED)
SET_TARGET_PROPERTIES(fec PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/external/libfec/libfec.a)

ADD_LIBRARY(liquid STATIC IMPORTED)
SET_TARGET_PROPERTIES(liquid PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/external/liquid/libliquid.a)

add_library(gretchen
  STATIC
  src/helper/src/helper.c
  src/helper/src/hashmap.c
  src/helper/src/ringbuffer_u.c
  src/modem/src/modemrx.c
  src/modem/src/modemtx.c
  src/modem/src/modulatorrx.c
  src/modem/src/modulatortx.c
  src/modem/src/options.c
  src/modem/src/rx.c
  src/modem/src/tx.c
)

target_include_directories(gretchen PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIAVTE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(gretchen PROPERTIES PUBLIC_HEADER "gretchen.h")
target_link_libraries(gretchen liquid fec)


# from cmake tutorial install headers and .a
#install (TARGETS MathFunctions DESTINATION bin)
#install (FILES MathFunctions.h DESTINATION include)





add_executable(testopt
        src/modem/tests/testoptions.c
        src/modem/src/options.c)
add_executable(testmodem 
        src/modem/tests/testmodem.c
        src/modem/src/modulatortx.c
        src/modem/src/modulatorrx.c)

add_executable(testrxenv 
        src/helper/tests/testrxenv.c 
        src/helper/src/helper.c 
        src/helper/src/hashmap.c)
add_executable(testrxtransm 
        src/helper/tests/testrxtransm.c 
        src/helper/src/helper.c 
        src/helper/src/hashmap.c)
add_executable(testrxreap 
        src/helper/tests/testrxreap.c 
        src/helper/src/helper.c 
        src/helper/src/hashmap.c)

add_executable(testmodtx 
        src/modem/tests/testtx.c 
        src/modem/src/tx.c 
        src/modem/src/modemtx.c 
        src/modem/src/modulatortx.c 
        src/modem/src/options.c 
        src/helper/src/helper.c 
        src/helper/src/ringbuffer_u.c 
        src/helper/src/hashmap.c)

add_executable(testmodrx 
        src/modem/tests/testrx.c 
        src/modem/src/rx.c 
        src/modem/src/modemrx.c 
        src/modem/src/modulatorrx.c 
        src/modem/src/options.c 
        src/helper/src/helper.c 
        src/helper/src/ringbuffer_u.c 
        src/helper/src/hashmap.c)

target_link_libraries(testopt gretchen)
target_link_libraries(testmodem gretchen) 
target_link_libraries(testrxenv gretchen) 
target_link_libraries(testrxtransm gretchen) 
target_link_libraries(testrxreap gretchen) 
target_link_libraries(testmodtx gretchen) 
target_link_libraries(testmodrx gretchen) 


