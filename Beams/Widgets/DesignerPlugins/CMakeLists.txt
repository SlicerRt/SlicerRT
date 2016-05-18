set(KIT ${PROJECT_NAME}Plugins)

set(${KIT}_EXPORT "Q_SLICER_MODULE_${MODULE_NAME_UPPER}_WIDGETS_PLUGINS_EXPORT")

set(${KIT}_SRCS
  qSlicerBeamsModuleWidgetsAbstractPlugin.cxx
  qSlicerBeamsModuleWidgetsAbstractPlugin.h
  qSlicerBeamsModuleWidgetsPlugin.cxx
  qSlicerBeamsModuleWidgetsPlugin.h
  qMRMLBeamsTableViewPlugin.cxx
  qMRMLBeamsTableViewPlugin.h
  )

set(${KIT}_MOC_SRCS
  qSlicerBeamsModuleWidgetsPlugin.h
  qMRMLBeamsTableViewPlugin.h
  )

set(${KIT}_TARGET_LIBRARIES
  qSlicerBeamsModuleWidgets
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
