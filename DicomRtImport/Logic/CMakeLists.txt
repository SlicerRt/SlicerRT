project(vtkSlicer${EXTENSION_NAME}ModuleLogic)

#
# See CMake/SlicerMacroBuildModuleLogic.cmake for details
#

set(module_logic_name "${PROJECT_NAME}")

# The header '${module_logic_name}Export.h' will be automatically configured.
set(module_logic_export_directive "VTK_SLICER_DICOMRTIMPORT_MODULE_LOGIC_EXPORT")

# Additional directories to include
set(module_logic_include_directories
  ${QT_QTSQL_INCLUDE_DIR}
  )

# Source files
set(module_logic_SRCS
  vtkDICOMImportInfo.cxx
  vtkDICOMImportInfo.h
  vtkSlicerDicomRtImportLogic.cxx
  vtkSlicerDicomRtImportLogic.h
  vtkSlicerDicomRtReader.cxx
  vtkSlicerDicomRtReader.h
  )

# Additional Target libraries
set(module_logic_target_libraries
  ${ITK_LIBRARIES}
  ${VTK_LIBRARIES}
  ${DCMTK_LIBRARIES}
  vtkSlicerVolumesModuleLogic
  vtkSlicerAnnotationsModuleMRML
  )

SlicerMacroBuildModuleLogic(
  NAME ${module_logic_name}
  EXPORT_DIRECTIVE ${module_logic_export_directive}
  INCLUDE_DIRECTORIES ${module_logic_include_directories}
  SRCS ${module_logic_SRCS}
  TARGET_LIBRARIES ${module_logic_target_libraries}
  )
