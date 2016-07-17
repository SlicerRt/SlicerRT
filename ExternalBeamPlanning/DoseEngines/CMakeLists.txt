project(qSlicer${MODULE_NAME}DoseEngines)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "Q_SLICER_${MODULE_NAME_UPPER}_DOSE_ENGINES_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${PlmCommon_INCLUDE_DIRS}
  ${SlicerRtCommon_INCLUDE_DIRS}
  ${vtkSlicerBeamsModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerIsodoseModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerDoseAccumulationModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerSegmentationsModuleLogic_INCLUDE_DIRS}
  ${PLASTIMATCH_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  qSlicerAbstractDoseEngine.cxx
  qSlicerAbstractDoseEngine.h
  qSlicerDoseEnginePluginHandler.cxx
  qSlicerDoseEnginePluginHandler.h
  qSlicerDoseEngineLogic.cxx
  qSlicerDoseEngineLogic.h
  qSlicerMockDoseEngine.cxx
  qSlicerMockDoseEngine.h
  qSlicerPlastimatchProtonDoseEngine.cxx
  qSlicerPlastimatchProtonDoseEngine.h
  )

set(${KIT}_UI_SRCS
  )

# Since the wrapper doesn't consider hierarchy of classes, we need to exclude
# classes deriving from pure abstract class and remaining abstract pure
set_source_files_properties(
  qSlicerAbstractDoseEngine.h
  WRAP_EXCLUDE
  )

set(${KIT}_MOC_SRCS
  qSlicerAbstractDoseEngine.h
  qSlicerDoseEnginePluginHandler.h
  qSlicerDoseEngineLogic.h
  qSlicerMockDoseEngine.h
  qSlicerPlastimatchProtonDoseEngine.h
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
  vtkSlicerIsodoseModuleLogic
  vtkSlicerDoseAccumulationModuleLogic
  )

set(${KIT}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${vtkSlicerBeamsModuleLogic_INCLUDE_DIRS} ${vtkSlicerSegmentationsModuleLogic_INCLUDE_DIRS} ${vtkSlicerIsodoseModuleLogic_INCLUDE_DIRS} ${vtkSlicerDoseAccumulationModuleLogic_INCLUDE_DIRS} CACHE INTERNAL "" FORCE)

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
