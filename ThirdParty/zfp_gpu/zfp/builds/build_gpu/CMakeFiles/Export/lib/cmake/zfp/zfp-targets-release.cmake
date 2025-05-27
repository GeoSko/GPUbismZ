#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "zfp::zfp" for configuration "Release"
set_property(TARGET zfp::zfp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(zfp::zfp PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libgpuzfp.so.1.0.1"
  IMPORTED_SONAME_RELEASE "libgpuzfp.so.1"
  )

list(APPEND _IMPORT_CHECK_TARGETS zfp::zfp )
list(APPEND _IMPORT_CHECK_FILES_FOR_zfp::zfp "${_IMPORT_PREFIX}/lib/libgpuzfp.so.1.0.1" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
