project(vtkSlicerRtCommon)

# --------------------------------------------------------------------------
# Sources
# --------------------------------------------------------------------------

SET (SlicerRtCommon_SRCS 
  SlicerRtCommon.cxx
  SlicerRtCommon.h
  SlicerRtCommon.txx
  vtkPolyDataToLabelmapFilter.cxx
  vtkPolyDataToLabelmapFilter.h
  vtkLabelmapToModelFilter.cxx
  vtkLabelmapToModelFilter.h
  vtkVolumesOrientedResampleUtility.cxx
  vtkVolumesOrientedResampleUtility.h
  )

SET (SlicerRtCommon_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${Slicer_Libs_INCLUDE_DIRS} CACHE INTERNAL "" FORCE)

# --------------------------------------------------------------------------
# Build the library
# --------------------------------------------------------------------------

set(lib_name ${PROJECT_NAME})
SET (SlicerRtCommon_LIBS ${SlicerRtCommon_LIBS}
  vtkCommon
  vtkFiltering
  vtkImaging
  )

INCLUDE_DIRECTORIES( ${SlicerRtCommon_INCLUDE_DIRS} )
ADD_LIBRARY(${lib_name} STATIC ${SlicerRtCommon_SRCS})
TARGET_LINK_LIBRARIES( ${lib_name} ${SlicerRtCommon_LIBS} )

IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND NOT WIN32)
  SET_TARGET_PROPERTIES(${lib_name} PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF()

# --------------------------------------------------------------------------
# Python wrapping
# --------------------------------------------------------------------------
if(VTK_WRAP_PYTHON AND BUILD_SHARED_LIBS)

  set(Slicer_Wrapped_LIBRARIES
    )

  foreach(library ${lib_name})
    if(TARGET ${library}PythonD)
      list(APPEND Slicer_Wrapped_LIBRARIES ${library}PythonD)
    endif()
  endforeach()

  SlicerMacroPythonWrapModuleVTKLibrary(
    NAME ${PROJECT_NAME}
    SRCS ${SlicerRtCommon_SRCS}
    WRAPPED_TARGET_LIBRARIES ${Slicer_Wrapped_LIBRARIES}
    RELATIVE_PYTHON_DIR "."
    )

  # Set python module logic output
  set_target_properties(${PROJECT_NAME}Python ${PROJECT_NAME}PythonD PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_BIN_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_LIB_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_LIB_DIR}"
    )

  # Export target
  set_property(GLOBAL APPEND PROPERTY Slicer_TARGETS ${PROJECT_NAME}Python ${PROJECT_NAME}PythonD)
endif()
