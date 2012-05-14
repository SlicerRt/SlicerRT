cmake_minimum_required(VERSION 2.8.7)

#-----------------------------------------------------------------------------
set(MODULE_NAME "DoseVolumeHistogram") # Do not use 'project()' - See SlicerConfig.cmake
set(MODULE_TITLE "Dose Volume Histogram")

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  set(MODULE_HOMEPAGE "https://www.assembla.com/spaces/slicerrt")
  set(MODULE_CATEGORY "Radiotherapy")
  set(MODULE_ICONURL "http://viewvc.slicer.org/viewvc.cgi/Slicer4/trunk/Extensions/Testing/LoadableExtensionTemplate/Resources/Icons/LoadableExtensionTemplate.png?revision=19437&view=co")
  set(MODULE_STATUS "Beta")
  set(MODULE_CONTRIBUTORS "Csaba Pinter, Andras Lasso, Kevin Wang")
  set(MODULE_DESCRIPTION "Compute and display dose volume histograms")
  set(MODULE_SCREENSHOTURLS "https://www.assembla.com/spaces/slicerrt/documents/bhB--4vSSr4BSNacwqjQWU/download/bhB--4vSSr4BSNacwqjQWU?notinline=true")
  set(MODULE_DEPENDS "NA") # Specified as a space separated list or 'NA' if any
endif()

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  find_package(Slicer REQUIRED)
  include(${Slicer_USE_FILE})
endif()

#-----------------------------------------------------------------------------
add_subdirectory(Logic)

#-----------------------------------------------------------------------------
set(qt_module_export_directive "Q_SLICER_QTMODULES_DOSEVOLUMEHISTOGRAM_EXPORT")

set(qt_module_include_directories
  ${CMAKE_CURRENT_SOURCE_DIR}/Logic
  ${CMAKE_CURRENT_BINARY_DIR}/Logic
  )

set(qt_module_SRCS
  qSlicerDoseVolumeHistogramModule.cxx
  qSlicerDoseVolumeHistogramModule.h
  qSlicerDoseVolumeHistogramModuleWidget.cxx
  qSlicerDoseVolumeHistogramModuleWidget.h
  )

set(qt_module_MOC_SRCS
  qSlicerDoseVolumeHistogramModule.h
  qSlicerDoseVolumeHistogramModuleWidget.h
  )

set(qt_module_UI_SRCS
  Resources/UI/qSlicerDoseVolumeHistogramModule.ui
)

set(qt_module_target_libraries
  vtkSlicerDoseVolumeHistogramModuleLogic
  )

set(qt_module_resources
  Resources/qSlicerDoseVolumeHistogramModule.qrc
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
  include(${Slicer_EXTENSION_CPACK})
endif()
