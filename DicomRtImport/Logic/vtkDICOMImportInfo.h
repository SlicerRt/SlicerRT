/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// .NAME vtkDICOMImportInfo - slicer logic class for volumes manipulation
// .SECTION Description
// This class stores the DICOM loadable information

#ifndef __vtkDICOMImportInfo_h
#define __vtkDICOMImportInfo_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"
#include "vtkSlicerVolumesLogic.h"

// STD includes
#include <cstdlib>

#include "vtkSlicerDicomRtImportModuleLogicExport.h"

class vtkStringArray;
struct DicomImportInfoPrivate;

/// \ingroup SlicerRt_DicomRtImport
class VTK_SLICER_DICOMRTIMPORT_LOGIC_EXPORT vtkDICOMImportInfo : 
  public vtkObject
{
public:
  static vtkDICOMImportInfo *New();
  vtkTypeMacro(vtkDICOMImportInfo, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent); 

public:
  
  /// Add a new input file list that will be examined. A file list typically corresponds to one series.
  /// All the items that can be loaded from the lists will be stored in the list of Loadables.
  /// Returns the index of the file list that has been inserted.
  int InsertNextFileList();

  /// Get an input file list. 
  vtkStringArray* GetFileList(unsigned int fileListIndex);
  
  /// Get the total number of input file lists
  int GetNumberOfFileLists();
  
  /// Remove all the input file lists
  void RemoveAllFileLists();  

  /// Insert a Loadable item into the list. A Loadable item describes the characteristics of a DICOM object that
  /// a plugin can load.
  int InsertNextLoadable(vtkStringArray* files, const char* name, const char* tooltip, const char* warning, bool selected, double confidence);

  /// Get the total number of Loadable items
  int GetNumberOfLoadables();

  /// The file list of the data to be loaded
  vtkStringArray* GetLoadableFiles(unsigned int loadableIndex);

  /// Name exposed to the user for the node
  const char* GetLoadableName(unsigned int loadableIndex);

  /// Extra information the user sees on mouse over of the thing
  const char* GetLoadableTooltip(unsigned int loadableIndex);

  /// Things the user should know before loading this data
  const char* GetLoadableWarning(unsigned int loadableIndex);
  
  /// Is the object checked for loading by default
  bool GetLoadableSelected(unsigned int loadableIndex);

  /// Confidence of the importer (1.0 means full confidence, default is 0.5)
  bool GetLoadableConfidence(unsigned int loadableIndex);
  
  /// Remove all the Loadable items
  void RemoveAllLoadables();

protected:
  vtkDICOMImportInfo();
  virtual ~vtkDICOMImportInfo();

private:
  vtkDICOMImportInfo(const vtkDICOMImportInfo&); // Not implemented
  void operator=(const vtkDICOMImportInfo&);               // Not implemented 

private:  
  DicomImportInfoPrivate *PrivateData;
};
    
#endif
