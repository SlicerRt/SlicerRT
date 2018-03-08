#-----------------------------------------------------------------------------
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/DoseEngines.__init__.py.in
  ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}/DoseEngines/__init__.py
  )

#-----------------------------------------------------------------------------
set(DoseEngines_PYTHON_SCRIPTS
  AbstractScriptedDoseEngine
  MockPythonDoseEngine
  OrthovoltageDoseEngineUtil
  OrthovoltageDoseEngine
  )

set(DoseEngines_PYTHON_RESOURCES
  )

ctkMacroCompilePythonScript(
  TARGET_NAME DoseEngines
  SCRIPTS "${DoseEngines_PYTHON_SCRIPTS}"
  RESOURCES "${DoseEngines_PYTHON_RESOURCES}"
  DESTINATION_DIR ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}/DoseEngines
  INSTALL_DIR ${Slicer_INSTALL_QTSCRIPTEDMODULES_LIB_DIR}/DoseEngines
  )
