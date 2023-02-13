#-----------------------------------------------------------------------------
# Add models
#-----------------------------------------------------------------------------

set(MACHINE_NAME SiemensArtiste)

set(MACHINE_MODELS
  SiemensArtiste.json
  Collimator.stl
  PatientSupport.stl
  FlatPanel.stl
  Gantry.stl
  TableTop.stl
)

SlicerMacroConfigureTreatmentMachineComponentModels(
  NAME ${MACHINE_NAME}
  MODELS ${MACHINE_MODELS}
)
