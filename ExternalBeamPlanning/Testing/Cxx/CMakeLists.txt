# Only build qSlicerIpoptOptimizer test if IPOPT is enabled
if(EXTENSION_BUILDS_IPOPT)

  if(WIN32)
    set(IPOPT_INCLUDE_DIRS "${Ipopt_INCLUDE_DIR}")
  else()
    find_path(IPOPT_INCLUDE_DIRS
      NAMES IpIpoptApplication.hpp
      PATHS
        ${Ipopt_DIR}/include/coin-or
        ${CMAKE_BINARY_DIR}/Ipopt-install/include/coin-or
      NO_DEFAULT_PATH
    )
  endif()

  if(IPOPT_INCLUDE_DIRS)
    # Link against the full widgets library instead of compiling the .cxx directly.
    # qSlicerIpoptOptimizer now inherits qSlicerAbstractPlanOptimizer which pulls in
    # VTK/Slicer/MRML headers that a standalone compilation cannot resolve.
    add_executable(qSlicerIpoptOptimizerTest qSlicerIpoptOptimizerTest.cxx)
    set_target_properties(qSlicerIpoptOptimizerTest PROPERTIES AUTOMOC ON)

    target_include_directories(qSlicerIpoptOptimizerTest PRIVATE
      ${IPOPT_INCLUDE_DIRS}
      ${CMAKE_CURRENT_SOURCE_DIR}/../../Widgets
      ${CMAKE_BINARY_DIR}/ExternalBeamPlanning/Widgets
    )

    # qSlicerExternalBeamPlanningModuleWidgets already links IPOPT transitively.
    target_link_libraries(qSlicerIpoptOptimizerTest
      qSlicerExternalBeamPlanningModuleWidgets
      Qt5::Core
    )

    if(WIN32 AND DEFINED Ipopt_DLL_DIR)
      file(GLOB IPOPT_DLLS "${Ipopt_DLL_DIR}/*.dll")
      foreach(DLL_FILE ${IPOPT_DLLS})
        add_custom_command(TARGET qSlicerIpoptOptimizerTest POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL_FILE}"
            "$<TARGET_FILE_DIR:qSlicerIpoptOptimizerTest>"
        )
      endforeach()
    endif()

    add_test(
      NAME qSlicerIpoptOptimizerBasicTest
      COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:qSlicerIpoptOptimizerTest>
    )

    set_tests_properties(qSlicerIpoptOptimizerBasicTest PROPERTIES
      TIMEOUT 120
      LABELS "IPOPT;ExternalBeamPlanning;Optimization"
    )

  else()
    message(WARNING "IPOPT include dirs not found. Cannot build qSlicerIpoptOptimizer test.")
  endif()

else()
  message(STATUS "IPOPT support disabled. Skipping qSlicerIpoptOptimizer test.")
endif()
