cmake_minimum_required(VERSION 2.8.7)


# Module name (usually identique to the directory name)
set(EXTENSION_NAME DoseAccumulation) # Do not use 'project()' - See SlicerConfig.cmake
set(qt_module_name "DoseAccumulation")
set(qt_module_title "Dose Accumulation")

if(NOT Slicer_SOURCE_DIR)
  set(EXTENSION_HOMEPAGE "https://www.assembla.com/spaces/slicerrt")
  set(EXTENSION_CATEGORY "Radiotherapy")
  set(EXTENSION_ICONURL "http://viewvc.slicer.org/viewvc.cgi/Slicer4/trunk/Extensions/Testing/LoadableExtensionTemplate/Resources/Icons/LoadableExtensionTemplate.png?revision=19437&view=co")
  set(EXTENSION_STATUS "Beta")
  set(EXTENSION_CONTRIBUTORS "Csaba Pinter, Andras Lasso, Kevin Wang")
  set(EXTENSION_DESCRIPTION "Accumulate dose volumes")
  set(EXTENSION_SCREENSHOTURLS "https://www.assembla.com/spaces/slicerrt/documents/bhB--4vSSr4BSNacwqjQWU/download/bhB--4vSSr4BSNacwqjQWU?notinline=true")
  set(EXTENSION_DEPENDS "NA") # Specified as a space separated list or 'NA' if any
endif()

if(NOT Slicer_SOURCE_DIR)
  find_package(Slicer REQUIRED)
  include(${Slicer_USE_FILE})
  include(SlicerEnableExtensionTesting)
endif()

# Build module sub libraries
add_subdirectory(Logic)

set(qt_module_export_directive "Q_SLICER_QTMODULES_DOSEACCUMULATION_EXPORT")

# Additional includes - Current_{source,binary} and Slicer_{Libs,Base} already included
set(qt_module_include_directories
  ${CMAKE_CURRENT_SOURCE_DIR}/Widgets
  ${CMAKE_CURRENT_BINARY_DIR}/Widgets
  ${CMAKE_CURRENT_SOURCE_DIR}/Logic
  ${CMAKE_CURRENT_BINARY_DIR}/Logic
  )

# Source files
set(qt_module_SRCS
  qSlicerDoseAccumulationModule.cxx
  qSlicerDoseAccumulationModule.h
  qSlicerDoseAccumulationModuleWidget.cxx
  qSlicerDoseAccumulationModuleWidget.h
  )

# Headers that should run through moc
set(qt_module_MOC_SRCS
  qSlicerDoseAccumulationModule.h
  qSlicerDoseAccumulationModuleWidget.h
  )

# UI files
set(qt_module_UI_SRCS
  Resources/UI/qSlicerDoseAccumulationModule.ui
  )

# Additional Target libraries
set(qt_module_target_libraries
  vtkSlicerDoseAccumulationModuleLogic
  )

# Resources
set(qt_module_resources
  Resources/qSlicerDoseAccumulationModule.qrc
  )

slicerMacroBuildQtModule(
  NAME ${qt_module_name}
  TITLE ${qt_module_title}
  EXPORT_DIRECTIVE ${qt_module_export_directive}
  INCLUDE_DIRECTORIES ${qt_module_include_directories}
  SRCS ${qt_module_SRCS}
  MOC_SRCS ${qt_module_MOC_SRCS}
  UI_SRCS ${qt_module_UI_SRCS}
  TARGET_LIBRARIES ${qt_module_target_libraries}
  RESOURCES ${qt_module_resources}
  )


if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()

if(NOT Slicer_SOURCE_DIR)
  include(${Slicer_EXTENSION_CPACK})
endif()
