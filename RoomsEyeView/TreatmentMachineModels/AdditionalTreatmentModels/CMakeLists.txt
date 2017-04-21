#-----------------------------------------------------------------------------
# Add models
#-----------------------------------------------------------------------------

set(MACHINE_NAME AdditionalTreatmentModels)

set(MACHINE_MODELS
  ApplicatorHolderModel.stl
  ElectronApplicatorModel.stl
)

SlicerMacroConfigureTreatmentMachineComponentModels(
  NAME ${MACHINE_NAME}
  MODELS ${MACHINE_MODELS}
)
