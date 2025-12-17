set(proj Ipopt)

# IPOPT - Interior Point Optimizer
# Ipopt is configured with both MUMPS and HSL linear solvers for optimal performance.
#
# UNIX: This configuration supports Linux and macOS systems
# and uses autotools build system with system LAPACK/BLAS libraries.
#
# Included Linear Solvers:
# - MUMPS: Open-source, automatically downloaded and built
# - HSL (Harwell Subroutines Library): High-performance solvers
#   * Requires manual download due to licensing (free for academic use)
#   * Download from: https://licences.stfc.ac.uk/product/coin-hsl
#   * Extract to 'coinhsl' directory in HSL source tree
#   * HSL provides MA27, MA57, MA77, MA86, MA97 solvers
#
# WINDOWS: Prebuilt binaries are downloaded and extracted, MUMPS included.
#
# For more information see: https://coin-or.github.io/Ipopt/INSTALL.html

# Ipopt requires optimization flag to be enabled and Unix-like system
if(NOT EXTENSION_BUILDS_IPOPT)
  message(STATUS "Ipopt build skipped. Enable with EXTENSION_BUILDS_IPOPT=ON")
  ExternalProject_Add_Empty(${proj} DEPENDS "")
  mark_as_superbuild(${proj}_DIR:PATH)
  return()
endif()

# Windows: Download and extract prebuilt Ipopt binaries
if(WIN32)
  set(IPOPT_WIN_URL "https://github.com/coin-or/Ipopt/releases/download/releases%2F3.14.19/Ipopt-3.14.19-win64-msvs2022-md.zip")
  set(IPOPT_WIN_ZIP "${CMAKE_BINARY_DIR}/Ipopt-3.14.19-win64-msvs2022-md.zip")
  set(IPOPT_WIN_DIR "${CMAKE_BINARY_DIR}/Ipopt-3.14.19-win64-msvs2022-md")

  ExternalProject_Add(${proj}
    URL ${IPOPT_WIN_URL}
    DOWNLOAD_DIR ${CMAKE_BINARY_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND
    "${CMAKE_COMMAND};-E;remove_directory;${IPOPT_WIN_DIR}"
    "${CMAKE_COMMAND};-E;make_directory;${IPOPT_WIN_DIR}"
    "${CMAKE_COMMAND};-E;tar;xvf;${IPOPT_WIN_ZIP};--directory=${IPOPT_WIN_DIR}"
    LOG_DOWNLOAD ON
    LOG_INSTALL ON
  )

  set(IPOPT_WIN_REAL_DIR "${CMAKE_BINARY_DIR}/Ipopt-prefix/src/Ipopt")

  set(${proj}_DIR ${IPOPT_WIN_REAL_DIR})
  set(${proj}_INCLUDE_DIR "${IPOPT_WIN_REAL_DIR}/include/coin-or")
  set(${proj}_LIBRARY_DIR "${IPOPT_WIN_REAL_DIR}/lib")
  set(${proj}_DLL_DIR "${IPOPT_WIN_REAL_DIR}/bin")
  set(${proj}_DLL "${IPOPT_WIN_REAL_DIR}/bin/ipopt-3.dll")

  # Cache variables for downstream use
  set(Ipopt_DIR "${${proj}_DIR}" CACHE PATH "Ipopt root directory")
  set(Ipopt_INCLUDE_DIR "${${proj}_INCLUDE_DIR}" CACHE PATH "Ipopt include directory")
  set(Ipopt_LIBRARY_DIR "${${proj}_LIBRARY_DIR}" CACHE PATH "Ipopt library directory")
  set(Ipopt_DLL_DIR "${${proj}_DLL_DIR}" CACHE PATH "Ipopt DLL directory")
  set(Ipopt_DLL "${${proj}_DLL}" CACHE FILEPATH "Ipopt DLL file")

  mark_as_superbuild(${proj}_DIR:PATH)
  return()
endif()

# Set dependency list
set(${proj}_DEPENDS "")
if(DEFINED Slicer_SOURCE_DIR)
  list(APPEND ${proj}_DEPENDS
    Mumps
    HSL
    )
endif()

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  message(FATAL_ERROR "Enabling ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} is not supported !")
endif()

# Sanity checks
if(DEFINED Ipopt_DIR AND NOT EXISTS ${Ipopt_DIR})
  message(FATAL_ERROR "Ipopt_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED ${proj}_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY
    "https://github.com/coin-or/Ipopt.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG
    "releases/3.14.19"
    QUIET
    )

  set(EP_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)
  set(EP_INSTALL_DIR ${CMAKE_BINARY_DIR}/${proj}-install)

  set(EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS)

  # Configure CFLAGS and CXXFLAGS for the build
  set(EP_C_FLAGS "${ep_common_c_flags}")
  set(EP_CXX_FLAGS "${ep_common_cxx_flags}")

  # Set up linker flags as variables
  set(MUMPS_LFLAGS "-L${Mumps_DIR}/lib -lcoinmumps -lmetis -llapack -lblas -lgfortran -lm -lquadmath -lpthread")
  # Uncomment the following lines when HSL is available:
  # set(HSL_LFLAGS "-L${HSL_DIR}/lib -lcoinhsl -lmetis -llapack -lblas -lgfortran -lm -lquadmath -lpthread")

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY}"
    GIT_TAG "${${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG}"
    SOURCE_DIR ${EP_SOURCE_DIR}
    BINARY_DIR ${EP_BINARY_DIR}
    INSTALL_DIR ${EP_INSTALL_DIR}
    CONFIGURE_COMMAND
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
        --with-mumps-cflags=-I${Mumps_DIR}/include/coin-or/mumps
        --with-mumps-lflags=${MUMPS_LFLAGS}
        # Uncomment the following lines when HSL is available:
        # --with-hsl-cflags=-I${HSL_DIR}/include/coin-or
        # --with-hsl-lflags=${HSL_LFLAGS}
        --disable-linear-solver-loader
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
