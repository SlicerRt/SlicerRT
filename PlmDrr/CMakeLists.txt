
#-----------------------------------------------------------------------------
set(MODULE_NAME plastimatch_slicer_drr)

#-----------------------------------------------------------------------------

find_package(Plastimatch QUIET PATHS ${Plastimatch_DIR} NO_DEFAULT_PATH)
if(NOT Plastimatch_FOUND)
  message("Plastimatch library is not found. The ${MODULE_NAME} module will not be built.")
  return()
endif() 

#
# SlicerExecutionModel
#
find_package(SlicerExecutionModel REQUIRED)
include(${SlicerExecutionModel_USE_FILE})

#
# ITK
#
set(${PROJECT_NAME}_ITK_COMPONENTS
  ITKIOImageBase
  ITKSmoothing
  )
find_package(ITK 4.6 COMPONENTS ${${PROJECT_NAME}_ITK_COMPONENTS} REQUIRED)
set(ITK_NO_IO_FACTORY_REGISTER_MANAGER 1) # See Libs/ITKFactoryRegistration/CMakeLists.txt
include(${ITK_USE_FILE})

#-----------------------------------------------------------------------------
set(MODULE_INCLUDE_DIRECTORIES
#  ${Plastimatch_DIR}
  ${PLASTIMATCH_INCLUDE_DIRS}
  ${SlicerRtCommon_INCLUDE_DIRS}
  ${vtkSlicerBeamsModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerPlanarImageModuleLogic_INCLUDE_DIRS}
  )

set(MODULE_SRCS
  )

set(MODULE_TARGET_LIBRARIES
  ${ITK_LIBRARIES}
  vtkSlicerBeamsModuleLogic
  vtkSlicerPlanarImageModuleLogic
  vtkSlicerMarkupsModuleMRML
  vtkSlicerRtCommon
  vtkPlmCommon
  vtkSlicerBeamsModuleMRML
  MRMLCore
  plmbase
  plmutil
  plmreconstruct
  ${PLASTIMATCH_LIBRARIES}
  )

#-----------------------------------------------------------------------------
SEMMacroBuildCLI(
  NAME ${MODULE_NAME}
  TARGET_LIBRARIES ${MODULE_TARGET_LIBRARIES}
  INCLUDE_DIRECTORIES ${MODULE_INCLUDE_DIRECTORIES}
  ADDITIONAL_SRCS ${MODULE_SRCS}
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${Slicer_CLIMODULES_BIN_DIR}"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${Slicer_CLIMODULES_LIB_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${Slicer_CLIMODULES_LIB_DIR}"
  #EXECUTABLE_ONLY
  )

# Set linker flags, needed for OpenMP
if (NOT ${PLASTIMATCH_LDFLAGS} STREQUAL "")
  set_target_properties (${MODULE_NAME}Lib
    PROPERTIES LINK_FLAGS ${PLASTIMATCH_LDFLAGS})
endif ()

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
