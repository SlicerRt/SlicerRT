cmake_minimum_required(VERSION 2.8.7)

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  set(EXTENSION_NAME "SlicerRT")
  set(EXTENSION_HOMEPAGE "https://www.assembla.com/spaces/slicerrt")
  set(EXTENSION_CATEGORY "Radiotherapy")
  set(EXTENSION_ICONURL "http://wiki.slicer.org/slicerWiki/images/f/f2/SlicerRtExtensionLogo.png")
  set(EXTENSION_STATUS "Beta")
  set(EXTENSION_CONTRIBUTORS "Csaba Pinter (PerkLab, Queen's University), Andras Lasso (PerkLab, Queen's University), Kevin Wang (Radiation Medicine Program, Princess Margaret Hospital, University Health Network Toronto)")
  set(EXTENSION_DESCRIPTION "Extensions for radiotherapy research (DICOM-RT import, dose volume histogram, dose accumulation, etc.")
  set(EXTENSION_SCREENSHOTURLS "http://wiki.slicer.org/slicerWiki/images/thumb/f/f1/SlicerRT-0.2_screenshot.png/800px-SlicerRT-0.2_screenshot.png")
  set(EXTENSION_DEPENDS "NA") # Specified as a space separated list or 'NA' if any
endif()

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  find_package(Slicer REQUIRED)
  include(${Slicer_USE_FILE})
endif()

#-----------------------------------------------------------------------------
OPTION(SLICERRT_ENABLE_EXPERIMENTAL_MODULES "Enable the building of work-in-progress, experimental modules." OFF)

#-----------------------------------------------------------------------------
add_subdirectory(DicomRtImport)
add_subdirectory(DoseAccumulation)
add_subdirectory(DoseVolumeHistogram)
add_subdirectory(Isodose)

if (SLICERRT_ENABLE_EXPERIMENTAL_MODULES)
  add_subdirectory(DoseComparison)
endif()

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  include(${Slicer_EXTENSION_CPACK})
endif()
