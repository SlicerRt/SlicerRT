#-----------------------------------------------------------------------------
# Add models
#-----------------------------------------------------------------------------

set(MACHINE_NAME VarianTrueBeamSTx)

set(MACHINE_MODELS
  CollimatorModel.stl
  GantryModel.stl
  ImagingPanelLeftModel.stl
  ImagingPanelRightModel.stl
  LinacBodyModel.stl
  PatientSupportModel.stl
  TableTopModel.stl
)

SlicerMacroConfigureTreatmentMachineComponentModels(
  NAME ${MACHINE_NAME}
  MODELS ${MACHINE_MODELS}
)
