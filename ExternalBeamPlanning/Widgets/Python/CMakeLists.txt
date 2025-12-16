add_subdirectory(DoseEngines)
add_subdirectory(PlanOptimizers)

#-----------------------------------------------------------------------------
set(Python_PYTHON_SCRIPTS
  PyRadPlanUtils.py
  TopasDoseEngineUtil.py
  )

set(Python_PYTHON_RESOURCES
  Resources/HUtoMaterialSchneider.txt
  )

set(DoseEngines_PYTHON_SCRIPTS
  AbstractScriptedDoseEngine
  # MockPythonDoseEngine
  # EGSnrcUtil
  # OrthovoltageDoseEngineUtil
  # OrthovoltageDoseEngine
  )

set(DoseEngines_PYTHON_RESOURCES
  )

ctkMacroCompilePythonScript(
  TARGET_NAME Python
  SCRIPTS "${Python_PYTHON_SCRIPTS}"
  RESOURCES "${Python_PYTHON_RESOURCES}"
  DESTINATION_DIR ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}/Python
  INSTALL_DIR ${Slicer_INSTALL_QTSCRIPTEDMODULES_LIB_DIR}/Python
)
