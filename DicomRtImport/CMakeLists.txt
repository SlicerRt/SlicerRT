cmake_minimum_required(VERSION 2.8.7)

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  set(EXTENSION_NAME "DicomRtImport")
  set(EXTENSION_HOMEPAGE "https://www.assembla.com/spaces/slicerrt")
  set(EXTENSION_CATEGORY "Radiotherapy")
  set(EXTENSION_ICONURL "http://viewvc.slicer.org/viewvc.cgi/Slicer4/trunk/Extensions/Testing/LoadableExtensionTemplate/Resources/Icons/LoadableExtensionTemplate.png?revision=19437&view=co")
  set(EXTENSION_STATUS "Beta")
  set(EXTENSION_CONTRIBUTORS "Csaba Pinter (PerkLab, Queen's University), Andras Lasso (PerkLab, Queen's University), Kevin Wang (Radiation Medicine Program, Princess Margaret Hospital, University Health Network Toronto)")
  set(EXTENSION_DESCRIPTION "DICOM-RT plugin for importing radiotherapy data from DICOM files")
  set(EXTENSION_SCREENSHOTURLS "https://www.assembla.com/spaces/slicerrt/documents/bhB--4vSSr4BSNacwqjQWU/download/bhB--4vSSr4BSNacwqjQWU?notinline=true")
  set(EXTENSION_DEPENDS "NA") # Specified as a space separated list or 'NA' if any
endif()

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  find_package(Slicer REQUIRED)
  include(${Slicer_USE_FILE})
  include(SlicerEnableExtensionTesting)
endif()

#-----------------------------------------------------------------------------
set(MODULE_NAME "DicomRtImport") # Do not use 'project()' - See SlicerConfig.cmake
set(MODULE_TITLE "DICOM-RT Import")

#-----------------------------------------------------------------------------
add_subdirectory(Logic)
#add_subdirectory(Algorithm)

#-----------------------------------------------------------------------------
set(qt_module_export_directive "Q_SLICER_QTMODULES_DICOMRTIMPORT_EXPORT")
# Current_{source,binary} and Slicer_{Libs,Base} already included
set(qt_module_include_directories
  ${CMAKE_CURRENT_SOURCE_DIR}/Logic
  ${CMAKE_CURRENT_BINARY_DIR}/Logic
  #${CMAKE_CURRENT_SOURCE_DIR}/Algorithm
  #${CMAKE_CURRENT_BINARY_DIR}/Algorithm
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
  include(${Slicer_EXTENSION_CPACK})
endif()
