project(qSlicer${MODULE_NAME}ModulePlugins)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "Q_SLICER_${MODULE_NAME_UPPER}_PLUGINS_EXPORT")

#TODO: Replace with MRMLWidgets_INCLUDE_DIRS when available
list(GET MRMLCore_INCLUDE_DIRS 0 MRMLCore_DIR)
list(GET MRMLCore_INCLUDE_DIRS 1 MRMLCore_BIN_DIR)
set(${KIT}_INCLUDE_DIRECTORIES
  ${vtkSlicerSubjectHierarchyModuleMRML_INCLUDE_DIRS}
  ${vtkSlicerSubjectHierarchyModuleLogic_INCLUDE_DIRS}
  ${MRMLLogic_INCLUDE_DIRS}
  ${MRMLCore_DIR}/../Widgets
  ${MRMLCore_BIN_DIR}/../Widgets
  )

set(${KIT}_SRCS
  qSlicerSubjectHierarchyAbstractPlugin.cxx
  qSlicerSubjectHierarchyAbstractPlugin.h
  qSlicerSubjectHierarchyPluginHandler.cxx
  qSlicerSubjectHierarchyPluginHandler.h
  qSlicerSubjectHierarchyDefaultPlugin.cxx
  qSlicerSubjectHierarchyDefaultPlugin.h
  qSlicerSubjectHierarchyDICOMPlugin.cxx
  qSlicerSubjectHierarchyDICOMPlugin.h
  qSlicerSubjectHierarchyVolumesPlugin.cxx
  qSlicerSubjectHierarchyVolumesPlugin.h
  )

set(${KIT}_MOC_SRCS
  qSlicerSubjectHierarchyAbstractPlugin.h
  qSlicerSubjectHierarchyDefaultPlugin.h
  qSlicerSubjectHierarchyDICOMPlugin.h
  qSlicerSubjectHierarchyVolumesPlugin.h
  )

set(${KIT}_UI_SRCS
  )

set(${KIT}_RESOURCES
  Resources/${KIT}.qrc
  )

SET (${KIT}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "" FORCE)

#-----------------------------------------------------------------------------
set(${KIT}_TARGET_LIBRARIES
  vtkSlicer${MODULE_NAME}ModuleMRML
  vtkSlicer${MODULE_NAME}ModuleLogic
  MRMLCore
  MRMLLogic
  ${QT_LIBRARIES}
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