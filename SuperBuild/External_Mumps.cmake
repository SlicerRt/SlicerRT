set(proj Mumps)

# MUMPS requires optimization flag to be enabled and Unix-like system
if(NOT EXTENSION_BUILDS_IPOPT)
  message(STATUS "MUMPS build skipped. Enable with EXTENSION_BUILDS_IPOPT=ON")
  ExternalProject_Add_Empty(${proj} DEPENDS "")
  mark_as_superbuild(${proj}_DIR:PATH)
  return()
endif()

if(WIN32)
  message(WARNING "MUMPS build is only supported on Unix-like systems (Linux, macOS). Skipping on Windows.")
  ExternalProject_Add_Empty(${proj} DEPENDS "")
  mark_as_superbuild(${proj}_DIR:PATH)
  return()
endif()

# Set dependency list
set(${proj}_DEPENDS "")

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  message(FATAL_ERROR "Enabling ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} is not supported !")
endif()

# Sanity checks
if(DEFINED Mumps_DIR AND NOT EXISTS ${Mumps_DIR})
  message(FATAL_ERROR "Mumps_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED ${proj}_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY
    "https://github.com/coin-or-tools/ThirdParty-Mumps.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG
    "stable/3.0"
    QUIET
    )

  set(EP_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)
  set(EP_INSTALL_DIR ${CMAKE_BINARY_DIR}/${proj}-install)

  set(EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS)

  # Configure CFLAGS and CXXFLAGS for the build
  set(EP_C_FLAGS "${ep_common_c_flags}")
  set(EP_CXX_FLAGS "${ep_common_cxx_flags}")

  # Handle LAPACK dependencies
  # Use standard LAPACK/BLAS libraries
  set(MUMPS_CONFIGURE_ARGS "--with-lapack-lflags=-llapack -lblas")

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY}"
    GIT_TAG "${${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG}"
    SOURCE_DIR ${EP_SOURCE_DIR}
    BINARY_DIR ${EP_BINARY_DIR}
    INSTALL_DIR ${EP_INSTALL_DIR}
    CONFIGURE_COMMAND
      ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> ./get.Mumps
      COMMAND
      ${CMAKE_COMMAND} -E env
        CC=${CMAKE_C_COMPILER}
        CXX=${CMAKE_CXX_COMPILER}
        FC=gfortran
        F77=gfortran
        CFLAGS=${EP_C_FLAGS}
        CXXFLAGS=${EP_CXX_FLAGS}
        FFLAGS=-fPIC
      <SOURCE_DIR>/configure
        --prefix=<INSTALL_DIR>
        --enable-shared=no
        --enable-static=yes
        ${MUMPS_CONFIGURE_ARGS}
    BUILD_COMMAND $(MAKE)
    INSTALL_COMMAND $(MAKE) install
    DEPENDS
      ${${proj}_DEPENDS}
    )
  set(${proj}_DIR ${EP_INSTALL_DIR})

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDS})
endif()

mark_as_superbuild(${proj}_DIR:PATH)
