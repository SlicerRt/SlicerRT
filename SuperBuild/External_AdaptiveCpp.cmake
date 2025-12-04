set(proj AdaptiveCpp)

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

# Sanity checks
if(DEFINED AdaptiveCpp_DIR AND NOT EXISTS ${AdaptiveCpp_DIR})
  message(FATAL_ERROR "AdaptiveCpp_DIR variable is defined but corresponds to nonexistent directory")
endif()

# Only build on Apple and Linux platforms
if(WIN32 OR NOT EXTENSION_BUILDS_ADAPTIVE_CPP)
  message(STATUS "AdaptiveCpp is not supported on Windows - skipping")
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDS})
  mark_as_superbuild(${proj}_DIR:PATH)
  return()
endif()

if(NOT DEFINED ${proj}_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY
    "https://github.com/AdaptiveCpp/AdaptiveCpp.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG
    "v25.10.0" # Latest stable release
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
      # Install directories - AdaptiveCpp needs proper installation
      -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/${proj}-install
      -DAdaptiveCpp_INSTALL_BIN_DIR:PATH=${Slicer_THIRDPARTY_BIN_DIR}
      -DAdaptiveCpp_INSTALL_LIB_DIR:PATH=${Slicer_THIRDPARTY_LIB_DIR}
      # Dependencies - AdaptiveCpp is largely self-contained
      # Options for AdaptiveCpp build
      -DBUILD_SHARED_LIBS:BOOL=OFF
      -DBUILD_TESTING:BOOL=OFF
      -DADAPTIVECPP_BUILD_EXAMPLES:BOOL=OFF
      -DADAPTIVECPP_BUILD_TESTS:BOOL=OFF
      -DADAPTIVECPP_BUILD_BENCHMARKS:BOOL=OFF
      # Enable the core SYCL functionality
      -DADAPTIVECPP_ENABLE_STDPAR:BOOL=ON
      -DADAPTIVECPP_ENABLE_OPENMP_BACKENDS:BOOL=ON
      # Disable potentially problematic features for a superbuild
      -DADAPTIVECPP_WITH_ROCM_BACKEND:BOOL=OFF
      -DADAPTIVECPP_WITH_CUDA_BACKEND:BOOL=OFF
      -DADAPTIVECPP_WITH_LEVEL_ZERO_BACKEND:BOOL=OFF
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
