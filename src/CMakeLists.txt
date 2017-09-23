find_package (PkgConfig REQUIRED)
pkg_check_modules (GLIB REQUIRED glib-2.0)
pkg_check_modules (GIO REQUIRED gio-2.0)

if(ENABLE_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEBUG")
endif()

set(SOURCE_FILES agent.c a2dp-endpoint.c main.c)

include_directories (${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
link_directories (${GLIB_LIBRARY_DIRS} ${GIO_LIBRARY_DIRS})

add_executable(bluez-player ${SOURCE_FILES})
target_link_libraries (bluez-player ${GLIB_LIBRARIES} ${GIO_LIBRARIES})
