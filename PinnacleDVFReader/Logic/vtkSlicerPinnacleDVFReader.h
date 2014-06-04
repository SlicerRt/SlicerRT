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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/


// .NAME vtkSlicerPinnacleDVFReader - 
// .SECTION Description
// This class manages the Reader associated with reading Pinnacle DVF file
// The reader load DVF in LPS and convert it to RAS cooridnate system mainly due to 
// it is used for Slicer.

#ifndef __vtkSlicerPinnacleDVFReader_h
#define __vtkSlicerPinnacleDVFReader_h

// VTK includes
#include "vtkObject.h"

#include "vtkSlicerPinnacleDVFReaderLogicExport.h"

class vtkMatrix4x4;
class vtkImageData;

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO #210: investigate why the wrapping fails
//BTX

/// \ingroup SlicerRt_DicomSroImport
class VTK_SLICER_PINNACLEDVFREADER_LOGIC_EXPORT vtkSlicerPinnacleDVFReader : public vtkObject
{
public:
  static vtkSlicerPinnacleDVFReader *New();
  vtkTypeMacro(vtkSlicerPinnacleDVFReader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Do reading
  void Update();

public:
  /// Get post deformation registration matrix
  vtkMatrix4x4* GetPostDeformationRegistrationMatrix();

  /// Get deformable registration grid (displacment vector field)
  vtkImageData* GetDeformableRegistrationGrid();

  /// Get deformable registration grid orientation matrix
  vtkMatrix4x4* GetDeformableRegistrationGridOrientationMatrix();

  /// Set input file name
  vtkSetStringMacro(FileName);

  /// Set deformation grid origin
  vtkSetVector3Macro(GridOrigin,double);
  vtkGetVector3Macro(GridOrigin,double);

  /// Get load deformable spatial registration successful flag
  vtkGetMacro(LoadDeformableSpatialRegistrationSuccessful, bool);

protected:
  void LoadDeformableSpatialRegistration(char*);

protected:
  /// Input file name
  char* FileName;

  /// Deformation grid origin
  double GridOrigin[3];

  /// Post deformation registration matrix
  vtkMatrix4x4* PostDeformationRegistrationMatrix;

  /// Deformable registration grid
  vtkImageData* DeformableRegistrationGrid;

  /// Deformable registration grid orientation matrix
  vtkMatrix4x4* DeformableRegistrationGridOrientationMatrix;

  /// Flag indicating if deformable spatial registration object has been successfully read from the input dataset
  bool LoadDeformableSpatialRegistrationSuccessful;

protected:
  vtkSlicerPinnacleDVFReader();
  virtual ~vtkSlicerPinnacleDVFReader();

private:
  vtkSlicerPinnacleDVFReader(const vtkSlicerPinnacleDVFReader&); // Not implemented
  void operator=(const vtkSlicerPinnacleDVFReader&);         // Not implemented
};
//ETX

#endif
