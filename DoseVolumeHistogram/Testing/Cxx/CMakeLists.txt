set(KIT qSlicer${MODULE_NAME}Module)

set(KIT_TEST_SRCS)
set(KIT_TEST_NAMES)
set(KIT_TEST_NAMES_CXX)
SlicerMacroConfigureGenericCxxModuleTests(${MODULE_NAME} KIT_TEST_SRCS KIT_TEST_NAMES KIT_TEST_NAMES_CXX)

set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "DEBUG_LEAKS_ENABLE_EXIT_ERROR();" )
create_test_sourcelist(Tests ${KIT}CxxTests.cxx
  ${KIT_TEST_NAMES_CXX}
  vtkSlicerDoseVolumeHistogramModuleLogicTest1.cxx
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
      DataDirectoryPath BaselineDvhTableCsvFile BaselineDvhMetricCsvFile
      TemporarySceneFile TemporaryDvhTableCsvFile TemporaryDvhMetricCsvFile
      VolumeDifferenceCriterion DoseToAgreementCriterion AgreementAcceptancePercentageThreshold MetricDifferenceThreshold
      DvhStartValue DvhStepSize RasterizationOversamplingFactor)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -DataDirectoryPath ${DataDirectoryPath}
    -BaselineDvhTableCsvFile ${BaselineDvhTableCsvFile}
    -BaselineDvhMetricCsvFile ${BaselineDvhMetricCsvFile}
    -TemporarySceneFile ${TemporarySceneFile}
    -TemporaryDvhTableCsvFile ${TemporaryDvhTableCsvFile}
    -TemporaryDvhMetricCsvFile ${TemporaryDvhMetricCsvFile}
    -VolumeDifferenceCriterion ${VolumeDifferenceCriterion}
    -DoseToAgreementCriterion ${DoseToAgreementCriterion}
    -AgreementAcceptancePercentageThreshold ${AgreementAcceptancePercentageThreshold}
    -MetricDifferenceThreshold ${MetricDifferenceThreshold}
    -DvhStartValue ${DvhStartValue}
    -DvhStepSize ${DvhStepSize}
    -RasterizationOversamplingFactor ${RasterizationOversamplingFactor}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/DvhTable_SlicerRT.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/DvhMetrics_SlicerRT.csv
  ${TEMP}/TestScene_EclipseProstate.mrml
  ${TEMP}/TestDvhTable_EclipseProstate_SlicerRT.csv
  ${TEMP}/TestDvhMetrics_EclipseProstate_SlicerRT.csv
  0.0
  0.0
  100.0
  0.0
  0.0
  0.0
  2.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_CERR
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/DvhTable_CERR.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/NoMetricComparison
  ${TEMP}/TestScene_EclipseProstate_CERR.mrml
  ${TEMP}/TestDvhTable_EclipseProstate_CERR_SlicerRT.csv
  ${TEMP}/TestDvhMetrics_EclipseProstate_CERR_SlicerRT.csv
  1.0
  1.0
  95.0
  3.0
  0.01
  0.01
  2.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_CERR PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseEnt_CERR
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseEnt
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseEnt/DvhTable_CERR.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseEnt/NoMetricComparison
  ${TEMP}/TestScene_EclipseEnt_CERR.mrml
  ${TEMP}/TestDvhTable_EclipseEnt_CERR_SlicerRT.csv
  ${TEMP}/TestDvhMetrics_EclipseEnt_CERR_SlicerRT.csv
  1.0
  1.0
  95.0
  3.0
  0.01
  0.01
  2.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseEnt_CERR PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseBreast_CERR
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseBreast
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseBreast/DvhTable_CERR.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseBreast/NoMetricComparison
  ${TEMP}/TestScene_EclipseBreast_CERR.mrml
  ${TEMP}/TestDvhTable_EclipseBreast_CERR_SlicerRT.csv
  ${TEMP}/TestDvhMetrics_EclipseBreast_CERR_SlicerRT.csv
  1.0
  1.0
  95.0
  3.0
  0.01
  0.01
  2.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseBreast_CERR PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_Eclipse
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/DvhTable_Eclipse.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/NoMetricComparison
  ${TEMP}/TestScene_EclipseProstate_Eclipse.mrml
  ${TEMP}/TestDvhTable_EclipseProstate_Eclipse_SlicerRT.csv
  ${TEMP}/TestDvhMetrics_EclipseProstate_Eclipse_SlicerRT.csv
  1.0
  1.0
  95.0
  3.0
  0.01
  0.01
  2.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_Eclipse PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseEnt_Eclipse
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseEnt
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseEnt/DvhTable_Eclipse.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseEnt/NoMetricComparison
  ${TEMP}/TestScene_EclipseEnt_Eclipse.mrml
  ${TEMP}/TestDvhTable_EclipseEnt_Eclipse_SlicerRT.csv
  ${TEMP}/TestDvhMetrics_EclipseEnt_Eclipse_SlicerRT.csv
  1.0
  1.0
  95.0
  3.0
  0.01
  0.01
  2.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseEnt_Eclipse PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseBreast_Eclipse
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseBreast
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseBreast/DvhTable_Eclipse.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseBreast/NoMetricComparison
  ${TEMP}/TestScene_EclipseBreast_Eclipse.mrml
  ${TEMP}/TestDvhTable_EclipseBreast_Eclipse_SlicerRT.csv
  ${TEMP}/TestDvhMetrics_EclipseBreast_Eclipse_SlicerRT.csv
  1.0
  1.0
  95.0
  3.0
  0.01
  0.01
  2.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseBreast_Eclipse PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )
