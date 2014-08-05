/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

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
// .NAME vtkRTBeamData - 
// .SECTION Description
// This class represents the beam-specific data that can be stored 
// in a vtkMRMLRTBeamNode and/or displayed in the UI

#ifndef __vtkRTBeamData_h
#define __vtkRTBeamData_h

// VTK includes
#include "vtkObject.h"

#include "vtkSlicerExternalBeamPlanningModuleMRMLExport.h"

class vtkImageData;

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO #210: investigate why the wrapping fails
//BTX

class VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_MRML_EXPORT vtkRTBeamData : public vtkObject
{
public:
  static vtkRTBeamData *New();
  vtkTypeMacro(vtkRTBeamData, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Set/Get structure name
  vtkGetStringMacro(BeamName);
  vtkSetStringMacro(BeamName);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(BeamNumber, int);
  vtkSetMacro(BeamNumber, int);

  /// Set/Get structure name
  vtkGetStringMacro(BeamDescription);
  vtkSetStringMacro(BeamDescription);

protected:
  /// Name of the structure that corresponds to this contour
  char* BeamName;
  int   BeamNumber;
  char* BeamDescription;

//protected:
public:
  vtkRTBeamData();
  virtual ~vtkRTBeamData();

private:
  vtkRTBeamData(const vtkRTBeamData&); // Not implemented
  void operator=(const vtkRTBeamData&); // Not implemented
};
//ETX

#endif
