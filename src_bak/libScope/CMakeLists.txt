
#
# Find libpico
find_package(Pico)

set(ENABLE_SCOPE ${LIBPICO_FOUND} CACHE BOOL "Enable libScope")

if ( NOT "${ENABLE_SCOPE}" )
  message("Disabling libScope due to missing libpico")
  return()
endif()

#
# Prepare the library

# Add the headers
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../libCom )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../libUtils )

# Add all *.c* files as source code
set(SrcFiles PicoScope.cpp
  PS6000.cpp
  Tektronix654C.cpp
)

include_directories( ${LIBPICO_INCLUDE_DIR} )
add_library(Scope SHARED ${SrcFiles})
target_link_libraries(Scope COM ${LIBPICO_LIBRARIES})
add_dependencies(Scope COM)
