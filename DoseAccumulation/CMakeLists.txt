cmake_minimum_required(VERSION 2.8.7)

#-----------------------------------------------------------------------------
set(EXTENSION_NAME DoseAccumulation) # Do not use 'project()' - See SlicerConfig.cmake

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  set(EXTENSION_HOMEPAGE "https://www.assembla.com/spaces/slicerrt")
  set(EXTENSION_CATEGORY "Radiotherapy")
  set(EXTENSION_ICONURL "http://viewvc.slicer.org/viewvc.cgi/Slicer4/trunk/Extensions/Testing/LoadableExtensionTemplate/Resources/Icons/LoadableExtensionTemplate.png?revision=19437&view=co")
  set(EXTENSION_STATUS "Beta")
  set(EXTENSION_CONTRIBUTORS "Csaba Pinter (PerkLab, Queen's University), Andras Lasso (PerkLab, Queen's University), Kevin Wang (Radiation Medicine Program, Princess Margaret Hospital, University Health Network Toronto)")
  set(EXTENSION_DESCRIPTION "Accumulate dose volumes")
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
set(MODULE_NAME DoseAccumulation)
set(MODULE_TITLE "Dose Accumulation")

#-----------------------------------------------------------------------------
add_subdirectory(Logic)

#-----------------------------------------------------------------------------
set(qt_module_export_directive "Q_SLICER_QTMODULES_DOSEACCUMULATION_EXPORT")

set(qt_module_include_directories
  ${CMAKE_CURRENT_SOURCE_DIR}/Widgets
  ${CMAKE_CURRENT_BINARY_DIR}/Widgets
  ${CMAKE_CURRENT_SOURCE_DIR}/Logic
  ${CMAKE_CURRENT_BINARY_DIR}/Logic
  )

set(qt_module_SRCS
  qSlicerDoseAccumulationModule.cxx
  qSlicerDoseAccumulationModule.h
  qSlicerDoseAccumulationModuleWidget.cxx
  qSlicerDoseAccumulationModuleWidget.h
  )

set(qt_module_MOC_SRCS
  qSlicerDoseAccumulationModule.h
  qSlicerDoseAccumulationModuleWidget.h
  )

set(qt_module_UI_SRCS
  Resources/UI/qSlicerDoseAccumulationModule.ui
  )

set(qt_module_target_libraries
  vtkSlicerDoseAccumulationModuleLogic
  )

set(qt_module_resources
  Resources/qSlicerDoseAccumulationModule.qrc
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
