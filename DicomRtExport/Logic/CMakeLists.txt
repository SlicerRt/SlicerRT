project(vtkSlicer${MODULE_NAME}ModuleLogic)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_LOGIC_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${SlicerRtCommon_INCLUDE_DIRS}
  ${vtkSlicerContoursModuleMRML_INCLUDE_DIRS}
  ${PLASTIMATCH_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  vtkSlicer${MODULE_NAME}ModuleLogic.cxx
  vtkSlicer${MODULE_NAME}ModuleLogic.h
  vtkSlicerDicomRtWriter.cxx
  vtkSlicerDicomRtWriter.h
  )
 
SET (${KIT}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "" FORCE)
 
set(${KIT}_TARGET_LIBRARIES
  vtkSlicerRtCommon
  vtkSlicerContoursModuleMRML
  ${ITK_LIBRARIES}
  ${VTK_LIBRARIES}
  ${DCMTK_LIBRARIES}
  ${PLASTIMATCH_LIBRARIES}
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleLogic(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
  
# Set linker flags, needed for OpenMP
if (NOT ${PLASTIMATCH_LDFLAGS} STREQUAL "")
  set_target_properties (${KIT}
    PROPERTIES LINK_FLAGS ${PLASTIMATCH_LDFLAGS})
endif ()
