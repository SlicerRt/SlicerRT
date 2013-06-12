#-----------------------------------------------------------------------------
set(EXTENSION_TEST_PYTHON_SCRIPTS
  NAMIC_Tutorial_2013June_SelfTest.py
  )

set(EXTENSION_TEST_PYTHON_RESOURCES
  )

ctkMacroCompilePythonScript(
  TARGET_NAME NAMIC_Tutorial_2013June_SelfTest
  SCRIPTS "${EXTENSION_TEST_PYTHON_SCRIPTS}"
  RESOURCES "${EXTENSION_TEST_PYTHON_RESOURCES}"
  DESTINATION_DIR ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}
  INSTALL_DIR ${Slicer_INSTALL_QTSCRIPTEDMODULES_LIB_DIR}
  NO_INSTALL_SUBDIR
  )

#-----------------------------------------------------------------------------
slicer_add_python_unittest(
  SCRIPT NAMIC_Tutorial_2013June_SelfTest.py
  SLICER_ARGS --additional-module-paths
                ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_LIB_DIR}
                ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_LIB_DIR}/Debug
                ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_LIB_DIR}/Release
                ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR} 
  )
