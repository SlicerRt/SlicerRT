set(KIT qSlicer${MODULE_NAME}Module)

set(KIT_TEST_SRCS
  vtkSlicerContoursModuleLogicTestConversions.cxx
  )

slicerMacroConfigureModuleCxxTestDriver(
  NAME ${KIT}
  SOURCES ${KIT_TEST_SRCS}
  TARGET_LIBRARIES vtkSlicer${MODULE_NAME}ModuleLogic
  WITH_VTK_DEBUG_LEAKS_CHECK
  )

#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

macro(TEST_WITH_DATA TestName TestExecutableName
      TestSceneFile TemporarySceneFile NonZeroVoxelCount
      LabelMapXMinExtent LabelMapXMaxExtent LabelMapYMinExtent
      LabelMapYMaxExtent LabelMapZMinExtent LabelMapZMaxExtent
      ExpectedNumberOfPoints ExpectedNumberOfCells ExpectedNumberOfPolys
      ClosedSurfaceXMinExtent ClosedSurfaceXMaxExtent ClosedSurfaceYMinExtent
      ClosedSurfaceYMaxExtent ClosedSurfaceZMinExtent ClosedSurfaceZMaxExtent )
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -TestSceneFile ${TestSceneFile}
    -TemporarySceneFile ${TemporarySceneFile}
    -NonZeroVoxelCount ${NonZeroVoxelCount}
    -LabelMapXMinExtent ${LabelMapXMinExtent}
    -LabelMapXMaxExtent ${LabelMapXMaxExtent}
    -LabelMapYMinExtent ${LabelMapYMinExtent}
    -LabelMapYMaxExtent ${LabelMapYMaxExtent}
    -LabelMapZMinExtent ${LabelMapZMinExtent}
    -LabelMapZMaxExtent ${LabelMapZMaxExtent}
    -ExpectedNumberOfPoints ${ExpectedNumberOfPoints}
    -ExpectedNumberOfCells ${ExpectedNumberOfCells}
    -ExpectedNumberOfPolys ${ExpectedNumberOfPolys}
    -ClosedSurfaceXMinExtent ${ClosedSurfaceXMinExtent}
    -ClosedSurfaceXMaxExtent ${ClosedSurfaceXMaxExtent}
    -ClosedSurfaceYMinExtent ${ClosedSurfaceYMinExtent}
    -ClosedSurfaceYMaxExtent ${ClosedSurfaceYMaxExtent}
    -ClosedSurfaceZMinExtent ${ClosedSurfaceZMinExtent}
    -ClosedSurfaceZMaxExtent ${ClosedSurfaceZMaxExtent}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContoursModuleLogicTest_EclipseProstate
  vtkSlicerContoursModuleLogicTestConversions
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseProstate_Contours_Scene.mrml
  ${TEMP}/TestScene_Contours_EclipseProstate.mrml 
  5448764
  -1
  271
  -1
  171
  -1
  183
  198748
  396728
  396728
  -172.799
  162.201
  -106.858
  105.642
  -123.125
  106.25
)
set_tests_properties(vtkSlicerContoursModuleLogicTest_EclipseProstate PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )