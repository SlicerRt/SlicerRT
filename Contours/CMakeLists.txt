#-----------------------------------------------------------------------------
set(MODULE_NAME "Contours")
set(MODULE_TITLE ${MODULE_NAME})

string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)

#-----------------------------------------------------------------------------
find_package(Plastimatch QUIET PATHS ${Plastimatch_DIR} NO_DEFAULT_PATH)
if(NOT Plastimatch_FOUND)
  message("Plastimatch library is not found. ${MODULE_NAME} module will not be built.")
  return()
endif() 

#-----------------------------------------------------------------------------
add_subdirectory(MRML)
add_subdirectory(Logic)
add_subdirectory(SubjectHierarchyPlugins)
add_subdirectory(Widgets)

#-----------------------------------------------------------------------------
set(MODULE_EXPORT_DIRECTIVE "Q_SLICER_QTMODULES_${MODULE_NAME_UPPER}_EXPORT")

set(MODULE_INCLUDE_DIRECTORIES
  ${SlicerRtCommon_INCLUDE_DIRS}
  ${qSlicerContoursSubjectHierarchyPlugins_INCLUDE_DIRS}
  ${qSlicerSubjectHierarchyModuleWidgets_INCLUDE_DIRS}
  )

  set(MODULE_SRCS
  qSlicer${MODULE_NAME}Module.cxx
  qSlicer${MODULE_NAME}Module.h
  qSlicer${MODULE_NAME}ModuleWidget.cxx
  qSlicer${MODULE_NAME}ModuleWidget.h
  qSlicer${MODULE_NAME}Reader.cxx
  qSlicer${MODULE_NAME}Reader.h
  )

set(MODULE_MOC_SRCS
  qSlicer${MODULE_NAME}Module.h
  qSlicer${MODULE_NAME}ModuleWidget.h
  qSlicer${MODULE_NAME}Reader.h
  )

set(MODULE_UI_SRCS
  Resources/UI/qSlicer${MODULE_NAME}Module.ui
  )

set(MODULE_TARGET_LIBRARIES
  vtkSlicerRtCommon
  qSlicerSubjectHierarchyModuleWidgets
  vtkSlicer${MODULE_NAME}ModuleLogic
  qSlicer${MODULE_NAME}SubjectHierarchyPlugins
  )

set(MODULE_RESOURCES
  Resources/qSlicer${MODULE_NAME}Module.qrc
  )

#-----------------------------------------------------------------------------
slicerMacroBuildQtModule(
  NAME ${MODULE_NAME}
  TITLE ${MODULE_TITLE}
  EXPORT_DIRECTIVE ${MODULE_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${MODULE_INCLUDE_DIRECTORIES}
  SRCS ${MODULE_SRCS}
  MOC_SRCS ${MODULE_MOC_SRCS}
  UI_SRCS ${MODULE_UI_SRCS}
  TARGET_LIBRARIES ${MODULE_TARGET_LIBRARIES}
  RESOURCES ${MODULE_RESOURCES}
  WITH_GENERIC_TESTS
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
