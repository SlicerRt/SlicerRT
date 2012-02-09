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
    super(DICOMPlugin,self).__init__()
    self.loadType = "RT"

  def examine(self,fileLists):
    """ Returns a list of DICOMLoadable instances
    corresponding to ways of interpreting the 
    fileLists parameter.
    """
    loadables = []
    for files in fileLists:
      loadables += self.examineFiles(files)
    return loadables

  def examineFiles(self,files):
    """ Returns a list of DICOMLoadable instances
    corresponding to ways of interpreting the 
    files parameter.
    """

    # get the series description to use as base for volume name
    slicer.dicomDatabase.loadFileHeader(files[0])
    seriesDescription = "0008,103E"
    d = slicer.dicomDatabase.headerValue(seriesDescription)
    try:
      name = d[d.index('[')+1:d.index(']')]
    except ValueError:
      name = "Unknown"

    # modality tag
    MODALITY = "0008,0060"

    # list of accepted modalities
    RtModalityValues = [
      "RTSTRUCT",
      "RTPLAN",
      "RTDOSE"
    ]

    loadables = []

    # Look for series with RT modality in files
    for file in files:
      slicer.dicomDatabase.loadFileHeader(file)
      # get modality
      v = slicer.dicomDatabase.headerValue(MODALITY)
      try:
        modality = v[v.index('[')+1:v.index(']')]
      except ValueError:
        modality = ""
      if modality in RtModalityValues:
        loadable = DICOMLib.DICOMLoadable()
        loadable.files = files
        loadable.name = name + ' - ' + modality
        loadable.selected = True
        loadables.append(loadable)

    return loadables

  def load(self,loadable):
    """Load the selection as an RT object
    using the DicomRtImport module
    """

  success = False
  if slicer.modules.dicomrtimport.logic().LoadDicomRT(loadable.files[0],loadable.name):
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
    parent.contributor = "SparKit/SlicerRt team"
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
      slicer.modules.dicomPlugins
    except AttributeError:
      slicer.modules.dicomPlugins = {}
    slicer.modules.dicomPlugins['DicomRtImportPlugin'] = DicomRtImportPluginClass

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
