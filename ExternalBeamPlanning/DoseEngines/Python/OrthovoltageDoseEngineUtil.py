import sys
from os import listdir
from os.path import isfile, join
import vtk, qt, ctk, slicer
import logging

#########################################################
#
#

comment = """

  OrthovoltageDoseEngineUtil contains utility functions
  for calling ctcreate from Slicer. Ctcreate converts an
  image file (ie DICOM CT) to an .egsphant file, which
  acts as input to DOSXYZnrc for dosimetry planning.

"""
#
#########################################################

# Note: need to supply folderpaths with 2 slashes (ie "C:\\d\\outputFolder") otherwise os.path.join may misbehave

#TODO: ensure that slicenames in textfile are in order of increasing z
#-----------------------------------------------------------------------------
def generateSlicenamesTextfile(dicomCtFilesFolder, slicenamesFilename, 
    outputFolder):
    fileNames = [f for f in listdir(dicomCtFilesFolder) if \
    isfile(join(dicomCtFilesFolder, f))]
    fileNames.reverse()

    outFile = open(join(outputFolder, slicenamesFilename), "wb")
    counter = 1
    numDicomFiles = len(fileNames)
    for sliceName in fileNames:
        outFile.write(join(dicomCtFilesFolder, sliceName))
        if counter != numDicomFiles:
            outFile.write("\n")
        counter += 1
    outFile.close()
    return

#-----------------------------------------------------------------------------
def generateCtcreateInputFile(slicenamesFilename, imageROI, voxelThickness, 
    outputFolder):
    outFile = open(join(outputFolder, "ctcreate.inp"), "wb")

    #CT Record 1 ctformat
    outFile.write("DICOM\n")

    #CT Record 2 CTFilename
    outFile.write(join(outputFolder, slicenamesFilename) + "\n")

    #CT Record 3 lower and upper boundaries (cm) to be considered for 
    #the dosxyznrc phantom
    outFile.write(", ".join(map(str, imageROI)) + "\n") 

    #CT Record 4 x, y, z voxel dimensions/thicknesses (cm) to be used 
    #for the dosxyznrc phantom         
    outFile.write(", ".join(map(str, voxelThickness)) + "\n")

    # CT Record 5 num_material, material_ct_lower_bound
    outFile.write("4, -1024\n")

    # CT Record 6 information about material (for i=1 to num_material)
    outFile.write("AIR521ICRU\n")
    outFile.write("-974, 0.001, 0.044\n")
    outFile.write("LUNG521ICRU\n")
    outFile.write("-724, 0.044, 0.302\n")
    outFile.write("ICRUTISSUE521ICRU\n")
    outFile.write("101, 0.302, 1.101\n")
    outFile.write("ICRPBONE521ICRU\n")
    outFile.write("1976, 1.101, 2.088\n")

    outFile.close()
    return

#-----------------------------------------------------------------------------
def generateCtcreateInput(volumeNode, dicomCtFilesFolder, imageROIMm, 
    voxelThicknessMm, outputFolder):
    slicenamesFilename = "slicenames.txt"

    # In slicer, get imageROIMm by either:
    #    v = getNode("volume") #vtkMRMLScalarVolumeNode
    #    imageROIMm = [0] * 6
    #    v.GetBounds(imageROIMm)
    # OR
    #    r = getNode("R") #vtkMRMLAnnotationROINode
    #    imageROIMm = [0] * 6 
    #    v.GetBounds(imageROIMm)

    # In slicer, get voxelThickness by:
    #    v = getNode("volume") #vtkMRMLScalarVolumeNode
    #    voxelThickness = v.GetSpacing()


    #TODO: check if imageROIMm and voxelSpacing are proper input within 
    #file dimensions: otherwise set to dimensions of volume

    # convert ROI and voxelThickness from mm to cm
    imageROICm = [dimension/10 for dimension in imageROIMm]
    voxelThicknessCm = [dimension/10 for dimension in voxelThicknessMm]


    generateSlicenamesTextfile(dicomCtFilesFolder, slicenamesFilename, 
        outputFolder)
    generateCtcreateInputFile(slicenamesFilename, imageROICm, voxelThicknessCm,
        outputFolder)
    return


#-----------------------------------------------------------------------------
def callCtcreate():
    #TODO implement this function
    print ("Call ctcreate with input data. Function not implemented yet.")
    pass