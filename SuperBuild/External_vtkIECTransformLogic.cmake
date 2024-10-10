
set(proj vtkIECTransformLogic)

# Set dependency list
set(${proj}_DEPENDS "")
if(DEFINED Slicer_SOURCE_DIR)
  list(APPEND ${proj}_DEPENDS
    VTK
    )
endif()

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  message(FATAL_ERROR "Enabling ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} is not supported !")
endif()


if(NOT DEFINED ${proj}_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  set(EP_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)


  #message("Slicer_THIRDPARTY_LIB_DIR ZZZZZZZZ: ${Slicer_THIRDPARTY_LIB_DIR}")

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY https://github.com/EBATINCA/RadiotherapyTransformsIEC.git
    GIT_TAG origin/main
    SOURCE_DIR ${EP_SOURCE_DIR}
    BINARY_DIR ${EP_BINARY_DIR}
    CMAKE_CACHE_ARGS
      # Compiler settings
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
      -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
      -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
      # Output directories
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${EP_BINARY_DIR}
      -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${EP_BINARY_DIR}
      -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=${EP_BINARY_DIR}
      # Install directories
      # NA
      -DVTK_DIR:STRING=${VTK_DIR}
      # Options
      -DBUILD_SHARED_LIBS:BOOL=OFF
      -DBUILD_TESTING:BOOL=OFF
      -DPLM_CONFIG_ENABLE_CUDA:BOOL=OFF
      -DPLM_CONFIG_LIBRARY_BUILD:BOOL=ON
      -DPLM_CONFIG_INSTALL_LIBRARIES:BOOL=ON
      -DPLM_PREFER_SYSTEM_DLIB:BOOL=OFF
      -DPLMLIB_CONFIG_ENABLE_REGISTER:BOOL=TRUE
      -DPLMLIB_CONFIG_ENABLE_RECONSTRUCT:BOOL=TRUE
      -DPLMLIB_CONFIG_ENABLE_DOSE:BOOL=TRUE
    INSTALL_COMMAND ""
    DEPENDS
      ${${proj}_DEPENDS}
    )
  set(${proj}_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/${proj}/src;${CMAKE_BINARY_DIR}/${proj}-build/Debug)
  set(${proj}_DIR ${CMAKE_BINARY_DIR}/${proj}-build)

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDS})
endif()

#mark_as_superbuild(${proj}_INCLUDE_DIRS:PATH)
mark_as_superbuild(${proj}_DIR:PATH)
mark_as_superbuild(${proj}_INCLUDE_DIRS:PATH)
