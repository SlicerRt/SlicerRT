import os
import vtk, qt, ctk, slicer
from DICOMLib import DICOMPlugin
import logging

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

  def examineForImport(self,fileLists):
    """ Returns a list of qSlicerDICOMLoadable instances
    corresponding to ways of interpreting the
    fileLists parameter.
    """
    # Export file lists to DicomExamineInfo
    examineInfo = slicer.vtkDICOMImportInfo()
    for files in fileLists:
      fileListIndex = examineInfo.InsertNextFileList()
      fileList = examineInfo.GetFileList(fileListIndex) # vtk.vtkStringArray()
      for f in files:
        fileList.InsertNextValue(f)

    # Examine files
    logic = slicer.vtkSlicerDicomSroImportModuleLogic()
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
    loadInfo = slicer.vtkDICOMImportInfo()
    fileListIndex = loadInfo.InsertNextFileList()
    fileList = loadInfo.GetFileList(fileListIndex) # vtk.vtkStringArray()
    for f in loadable.files:
      fileList.InsertNextValue(f)
    loadInfo.InsertNextLoadable(fileList, loadable.name, loadable.tooltip, loadable.warning, loadable.selected, loadable.confidence)

    logic = slicer.vtkSlicerDicomSroImportModuleLogic()
    logic.SetMRMLScene(slicer.mrmlScene)
    if logic.LoadDicomSro(loadInfo):
      success = True

    return success

  def examineForExport(self,subjectHierarchyItemID):
    """Return a list of DICOMExportable instances that describe the
    available techniques that this plugin offers to convert MRML
    data into DICOM data
    """
    shn = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    transformNode = shn.GetItemDataNode(subjectHierarchyItemID)
    if transformNode is None or not transformNode.IsA('vtkMRMLTransformNode'):
      return []

    #TODO: Remove when deformable transforms are supported
    if not transformNode.IsLinear():
      logging.warning('Non-linear transform is detected in node ' + transformNode.GetName() \
        + '. Non-linear transform export is not yet supported through the plugin. Please use the DICOM Registration Export module instead.')
      return []

    # Get moving and fixed volumes involved in the registration
    movingVolumeNode = transformNode.GetNodeReference(slicer.vtkMRMLTransformNode.GetMovingNodeReferenceRole())
    fixedVolumeNode = transformNode.GetNodeReference(slicer.vtkMRMLTransformNode.GetFixedNodeReferenceRole())
    if movingVolumeNode is None or fixedVolumeNode is None:
      logging.error('Failed to find moving and/or fixed image for transform ' + transformNode.GetName() \
        + '. These references are needed in order to export the transform into DICOM SRO. Please make sure the transform is created by a registration module.')
      return []

    exportable = slicer.qSlicerDICOMExportable()
    exportable.confidence = 1.0
    # Define type-specific required tags and default values
    exportable.setTag('Modality', 'REG')
    exportable.name = self.loadType
    exportable.tooltip = "Create DICOM file from registration result"
    exportable.subjectHierarchyItemID = subjectHierarchyItemID
    exportable.pluginClass = self.__module__
    # Define common required tags and default values
    exportable.setTag('SeriesDescription', 'No series description')
    exportable.setTag('SeriesNumber', '1')
    return [exportable]

  def export(self,exportables):
    errorMessage = ''
    for exportable in exportables:
      # Get transform node
      shn = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
      transformNode = shn.GetItemDataNode(exportable.subjectHierarchyItemID)
      if transformNode is None or not transformNode.IsA('vtkMRMLTransformNode'):
        return 'Invalid transform node in exportable (ItemID:' + str(exportable.subjectHierarchyItemID)

      # Get moving and fixed volumes involved in the registration
      movingVolumeNode = transformNode.GetNodeReference(slicer.vtkMRMLTransformNode.GetMovingNodeReferenceRole())
      fixedVolumeNode = transformNode.GetNodeReference(slicer.vtkMRMLTransformNode.GetFixedNodeReferenceRole())
      if movingVolumeNode is None or fixedVolumeNode is None:
        currentError = 'Failed to find moving and/or fixed image for transform ' + transformNode.GetName()
        logging.error(currentError + '. These references are needed in order to export the transform into DICOM SRO. Please make sure the transform is created by a registration module.')
        errorMessage += currentError + '\n'
        continue

      import sys
      loadablePath = os.path.join(slicer.modules.plastimatch_slicer_bspline.path,'..'+os.sep+'..'+os.sep+'qt-loadable-modules')
      if loadablePath not in sys.path:
        sys.path.append(loadablePath)
      sro = slicer.vtkPlmpyDicomSroExport()
      sro.SetMRMLScene(slicer.mrmlScene)
      sro.SetFixedImageID(fixedVolumeNode.GetID())
      sro.SetMovingImageID(movingVolumeNode.GetID())
      sro.SetXformID(transformNode.GetID())
      sro.SetOutputDirectory(exportable.directory)
      success = sro.DoExport()
      if success != 0:
        currentError = 'Failed to export transform node to DICOM SRO: ' + transformNode.GetName()
        logging.error(currentError)
        errorMessage += currentError + '\n'
        continue

    return errorMessage

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
    parent.contributors = ["Kevin Wang (RMP, PMH), Csaba Pinter (Queen's)"]
    parent.helpText = """
    Plugin to the DICOM Module to parse and load spatial registration
    from DICOM files.
    No module interface here, only in the DICOM module
    """
    parent.acknowledgementText = """
    This DICOM Plugin was developed by
    Kevin Wang, RMP, PMH and
    Csaba Pinter, PerkLab, Queen's University, Kingston, ON, CA
    and was funded by OCAIRO, CCO's ACCU program, and CANARIE
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
