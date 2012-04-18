set(KIT_TEST_SRCS)
set(KIT_TEST_NAMES)
set(KIT_TEST_NAMES_CXX)
SlicerMacroConfigureGenericCxxModuleTests(${EXTENSION_NAME} KIT_TEST_SRCS KIT_TEST_NAMES KIT_TEST_NAMES_CXX)

#set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "DEBUG_LEAKS_ENABLE_EXIT_ERROR();" )
create_test_sourcelist(Tests ${EXTENSION_NAME}CxxTests.cxx
  ${KIT_TEST_NAMES_CXX}
  # Add source of your tests after this line.
  vtkSlicerDoseVolumeHistogramLogicTest1.cxx
  #EXTRA_INCLUDE TestingMacros.h
  )
list(REMOVE_ITEM Tests ${KIT_TEST_NAMES_CXX})
list(APPEND Tests ${KIT_TEST_SRCS})

set(TestsToRun ${Tests})
#list(REMOVE_ITEM TestsToRun ${KIT}CxxTests.cxx)

set(LIBRARY_NAME qSlicer${EXTENSION_NAME}Module)

add_executable(${EXTENSION_NAME}CxxTests ${Tests})
target_link_libraries(${EXTENSION_NAME}CxxTests ${lib_name})

set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

# Add test
macro(SIMPLE_TEST TESTNAME)
  add_test(
    NAME ${TESTNAME}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${EXTENSION_NAME}CxxTests> ${TESTNAME} ${ARGN}
    -D ${CMAKE_CURRENT_SOURCE_DIR}/../
    -T ${TEMP}
  )
endmacro()

foreach(testname ${KIT_TEST_NAMES})
  SIMPLE_TEST( ${testname} )
endforeach()

# Using SIMPLE_TEST(), you coudl add your test after this line.
SIMPLE_TEST(
  vtkSlicerDoseVolumeHistogramLogicTest1
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest1 PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

# Compare exported DVH table with baseline
add_test(
  NAME vtkSlicerDoseVolumeHistogramLogicTest2
  COMMAND ${CMAKE_COMMAND} -E compare_files ${TEMP}/DvhTestTable.csv ${CMAKE_CURRENT_SOURCE_DIR}/../Data/DvhTable.csv
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest2 PROPERTIES DEPENDS vtkSlicerDoseVolumeHistogramLogicTest1)

# Compare exported DVH table with baseline
add_test(
  NAME vtkSlicerDoseVolumeHistogramLogicTest3
  COMMAND ${CMAKE_COMMAND} -E compare_files ${TEMP}/DvhTestMetrics.csv ${CMAKE_CURRENT_SOURCE_DIR}/../Data/DvhMetrics.csv
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest3 PROPERTIES DEPENDS vtkSlicerDoseVolumeHistogramLogicTest1)
