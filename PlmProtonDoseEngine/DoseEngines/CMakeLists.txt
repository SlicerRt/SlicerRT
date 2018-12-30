project(qSlicer${MODULE_NAME}DoseEngines)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "Q_SLICER_${MODULE_NAME_UPPER}_DOSE_ENGINES_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${PlmCommon_INCLUDE_DIRS}
  ${SlicerRtCommon_INCLUDE_DIRS}
  ${vtkSlicerBeamsModuleLogic_INCLUDE_DIRS}
  ${qSlicerExternalBeamPlanningModuleWidgets_INCLUDE_DIRS}
  ${vtkSlicerSegmentationsModuleLogic_INCLUDE_DIRS}
  ${qSlicerBeamsModuleWidgets_INCLUDE_DIRS}
  ${MRMLLogic_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  qSlicerPlmProtonDoseEngine.cxx
  qSlicerPlmProtonDoseEngine.h
)

set(${KIT}_UI_SRCS
  )

set(${KIT}_MOC_SRCS
  qSlicerPlmProtonDoseEngine.h
)

set(${KIT}_RESOURCES
  # Resources/${KIT}.qrc
  )

set(${KIT}_TARGET_LIBRARIES
  vtkPlmCommon
  vtkSlicerRtCommon
  vtkSlicerBeamsModuleMRML
  vtkSlicerBeamsModuleLogic
  vtkSlicerSegmentationsModuleMRML
  vtkSlicerSegmentationsModuleLogic
  qSlicerBeamsModuleWidgets
  qSlicerExternalBeamPlanningModuleWidgets
  ${PLASTIMATCH_LIBRARIES}
  )

set(${KIT}_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${vtkSlicerBeamsModuleLogic_INCLUDE_DIRS}
  ${qSlicerExternalBeamPlanningModuleWidgets_INCLUDE_DIRS}
  ${vtkSlicerSegmentationsModuleLogic_INCLUDE_DIRS}
  CACHE INTERNAL "" FORCE)

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleQtLibrary(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  FOLDER "Module-${MODULE_NAME}"
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  MOC_SRCS ${${KIT}_MOC_SRCS}
  UI_SRCS ${${KIT}_UI_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  RESOURCES ${${KIT}_RESOURCES}
  WRAP_PYTHONQT
  )

# Set linker flags, needed for OpenMP
if (NOT ${PLASTIMATCH_LDFLAGS} STREQUAL "")
  set_target_properties (${KIT}
    PROPERTIES LINK_FLAGS ${PLASTIMATCH_LDFLAGS})
endif ()
