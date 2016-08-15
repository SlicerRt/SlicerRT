# --------------------------------------------------------------------------
# Macro for configuring treatment machine component models
# --------------------------------------------------------------------------
macro(SlicerMacroConfigureTreatmentMachineComponentModels)
  set(options)
  set(oneValueArgs
    NAME
    )
  set(multiValueArgs
    MODELS
    )
  CMAKE_PARSE_ARGUMENTS(TREATMENTMACHINE
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
    )

  if(TREATMENTMACHINE_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unknown keywords given to SlicerMacroConfigureTreatmentMachineComponentModels(): \"${TREATMENTMACHINE_UNPARSED_ARGUMENTS}\"")
  endif()

  message(STATUS "Configuring treatment machine models: ${TREATMENTMACHINE_NAME}")

  foreach(MODEL ${TREATMENTMACHINE_MODELS})
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/${MODEL}
      ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_SHARE_DIR}/${MODULE_NAME}/${TREATMENTMACHINE_NAME}/${MODEL}
      COPYONLY)

    install(
      FILES ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_SHARE_DIR}/${MODULE_NAME}/${TREATMENTMACHINE_NAME}/${MODEL}
      DESTINATION ${Slicer_INSTALL_QTLOADABLEMODULES_SHARE_DIR}/${MODULE_NAME}/${TREATMENTMACHINE_NAME} COMPONENT Runtime)
    endforeach()

endmacro()
