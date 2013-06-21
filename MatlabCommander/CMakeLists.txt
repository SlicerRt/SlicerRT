#-----------------------------------------------------------------------------
set(MODULE_NAME MatlabCommander)

find_package(OpenIGTLink QUIET PATHS ${OpenIGTLink_DIR} )
if(NOT OpenIGTLink_FOUND)
  message("OpenIGTLink library is not found. The ${MODULE_NAME} module will not be built.")
  return()
endif() 

include(${OpenIGTLink_USE_FILE})

#-----------------------------------------------------------------------------
set(MODULE_INCLUDE_DIRECTORIES
  ${OpenIGTLink_INCLUDE_DIRS}  
  )

set(MODULE_SRCS
  )

set(MODULE_TARGET_LIBRARIES
  ${ITK_LIBRARIES}
  OpenIGTLink
  vtksys
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

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  #add_subdirectory(Testing)
endif()

#-----------------------------------------------------------------------------

set(SCRIPT_FILES
  commandserver.m 
  getarg.m 
  setarg.m
  nrrdread.m 
  nrrdwrite.m 
  )

foreach(SCRIPT_FILE ${SCRIPT_FILES})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/${SCRIPT_FILE}
    ${CMAKE_BINARY_DIR}/${Slicer_CLIMODULES_BIN_DIR}/Debug/${SCRIPT_FILE}    
    COPYONLY)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/${SCRIPT_FILE}
    ${CMAKE_BINARY_DIR}/${Slicer_CLIMODULES_BIN_DIR}/Release/${SCRIPT_FILE}    
    COPYONLY)    
  install(
    FILES ${CMAKE_BINARY_DIR}/${Slicer_CLIMODULES_BIN_DIR}/Release/${SCRIPT_FILE}
    DESTINATION ${Slicer_INSTALL_CLIMODULES_BIN_DIR} COMPONENT Runtime)
endforeach()
