import os
import vtk, qt, ctk, slicer
import logging

#------------------------------------------------------------------------------
#
# EGSnrcUtil
#
# Common utility functions for using EGSnrc (DOSXYZnrc) based dose engines
# from SlicerRT.
#
#------------------------------------------------------------------------------

#-----------------------------------------------------------------------------
def generateSlicenamesTextfile(ctDicomSeriesUID, slicenamesFilename, outputFolder):
  """ Generate slicenames.txt file, with list of ct dicom slices, in increasing slice order (IS direction)
  """
  filePaths = slicer.dicomDatabase.filesForSeries(ctDicomSeriesUID)
  if len(filePaths) == 0:
    logging.error('Failed to find files in DICOM database for UID ' + str(ctDicomSeriesUID))
    return False

  from DICOMLib import DICOMUtils
  unsortedFileList = slicer.dicomDatabase.filesForSeries(ctDicomSeriesUID)
  sortedFileList, distances, warnings = DICOMUtils.getSortedImageFiles(unsortedFileList)

  outFile = open(os.path.join(outputFolder, slicenamesFilename), "w")
  counter = 1
  numDicomFiles = len(sortedFileList)
  for sliceFileName in sortedFileList:
    outFile.write(sliceFileName)
    if counter != numDicomFiles:
      outFile.write("\n")
    counter += 1
  outFile.close()
  return True

#-----------------------------------------------------------------------------
def callCtcreate(ctcreateExecFilePath, outputFolder, ctcreateInputFilename="ctcreate.inp"):
  """ Call ctcreate executable. Use this function after generating input for ctcreate
  """
  # If egsphant file exists, remove it
  outputCtcreatePhantomPath = os.path.join(outputFolder, "slicenames.txt.egsphant")
  if os.path.exists(outputCtcreatePhantomPath):
    logging.warning("Ctcreate phantom already exists in specifying directory. Overwriting it...")
    os.remove(outputCtcreatePhantomPath)

  # User must have ctcreate installed
  ctcreateInputFilePath = os.path.join(outputFolder, ctcreateInputFilename)
  logging.info("Run ctcreate: " + ctcreateExecFilePath + ' ' + ctcreateInputFilePath)

  import subprocess
  proc = subprocess.Popen([ctcreateExecFilePath, ctcreateInputFilePath], stdout=subprocess.PIPE, shell=True)
  (out, err) = proc.communicate()

  outStr = str(out).replace('\\r','').replace('\\n','\n')
  logging.debug("-----------------------------\n")
  logging.debug("ctcreate output: \n" + outStr)
  outStr = outStr[:outStr.rfind("'")] # Strip closing single quote
  if outStr[len(outStr)-1:] == '\n':
    outStr = outStr[:len(outStr)-1] # Strip last empty line if any
  logging.info("ctcreate output (last paragraph): \n" + outStr[outStr.rfind('\n\n')+2:])
  if err is not None and str(err) != '':
    logging.error("ctcreate error: \n" + str(err))

#-----------------------------------------------------------------------------
def dcm2dosxyz(AngleGantry, AngleCouch, AngleCollimator, Method='Zhan'):
  """
  Coordinate transformation from DICOM to DOSXYZnrc
  Input: gantry angle, couch angle and collimator angle from DICOM
  Output: theta, phi, phicol for DOSXYZnrc (using BEAMnrc phsp)
  
  The default method is what proposed in the article
  Zhan, L., Jiang, R., & Osei, E. K. (2012). Beam coordinate transformations
  from DICOM to DOSXYZnrc. Physics in Medicine & Biology, 57(24), N513.
  Also provided are the implementations based on Thebaut's and Bush's methods
  for readers' convenience. The authors assume no guarantee that these
  are the best implementations.
  """
  import numpy as np
  gamma = AngleGantry*np.pi/180.0
  col = AngleCollimator*np.pi/180.0
  rho = AngleCouch*np.pi/180
  # distort Couch and Gantry angles slightly to avoid
  # very special cases
  if AngleCouch in (90.0,270.0) and AngleGantry in (90.0,270.0):
    rho = rho * 0.999999
    gamma = gamma * 0.999999
  if Method=='Zhan': # this paper
    sgsr = np.sin(gamma)*np.sin(rho)
    sgcr = np.sin(gamma)*np.cos(rho)
    theta = np.arccos(-sgsr)
    phi = np.arctan2(-np.cos(gamma),sgcr)
    CouchAngle2CollPlane = np.arctan2(-np.sin(rho)*np.cos(gamma), np.cos(rho))
    phicol = (col-np.pi/2) + CouchAngle2CollPlane
    # coord. trans. for BEAMnrc generated phsp to DOSXYZnrc.
    phicol = np.pi - phicol
  elif Method=='Bush': # Bush, Australas. Phys. Eng. Sci.
    # (2010) 33:351
    rho = 2*np.pi-rho
    sgsr = np.sin(gamma)*np.sin(rho)
    sgcr = np.sin(gamma)*np.cos(rho)
    cgsr = np.cos(gamma)*np.sin(rho)
    theta = np.arctan2(np.sqrt(1.0-sgsr**2),sgsr)
    phi = np.arctan2(-np.cos(gamma),sgcr)
    col = col - np.pi/2
    phicol = np.arctan2( (-cgsr*np.cos(col)-np.cos(rho)*np.sin(col)),
    (cgsr*np.sin(col)-np.cos(rho)*np.cos(col)) )
    phicol = 2*np.pi-phicol
  elif Method=='Thebaut': # Thebaut, Phys. Med. Biol. (2006) 51:N441
    rho = 2*np.pi-rho
    sgsr = np.sin(gamma)*np.sin(rho)
    sgcr = np.sin(gamma)*np.cos(rho)
    cgcr = np.cos(gamma)*np.cos(rho)
    theta = np.arccos(sgsr)
    phi = np.arctan2(-np.cos(gamma),sgcr)
    col = col - np.pi/2
    cos_phicol = np.cos(col)*cgcr*np.sin(phi) - \
                 np.sin(col)*np.sin(rho)*np.sin(phi) - \
                 np.cos(col)*np.sin(gamma)*np.cos(phi)
    if cos_phicol>1.0: # for possible binary express problem.
      cos_phicol = 1.0
    elif cos_phicol<-1.0:
      cos_phicol = -1.0
    phicol = np.arccos(cos_phicol) # return value within [0,pi]
    direct = np.cos(col)*np.sin(rho)*np.sin(phi) + \
             np.sin(col)*(cgcr*np.sin(phi)- \
             np.sin(gamma)*np.cos(phi))
    if direct>0:
      phicol = -np.abs(phicol)
    else:
      phicol = np.abs(phicol)
  else:
    logging.error('Incorrect c.s. transformation method')
    return None

  return (theta*180/np.pi, np.mod(phi*180/np.pi,360), \
          np.mod(phicol*180/np.pi,360))