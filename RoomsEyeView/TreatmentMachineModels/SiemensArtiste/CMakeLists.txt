#-----------------------------------------------------------------------------
# Add models
#-----------------------------------------------------------------------------

set(MACHINE_NAME SiemensArtiste)

set(MACHINE_MODELS
  collimator.stl
  column.stl
  flatpanel.stl
  gantry2.stl
  table.stl
)

SlicerMacroConfigureTreatmentMachineComponentModels(
  NAME ${MACHINE_NAME}
  MODELS ${MACHINE_MODELS}
)
