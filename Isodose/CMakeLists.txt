cmake_minimum_required(VERSION 2.8.7)

#-----------------------------------------------------------------------------
set(MODULE_NAME "Isodose") # Do not use 'project()' - See SlicerConfig.cmake
set(MODULE_TITLE "Isodose")

string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)

#-----------------------------------------------------------------------------
add_subdirectory(Logic)

#-----------------------------------------------------------------------------
set(qt_module_export_directive "Q_SLICER_QTMODULES_${MODULE_NAME_UPPER}_EXPORT")

set(qt_module_include_directories
  ${CMAKE_CURRENT_SOURCE_DIR}/Logic
  ${CMAKE_CURRENT_BINARY_DIR}/Logic
  )

set(qt_module_SRCS
  qSlicerIsodoseModule.cxx
  qSlicerIsodoseModule.h
  qSlicerIsodoseModuleWidget.cxx
  qSlicerIsodoseModuleWidget.h
  )

set(qt_module_MOC_SRCS
  qSlicerIsodoseModule.h
  qSlicerIsodoseModuleWidget.h
  )

set(qt_module_UI_SRCS
  Resources/UI/qSlicerIsodoseModule.ui
)

set(qt_module_target_libraries
  vtkSlicerIsodoseModuleLogic
  )

set(qt_module_resources
  Resources/qSlicerIsodoseModule.qrc
)

#-----------------------------------------------------------------------------
slicerMacroBuildQtModule(
  NAME ${MODULE_NAME}
  TITLE ${MODULE_TITLE}
  EXPORT_DIRECTIVE ${qt_module_export_directive}
  INCLUDE_DIRECTORIES ${qt_module_include_directories}
  SRCS ${qt_module_SRCS}
  MOC_SRCS ${qt_module_MOC_SRCS}
  UI_SRCS ${qt_module_UI_SRCS}
  TARGET_LIBRARIES ${qt_module_target_libraries}
  RESOURCES ${qt_module_resources}
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
#  include(${Slicer_EXTENSION_CPACK})
endif()
