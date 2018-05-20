message(STATUS "processing gretchen")


# liquid sdr dependency
find_path(liquid_include NAMES liquid/liquid.h)
find_library(liquid_library NAMES liquid)
find_package_handle_standard_args(Liquid DEFAULT_MSG liquid_library liquid_include)
mark_as_advanced(liquid_include liquid_library)
if(LIQUID_FOUND)
    add_library(liquid STATIC IMPORTED)
    set_target_properties(liquid PROPERTIES IMPORTED_LOCATION "${liquid_library}" INTERFACE_INCLUDE_DIRECTORIES "${liquid_include}")
else()
    unset(LIQUID_FOUND CACHE)
    message(FATAL_ERROR "

could not find libliquid.
you can get libliquid from http://liquidsdr.org

")
endif()

add_library(gretchen
  STATIC
  src/auxm/src/grt_aux.c
  src/auxm/src/hashmap.c
  src/auxm/src/ringbuffer_u.c
  src/modem/src/grt_modemrx.c
  src/modem/src/grt_modemtx.c
  src/modem/src/grt_modulatorrx.c
  src/modem/src/grt_modulatortx.c
  src/modem/src/grt_options.c
  src/modem/src/grt_rx.c
  src/modem/src/grt_tx.c
)

target_include_directories(gretchen PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIAVTE include/internal)
set_target_properties(gretchen PROPERTIES PUBLIC_HEADER "gretchen.h")
target_link_libraries(gretchen liquid)


add_executable(testopt
        src/modem/tests/testoptions.c
        src/modem/src/grt_options.c)
add_executable(testmodem 
        src/modem/tests/testmodem.c
        src/modem/src/grt_modulatortx.c
        src/modem/src/grt_modulatorrx.c)

add_executable(testrxenv 
        src/auxm/tests/testrxenv.c 
        src/auxm/src/grt_aux.c 
        src/auxm/src/hashmap.c)
add_executable(testrxtransm 
        src/auxm/tests/testrxtransm.c 
        src/auxm/src/grt_aux.c 
        src/auxm/src/hashmap.c)
add_executable(testrxreap 
        src/auxm/tests/testrxreap.c 
        src/auxm/src/grt_aux.c 
        src/auxm/src/hashmap.c)

add_executable(testmodtx 
        src/modem/tests/testtx.c 
        src/modem/src/grt_tx.c 
        src/modem/src/grt_modemtx.c 
        src/modem/src/grt_modulatortx.c 
        src/modem/src/grt_options.c 
        src/auxm/src/grt_aux.c 
        src/auxm/src/ringbuffer_u.c 
        src/auxm/src/hashmap.c)

add_executable(testmodrx 
        src/modem/tests/testrx.c 
        src/modem/src/grt_rx.c 
        src/modem/src/grt_modemrx.c 
        src/modem/src/grt_modulatorrx.c 
        src/modem/src/grt_options.c 
        src/auxm/src/grt_aux.c 
        src/auxm/src/ringbuffer_u.c 
        src/auxm/src/hashmap.c)

target_link_libraries(testopt gretchen)
target_link_libraries(testmodem gretchen) 
target_link_libraries(testrxenv gretchen) 
target_link_libraries(testrxtransm gretchen) 
target_link_libraries(testrxreap gretchen) 
target_link_libraries(testmodtx gretchen) 
target_link_libraries(testmodrx gretchen) 


