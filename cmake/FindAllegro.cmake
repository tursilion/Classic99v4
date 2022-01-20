# Try to find Allegro
#  ALLEGRO_FOUND - System has Allegro
#  ALLEGRO_INCLUDE_DIRS - The Allegro include directories
#  ALLEGRO_LIBRARIES - The libraries needed to use Allegro
#  ALLEGRO_DEFINITIONS - Compiler switches required for using Allegro
# warning: only searches for the libs I need for Classic99

find_package(PkgConfig)
pkg_check_modules(PC_ALLEGRO QUIET allegro-5.0)
set(ALLEGRO_DEFINITIONS ${PC_ALLLEGRO_CFLAGS_OTHER})

set(ALLEGRO_INCLUDE_HINTS
    ${PC_ALLEGRO_INCLUDEDIR} ${PC_ALLEGRO_INCLUDE_DIRS})
set(ALLEGRO_LIBRARY_HINTS
    ${PC_ALLEGRO_LIBDIR} ${PC_ALLEGRO_LIBRARY_DIRS})

find_path(ALLEGRO_INCLUDE_DIR allegro5/allegro.h
    HINTS ${ALLEGRO_INCLUDE_HINTS})

find_library(ALLEGRO_LIBRARY
    NAMES allegro allegro-5.0.2-mt
    HINTS ${ALLEGRO_LIBRARY_HINTS})

find_library(ALLEGRO_MAIN_LIBRARY
    NAMES allegro_main allegro_main-5.0.2-mt
    HINTS ${ALLEGRO_LIBRARY_HINTS})

find_library(ALLEGRO_DIALOG_LIBRARY
    NAMES allegro_dialog allegro_dialog-5.0.2-mt
    HINTS ${ALLEGRO_LIBRARY_HINTS})

find_library(ALLEGRO_IMAGE_LIBRARY
    NAMES allegro_image allegro_image-5.0.2-mt
    HINTS ${ALLEGRO_LIBRARY_HINTS})

find_library(ALLEGRO_AUDIO_LIBRARY
    NAMES allegro_audio allegro_audio-5.0.2-mt
    HINTS ${ALLEGRO_LIBRARY_HINTS})

set(ALLEGRO_LIBRARIES
        ${ALLEGRO_LIBRARY}
        ${ALLEGRO_MAIN_LIBRARY}
        ${ALLEGRO_DIALOG_LIBRARY}
        ${ALLEGRO_IMAGE_LIBRARY}
        ${ALLEGRO_AUDIO_LIBRARY}
)
set(ALLEGRO_INCLUDE_DIRS ${ALLEGRO_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ALLEGRO_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Allegro DEFAULT_MSG
    ALLEGRO_LIBRARY
    ALLEGRO_MAIN_LIBRARY
    ALLEGRO_DIALOG_LIBRARY
    ALLEGRO_AUDIO_LIBRARY
    ALLEGRO_INCLUDE_DIR
)

mark_as_advanced(ALLEGRO_INCLUDE_DIR ALLEGRO_LIBRARY ALLEGRO_MAIN_LIBRARY
    ALLEGRO_DIALOG_LIBRARY ALLEGRO_AUDIO_LIBRARY)

