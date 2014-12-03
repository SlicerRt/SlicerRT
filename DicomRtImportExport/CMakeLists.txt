#-----------------------------------------------------------------------------
set(MODULE_NAME "DicomRtImportExport")
set(MODULE_TITLE "DICOM RT Import-Export")

string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)

#-----------------------------------------------------------------------------
find_package(Plastimatch QUIET PATHS ${Plastimatch_DIR} )
if(NOT Plastimatch_FOUND)
  message("Plastimatch library is not found. DoseComparison module will not be built.")
  return()
endif()

#-----------------------------------------------------------------------------
add_subdirectory(Logic)
add_subdirectory(SubjectHierarchyPlugins)

#-----------------------------------------------------------------------------
set(MODULE_EXPORT_DIRECTIVE "Q_SLICER_QTMODULES_${MODULE_NAME_UPPER}_EXPORT")

set(MODULE_INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}/Logic
  ${CMAKE_CURRENT_BINARY_DIR}/Logic
  ${vtkSlicerVolumesModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerBeamsModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerIsodoseModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerPlanarImageModuleLogic_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/SubjectHierarchyPlugins
  ${CMAKE_CURRENT_BINARY_DIR}/SubjectHierarchyPlugins
  ${qSlicerSubjectHierarchyModuleWidgets_INCLUDE_DIRS}
  )

set(MODULE_SRCS
  qSlicer${MODULE_NAME}Module.cxx
  qSlicer${MODULE_NAME}Module.h
  qSlicer${MODULE_NAME}ModuleWidget.cxx
  qSlicer${MODULE_NAME}ModuleWidget.h
  )

set(MODULE_MOC_SRCS
  qSlicer${MODULE_NAME}Module.h
  qSlicer${MODULE_NAME}ModuleWidget.h
  )

set(MODULE_UI_SRCS
  Resources/UI/qSlicer${MODULE_NAME}Module.ui
  )

set(MODULE_TARGET_LIBRARIES
  vtkSlicer${MODULE_NAME}ModuleLogic
  vtkSlicerVolumesModuleLogic
  vtkSlicerBeamsModuleLogic
  vtkSlicerIsodoseModuleLogic
  vtkSlicerPlanarImageModuleLogic
  qSlicer${MODULE_NAME}SubjectHierarchyPlugins
  )

#-----------------------------------------------------------------------------
# Plugin python script
set(KIT_PYTHON_SCRIPTS
  DicomRtImportExportPlugin.py
  )

set(KIT_PYTHON_RESOURCES
  )

ctkMacroCompilePythonScript(
  TARGET_NAME SlicerDICOMPluginModules
  SCRIPTS "${KIT_PYTHON_SCRIPTS}"
  RESOURCES "${KIT_PYTHON_RESOURCES}"
  DESTINATION_DIR ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}
  INSTALL_DIR ${Slicer_INSTALL_QTSCRIPTEDMODULES_LIB_DIR}
  NO_INSTALL_SUBDIR
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
  WITH_GENERIC_TESTS
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
