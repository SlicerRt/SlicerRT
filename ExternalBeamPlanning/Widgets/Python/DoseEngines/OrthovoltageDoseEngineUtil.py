import os
import vtk, qt, ctk, slicer
import logging
from DoseEngines import EGSnrcUtil

#------------------------------------------------------------------------------
#
# OrthovoltageDoseEngineUtil
#
# Utility functions for calling ctcreate from Slicer.
# Ctcreate converts an image file (ie DICOM CT) to an .egsphant file,
# which acts as input to DOSXYZnrc for dosimetry planning.
#
#------------------------------------------------------------------------------


#-----------------------------------------------------------------------------
def generateCtcreateInputFile(slicenamesFilename, volumeRoiArrayCm, voxelThickness, outputFolder, materials, lowerBound):
  """ Generate ctcreate.inp file, which is used to execute ctcreate
      Input records are described in the DOSXYZnrc Users Manual.
  """
  outFile = open(os.path.join(outputFolder, "ctcreate.inp"), "w")

  # CT Record 1 ctformat
  outFile.write("DICOM\n")

  # CT Record 2 CTFilename
  slicenamesFilePath = os.path.join(outputFolder, slicenamesFilename)
  outFile.write(slicenamesFilePath + "\n")

  # CT Record 3 lower and upper boundaries (cm) to be considered for the dosxyznrc phantom
  outFile.write(", ".join(map(str, volumeRoiArrayCm)) + "\n")

  # CT Record 4 x, y, z voxel dimensions/thicknesses (cm) to be used for the dosxyznrc phantom
  outFile.write(", ".join(map(str, voxelThickness)) + "\n")

  # CT Record 5 num_material, material_ct_lower_bound
  outFile.write("{numMaterials}, {lowerBound}\n".format(numMaterials=len(materials), lowerBound=lowerBound))

  """ Record 6 defines the material name, followed by the ramp parameters
      for each material. The ramp parameters are: material ct upper bound,
      material density lower bound, material density upper bound.

      The CT ramp is used to determine the medium and density in each voxel
      of the CT data. The material names must correspond to materials in the
      PEGS4 data file being used in the DOSXYZnrc simulation.

      The values used here are the default values specified for DOSXYZnrc.
      It is possible to create a custom CT ramp based on the imager and the
      data acquisition method, but since Slicer users will likely not have
      a custom CT ramp to accompany their input CT, material information
      is made non-configurable here and default values are used.

      For more information on materials and ramps, please see the DOSXYZnrc
      Users Manual.
  """
  # CT Record 6 information about material (for i=1 to num_material)
  for material in materials:
    outFile.write("{0}\n".format(material["Name"]))
    outFile.write("{0}\n".format(material["Ramp"]))
  outFile.close()

#-----------------------------------------------------------------------------
def generateCtcreateInput(volumeNode, ctDicomSeriesUID, outputFolder, materials, lowerBound, roiNode=None, voxelThicknessMm=None):
  """ Generate all files needed as input to ctcreate

      NOTE: need to supply outputFolder path with 2 slashes (ie "C:\\d\\outputFolder")
            otherwise os.path.join may misbehave
  """
  slicenamesFilename = "slicenames.txt"

  # Determine ROI
  volumeRoiArrayMm = [0] * 6
  if roiNode is None and volumeNode is None:
    logging.error('No information provided for desired image ROI in ctcreate \
      phantom. Please provide a volume node, or roiNode parameter.')
    return False
  # If no ROI list provided, get ROI from volume node
  elif roiNode is None:
    volumeNode.GetBounds(volumeRoiArrayMm)
  else:
    roiNode.GetBounds(volumeRoiArrayMm)
  # Convert ROI from RAS to LPS
  import copy
  volumeRoiArrayMmCopy = copy.deepcopy(volumeRoiArrayMm)
  volumeRoiArrayMm[0] = -volumeRoiArrayMmCopy[1]
  volumeRoiArrayMm[1] = -volumeRoiArrayMmCopy[0]
  volumeRoiArrayMm[2] = -volumeRoiArrayMmCopy[3]
  volumeRoiArrayMm[3] = -volumeRoiArrayMmCopy[2]

  # Determine voxel thickness
  if voxelThicknessMm is None and volumeNode is None:
    logging.error('No information provided for desired voxel thickness in ctcreate \
      phantom. Please provide a volume node, or volumeThicknessMm parameter.')
    return False
  # If no voxel thickness list provided, get voxel thickness from volume node
  elif voxelThicknessMm is None:
    voxelThicknessMm = volumeNode.GetSpacing()

  # Convert ROI and voxelThickness from mm to cm
  volumeRoiArrayCm = [dimension/10 for dimension in volumeRoiArrayMm]
  voxelThicknessCm = [dimension/10 for dimension in voxelThicknessMm]

  EGSnrcUtil.generateSlicenamesTextfile(ctDicomSeriesUID, slicenamesFilename, outputFolder)
  generateCtcreateInputFile(slicenamesFilename, volumeRoiArrayCm, voxelThicknessCm, outputFolder, materials, lowerBound)
  return True
