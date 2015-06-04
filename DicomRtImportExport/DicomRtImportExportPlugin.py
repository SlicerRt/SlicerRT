import os
from __main__ import vtk, qt, ctk, slicer
from DICOMLib import DICOMPlugin

#
# This is the plugin to handle translation of DICOM RT objects
# from DICOM files into MRML nodes.  It follows the DICOM module's
# plugin architecture.
#

class DicomRtImportExportPluginClass(DICOMPlugin):
  """ RT specific interpretation code
  """

  def __init__(self,epsilon=0.01):
    super(DicomRtImportExportPluginClass,self).__init__()
    self.loadType = "RT"

    self.tags = {}
    self.tags['Modality'] = "0008,0060"
    self.tags['RTPlanLabel'] = "300a,0002"
    self.tags['ReferencedSOPInstanceUID'] = "0008,1155"

  def examineForImport(self,fileLists):
    """ Returns a list of qSlicerDICOMLoadable
    instances corresponding to ways of interpreting the 
    fileLists parameter.
    """    
    import vtkSlicerDicomRtImportExportModuleLogic

    # Create loadables for each file list
    loadables = []
    for fileList in fileLists: # Each file list corresponds to one series, so do loadables
      # Convert file list to VTK object to be able to pass it for examining
      # (VTK class cannot have Qt object as argument, otherwise it is not python wrapped)
      vtkFileList = vtk.vtkStringArray()
      for file in fileList:
        vtkFileList.InsertNextValue(file)

      # Examine files
      loadablesCollection = vtk.vtkCollection()
      slicer.modules.dicomrtimportexport.logic().ExamineForLoad(vtkFileList, loadablesCollection)

      for loadableIndex in xrange(0,loadablesCollection.GetNumberOfItems()):
        vtkLoadable = loadablesCollection.GetItemAsObject(loadableIndex)
        # Create Qt loadable if confidence is greater than 0
        if vtkLoadable.GetConfidence() > 0:
          # Convert to Qt loadable to pass it back
          qtLoadable = slicer.qSlicerDICOMLoadable()
          qtLoadable.copyFromVtkLoadable(vtkLoadable)
          qtLoadable.tooltip = 'Valid RT object in selection'
          qtLoadable.selected = True
          loadables.append(qtLoadable)

    return loadables
  
  def load(self,loadable):
    """Load the selection as an RT object
    using the DicomRtImportExport module
    """
    if len(loadable.files) > 1:
      print('ERROR: RT objects must be contained by a single file!')
    vtkLoadable = slicer.vtkSlicerDICOMLoadable()
    loadable.copyToVtkLoadable(vtkLoadable)
    success = slicer.modules.dicomrtimportexport.logic().LoadDicomRT(vtkLoadable)
    return success

  def examineForExport(self,node):
    """Return a list of DICOMExportable instances that describe the
    available techniques that this plugin offers to convert MRML
    data into DICOM data
    """
    from vtkSlicerRtCommon import SlicerRtCommon
    from vtkSlicerSubjectHierarchyModuleMRMLPython import vtkMRMLSubjectHierarchyConstants
    exportable = None

    # RT dose volume
    if node.GetAssociatedNode() and SlicerRtCommon.IsDoseVolumeNode(node.GetAssociatedNode()):
      exportable = slicer.qSlicerDICOMExportable()
      exportable.confidence = 1.0
      # Define type-specific required tags and default values
      exportable.setTag('Modality', 'RTDOSE')
    # RT structure set
    elif node.GetAssociatedNode() and node.GetAssociatedNode().IsA('vtkMRMLSegmentationNode'):
      exportable = slicer.qSlicerDICOMExportable()
      exportable.confidence = 1.0
      # Define type-specific required tags and default values
      exportable.setTag('Modality', 'RTSTRUCT')
    # Potential anatomical image for an RT study
    elif node.GetAssociatedNode() and node.GetAssociatedNode().IsA('vtkMRMLScalarVolumeNode'):
      exportable = slicer.qSlicerDICOMExportable()
      exportable.confidence = 0.3 # Might be some other kind of scalar volume, but also anatomical volume in an RT study
      # Define type-specific required tags and default values
      exportable.setTag('Modality', 'CT')
      exportable.setTag('Manufacturer', 'Unknown manufacturer')
      exportable.setTag('Model', 'Unknown model')
      exportable.setTag('SeriesUID', 'XXXXXXX')

    # Node is exportable as RT series
    if exportable != None:
      # Set common properties for RT exportable
      exportable.name = self.loadType
      exportable.tooltip = "Create DICOM files from RT study"
      exportable.nodeID = node.GetID()
      exportable.pluginClass = self.__module__
      # Define common required tags and default values
      exportable.setTag('SeriesDescription', 'No series description')
      exportable.setTag('SeriesNumber', '1')
      return [exportable]

    # Not recognized as potential RT object
    return []

  def export(self,exportables):
    import vtkSlicerDicomRtImportExportModuleLogic
    from vtkSlicerDICOMLibModuleLogicPython import vtkSlicerDICOMExportable
    
    # Convert Qt loadables to VTK ones for the RT export logic
    exportablesCollection = vtk.vtkCollection()
    for exportable in exportables:
      vtkExportable = slicer.vtkSlicerDICOMExportable()
      exportable.copyToVtkExportable(vtkExportable)
      exportablesCollection.AddItem(vtkExportable)

    # Export RT study
    message = slicer.modules.dicomrtimportexport.logic().ExportDicomRTStudy(exportablesCollection)    
    return message

#
# DicomRtImportExportPlugin
#

class DicomRtImportExportPlugin:
  """
  This class is the 'hook' for slicer to detect and recognize the plugin
  as a loadable scripted module
  """
  def __init__(self, parent):
    parent.title = "DICOM RT Import-Export Plugin"
    parent.categories = ["Developer Tools.DICOM Plugins"]
    parent.dependencies = []
    parent.contributors = "Csaba Pinter (Queen's), Andras Lasso (Queen's), Kevin Wang (UHN, Toronto)"
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
    except AttributeError:
      slicer.modules.dicomPlugins = {}

    slicer.modules.dicomPlugins['DicomRtImportExportPlugin'] = DicomRtImportExportPluginClass

#
# DicomRtImportExportWidget
#
class DicomRtImportExportWidget:
  def __init__(self, parent = None):
    self.parent = parent
    
  def setup(self):
    # don't display anything for this widget - it will be hidden anyway
    pass

  def enter(self):
    pass
    
  def exit(self):
    pass
