set(KIT qSlicer${MODULE_NAME}Module)

#-----------------------------------------------------------------------------
set(KIT_TEST_SRCS)
set(KIT_TEST_NAMES)
set(KIT_TEST_NAMES_CXX)
SlicerMacroConfigureGenericCxxModuleTests(${MODULE_NAME} KIT_TEST_SRCS KIT_TEST_NAMES KIT_TEST_NAMES_CXX)

#-----------------------------------------------------------------------------
#set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "DEBUG_LEAKS_ENABLE_EXIT_ERROR();" )
create_test_sourcelist(Tests ${MODULE_NAME}CxxTests.cxx
  ${KIT_TEST_NAMES_CXX}
  # Add source of your tests after this line.
  #EXTRA_INCLUDE vtkMRMLDebugLeaksMacro.h
  vtkSlicerDoseVolumeHistogramLogicTest1.cxx
  )

#-----------------------------------------------------------------------------
list(REMOVE_ITEM Tests ${KIT_TEST_NAMES_CXX})
list(APPEND Tests ${KIT_TEST_SRCS})

#-----------------------------------------------------------------------------
add_executable(${KIT}CxxTests ${Tests})
target_link_libraries(${KIT}CxxTests ${KIT})

#-----------------------------------------------------------------------------
foreach(testname ${KIT_TEST_NAMES})
  SIMPLE_TEST( ${testname} )
endforeach()

#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

macro(TEST_WITH_DATA TestName TestExecutableName DataDirectoryPath BaselineDirectoryPath TemporaryDirectoryPath VolumeDifferenceCriterion DoseToAgreementCriterion AgreementAcceptancePercentageThreshold MetricDifferenceThreshold DvhStartValue DvhStepSize)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -DataDirectoryPath ${DataDirectoryPath}
    -BaselineDirectoryPath ${BaselineDirectoryPath}
    -TemporaryDirectoryPath ${TemporaryDirectoryPath}
    -VolumeDifferenceCriterion ${VolumeDifferenceCriterion}
	-DoseToAgreementCriterion ${DoseToAgreementCriterion}
    -AgreementAcceptancePercentageThreshold ${AgreementAcceptancePercentageThreshold}
	-MetricDifferenceThreshold ${MetricDifferenceThreshold}
	-DvhStartValue ${DvhStartValue}
	-DvhStepSize ${DvhStepSize}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramLogicTest_EclipseProstate
  vtkSlicerDoseVolumeHistogramLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Data_EclipseProstate
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline_EclipseProstate
  ${TEMP}
  0.0
  0.0
  100.0
  0.0
  0.0
  0.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest_EclipseProstate PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramLogicTest_EclipseProstate_CERR
  vtkSlicerDoseVolumeHistogramLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Data_EclipseProstate
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline_EclipseProstate_CERR
  ${TEMP}
  2.0
  1.0
  95.0
  3.0
  0.0
  0.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest_EclipseProstate_CERR PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramLogicTest_EclipseEnt_CERR
  vtkSlicerDoseVolumeHistogramLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Data_EclipseEnt
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline_EclipseEnt_CERR
  ${TEMP}
  2.0
  1.0
  66.9
  3.0
  0.0
  0.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest_EclipseEnt_CERR PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramLogicTest_EclipseBreast_CERR
  vtkSlicerDoseVolumeHistogramLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Data_EclipseBreast
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline_EclipseBreast_CERR
  ${TEMP}
  2.0
  1.0
  95.0
  3.0
  0.0
  0.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest_EclipseBreast_CERR PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramLogicTest_EclipseProstate_Eclipse
  vtkSlicerDoseVolumeHistogramLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Data_EclipseProstate
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline_EclipseProstate_Eclipse
  ${TEMP}
  2.0
  1.0
  95.0
  3.0
  0.01
  0.02
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest_EclipseProstate_Eclipse PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramLogicTest_EclipseEnt_Eclipse
  vtkSlicerDoseVolumeHistogramLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Data_EclipseEnt
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline_EclipseEnt_Eclipse
  ${TEMP}
  2.0
  1.0
  70.3
  3.0
  0.01
  0.02
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest_EclipseEnt_Eclipse PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramLogicTest_EclipseBreast_Eclipse
  vtkSlicerDoseVolumeHistogramLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Data_EclipseBreast
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline_EclipseBreast_Eclipse
  ${TEMP}
  2.0
  1.0
  95.0
  3.0
  0.01
  0.02
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest_EclipseBreast_Eclipse PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )
