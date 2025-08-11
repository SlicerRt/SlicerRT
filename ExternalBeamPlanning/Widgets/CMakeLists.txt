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

set(${KIT}_SRCS
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
  
  qSlicerAbstractObjective.cxx
  qSlicerAbstractObjective.h
  qSlicerObjectivePluginHandler.cxx
  qSlicerObjectivePluginHandler.h
  qSlicerObjectiveLogic.cxx
  qSlicerObjectiveLogic.h
  qSlicerSquaredDeviationObjective.cxx
  qSlicerSquaredDeviationObjective.h

  qMRMLObjectivesTableWidget.cxx
  qMRMLObjectivesTableWidget.h
  )

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
