set(proj HSL)

# HSL (Harwell Subroutines Library) - High-performance linear solvers for Ipopt
# HSL can provide better performance than MUMPS for some optimization problems
#
# Two ways to use HSL on Linux — no license redistribution is possible either way:
#
# Option A — Runtime (no rebuild needed):
#   1. Get libhsl.so from STFC (e.g. https://www.hsl.rl.ac.uk/download/MA57/3.11.3/)
#   2. export LD_LIBRARY_PATH=/path/to/libhsl.so/dir:$LD_LIBRARY_PATH
#   3. Set Ipopt option linear_solver=ma57 (or ma27/ma97) at runtime
#
# Option B — Compiled in (self-contained build, all HSL solvers):
#   1. Download Coin-HSL source from https://licences.stfc.ac.uk/product/coin-hsl
#   2. Extract the archive (gives a directory with MA27/MA57/MA77/MA86/MA97 source)
#   3. cmake -DEXTENSION_HSL_SOURCE_DIR=/path/to/extracted/coinhsl ...
#      The build system copies the source and compiles it automatically.

if(NOT EXTENSION_BUILDS_IPOPT OR NOT EXTENSION_USES_HSL)
  message(STATUS "HSL build skipped. Enable with EXTENSION_BUILDS_IPOPT=ON and EXTENSION_USES_HSL=ON")
  ExternalProject_Add_Empty(${proj} DEPENDS "")
  mark_as_superbuild(${proj}_DIR:PATH)
  return()
endif()

if(WIN32)
  message(STATUS "HSL source build is not supported on Windows. "
    "To use HSL on Windows, obtain libhsl.dll from https://licences.stfc.ac.uk/product/coin-hsl. "
    "Option A (no rebuild): copy libhsl.dll next to ipopt-3.dll in the Slicer install directory. "
    "Option B (automated, requires rebuild): set EXTENSION_HSL_DLL_DIR to the directory containing libhsl.dll.")
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
if(DEFINED HSL_DIR AND NOT EXISTS ${HSL_DIR})
  message(FATAL_ERROR "HSL_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED ${proj}_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY
    "https://github.com/coin-or-tools/ThirdParty-HSL.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG
    "stable/2.2"
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
  # Use standard LAPACK/BLAS libraries (same as MUMPS)
  set(HSL_CONFIGURE_ARGS "--with-lapack-lflags=-llapack -lblas")

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY}"
    GIT_TAG "${${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG}"
    SOURCE_DIR ${EP_SOURCE_DIR}
    BINARY_DIR ${EP_BINARY_DIR}
    INSTALL_DIR ${EP_INSTALL_DIR}
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_directory "${EXTENSION_HSL_SOURCE_DIR}" <SOURCE_DIR>/coinhsl
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
        ${HSL_CONFIGURE_ARGS}
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
