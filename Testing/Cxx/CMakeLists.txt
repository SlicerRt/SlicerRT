# Ipopt test executable
set(IPOPT_TEST_SRCS
  IpoptTest.cxx
  MyNLP.cpp
)

# Only build Ipopt test if Ipopt is enabled
if(EXTENSION_BUILDS_IPOPT)
  # Find Ipopt installation
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
    add_executable(IpoptTest ${IPOPT_TEST_SRCS})

    # Set include directories
    target_include_directories(IpoptTest PRIVATE ${IPOPT_INCLUDE_DIRS})

    # Link libraries
    target_link_libraries(IpoptTest ${IPOPT_LIBRARIES})

    # Add additional system libraries that Ipopt might need
    if(UNIX)
      # Link Fortran runtime and METIS with explicit linker flags
      target_link_options(IpoptTest PRIVATE "-Wl,--no-as-needed")
      target_link_libraries(IpoptTest gfortran quadmath gcc_s metis)

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

      find_library(PTHREAD_LIBRARY pthread)
      if(PTHREAD_LIBRARY)
        list(APPEND IPOPT_DEPS ${PTHREAD_LIBRARY})
      endif()

      # Fortran runtime - use full library paths
      find_library(GFORTRAN_LIBRARY NAMES gfortran PATHS /usr/lib/gcc/x86_64-linux-gnu/11 /usr/lib/x86_64-linux-gnu NO_DEFAULT_PATH)
      if(GFORTRAN_LIBRARY)
        list(APPEND IPOPT_DEPS ${GFORTRAN_LIBRARY})
      else()
        list(APPEND IPOPT_DEPS gfortran)
      endif()

      find_library(QUADMATH_LIBRARY NAMES quadmath PATHS /usr/lib/gcc/x86_64-linux-gnu/11 /usr/lib/x86_64-linux-gnu NO_DEFAULT_PATH)
      if(QUADMATH_LIBRARY)
        list(APPEND IPOPT_DEPS ${QUADMATH_LIBRARY})
      else()
        list(APPEND IPOPT_DEPS quadmath)
      endif()

      # Add gcc runtime for Fortran support
      find_library(GCC_S_LIBRARY NAMES gcc_s PATHS /usr/lib/gcc/x86_64-linux-gnu/11 /usr/lib/x86_64-linux-gnu NO_DEFAULT_PATH)
      if(GCC_S_LIBRARY)
        list(APPEND IPOPT_DEPS ${GCC_S_LIBRARY})
      endif()

      # BLAS/LAPACK
      find_package(BLAS QUIET)
      if(BLAS_FOUND)
        list(APPEND IPOPT_DEPS ${BLAS_LIBRARIES})
      endif()

      find_package(LAPACK QUIET)
      if(LAPACK_FOUND)
        list(APPEND IPOPT_DEPS ${LAPACK_LIBRARIES})
      endif()

      # MUMPS library (focus on getting this right first)
      find_library(MUMPS_LIBRARY
        NAMES coinmumps libcoinmumps
        PATHS
          ${CMAKE_BINARY_DIR}/Mumps-install/lib
          ${Mumps_DIR}/lib
        NO_DEFAULT_PATH
      )
      if(MUMPS_LIBRARY)
        list(APPEND IPOPT_DEPS ${MUMPS_LIBRARY})
        message(STATUS "Found MUMPS library: ${MUMPS_LIBRARY}")
      else()
        message(STATUS "MUMPS library not found. Searched in:")
        message(STATUS "  ${CMAKE_BINARY_DIR}/Mumps-install/lib")
        message(STATUS "  ${Mumps_DIR}/lib")
      endif()

      # Remove HSL for now as it is not configured properly yet
      # # HSL library
      # find_library(HSL_LIBRARY
      #   NAMES coinhsl libcoinhsl
      #   PATHS
      #     ${CMAKE_BINARY_DIR}/HSL-install/lib
      #     ${HSL_DIR}/lib
      #   NO_DEFAULT_PATH
      # )
      # if(HSL_LIBRARY)
      #   list(APPEND IPOPT_DEPS ${HSL_LIBRARY})
      # endif()

      # Link all dependencies
      if(IPOPT_DEPS)
        target_link_libraries(IpoptTest ${IPOPT_DEPS})
      endif()
    endif()

    # Add the test
    add_test(
      NAME IpoptBasicTest
      COMMAND IpoptTest
    )

    # Set test properties
    set_tests_properties(IpoptBasicTest
      PROPERTIES
        PASS_REGULAR_EXPRESSION "The problem solved in .* iterations"
        FAIL_REGULAR_EXPRESSION "Error;ERROR"
    )

    # Copy DLL on Windows if needed
    if(WIN32 AND IPOPT_DLL)
      # Copy all DLLs from Ipopt_DLL_DIR to the test output directory
      file(GLOB IPOPT_DLLS "${Ipopt_DLL_DIR}/*.dll")
      foreach(DLL_FILE ${IPOPT_DLLS})
        add_custom_command(TARGET IpoptTest POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL_FILE}"
            "$<TARGET_FILE_DIR:IpoptTest>"
        )
      endforeach()
    endif()

    message(STATUS "Ipopt test configured successfully")
  else()
    message(WARNING "Ipopt libraries not found. Ipopt test will be skipped.")
  endif()
else()
  message(STATUS "Ipopt test skipped. Enable with EXTENSION_BUILDS_IPOPT=ON")
endif()
