project(qSlicer${MODULE_NAME}ModulePlugins)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "Q_SLICER_${MODULE_NAME_UPPER}_PLUGINS_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${SlicerRtCommon_INCLUDE_DIRS}
  ${vtkSlicerSubjectHierarchyModuleMRML_INCLUDE_DIRS}
  ${vtkSlicerDoseVolumeHistogramModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerIsodoseModuleLogic_INCLUDE_DIRS}
  ${qSlicerSubjectHierarchyModulePlugins_INCLUDE_DIRS}
  ${Slicer_Base_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  qSlicerSubjectHierarchyRtImagePlugin.cxx
  qSlicerSubjectHierarchyRtImagePlugin.h
  qSlicerSubjectHierarchyRtDoseVolumePlugin.cxx
  qSlicerSubjectHierarchyRtDoseVolumePlugin.h
  qSlicerSubjectHierarchyDvhPlugin.cxx
  qSlicerSubjectHierarchyDvhPlugin.h
  qSlicerSubjectHierarchyRtPlanPlugin.cxx
  qSlicerSubjectHierarchyRtPlanPlugin.h
  qSlicerSubjectHierarchyRtBeamPlugin.cxx
  qSlicerSubjectHierarchyRtBeamPlugin.h
  qSlicerSubjectHierarchyIsodosePlugin.cxx
  qSlicerSubjectHierarchyIsodosePlugin.h
  )

set(${KIT}_MOC_SRCS
  qSlicerSubjectHierarchyRtImagePlugin.h
  qSlicerSubjectHierarchyRtDoseVolumePlugin.h
  qSlicerSubjectHierarchyDvhPlugin.h
  qSlicerSubjectHierarchyRtPlanPlugin.h
  qSlicerSubjectHierarchyRtBeamPlugin.h
  qSlicerSubjectHierarchyIsodosePlugin.h
  )

set(${KIT}_UI_SRCS
  )

set(${KIT}_RESOURCES
  Resources/${KIT}.qrc
  )

SET (${KIT}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${qSlicerSubjectHierarchyModulePlugins_INCLUDE_DIRS} CACHE INTERNAL "" FORCE)

#-----------------------------------------------------------------------------
set(${KIT}_TARGET_LIBRARIES
  vtkSlicerRtCommon
  vtkSlicerSubjectHierarchyModuleMRML
  qSlicerSubjectHierarchyModulePlugins
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleQtLibrary(
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