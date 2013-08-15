set(KIT qSlicer${MODULE_NAME}Module)

set(KIT_TEST_SRCS
  vtkSlicerDoseAccumulationModuleLogicTest1.cxx
  )

slicerMacroConfigureModuleCxxTestDriver(
  NAME ${KIT}
  SOURCES ${KIT_TEST_SRCS}
  TARGET_LIBRARIES vtkSlicerDoseAccumulationModuleLogic
  WITH_VTK_DEBUG_LEAKS_CHECK
  )

#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

macro(TEST_WITH_DATA TestName TestExecutableName
      TestSceneFile TemporarySceneFile DoseDifferenceCriterion)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -TestSceneFile ${TestSceneFile}
    -TemporarySceneFile ${TemporarySceneFile}
    -DoseDifferenceCriterion ${DoseDifferenceCriterion}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSliceDoseAccumulationModuleLogicTest_EclipseProstate
  vtkSlicerDoseAccumulationModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/DoseAccumulation_Scene.mrml
  ${TEMP}/TestScene_DoseAccumulation_EclipseProstate.mrml
  1.0
)
set_tests_properties(vtkSliceDoseAccumulationModuleLogicTest_EclipseProstate PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#ADD_TEST(vtkSlicerDoseAccumulationModuleCompareToBaselineTest
#   ${CMAKE_COMMAND} -E compare_files 
#   ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/Dose.nrrd 
#   ${TEMP}/AccumulatedDose.nrrd
#  )
