project(qSlicer${MODULE_NAME}ModuleWidgets)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "Q_SLICER_MODULE_${MODULE_NAME_UPPER}_WIDGETS_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${SlicerRtCommon_INCLUDE_DIRS}
  ${vtkSlicerBeamsModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerIsodoseModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerDoseAccumulationModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerSegmentationsModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerExternalBeamPlanningModuleLogic_INCLUDE_DIRS}
  ${qSlicerBeamsModuleWidgets_INCLUDE_DIRS}
  )

# Add IPOPT include directories if IPOPT support is enabled
if(EXTENSION_BUILDS_IPOPT)
  # Find IPOPT installation
  if(WIN32)
    # Windows uses prebuilt binaries
    set(IPOPT_INCLUDE_DIRS "${Ipopt_INCLUDE_DIR}")
  else()
    # Unix systems use built libraries
    find_path(IPOPT_INCLUDE_DIRS
      NAMES IpIpoptApplication.hpp
      PATHS
        ${Ipopt_DIR}/include/coin-or
        ${CMAKE_BINARY_DIR}/Ipopt-install/include/coin-or
      NO_DEFAULT_PATH
    )
  endif()

  if(IPOPT_INCLUDE_DIRS)
    list(APPEND ${KIT}_INCLUDE_DIRECTORIES ${IPOPT_INCLUDE_DIRS})
  endif()
endif()

set(${KIT}_SRCS
  # Dose engines
  qSlicerAbstractDoseEngine.cxx
  qSlicerAbstractDoseEngine.h
  qSlicerDoseEnginePluginHandler.cxx
  qSlicerDoseEnginePluginHandler.h
  qSlicerDoseEngineLogic.cxx
  qSlicerDoseEngineLogic.h
  qSlicerMockDoseEngine.cxx
  qSlicerMockDoseEngine.h
  qSlicerScriptedDoseEngine.cxx
  qSlicerScriptedDoseEngine.h
  # Plan optimizers
  qSlicerAbstractPlanOptimizer.cxx
  qSlicerAbstractPlanOptimizer.h
  qSlicerPlanOptimizerPluginHandler.cxx
  qSlicerPlanOptimizerPluginHandler.h
  qSlicerPlanOptimizerLogic.cxx
  qSlicerPlanOptimizerLogic.h
  qSlicerMockPlanOptimizer.cxx
  qSlicerMockPlanOptimizer.h
  qSlicerScriptedPlanOptimizer.cxx
  qSlicerScriptedPlanOptimizer.h
  # Objectives
  qSlicerAbstractObjective.cxx
  qSlicerAbstractObjective.h
  qSlicerObjectivePluginHandler.cxx
  qSlicerObjectivePluginHandler.h
  qSlicerObjectiveLogic.cxx
  qSlicerObjectiveLogic.h
  qSlicerSquaredDeviationObjective.cxx
  qSlicerSquaredDeviationObjective.h
  # Widgets
  qMRMLObjectivesTableWidget.cxx
  qMRMLObjectivesTableWidget.h
  )

# Add IPOPT optimizer sources if IPOPT support is enabled
if(EXTENSION_BUILDS_IPOPT)
  list(APPEND ${KIT}_SRCS
    qSlicerIpoptOptimizer.cxx
    qSlicerIpoptOptimizer.h
    )
endif()

set(${KIT}_UI_SRCS
  )

# Since the wrapper doesn't consider hierarchy of classes, we need to exclude
# classes deriving from pure abstract class and remaining abstract pure
set_source_files_properties(
  qSlicerAbstractDoseEngine.h
  qSlicerAbstractPlanOptimizer.h
  qSlicerAbstractObjective.h
  WRAP_EXCLUDE
  )

set(${KIT}_MOC_SRCS
  qSlicerAbstractDoseEngine.h
  qSlicerDoseEnginePluginHandler.h
  qSlicerDoseEngineLogic.h
  qSlicerMockDoseEngine.h
  qSlicerScriptedDoseEngine.h
  qSlicerAbstractPlanOptimizer.h
  qSlicerPlanOptimizerPluginHandler.h
  qSlicerPlanOptimizerLogic.h
  qSlicerMockPlanOptimizer.h
  qSlicerScriptedPlanOptimizer.h
  qSlicerAbstractObjective.h
  qSlicerObjectivePluginHandler.h
  qSlicerObjectiveLogic.h
  qSlicerSquaredDeviationObjective.h
  qMRMLObjectivesTableWidget.h
)

# Add IPOPT optimizer to MOC sources if IPOPT support is enabled
if(EXTENSION_BUILDS_IPOPT)
  list(APPEND ${KIT}_MOC_SRCS
    qSlicerIpoptOptimizer.h
    )
endif()

set(${KIT}_UI_SRCS
  UI/qMRMLObjectivesTableWidget.ui
  )

set(${KIT}_RESOURCES
  # Resources/${KIT}.qrc
  )

set(${KIT}_TARGET_LIBRARIES
  vtkSlicerRtCommon
  vtkSlicerBeamsModuleMRML
  vtkSlicerBeamsModuleLogic
  vtkSlicerSegmentationsModuleMRML
  vtkSlicerSegmentationsModuleLogic
  vtkSlicerIsodoseModuleLogic
  vtkSlicerDoseAccumulationModuleLogic
  vtkSlicerExternalBeamPlanningModuleLogic
  vtkSlicerExternalBeamPlanningModuleMRML
  qSlicerBeamsModuleWidgets
  )

# Add IPOPT libraries if IPOPT support is enabled
if(EXTENSION_BUILDS_IPOPT)
  if(WIN32)
    # Windows uses prebuilt binaries
    set(IPOPT_LIBRARIES "${Ipopt_LIBRARY_DIR}/ipopt.dll.lib")
  else()
    # Unix systems use built libraries
    find_library(IPOPT_LIBRARIES
      NAMES ipopt libipopt
      PATHS
        ${Ipopt_DIR}/lib
        ${CMAKE_BINARY_DIR}/Ipopt-install/lib
      NO_DEFAULT_PATH
    )
  endif()

  if(IPOPT_LIBRARIES)
    list(APPEND ${KIT}_TARGET_LIBRARIES ${IPOPT_LIBRARIES})

    # Add additional system libraries that IPOPT might need on Unix
    if(UNIX)
      list(APPEND ${KIT}_TARGET_LIBRARIES gfortran quadmath gcc_s)

      # Find and link required dependencies
      find_library(DL_LIBRARY dl)
      if(DL_LIBRARY)
        list(APPEND ${KIT}_TARGET_LIBRARIES ${DL_LIBRARY})
      endif()

      find_library(M_LIBRARY m)
      if(M_LIBRARY)
        list(APPEND ${KIT}_TARGET_LIBRARIES ${M_LIBRARY})
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
        list(APPEND ${KIT}_TARGET_LIBRARIES ${MUMPS_LIBRARY})
      endif()

      # Also need METIS (required by MUMPS)
      find_library(METIS_LIBRARY
        NAMES metis libmetis
        PATHS
          ${CMAKE_BINARY_DIR}/Mumps-install/lib
          ${Mumps_DIR}/lib
        NO_DEFAULT_PATH
      )
      if(METIS_LIBRARY)
        list(APPEND ${KIT}_TARGET_LIBRARIES ${METIS_LIBRARY})
      endif()

      # BLAS and LAPACK (required by MUMPS)
      find_library(LAPACK_LIBRARY lapack)
      if(LAPACK_LIBRARY)
        list(APPEND ${KIT}_TARGET_LIBRARIES ${LAPACK_LIBRARY})
      endif()

      find_library(BLAS_LIBRARY blas)
      if(BLAS_LIBRARY)
        list(APPEND ${KIT}_TARGET_LIBRARIES ${BLAS_LIBRARY})
      endif()
    endif()
  endif()
endif()

set(${KIT}_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${vtkSlicerBeamsModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerSegmentationsModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerIsodoseModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerDoseAccumulationModuleLogic_INCLUDE_DIRS}
  ${qSlicerBeamsModuleWidgets_INCLUDE_DIRS}
  ${vtkSlicerExternalBeamPlanningModuleLogic_INCLUDE_DIRS}
  CACHE INTERNAL "" FORCE)

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleWidgets(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  MOC_SRCS ${${KIT}_MOC_SRCS}
  UI_SRCS ${${KIT}_UI_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  RESOURCES ${${KIT}_RESOURCES}
  WRAP_PYTHONQT
  )

#-----------------------------------------------------------------------------
if(Slicer_USE_PYTHONQT)
  add_subdirectory(Python)
endif()
