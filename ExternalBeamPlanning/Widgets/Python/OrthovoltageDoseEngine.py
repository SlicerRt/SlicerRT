import os, shutil
import vtk, qt, ctk, slicer
import logging
from DoseEngines import *

class OrthovoltageDoseEngine(AbstractScriptedDoseEngine):
  """ Orthovoltage python dose engine
  """

  def __init__(self, scriptedEngine):
    scriptedEngine.name = 'Orthovoltage python'
    AbstractScriptedDoseEngine.__init__(self, scriptedEngine)

  def defineBeamParameters(self):
    ##########################################
    # Generate ctcrate phantom parameters tab
    ##########################################

    self.scriptedEngine.addBeamParameterLineEdit(
    "Generate ctcreate phantom", "CtcreateOutputPath", "Ctcreate output filepath:", 
    "Enter filepath to store ctcreate phantom and related files", "C:\\\\d\\\\tmp") 

    self.scriptedEngine.addBeamParameterLineEdit(
    "Generate ctcreate phantom", "ROIName", "ROI node name for cropping image (optional):", 
    "Enter name of ROI, if you wish to crop CT image which will be converted to ctcreate phantom format. \
    If no ROI entered, will use image bounds from original CT image volume", "") 

    self.scriptedEngine.addBeamParameterLineEdit(
    "Generate ctcreate phantom", "SliceThicknessX", "Slice thickness (mm) in X direction (optional):", 
    "Enter desired slice thickness in X direction for output ctcreate phantom. If not x,y,z slice \
    thickness not provided, will use same thickness from original CT image volume", "") 

    self.scriptedEngine.addBeamParameterLineEdit(
    "Generate ctcreate phantom", "SliceThicknessY", "Slice thickness (mm) in Y direction (optional):", 
    "Enter desired slice thickness in Y direction for output ctcreate phantom. If not x,y,z slice \
    thickness not provided, will use same thickness from original CT image volume", "") 

    self.scriptedEngine.addBeamParameterLineEdit(
    "Generate ctcreate phantom", "SliceThicknessZ", "Slice thickness (mm) in Z direction (optional):", 
    "Enter desired slice thickness in Z direction for output ctcreate phantom. If not x,y,z slice \
    thickness not provided, will use same thickness from original CT image volume", "") 

    ##########################################
    # Orthovoltage dose parameters tab
    ##########################################

    self.scriptedEngine.addBeamParameterLineEdit(
    "Orthovoltage dose", "SimulationTitle", "Simulation title:", 
    "Enter a title for the DOSXYZNrc simulation", "DOSXYZnrc simulation")

    self.scriptedEngine.addBeamParameterLineEdit(
    "Orthovoltage dose", "PhaseSpaceFilePath", "Phase space filepath:", 
    "Enter full path to phase space file", "C:/d/6MV 8x6/DOSXYZnrc_files/egsphsp/6MV_8x6asym_sum.egsphsp1")

    self.scriptedEngine.addBeamParameterLineEdit(
    "Orthovoltage dose", "ECut", "Global electron cutoff energy - ECUT (MeV):", 
    "Select global electron cutoff energy (MeV)", "0.521")

    self.scriptedEngine.addBeamParameterLineEdit(
    "Orthovoltage dose", "PCut", "Global photon cutoff energy - PCUT (MeV):", 
    "Select global electron cutoff energy (MeV)", "0.01")

    self.scriptedEngine.addBeamParameterLineEdit(
    "Orthovoltage dose", "IncidentBeamSize", "Incident beam size (cm):", 
    "SBEAM SIZE is the side of a square field in cm. The default value for \
    BEAM SIZE is 100 cm. When phase-space particles are read from a data file \
    or reconstructed by a multiple-source model, DOSXYZnrcwill check their \
    positions and discard those that fall outside of the specified field. \
    Use with caution.", "100.0")

    self.scriptedEngine.addBeamParameterLineEdit(
    "Orthovoltage dose", "DistanceSourceToIsocenter", "Distance from source to isocenter (cm):", 
    "Enter absolute distance from source to isocenter (cm):", "")

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "NumHistories", "Number of histories:",
      "Number of histories to use for simulation", "129796480")

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "Inseed1", "Random number generator seed 1:",
      "The random number generator used requires 2 seeds between 1 and \
      31328 and 1 and 30081 respectively.  For each different pair of \
      seeds, an independent random number sequence is generated.  Once \
      the seeds are used to establish the state of the generator, the \
      \"seeds\" output in log files etc. are really just pointers between 1 and 98.",
      "33")

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "Inseed2", "Random number generator seed 2:",
      "The random number generator used requires 2 seeds between 1 and \
      31328 and 1 and 30081 respectively.  For each different pair of \
      seeds, an independent random number sequence is generated.  Once \
      the seeds are used to establish the state of the generator, the \
      \"seeds\" output in log files etc. are really just pointers between 1 and 98.",
      "97")

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "DosxyznrcPath", "Path to dosxyznrc executable:",
      "Enter filepath to dosxyznrc executable", "C:\\\\EGSnrc\\\\egs_home\\\\dosxyznrc")

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "PegsFilepath", "Path to pegs4 data file:",
      "Enter filepath to .pegs5dat file", "C:\\\\EGSnrc\\\\HEN_HOUSE\\\\pegs4\\\\data\\\\521icru.pegs4dat")

    ####################

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "CollimatorAngle", "Collimator angle (degrees):",
      "Angle by which the collimator is rotates in the collimator plane perpendicular to beam direction.", "270")

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "ThetaAngle", "Incident theta angle (degrees):",
      "Angle between the +z direction and a line joining the center of the beam where it strikes \
      the phantom surface to the isocenter.", "90")

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "PhiAngle", "Incident phi angle (degrees):",
      "Angle between the +x direction and the projection on the x-y plane of the line joining the \
      center of the beam on the phantom surface to the isocenter on the xy plane.", "270")

  #todo: verify that all entered parameters are valid
  def calculateDoseUsingEngine(self, beamNode, resultDoseVolumeNode):
    import qSlicerExternalBeamPlanningDoseEnginesPythonQt as engines
    orthovoltageEngine = engines.qSlicerMockDoseEngine()

    parentPlan = beamNode.GetParentPlanNode()
    volumeNode = parentPlan.GetReferenceVolumeNode()

    subjectHierarchyNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    subjectID = subjectHierarchyNode.GetItemByDataNode(volumeNode)
    seriesUID = subjectHierarchyNode.GetItemUID(subjectID, 'DICOM')

    isocenter = [0]*3
    parentPlan.GetIsocenterPosition(isocenter)
    isocenter = [x/10.0 for x in isocenter]     #convert from mm to cm
    print(isocenter)

    ##########################################
    # Get ctcreate parameters
    ##########################################

    ctcreateOutputPath = self.scriptedEngine.parameter(beamNode, "CtcreateOutputPath")
    ROIName = self.scriptedEngine.parameter(beamNode, "ROIName")
    volumeName = self.scriptedEngine.parameter(beamNode, "VolumeName")
    sliceThicknessX = self.scriptedEngine.parameter(beamNode, "SliceThicknessX")
    sliceThicknessY = self.scriptedEngine.parameter(beamNode, "SliceThicknessY")
    sliceThicknessZ = self.scriptedEngine.parameter(beamNode, "SliceThicknessZ")

    if ROIName == "":
      ROI = None
    else:
      try:
        ROI = slicer.util.getNode(ROIName)
      except:
        print("Unable to get specified ROI, check for typo")  #todo: this should be output to the Warnings log
        ROI = None

    if sliceThicknessX == "" or sliceThicknessY == "" or sliceThicknessZ == "":
      thicknesses = None
    else:
      thicknesses = [float(sliceThicknessX), float(sliceThicknessY), float(sliceThicknessZ)]

    ##########################################
    # Call ctcreate
    ##########################################

    generateCtcreateInput(volumeNode,seriesUID, ctcreateOutputPath, ROI, thicknesses)
    callCtcreate(ctcreateOutputPath)

    ##########################################
    # Get DOSXYZnrc parameters
    ##########################################
    title = self.scriptedEngine.parameter(beamNode, "SimulationTitle")
    phaseSpaceFilePath = self.scriptedEngine.parameter(beamNode, "PhaseSpaceFilePath")
    ecut = self.scriptedEngine.parameter(beamNode, "ECut")
    pcut = self.scriptedEngine.parameter(beamNode, "PCut")
    incidentBeamSize = self.scriptedEngine.parameter(beamNode,"IncidentBeamSize")
    dosxyznrcPath = self.scriptedEngine.parameter(beamNode, "DosxyznrcPath")
    pegsFilepath = self.scriptedEngine.parameter(beamNode, "PegsFilepath")
    nmed = 0 #nmed is number of media
    smax = 0 #dummy input, used to be max step length
    zeroairdose = 0
    doseprint = 0
    MAX20 = 0

    #Record SC1-8
    iqin = 2              #charge of incident beam
    isource = 8
    xiso = isocenter[0]   # isocenter x
    yiso = isocenter[1]   # isocenter y
    ziso = isocenter[2]   # isocenter z

    nang = 1 #number of indident theta-phi pairs
    dsource = self.scriptedEngine.parameter(beamNode, "DistanceSourceToIsocenter") #absolute distance from isocenter to source center
    phicol = self.scriptedEngine.parameter(beamNode,"CollimatorAngle")            #todo: how to calculate this??
    i_dbs = 0
    r_dbs = 0
    ssd_dbs = 0
    z_dbs = 0
    e_split = 0

    #Record SC1-8a          todo: how to calculate theta and phi angles based on gantry, collimator and couch angles?
    nang1_theta = self.scriptedEngine.parameter(beamNode,"ThetaAngle")   # incident theta angle 
    nang1_phi = self.scriptedEngine.parameter(beamNode,"PhiAngle")     # incident phi angle
    nang1_pang = 1              # probability of a particle being incident at theta(i)-phi(i) 
                                # (probabilities are automatically normalized to 1).

    #Record SC2
    enflag = 2        # for ph-sp beam input or full BEAM sim.
    mode = 0          # default file format for ph-sp data (enflag=2)
    medsur = 1        # medium number for the region outside the phantom
    dsurround_1 = 30  # thickness (cm) of region surrounding phantom in x direction
    dflag = 1         # dsurround(1) applied to x direction only
    dsurround_2 = 60  # thickness (cm) of region surrounding phantom in y direction
    dsurround_3 = 30  # thickness (cm) of region surrounding phantom in +z direction
    dsurround_4 = 30  # thickness (cm) of region surrounding phantom in -z direction

    #Record 13
    ncase = self.scriptedEngine.parameter(beamNode, "NumHistories")
    iwatch = 0        # no tracking output
    timmax = 99       # max CPU time in hours allowed for a simulation, not activated in the current version of DOSXYZnrc
    inseed1 = self.scriptedEngine.parameter(beamNode, "Inseed1")      # starting random seed number
    inseed2 = self.scriptedEngine.parameter(beamNode, "Inseed2")      # starting random seed number
    beam_size = incidentBeamSize
    ismooth = 0       # re-use the ph-sp data once run out (no redistribution)
    irestart = 0      # first run for this data set (default)
    idat = 2          # output the data file for restart at end only
    ireject = 0       # do not perform charged particle range rejection (default)
    esave_global = "" # energy (MeV) below which charged particle will be considered for range rejection
    nrcycl = 0        # use entire phase space file with no restarts
    iparallel = 0     # only relevalnt when manually creating/submitting paralel jobs or using unix pproccess script
    parnum = 0        # only relevalnt when manually creating/submitting paralel jobs or using unix pproccess script
    n_split = 1       
    ihowfarless = 0
    i_phsp_out = 0    # no phase space output (default)

    #EGSnrc inputs
    globalEcut = ecut
    globalPcut = pcut
    globalSmax = 5    # 
    estepe = 0.25     # max fractional energy loss per step (default)
    ximax = 0.5       # max first elastic scattering moment per step (default)

    DOSXYZNRC_INPUT_FILENAME = "dosxyznrcInput.egsinp"

    with open(os.path.join(ctcreateOutputPath, DOSXYZNRC_INPUT_FILENAME), "wb") as outFile:
      outFile.write(title + "\n")
      outFile.write(str(nmed) + "\n")
      outFile.write(os.path.join(ctcreateOutputPath, "slicenames.txt.egsphant") + "\n") # todo: remove hardcode
      outFile.write("{}, {}, {}\n".format(ecut, pcut, smax))
      outFile.write("{}, {}, {},\n".format(zeroairdose, doseprint, MAX20))
      outFile.write("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}\n".format(iqin, isource, 
        xiso, yiso, ziso, nang, dsource, phicol, i_dbs, r_dbs, ssd_dbs, z_dbs, e_split))
      outFile.write("{}, {}, {}\n".format(nang1_theta, nang1_phi, nang1_pang))
      outFile.write("{}, {}, {}, {}, {}, {}, {}, {}\n".format(enflag, mode, medsur, dsurround_1,
        dflag, dsurround_2, dsurround_3, dsurround_4))
      outFile.write(phaseSpaceFilePath + "\n")
      outFile.write("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}\n".format(
        ncase, iwatch, timmax, inseed1, inseed2, beam_size, ismooth, irestart, idat, ireject,
        esave_global, nrcycl, iparallel, parnum, n_split, ihowfarless, i_phsp_out))
      outFile.write(" #########################\n :Start MC Transport Parameter:\n\n")
      outFile.write(" Global ECUT= {}\n".format(globalEcut))
      outFile.write(" Global PCUT= {}\n".format(globalPcut))
      outFile.write(" Global SMAX= {}\n".format(globalSmax))
      outFile.write(" ESTEPE= {}\n".format(estepe))
      outFile.write(" XIMAX= {}\n".format(ximax))
      outFile.write(" Boundary crossing algorithm= PRESTA-I\n")   #todo: add comments
      outFile.write(" Skin depth for BCA= 0\n")
      outFile.write(" Electron-step algorithm= PRESTA-II\n")
      outFile.write(" Spin effects= On\n")
      outFile.write(" Brems angular sampling= Simple\n")
      outFile.write(" Brems cross sections= BH\n")
      outFile.write(" Bound Compton scattering= Off\n")
      outFile.write(" Compton cross sections= default\n")
      outFile.write(" Pair angular sampling= Simple\n")
      outFile.write(" Pair cross sections= BH\n")
      outFile.write(" Photoelectron angular sampling= Off\n")
      outFile.write(" Rayleigh scattering= Off\n")
      outFile.write(" Atomic relaxations= Off\n")
      outFile.write(" Electron impact ionization= Off\n")
      outFile.write(" Photon cross sections= xcom\n")
      outFile.write(" Photon cross-sections output= Off\n")
      outFile.write("\n :Stop MC Transport Parameter:\n #########################\n\n")

    ##########################################
    # Call DOSXYZnrc
    ##########################################

    # copy DOSXYZnrc input file to dosxyznrc directory
    shutil.copy2(os.path.join(ctcreateOutputPath, DOSXYZNRC_INPUT_FILENAME), dosxyznrcPath)

    os.system("dosxyznrc -i {} -p {}".format(DOSXYZNRC_INPUT_FILENAME, pegsFilepath)) 


    #Call C++ orthovoltage engine to calculate orthovoltage dose
    return orthovoltageEngine.calculateDoseUsingEngine(beamNode, resultDoseVolumeNode)
