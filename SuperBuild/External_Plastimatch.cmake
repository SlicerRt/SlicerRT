
set(proj Plastimatch)

# Set dependency list
set(${proj}_DEPENDS "")
if(DEFINED Slicer_SOURCE_DIR)
  list(APPEND ${proj}_DEPENDS
    DCMTK
    ITK
    VTK
    )
endif()

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  message(FATAL_ERROR "Enabling ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} is not supported !")
endif()

# Sanity checks
if(DEFINED Plastimatch_DIR AND NOT EXISTS ${Plastimatch_DIR})
  message(FATAL_ERROR "Plastimatch_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED ${proj}_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY
    "https://github.com/SlicerRT/plastimatch.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG
    "889a36f0340e4723e79747562683c45523caa338" # Temporary fix to prevent compilation error with ITK-5.3 and compile errors with C++17
    QUIET
    )

  set(EP_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)

  set(EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS)

  set(PYTHON_VARS)
  if (VTK_WRAP_PYTHON)
    list(APPEND PYTHON_VARS
    -DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
    -DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}
    -DPYTHON_INCLUDE_DIR:FILEPATH=${PYTHON_INCLUDE_DIR}
    -DPython3_EXECUTABLE:FILEPATH=${Python3_EXECUTABLE}
    -DPython3_LIBRARY:FILEPATH=${Python3_LIBRARY}
    -DPython3_INCLUDE_DIR:PATH=${Python3_INCLUDE_DIR}
    )
  endif()

  if(WIN32)
    # Disable OpenMP on Windows until a better solution can be found
    # http://www.na-mic.org/Bug/view.php?id=3823
    list(APPEND EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS
      "-DPLM_CONFIG_NOMANIFEST:BOOL=ON"  # Internally set PLM_LINK_FLAGS to build without manifest on windows
      )
  endif()

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY}"
    GIT_TAG "${${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG}"
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
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_BIN_DIR}
      -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_LIB_DIR}
      -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
      # Install directories
      # NA
      # Dependencies (satisfied by Slicer)
      -DDCMTK_DIR:STRING=${DCMTK_DIR}  # XXX What happen is Slicer is built without DICOM support ?
      -DITK_DIR:STRING=${ITK_DIR}
      -DVTK_DIR:STRING=${VTK_DIR}
      # Options
      -DBUILD_SHARED_LIBS:BOOL=OFF
      -DBUILD_TESTING:BOOL=OFF
      -DPLM_CONFIG_ENABLE_CUDA:BOOL=OFF  # CUDA build is disabled until ticket #226 can be resolved.
      -DPLM_CONFIG_LIBRARY_BUILD:BOOL=ON
      -DPLM_CONFIG_INSTALL_LIBRARIES:BOOL=ON
      -DPLM_PREFER_SYSTEM_DLIB:BOOL=OFF
      -DPLMLIB_CONFIG_ENABLE_REGISTER:BOOL=TRUE
      -DPLMLIB_CONFIG_ENABLE_RECONSTRUCT:BOOL=TRUE
      -DPLMLIB_CONFIG_ENABLE_DOSE:BOOL=TRUE
      ${EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS}
      ${PYTHON_VARS}
    INSTALL_COMMAND ""
    DEPENDS
      ${${proj}_DEPENDS}
    )
  set(${proj}_DIR ${CMAKE_BINARY_DIR}/${proj}-build)

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDS})
endif()

mark_as_superbuild(${proj}_DIR:PATH)

