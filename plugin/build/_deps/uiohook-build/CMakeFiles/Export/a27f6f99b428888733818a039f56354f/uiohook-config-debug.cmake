#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "uiohook" for configuration "Debug"
set_property(TARGET uiohook APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(uiohook PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/uiohook.lib"
  )

list(APPEND _cmake_import_check_targets uiohook )
list(APPEND _cmake_import_check_files_for_uiohook "${_IMPORT_PREFIX}/lib/uiohook.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
