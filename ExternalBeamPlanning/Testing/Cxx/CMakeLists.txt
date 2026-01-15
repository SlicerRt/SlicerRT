# Only build qSlicerIpoptOptimizer test if IPOPT is enabled
if(EXTENSION_BUILDS_IPOPT)
  # qSlicerIpoptOptimizer test executable
  set(IPOPT_OPTIMIZER_TEST_SRCS
    qSlicerIpoptOptimizerTest.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Widgets/qSlicerIpoptOptimizer.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Widgets/qSlicerIpoptOptimizer.h
  )

  # Find IPOPT installation (same pattern as main Testing/Cxx)
  if(WIN32)
    # Windows uses prebuilt binaries
    set(IPOPT_INCLUDE_DIRS "${Ipopt_INCLUDE_DIR}")
    set(IPOPT_LIBRARIES "${Ipopt_LIBRARY_DIR}/ipopt.dll.lib")
    set(IPOPT_DLL "${Ipopt_DLL}")
  else()
    # Unix systems use built libraries
    find_path(IPOPT_INCLUDE_DIRS
      NAMES IpIpoptApplication.hpp
      PATHS
        ${Ipopt_DIR}/include/coin-or
        ${CMAKE_BINARY_DIR}/Ipopt-install/include/coin-or
      NO_DEFAULT_PATH
    )

    find_library(IPOPT_LIBRARIES
      NAMES ipopt libipopt
      PATHS
        ${Ipopt_DIR}/lib
        ${CMAKE_BINARY_DIR}/Ipopt-install/lib
      NO_DEFAULT_PATH
    )
  endif()

  if(IPOPT_INCLUDE_DIRS AND IPOPT_LIBRARIES)
    # Create the test executable
    add_executable(qSlicerIpoptOptimizerTest ${IPOPT_OPTIMIZER_TEST_SRCS})
    set_target_properties(qSlicerIpoptOptimizerTest PROPERTIES AUTOMOC ON)

    # Set include directories
    target_include_directories(qSlicerIpoptOptimizerTest PRIVATE
      ${IPOPT_INCLUDE_DIRS}
      ${CMAKE_CURRENT_SOURCE_DIR}/../../Widgets
      ${CMAKE_BINARY_DIR}/ExternalBeamPlanning/Widgets
    )

    # Neutralize dllimport so qSlicerIpoptOptimizer compiles standalone
    target_compile_definitions(qSlicerIpoptOptimizerTest PRIVATE
      qSlicerExternalBeamPlanningModuleWidgets_STATIC
    )

    # Link libraries
    target_link_libraries(qSlicerIpoptOptimizerTest
      ${IPOPT_LIBRARIES}
      Qt5::Core
    )

    # Add additional system libraries that Ipopt might need on Unix
    if(UNIX)
      # Link Fortran runtime and METIS with explicit linker flags
      target_link_options(qSlicerIpoptOptimizerTest PRIVATE "-Wl,--no-as-needed")
      target_link_libraries(qSlicerIpoptOptimizerTest gfortran quadmath gcc_s metis)

      # Find and link all required dependencies
      set(IPOPT_DEPS)

      # System math libraries
      find_library(DL_LIBRARY dl)
      if(DL_LIBRARY)
        list(APPEND IPOPT_DEPS ${DL_LIBRARY})
      endif()

      find_library(M_LIBRARY m)
      if(M_LIBRARY)
        list(APPEND IPOPT_DEPS ${M_LIBRARY})
      endif()

      # BLAS/LAPACK libraries
      find_library(LAPACK_LIBRARY lapack)
      if(LAPACK_LIBRARY)
        list(APPEND IPOPT_DEPS ${LAPACK_LIBRARY})
      endif()

      find_library(BLAS_LIBRARY blas)
      if(BLAS_LIBRARY)
        list(APPEND IPOPT_DEPS ${BLAS_LIBRARY})
      endif()

      # MUMPS library (coinmumps is the COIN-OR wrapped version)
      find_library(MUMPS_LIBRARY
        NAMES coinmumps libcoinmumps
        PATHS
          ${CMAKE_BINARY_DIR}/Mumps-install/lib
          ${Mumps_DIR}/lib
        NO_DEFAULT_PATH
      )
      if(MUMPS_LIBRARY)
        list(APPEND IPOPT_DEPS ${MUMPS_LIBRARY})
      endif()

      # Link all dependencies
      if(IPOPT_DEPS)
        target_link_libraries(qSlicerIpoptOptimizerTest ${IPOPT_DEPS})
      endif()
    endif()

    # Add the test to CTest
    add_test(
      NAME qSlicerIpoptOptimizerBasicTest
      COMMAND qSlicerIpoptOptimizerTest
    )

    # Set test properties
    set_tests_properties(qSlicerIpoptOptimizerBasicTest PROPERTIES
      TIMEOUT 120
      LABELS "IPOPT;ExternalBeamPlanning;Optimization"
      ENVIRONMENT "PATH=${Qt5_DIR}/../../../bin\;$ENV{PATH}"
    )

    if(WIN32 AND IPOPT_DLL)
      # Copy Ipopt DLLs to the test output directory
      file(GLOB IPOPT_DLLS "${Ipopt_DLL_DIR}/*.dll")
      foreach(DLL_FILE ${IPOPT_DLLS})
        add_custom_command(TARGET qSlicerIpoptOptimizerTest POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL_FILE}"
            "$<TARGET_FILE_DIR:qSlicerIpoptOptimizerTest>"
        )
      endforeach()
    endif()
  else()
    message(WARNING "IPOPT libraries not found. Cannot build qSlicerIpoptOptimizer test.")
  endif()
else()
  message(STATUS "IPOPT support disabled. Skipping qSlicerIpoptOptimizer test.")
endif()
