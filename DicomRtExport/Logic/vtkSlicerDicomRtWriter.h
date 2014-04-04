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

// .NAME vtkSlicerDicomRtExportReader -
// .SECTION Description
// This class manages the Reader associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDicomRtExportReader_h
#define __vtkSlicerDicomRtExportReader_h

#include "vtkSlicerDicomRtExportModuleLogicExport.h"

#include "vtkObject.h"

//BTX // comment out since it is giving python wrapping errors
#include "rt_study.h"

/// \ingroup SlicerRt_QtModules_DicomRtExport
class VTK_SLICER_DICOMRTEXPORT_LOGIC_EXPORT vtkSlicerDicomRtWriter :  public vtkObject
{
public:
  static vtkSlicerDicomRtWriter *New();
  vtkTypeMacro(vtkSlicerDicomRtWriter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// TODO: Description, argument names and descriptions
  void SetImage(const Plm_image::Pointer&);

  /// TODO: Description, argument names and descriptions
  void SetDose(const Plm_image::Pointer&);

  /// TODO: Description, argument names and descriptions
  void AddContour(UCharImageType::Pointer, const char*, double*);
  
  /// TODO: Description, argument names and descriptions
  void Write();

public:
  /// Get file name
  vtkGetStringMacro(FileName);
  /// Set file name
  vtkSetStringMacro(FileName);
  
protected:
  vtkSlicerDicomRtWriter();
  virtual ~vtkSlicerDicomRtWriter();

  /// TODO:
  char* FileName;

  /// TODO:
  Rt_study RtStudy;

private:
  vtkSlicerDicomRtWriter(const vtkSlicerDicomRtWriter&); // Not implemented
  void operator=(const vtkSlicerDicomRtWriter&);               // Not implemented
};
//ETX

#endif
