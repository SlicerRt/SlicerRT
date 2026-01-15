set(proj Ipopt)

# IPOPT - Interior Point Optimizer
# UNIX: Built from source using autotools with system LAPACK/BLAS.
#   Linear solvers:
#   - MUMPS: statically linked, built automatically
#   - HSL (MA27/MA57/MA77/MA86/MA97): two options:
#       (a) Compile in: set EXTENSION_HSL_SOURCE_DIR to your Coin-HSL source
#           (free academic license: https://licences.stfc.ac.uk/product/coin-hsl)
#       (b) Runtime load: set LD_LIBRARY_PATH to a directory containing libhsl.so
#           (e.g. https://www.hsl.rl.ac.uk/download/MA57/3.11.3/), no rebuild needed
#
# WINDOWS: Prebuilt binaries downloaded from COIN-OR releases (MUMPS included).
#   HSL: place libhsl.dll directory in EXTENSION_HSL_DLL_DIR; the linear solver
#   loader in ipopt-3.dll will find it automatically at runtime.
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

  # Copy HSL DLL alongside ipopt-3.dll so the linear solver loader finds it at runtime.
  # This runs as a build-time step (after install) because the bin/ directory is
  # populated by ExternalProject, not available at CMake configure time.
  if(EXTENSION_USES_HSL)
    ExternalProject_Add_Step(${proj} copy_hsl_dlls
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${EXTENSION_HSL_DLL_DIR}" "${IPOPT_WIN_REAL_DIR}/bin"
      DEPENDEES install
    )
  endif()

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
if(EXTENSION_BUILDS_IPOPT AND UNIX)
  list(APPEND ${proj}_DEPENDS Mumps)
  if(EXTENSION_USES_HSL)
    list(APPEND ${proj}_DEPENDS HSL)
  endif()
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

  # Set up linker flags
  set(MUMPS_LFLAGS "-L${Mumps_DIR}/lib -lcoinmumps -lmetis -llapack -lblas -lgfortran -lm -lquadmath -lpthread")

  set(IPOPT_HSL_CONFIGURE_ARGS "")
  if(EXTENSION_USES_HSL)
    set(HSL_LFLAGS "-L${HSL_DIR}/lib -lcoinhsl -llapack -lblas -lgfortran -lm -lquadmath -lpthread")
    set(IPOPT_HSL_CONFIGURE_ARGS
      "--with-hsl-cflags=-I${HSL_DIR}/include/coin-or"
      "--with-hsl-lflags=${HSL_LFLAGS}"
    )
  endif()

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
        ${IPOPT_HSL_CONFIGURE_ARGS}
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
