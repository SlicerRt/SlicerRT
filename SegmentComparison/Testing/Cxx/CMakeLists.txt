set(KIT qSlicer${MODULE_NAME}Module)

set(KIT_TEST_SRCS
  vtkSlicerSegmentComparisonModuleLogicTest1.cxx
  vtkPolyDataDistanceHistogramFilterTest.cxx
  )

slicerMacroConfigureModuleCxxTestDriver(
  NAME ${KIT}
  SOURCES ${KIT_TEST_SRCS}
  TARGET_LIBRARIES vtkSlicerSegmentComparisonModuleLogic vtkSlicerDicomRtImportExportConversionRules
  WITH_VTK_DEBUG_LEAKS_CHECK
  )

#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

macro(TEST_WITH_DATA TestName TestExecutableName
      DataDirectoryPath InputSegmentationReferenceFile InputSegmentationCompareFile
      TemporarySceneFile ApplySimpleTransformToInputCompare
      HausdorffMaximumMm HausdorffAverageMm Hausdorff95PercentMm
      DiceCoefficient TruePositivesPercent TrueNegativesPercent
      FalsePositivesPercent FalseNegativesPercent)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -DataDirectoryPath ${DataDirectoryPath}
    -InputSegmentationReferenceFile ${InputSegmentationReferenceFile}
    -InputSegmentationCompareFile ${InputSegmentationCompareFile}
    -TemporarySceneFile ${TemporarySceneFile}
    -ApplySimpleTransformToInputCompare ${ApplySimpleTransformToInputCompare}
    -HausdorffMaximumMm ${HausdorffMaximumMm}
    -HausdorffAverageMm ${HausdorffAverageMm}
    -Hausdorff95PercentMm ${Hausdorff95PercentMm}
    -DiceCoefficient ${DiceCoefficient}
    -TruePositivesPercent ${TruePositivesPercent}
    -TrueNegativesPercent ${TrueNegativesPercent}
    -FalsePositivesPercent ${FalsePositivesPercent}
    -FalseNegativesPercent ${FalseNegativesPercent}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerSegmentComparisonModuleLogicTest_EclipseProstate_Base
  vtkSlicerSegmentComparisonModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/
  EclipseProstate_Rectum.seg.vtm
  EclipseProstate_Expanded_5_5_5_Rectum.seg.nrrd
  ${TEMP}/TestScene_SegmentComparison_EclipseProstate.mrml
  0
  8.34375
  5.17621
  6.07853
  0.542084
  11.2075
  69.8579
  18.9346
  0.0
)
set_tests_properties(vtkSlicerSegmentComparisonModuleLogicTest_EclipseProstate_Base PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerSegmentComparisonModuleLogicTest_EclipseProstate_SameInput
  vtkSlicerSegmentComparisonModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/
  EclipseProstate_Rectum.seg.vtm
  EclipseProstate_Rectum.seg.vtm
  ${TEMP}/TestScene_SegmentComparison_EclipseProstate_SameInput.mrml
  0
  0.0
  0.0
  0.0
  1
  22.528
  77.472
  0.0
  0.0
)
set_tests_properties(vtkSlicerSegmentComparisonModuleLogicTest_EclipseProstate_SameInput PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerSegmentComparisonModuleLogicTest_EclipseProstate_Transformed
  vtkSlicerSegmentComparisonModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/
  EclipseProstate_Rectum.seg.vtm
  EclipseProstate_Rectum.seg.vtm
  ${TEMP}/TestScene_SegmentComparison_EclipseProstate_Transformed.mrml
  1
  4.88281
  2.13749
  4.68861
  0.700964
  13.431
  75.1094
  5.72977
  5.72977
)
set_tests_properties(vtkSlicerSegmentComparisonModuleLogicTest_EclipseProstate_Transformed PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
set(POLY_DATA_DISTANCES_RAW_OUTPUT_FILE "${TEMP}/PolyDataDistancesRawOutput.csv")
set(POLY_DATA_DISTANCES_HISTOGRAM_OUTPUT_FILE "${TEMP}/PolyDataDistancesHistogramOutput.csv")

add_test(
  NAME vtkPolyDataDistanceHistogramFilterExecutionTest
  COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> vtkPolyDataDistanceHistogramFilterTest ${ARGN}
  -RawDistancesPath ${POLY_DATA_DISTANCES_RAW_OUTPUT_FILE}
  -HistogramPath ${POLY_DATA_DISTANCES_HISTOGRAM_OUTPUT_FILE}
)

#-----------------------------------------------------------------------------
if(UNIX)
  set(POLY_DATA_DISTANCES_RAW_GROUNDTRUTH_TARBALL "${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/PolyDataDistancesRawGroundTruth.tgz")
elseif(WIN32)
  set(POLY_DATA_DISTANCES_RAW_GROUNDTRUTH_TARBALL "${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/PolyDataDistancesRawGroundTruthWindows.tgz")
endif()

add_test(
  NAME vtkPolyDataDistancesRawOutputUnpackTest
  COMMAND ${CMAKE_COMMAND} -E tar xvfz ${POLY_DATA_DISTANCES_RAW_GROUNDTRUTH_TARBALL}
  WORKING_DIRECTORY ${TEMP}
)
set_tests_properties(vtkPolyDataDistancesRawOutputUnpackTest PROPERTIES DEPENDS vtkPolyDataDistanceHistogramFilterExecutionTest REQUIRED_FILES ${POLY_DATA_DISTANCES_RAW_OUTPUT_FILE})

#-----------------------------------------------------------------------------
set(POLY_DATA_DISTANCES_RAW_GROUNDTRUTH_FILE "${TEMP}/PolyDataDistancesRawGroundTruth.csv")

add_test(
  NAME vtkPolyDataDistancesRawOutputComparisonTest
  COMMAND ${CMAKE_COMMAND} -E compare_files
  ${POLY_DATA_DISTANCES_RAW_OUTPUT_FILE}
  ${POLY_DATA_DISTANCES_RAW_GROUNDTRUTH_FILE}
)
set_tests_properties(vtkPolyDataDistancesRawOutputComparisonTest PROPERTIES DEPENDS vtkPolyDataDistancesRawOutputUnpackTest REQUIRED_FILES ${POLY_DATA_DISTANCES_RAW_OUTPUT_FILE})

#-----------------------------------------------------------------------------
set(POLY_DATA_DISTANCES_HISTOGRAM_GROUNDTRUTH_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/PolyDataDistancesHistogramGroundTruthFixedLineEnding.csv")
# Make sure the ground truth file has line endings consistent with the OS default
if(UNIX)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/PolyDataDistancesHistogramGroundTruth.csv" "${POLY_DATA_DISTANCES_HISTOGRAM_GROUNDTRUTH_FILE}" NEWLINE_STYLE UNIX)
elseif(WIN32)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/PolyDataDistancesHistogramGroundTruth.csv" "${POLY_DATA_DISTANCES_HISTOGRAM_GROUNDTRUTH_FILE}" NEWLINE_STYLE CRLF)
endif()

add_test(
  vtkPolyDataDistancesHistogramOutputComparisonTest
  ${CMAKE_COMMAND} -E compare_files 
  ${POLY_DATA_DISTANCES_HISTOGRAM_OUTPUT_FILE}
  ${POLY_DATA_DISTANCES_HISTOGRAM_GROUNDTRUTH_FILE}
)

set_tests_properties(vtkPolyDataDistancesHistogramOutputComparisonTest PROPERTIES DEPENDS vtkPolyDataDistanceHistogramFilterExecutionTest REQUIRED_FILES ${POLY_DATA_DISTANCES_HISTOGRAM_OUTPUT_FILE})
