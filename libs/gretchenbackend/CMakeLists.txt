message(STATUS "processing gretchen audio backend")


# portaudio dependency
find_path(portaudio_include NAMES portaudio.h)
find_library(portaudio_library NAMES portaudio)
find_package_handle_standard_args(Portaudio DEFAULT_MSG portaudio_library portaudio_include)
mark_as_advanced(portaudio_include portaudio_library)
if (PORTAUDIO_FOUND)
        add_library(portaudio STATIC IMPORTED)
    set_target_properties(portaudio PROPERTIES IMPORTED_LOCATION "${portaudio_library}" INTERFACE_INCLUDE_DIRECTORIES "${portaudio_include}")
    set(DEP_LIBRARIES ${DEP_LIBRARIES} portaudio)
else()
    unset(PORTAUDIO CACHE)
    message(WARNING "

could not find portaudio. 
you can get portaudio from http://www.portaudio.com/download.html

")
endif()

add_library(gretchenbackend
  STATIC
  playback.c
  record.c
  sigcatch.c 
)

target_include_directories(gretchenbackend PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIAVTE src)
set_target_properties(gretchenbackend PROPERTIES PUBLIC_HEADER "gretchen.backend.h")
target_link_libraries(gretchenbackend portaudio)


add_executable(querypa testpa.c)
target_link_libraries(querypa gretchenbackend)

add_executable(testsig testsigaction.c)
target_link_libraries(testsig gretchenbackend)


