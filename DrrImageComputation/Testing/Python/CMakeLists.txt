#-----------------------------------------------------------------------------
set(MODULE_TEST_PYTHON_SCRIPTS
  DrrImageComputationTest.py
  )

set(MODULE_TEST_PYTHON_RESOURCES
  )

#-----------------------------------------------------------------------------
set(KIT_UNITTEST_SCRIPTS)
SlicerMacroConfigureGenericPythonModuleTests("${MODULE_NAME}" KIT_UNITTEST_SCRIPTS)

#-----------------------------------------------------------------------------
# Make sure the generic tests have access to the other modules
if(CMAKE_CONFIGURATION_TYPES)
  set(MODULE_BUILD_DIR "")
  foreach(config ${CMAKE_CONFIGURATION_TYPES})
    list(APPEND MODULE_BUILD_DIR "${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_LIB_DIR}/${config}")
    list(APPEND MODULE_BUILD_DIR "${CMAKE_BINARY_DIR}/${Slicer_CLIMODULES_LIB_DIR}/${config}")
  endforeach()
else()
  set(MODULE_BUILD_DIR "${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_LIB_DIR}")
endif()

foreach(script_name ${KIT_UNITTEST_SCRIPTS})
  slicer_add_python_unittest(
    SCRIPT ${script_name}
    SLICER_ARGS --no-main-window
                --additional-module-paths
                  ${MODULE_BUILD_DIR}
                  ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}
    TESTNAME_PREFIX nomainwindow_
    )
endforeach()

slicer_add_python_unittest(
  SCRIPT DrrImageComputationTest.py
  SLICER_ARGS --no-main-window
              --additional-module-paths
                ${MODULE_BUILD_DIR}
                ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR} 
  # TESTNAME_PREFIX nomainwindow_
  )
