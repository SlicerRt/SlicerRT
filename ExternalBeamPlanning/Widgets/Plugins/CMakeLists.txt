set(KIT ${PROJECT_NAME}Plugins)

set(${KIT}_EXPORT "Q_SLICER_MODULE_${MODULE_NAME_UPPER}_WIDGETS_PLUGINS_EXPORT")

set(${KIT}_SRCS
  qMRMLObjectivesTableWidgetPlugin.cxx
  qMRMLObjectivesTableWidgetPlugin.h
  )

set(${KIT}_MOC_SRCS
  qMRMLObjectivesTableWidgetPlugin.h
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
