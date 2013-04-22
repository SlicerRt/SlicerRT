set(KIT qSlicer${MODULE_NAME}Module)

#-----------------------------------------------------------------------------
set(KIT_TEST_SRCS)
set(KIT_TEST_NAMES)
set(KIT_TEST_NAMES_CXX)
SlicerMacroConfigureGenericCxxModuleTests(${MODULE_NAME} KIT_TEST_SRCS KIT_TEST_NAMES KIT_TEST_NAMES_CXX)

#-----------------------------------------------------------------------------
#set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "DEBUG_LEAKS_ENABLE_EXIT_ERROR();" )
create_test_sourcelist(Tests ${KIT}CxxTests.cxx
  ${KIT_TEST_NAMES_CXX}
  vtkSlicerContourMorphologyModuleLogicTest1.cxx
  EXTRA_INCLUDE vtkMRMLDebugLeaksMacro.h
  )
list(REMOVE_ITEM Tests ${KIT_TEST_NAMES_CXX})
list(APPEND Tests ${KIT_TEST_SRCS})

#-----------------------------------------------------------------------------
add_executable(${KIT}CxxTests ${Tests})
set_target_properties(${KIT}CxxTests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${Slicer_BIN_DIR})
target_link_libraries(${KIT}CxxTests ${KIT})

#-----------------------------------------------------------------------------
foreach(testname ${KIT_TEST_NAMES})
  SIMPLE_TEST( ${testname} )
endforeach()

# Add your test after this line, using SIMPLE_TEST( <testname> )
#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

macro(TEST_WITH_DATA TestName TestExecutableName
      DataDirectoryPath InputContourALabelmapFile
      InputContourBLabelmapFile ReferenceVolumeFile BaselineContourLabelmapFile 
      TemporarySceneFile MorphologicalOperation
      MorphologicalParameter ApplySimpleTransformToInput VolumeDifferenceToleranceVoxel)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -DataDirectoryPath ${DataDirectoryPath}
    -InputContourALabelmapFile  ${InputContourALabelmapFile}
    -InputContourBLabelmapFile  ${InputContourBLabelmapFile}
    -ReferenceVolumeFile  ${ReferenceVolumeFile}
    -BaselineContourLabelmapFile  ${BaselineContourLabelmapFile}
    -TemporarySceneFile ${TemporarySceneFile}
    -MorphologicalOperation ${MorphologicalOperation}
    -MorphologicalParameter ${MorphologicalParameter}
    -ApplySimpleTransformToInput ${ApplySimpleTransformToInput}
    -VolumeDifferenceToleranceVoxel ${VolumeDifferenceToleranceVoxel}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Expand
  vtkSlicerContourMorphologyModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/
  PTV_Contour_Labelmap.nrrd
  PTV_Contour_Labelmap.nrrd
  RTDOSE.nrrd
  ContourMorphology_Expand_Baseline.nrrd
  ${TEMP}/TestScene_ContourMorphology_EclipseProstate.mrml
  Expand
  5.0
  0
  100.0
)
set_tests_properties(vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Expand PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Shrink
  vtkSlicerContourMorphologyModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/
  PTV_Contour_Labelmap.nrrd
  PTV_Contour_Labelmap.nrrd
  RTDOSE.nrrd
  ContourMorphology_Shrink_Baseline.nrrd
  ${TEMP}/TestScene_ContourMorphology_EclipseProstate.mrml
  Shrink
  5.0
  0
  100.0
)
set_tests_properties(vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Shrink PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Union
  vtkSlicerContourMorphologyModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/
  PTV_Contour_Labelmap.nrrd
  Bladder_Contour_Labelmap.nrrd
  RTDOSE.nrrd
  ContourMorphology_Union_Baseline.nrrd
  ${TEMP}/TestScene_ContourMorphology_EclipseProstate.mrml
  Union
  0.0
  0
  100.0
)
set_tests_properties(vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Union PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Intersect
  vtkSlicerContourMorphologyModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/
  PTV_Contour_Labelmap.nrrd
  Bladder_Contour_Labelmap.nrrd
  RTDOSE.nrrd
  ContourMorphology_Intersect_Baseline.nrrd
  ${TEMP}/TestScene_ContourMorphology_EclipseProstate.mrml
  Intersect
  0.0
  0
  100.0
)
set_tests_properties(vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Intersect PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Subtract
  vtkSlicerContourMorphologyModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/
  PTV_Contour_Labelmap.nrrd
  Bladder_Contour_Labelmap.nrrd
  RTDose.nrrd
  ContourMorphology_Subtract_Baseline.nrrd
  ${TEMP}/TestScene_ContourMorphology_EclipseProstate.mrml
  Subtract
  0.0
  0
  100.0
)
set_tests_properties(vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Subtract PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Intersect_ApplyTransform
  vtkSlicerContourMorphologyModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate/
  PTV_Contour_Labelmap.nrrd
  Bladder_Contour_Labelmap.nrrd
  RTDOSE.nrrd
  ContourMorphology_Intersect_ApplyTransform_Baseline.nrrd
  ${TEMP}/TestScene_ContourMorphology_EclipseProstate.mrml
  Intersect
  0.0
  1
  100.0
)
set_tests_properties(vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Intersect_ApplyTransform PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )
