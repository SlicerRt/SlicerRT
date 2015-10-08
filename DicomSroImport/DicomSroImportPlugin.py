import os
import vtk, qt, ctk, slicer
from DICOMLib import DICOMPlugin

#
# This is the plugin to handle translation of spatial registration object
# from DICOM files into MRML nodes.  It follows the DICOM module's
# plugin architecture.
#

class DicomSroImportPluginClass(DICOMPlugin):
  """ Spatial registration specific interpretation code
  """

  def __init__(self,epsilon=0.01):
    super(DicomSroImportPluginClass,self).__init__()
    self.loadType = "REG"

    self.tags = {}
    self.tags['Modality'] = "0008,0060"
    self.tags['ReferencedSOPInstanceUID'] = "0008,1155"

  def examine(self,fileLists):
    """ Returns a list of qSlicerDICOMLoadable instances
    corresponding to ways of interpreting the 
    fileLists parameter.
    """    
    import vtkSlicerDicomSroImportModuleLogic

    # Export file lists to DicomExamineInfo
    examineInfo = vtkSlicerDicomSroImportModuleLogic.vtkDICOMImportInfo()
    for files in fileLists:
      fileListIndex = examineInfo.InsertNextFileList() 
      fileList = examineInfo.GetFileList(fileListIndex) # vtk.vtkStringArray()  	  
      for f in files:
        fileList.InsertNextValue(f)	

    # Examine files
    logic = vtkSlicerDicomSroImportModuleLogic.vtkSlicerDicomSroImportModuleLogic()
    #logic = slicer.modules.DicomSroimport.logic()
    print "reg inside examine"
    logic.Examine(examineInfo)	
    
    # Import loadables from DicomExamineInfo
    loadables = []
    for loadableIndex in xrange(examineInfo.GetNumberOfLoadables()):
      loadable = slicer.qSlicerDICOMLoadable()
      loadableFilesVtk = examineInfo.GetLoadableFiles(loadableIndex)
      loadableFilesPy = []
      for fileIndex in xrange(loadableFilesVtk.GetNumberOfValues()):
        loadableFilesPy.append(loadableFilesVtk.GetValue(fileIndex))
      loadable.files = loadableFilesPy

      name = examineInfo.GetLoadableName(loadableIndex)
      loadable.name = name
      loadable.tooltip = examineInfo.GetLoadableTooltip(loadableIndex)
      loadable.selected = examineInfo.GetLoadableSelected(loadableIndex)
      loadable.confidence = examineInfo.GetLoadableConfidence(loadableIndex)
      loadables.append(loadable)

    return loadables
  
  def load(self,loadable):
    """Load the selection as an RT object
    using the DicomSroImport module
    """
    success = False

    # Export file lists to DicomExamineInfo
    import vtkSlicerDicomSroImportModuleLogic
    loadInfo = vtkSlicerDicomSroImportModuleLogic.vtkDICOMImportInfo()
    fileListIndex = loadInfo.InsertNextFileList() 
    fileList = loadInfo.GetFileList(fileListIndex) # vtk.vtkStringArray()      
    for f in loadable.files:
      fileList.InsertNextValue(f) 
    loadInfo.InsertNextLoadable(fileList, loadable.name, loadable.tooltip, loadable.warning, loadable.selected, loadable.confidence)

    logic = vtkSlicerDicomSroImportModuleLogic.vtkSlicerDicomSroImportModuleLogic()
    #logic = slicer.modules.DicomSroimport.logic()
    logic.SetMRMLScene(slicer.mrmlScene)
    if logic.LoadDicomSro(loadInfo):
      success = True

    return success

#
# DicomSroImportPlugin
#

class DicomSroImportPlugin:
  """
  This class is the 'hook' for slicer to detect and recognize the plugin
  as a loadable scripted module
  """
  def __init__(self, parent):
    parent.title = "DICOM Spatial Registration Object Plugin"
    parent.categories = ["Developer Tools.DICOM Plugins"]
    parent.contributors = ["Kevin Wang (RMP, PMH)"]
    parent.helpText = """
    Plugin to the DICOM Module to parse and load spatial registration
    from DICOM files.
    No module interface here, only in the DICOM module
    """
    parent.acknowledgementText = """
    This DICOM Plugin was developed by 
    Kevin Wang, RMP, PMH.
    and was funded by OCAIRO and CCO's ACCU program.
    """

    # don't show this module - it only appears in the DICOM module
    parent.hidden = True

    # Add this extension to the DICOM module's list for discovery when the module
    # is created.  Since this module may be discovered before DICOM itself,
    # create the list if it doesn't already exist.
    try:
      slicer.modules.dicomPlugins
    except AttributeError:
      slicer.modules.dicomPlugins = {}
    slicer.modules.dicomPlugins['DicomSroImportPlugin'] = DicomSroImportPluginClass

#
# DicomSroImportWidget
#

class DicomSroImportWidget:
  def __init__(self, parent = None):
    self.parent = parent
    
  def setup(self):
    # don't display anything for this widget - it will be hidden anyway
    pass

  def enter(self):
    pass
    
  def exit(self):
    pass

