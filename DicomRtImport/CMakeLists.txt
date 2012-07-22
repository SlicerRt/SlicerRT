cmake_minimum_required(VERSION 2.8.7)

#-----------------------------------------------------------------------------
set(MODULE_NAME "DicomRtImport") # Do not use 'project()' - See SlicerConfig.cmake
set(MODULE_TITLE "DICOM RT Import")

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
  qSlicerDicomRtImportModule.cxx
  qSlicerDicomRtImportModule.h
  qSlicerDicomRtImportModuleWidget.cxx
  qSlicerDicomRtImportModuleWidget.h
  )

set(qt_module_MOC_SRCS
  qSlicerDicomRtImportModule.h
  qSlicerDicomRtImportModuleWidget.h
  )

set(qt_module_UI_SRCS
  Resources/UI/qSlicerDicomRtImportModule.ui
  )

set(qt_module_target_libraries
  vtkSlicerDicomRtImportModuleLogic
  #vtkSlicerDicomRtImportAlgorithm
  vtkSlicerVolumesModuleLogic
  )

set(qt_module_resources
  Resources/qSlicerDicomRtImportModule.qrc
  )

#-----------------------------------------------------------------------------
# Plugin python script
set(KIT_PYTHON_SCRIPTS
  DicomRtImportPlugin.py
  )

set(KIT_PYTHON_RESOURCES
  )

ctkMacroCompilePythonScript(
  TARGET_NAME SlicerDICOMPluginModules
  SCRIPTS "${KIT_PYTHON_SCRIPTS}"
  RESOURCES "${KIT_PYTHON_RESOURCES}"
  DESTINATION_DIR ${CMAKE_CURRENT_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}
  INSTALL_DIR ${Slicer_INSTALL_QTSCRIPTEDMODULES_LIB_DIR}
  NO_INSTALL_SUBDIR
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
  # TODO: add testing
  # add_subdirectory(Testing)
endif()

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
#  include(${Slicer_EXTENSION_CPACK})
endif()
