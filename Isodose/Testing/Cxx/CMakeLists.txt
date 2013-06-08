set(KIT qSlicer${MODULE_NAME}Module)

set(KIT_TEST_SRCS)
set(KIT_TEST_NAMES)
set(KIT_TEST_NAMES_CXX)
SlicerMacroConfigureGenericCxxModuleTests(${MODULE_NAME} KIT_TEST_SRCS KIT_TEST_NAMES KIT_TEST_NAMES_CXX)

set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "DEBUG_LEAKS_ENABLE_EXIT_ERROR();" )
create_test_sourcelist(Tests ${KIT}CxxTests.cxx
  ${KIT_TEST_NAMES_CXX}
  vtkSlicerIsodoseModuleLogicTest1.cxx
  EXTRA_INCLUDE vtkMRMLDebugLeaksMacro.h
  )

list(REMOVE_ITEM Tests ${KIT_TEST_NAMES_CXX})
list(APPEND Tests ${KIT_TEST_SRCS})

add_executable(${KIT}CxxTests ${Tests})
set_target_properties(${KIT}CxxTests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${Slicer_BIN_DIR})
target_link_libraries(${KIT}CxxTests ${KIT})

foreach(testname ${KIT_TEST_NAMES})
  SIMPLE_TEST( ${testname} )
endforeach()

#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

macro(TEST_WITH_DATA TestName TestExecutableName
      DataDirectoryPath BaselineIsodoseSurfaceFile 
      TemporarySceneFile VolumeDifferenceToleranceCc)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -DataDirectoryPath ${DataDirectoryPath}
    -BaselineIsodoseSurfaceFile ${BaselineIsodoseSurfaceFile}
    -TemporarySceneFile ${TemporarySceneFile}
    -VolumeDifferenceToleranceCc ${VolumeDifferenceToleranceCc}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerIsodoseModuleLogicTest_EclipseProstate
  vtkSlicerIsodoseModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/IsodoseBaseline.vtk
  ${TEMP}/TestScene_Isodose_EclipseProstate.mrml
  1.0
)
set_tests_properties(vtkSlicerIsodoseModuleLogicTest_EclipseProstate PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )
