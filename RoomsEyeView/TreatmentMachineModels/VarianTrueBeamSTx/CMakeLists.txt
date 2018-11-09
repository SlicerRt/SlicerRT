#-----------------------------------------------------------------------------
# Add models
#-----------------------------------------------------------------------------

set(MACHINE_NAME VarianTrueBeamSTx)

set(MACHINE_MODELS
  Collimator.stl
  Gantry.stl
  ImagingPanelLeft.stl
  ImagingPanelRight.stl
  LinacBody.stl
  PatientSupport.stl
  TableTop.stl
)

SlicerMacroConfigureTreatmentMachineComponentModels(
  NAME ${MACHINE_NAME}
  MODELS ${MACHINE_MODELS}
)
