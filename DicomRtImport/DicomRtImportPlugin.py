import os
from __main__ import vtk, qt, ctk, slicer
from DICOMLib import DICOMPlugin
from DICOMLib import DICOMLoadable

#
# This is the plugin to handle translation of DICOM RT objects
# from DICOM files into MRML nodes.  It follows the DICOM module's
# plugin architecture.
#

class DicomRtImportPluginClass(DICOMPlugin):
  """ RT specific interpretation code
  """

  def __init__(self,epsilon=0.01):
    super(DicomRtImportPluginClass,self).__init__()
    self.loadType = "RT"

    self.tags = {}
    self.tags['Modality'] = "0008,0060"
    self.tags['RTPlanLabel'] = "300a,0002"
    self.tags['ReferencedSOPInstanceUID'] = "0008,1155"

  def examine(self,fileLists):
    """ Returns a list of DICOMLoadable instances
    corresponding to ways of interpreting the 
    fileLists parameter.
    """    
    import vtkSlicerDicomRtImportModuleLogic

    # Export file lists to DicomExamineInfo
    examineInfo = vtkSlicerDicomRtImportModuleLogic.vtkDICOMImportInfo()
    for files in fileLists:
      fileListIndex = examineInfo.InsertNextFileList() 
      fileList = examineInfo.GetFileList(fileListIndex) # vtk.vtkStringArray()  	  
      for f in files:
        fileList.InsertNextValue(f)	

    # Examine files
    slicer.modules.dicomrtimport.logic().Examine(examineInfo)	
    
    # Import loadables from DicomExamineInfo
    loadables = []
    for loadableIndex in xrange(examineInfo.GetNumberOfLoadables()):
      loadable = DICOMLib.DICOMLoadable()
      loadableFilesVtk = examineInfo.GetLoadableFiles(loadableIndex)
      loadableFilesPy = []
      for fileIndex in xrange(loadableFilesVtk.GetNumberOfValues()):
        loadableFilesPy.append(loadableFilesVtk.GetValue(fileIndex))
      loadable.files = loadableFilesPy

      name = examineInfo.GetLoadableName(loadableIndex)
      if "RTDOSE" in name:
        # if this is RTDose, then we need to find the RTPlan name for it
        # TODO: Move it to the reader class #407
        seriesType = slicer.dicomDatabase.fileValue(loadable.files[0], self.tags['Modality'])
        if seriesType == "RTDOSE":
          slicer.dicomDatabase.loadFileHeader(loadable.files[0])
          referencedSOPInstanceFullString = slicer.dicomDatabase.headerValue(self.tags['ReferencedSOPInstanceUID'])
          if len(referencedSOPInstanceFullString) > 0:
            referenceRTPlanInstanceUID = referencedSOPInstanceFullString.split("[")[1].split("]")[0]
            if len(referenceRTPlanInstanceUID) > 0:
              rtPlanFileName = slicer.dicomDatabase.fileForInstance(referenceRTPlanInstanceUID)
              if len(rtPlanFileName) > 0:
                name = name + ": " + slicer.dicomDatabase.fileValue(rtPlanFileName,self.tags['RTPlanLabel'])
        else:
          pass
      loadable.name = name
      loadable.tooltip = examineInfo.GetLoadableTooltip(loadableIndex)
      loadable.selected = examineInfo.GetLoadableSelected(loadableIndex)
      loadable.confidence = examineInfo.GetLoadableConfidence(loadableIndex)
      loadables.append(loadable)

    return loadables
  
  def load(self,loadable):
    """Load the selection as an RT object
    using the DicomRtImport module
    """
    success = False

    # Export file lists to DicomExamineInfo
    import vtkSlicerDicomRtImportModuleLogic
    loadInfo = vtkSlicerDicomRtImportModuleLogic.vtkDICOMImportInfo()
    fileListIndex = loadInfo.InsertNextFileList() 
    fileList = loadInfo.GetFileList(fileListIndex) # vtk.vtkStringArray()      
    for f in loadable.files:
      fileList.InsertNextValue(f) 
    loadInfo.InsertNextLoadable(fileList, loadable.name, loadable.tooltip, loadable.warning, loadable.selected, loadable.confidence)

    if slicer.modules.dicomrtimport.logic().LoadDicomRT(loadInfo):
      success = True

    return success

#
# DicomRtImportPlugin
#

class DicomRtImportPlugin:
  """
  This class is the 'hook' for slicer to detect and recognize the plugin
  as a loadable scripted module
  """
  def __init__(self, parent):
    parent.title = "DICOM RT Import Plugin"
    parent.categories = ["Developer Tools.DICOM Plugins"]
    parent.dependencies = []
    parent.contributors = "SparKit/SlicerRt team"
    parent.helpText = """
    Plugin to the DICOM Module to parse and load RT entities from DICOM files.
    No module interface here, only in the DICOM module
    """
    parent.acknowledgementText = """
    This DICOM Plugin was developed by 
    Csaba Pinter, PerkLab, Queen's University, Kingston, ON, CA
    and was funded by OCAIRO, CCO/CINO
    """

    # don't show this module - it only appears in the DICOM module
    parent.hidden = True

    # Add this extension to the DICOM module's list for discovery when the module
    # is created.  Since this module may be discovered before DICOM itself,
    # create the list if it doesn't already exist.
    try:
      # Check for dicomPlugins dictionary
      slicer.modules.dicomPlugins
      # Add PatientHierarchy handling function to scalar volume plugin
      slicer.modules.dicomPlugins['DICOMScalarVolumePlugin'].addSeriesInHierarchy = self.addSeriesInHierarchy
    except AttributeError:
      slicer.modules.dicomPlugins = {}

    slicer.modules.dicomPlugins['DicomRtImportPlugin'] = DicomRtImportPluginClass

  # TODO
  def addSeriesInHierarchy(self,loadable,volumeNode):
    tags = {}
    tags['seriesInstanceUID'] = "0020,000E"
    tags['seriesDescription'] = "0008,103E"
    tags['seriesModality'] = "0008,0060"
    tags['studyInstanceUID'] = "0020,000D"
    tags['studyDescription'] = "0008,1030"
    tags['studyDate'] = "0008,0020"
    tags['studyTime'] = "0008,0030"
    tags['patientID'] = "0010,0020"
    tags['patientName'] = "0010,0010"
    tags['patientSex'] = "0010,0040"
    tags['patientBirthDate'] = "0010,0030"

    from vtkSlicerPatientHierarchyModuleLogic import vtkSlicerPatientHierarchyModuleLogic
    try:
      vtkSlicerPatientHierarchyModuleLogic
    except AttributeError:
      import sys
      sys.stderr.write('Unable to create PatientHierarchy nodes: PatientHierarchy module not found!')
      return

    firstFile = loadable.files[0]

    seriesInstanceUid = slicer.dicomDatabase.fileValue(firstFile,tags['seriesInstanceUID'])
    seriesNode = vtkSlicerPatientHierarchyModuleLogic.GetPatientHierarchyNodeByUID(slicer.mrmlScene, seriesInstanceUid)
    if seriesNode == None:
      seriesNode = slicer.vtkMRMLHierarchyNode()
      slicer.mrmlScene.AddNode(seriesNode)
    elif seriesNode.GetAttribute('PatientHierarchy.SeriesModality') != None:
      import sys
      sys.stderr.write('Volume with the same UID has been already loaded!')
      return

    seriesNode.HideFromEditorsOff()
    seriesNode.SetAssociatedNodeID(volumeNode.GetID())
    seriesNode.SetAttribute('HierarchyType','PatientHierarchy')
    seriesNode.SetAttribute('DicomLevel','Series')
    seriesDescription = slicer.dicomDatabase.fileValue(firstFile,tags['seriesDescription'])
    if seriesDescription == '':
      seriesDescription = 'No description'
    seriesDescription = seriesDescription + '_PatientHierarchy'
    seriesNode.SetName(seriesDescription)
    seriesNode.SetAttribute('DicomUid',seriesInstanceUid)
    seriesNode.SetAttribute('PatientHierarchy.SeriesModality',slicer.dicomDatabase.fileValue(firstFile, tags['seriesModality']))
    seriesNode.SetAttribute('PatientHierarchy.StudyDate',slicer.dicomDatabase.fileValue(firstFile, tags['studyDate']))
    seriesNode.SetAttribute('PatientHierarchy.StudyTime',slicer.dicomDatabase.fileValue(firstFile, tags['studyTime']))
    seriesNode.SetAttribute('PatientHierarchy.PatientSex',slicer.dicomDatabase.fileValue(firstFile, tags['patientSex']))
    seriesNode.SetAttribute('PatientHierarchy.PatientBirthDate',slicer.dicomDatabase.fileValue(firstFile, tags['patientBirthDate']))

    patientId = slicer.dicomDatabase.fileValue(firstFile,tags['patientID'])
    patientNode = vtkSlicerPatientHierarchyModuleLogic.GetPatientHierarchyNodeByUID(slicer.mrmlScene, patientId)
    studyInstanceUid = slicer.dicomDatabase.fileValue(firstFile,tags['studyInstanceUID'])
    studyNode = vtkSlicerPatientHierarchyModuleLogic.GetPatientHierarchyNodeByUID(slicer.mrmlScene, studyInstanceUid)
    vtkSlicerPatientHierarchyModuleLogic.InsertDicomSeriesInHierarchy(slicer.mrmlScene, patientId, studyInstanceUid, seriesInstanceUid)

    if patientNode == None:
      patientNode = vtkSlicerPatientHierarchyModuleLogic.GetPatientHierarchyNodeByUID(slicer.mrmlScene, patientId)
      if patientNode != None:
        patientName = slicer.dicomDatabase.fileValue(firstFile,tags['patientName'])
        if patientName == '':
          patientName = 'No name'
        patientNode.SetName(patientName + '_PatientHierarchy')

    if studyNode == None:
      studyNode = vtkSlicerPatientHierarchyModuleLogic.GetPatientHierarchyNodeByUID(slicer.mrmlScene, studyInstanceUid)
      if studyNode != None:
        studyDescription = slicer.dicomDatabase.fileValue(firstFile,tags['studyDescription'])
        if studyDescription == '':
          studyDescription = 'No description'
        studyNode.SetName(studyDescription + '_PatientHierarchy')

#
# DicomRtImportWidget
#
class DicomRtImportWidget:
  def __init__(self, parent = None):
    self.parent = parent
    
  def setup(self):
    # don't display anything for this widget - it will be hidden anyway
    pass

  def enter(self):
    pass
    
  def exit(self):
    pass
