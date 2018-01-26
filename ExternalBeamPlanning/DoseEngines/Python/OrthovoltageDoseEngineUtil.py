import os
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
    fileNames = [f for f in os.listdir(dicomCtFilesFolder) if \
    os.path.isfile(os.path.join(dicomCtFilesFolder, f))]
    fileNames.reverse()

    outFile = open(os.path.join(outputFolder, slicenamesFilename), "wb")
    counter = 1
    numDicomFiles = len(fileNames)
    for sliceName in fileNames:
        outFile.write(os.path.join(dicomCtFilesFolder, sliceName))
        if counter != numDicomFiles:
            outFile.write("\n")
        counter += 1
    outFile.close()
    return

#-----------------------------------------------------------------------------
def generateCtcreateInputFile(slicenamesFilename, imageROI, voxelThickness, 
    outputFolder):
    outFile = open(os.path.join(outputFolder, "ctcreate.inp"), "wb")

    #CT Record 1 ctformat
    outFile.write("DICOM\n")

    #CT Record 2 CTFilename
    outFile.write(os.path.join(outputFolder, slicenamesFilename) + "\n")

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
    outFile.write("1976, 1.101, 2.088")

    outFile.close()
    return

#-----------------------------------------------------------------------------
def generateCtcreateInput(volumeNode, dicomCtFilesFolder, outputFolder, imageROIMm=None, 
    voxelThicknessMm=None):
    slicenamesFilename = "slicenames.txt"

    if imageROIMm is None and volumeNode is None:
        logging.error('No information provided for desired image ROI in ctcreate \
            phantom. Please provide a volume node, or imageROIMm parameter.')
        return False
    # if no ROI list provided, get ROI from volume node
    elif imageROIMm is None:
        imageROIMm = [0] * 6
        volumeNode.GetBounds(imageROIMm)

    if voxelThicknessMm is None and volumeNode is None:
        logging.error('No information provided for desired voxel thickness in ctcreate \
            phantom. Please provide a volume node, or volumeThicknessMm parameter.')
        return False
    # if no voxel thickness list provided, get voxel thickness from volume node
    elif voxelThicknessMm is None: 
        voxelThicknessMm = volumeNode.GetSpacing()

    # convert ROI and voxelThickness from mm to cm
    imageROICm = [dimension/10 for dimension in imageROIMm]
    voxelThicknessCm = [dimension/10 for dimension in voxelThicknessMm]

    generateSlicenamesTextfile(dicomCtFilesFolder, slicenamesFilename, 
        outputFolder)
    generateCtcreateInputFile(slicenamesFilename, imageROICm, voxelThicknessCm,
        outputFolder)
    return


#-----------------------------------------------------------------------------
def callCtcreate(outputFolder, ctcreateInputFilename="ctcreate.inp"):
    #if egsphant file exists, remove it
    outputCtcreatePhantomPath = os.path.join(outputFolder, "slicenames.txt.egsphant")
    if os.path.exists(outputCtcreatePhantomPath):
        logging.warning("Ctcreate phantom already exists in specifying directory. Overwriting it.")
        os.remove(outputCtcreatePhantomPath)

    # user must have ctcreate installed and in path
    os.system("ctcreate " + os.path.join(outputFolder, ctcreateInputFilename))
    return