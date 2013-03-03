
set(KIT ${PROJECT_NAME}Plugins)

set(${KIT}_EXPORT "Q_SLICER_MODULE_${MODULE_NAME_UPPER}_WIDGETS_PLUGINS_EXPORT")

set(${KIT}_SRCS
  qSlicerPatientHierarchyModuleWidgetsAbstractPlugin.cxx
  qSlicerPatientHierarchyModuleWidgetsAbstractPlugin.h
  qSlicerPatientHierarchyModuleWidgetsPlugin.cxx
  qSlicerPatientHierarchyModuleWidgetsPlugin.h
  qMRMLPatientHierarchyTreeViewPlugin.cxx
  qMRMLPatientHierarchyTreeViewPlugin.h
  )

set(${KIT}_MOC_SRCS
  qSlicerPatientHierarchyModuleWidgetsPlugin.h
  qMRMLPatientHierarchyTreeViewPlugin.h
  )

set(${KIT}_TARGET_LIBRARIES
  qSlicerPatientHierarchyModuleWidgets
  )

#-----------------------------------------------------------------------------
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${Slicer_DIR}/bin)

ctkMacroBuildQtDesignerPlugin(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT}
  SRCS ${${KIT}_SRCS}
  MOC_SRCS ${${KIT}_MOC_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
