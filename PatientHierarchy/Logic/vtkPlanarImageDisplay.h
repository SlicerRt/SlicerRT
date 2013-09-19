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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// .NAME vtkPlanarImageDisplay - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkPlanarImageDisplay_h
#define __vtkPlanarImageDisplay_h

#include "vtkObject.h"

#include "vtkSlicerPatientHierarchyModuleLogicExport.h"

class vtkMRMLScalarVolumeNode;

/// \ingroup Slicer_QtModules_PatientHierarchy
class VTK_SLICER_PATIENTHIERARCHY_LOGIC_EXPORT vtkPlanarImageDisplay : public vtkObject
{
public:
  static vtkPlanarImageDisplay *New();
  vtkTypeMacro(vtkPlanarImageDisplay,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Name of the model node that displays the planar image
  static const char* PLANAR_IMAGE_MODEL_NODE_NAME;

public:
  /// Show planar image as a rectangular model with the input volume as the texture
  static void DisplayPlanarImage(vtkMRMLScalarVolumeNode* volumeToDisplay, bool preserveWindowLevel=true);

protected:
  vtkPlanarImageDisplay();
  virtual ~vtkPlanarImageDisplay();

private:
  vtkPlanarImageDisplay(const vtkPlanarImageDisplay&); // Not implemented
  void operator=(const vtkPlanarImageDisplay&);               // Not implemented
};

#endif
