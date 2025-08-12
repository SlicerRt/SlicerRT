add_subdirectory(DoseEngines)
add_subdirectory(PlanOptimizers)

#-----------------------------------------------------------------------------
set(Python_PYTHON_SCRIPTS
  PyRadPlanUtils.py
  )

ctkMacroCompilePythonScript(
TARGET_NAME Python
SCRIPTS "${Python_PYTHON_SCRIPTS}"
DESTINATION_DIR ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}/Python
INSTALL_DIR ${Slicer_INSTALL_QTSCRIPTEDMODULES_LIB_DIR}/Python
)
