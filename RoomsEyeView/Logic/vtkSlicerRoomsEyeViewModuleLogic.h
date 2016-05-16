/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

// .NAME vtkSlicerRoomsEyeViewModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerRoomsEyeViewModuleLogic_h
#define __vtkSlicerRoomsEyeViewModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// STD includes
#include "vtkSlicerRoomsEyeViewModuleLogicExport.h"

/// \ingroup SlicerRt_QtModules_RoomsEyeView
class VTK_SLICER_RoomsEyeView_LOGIC_EXPORT vtkSlicerRoomsEyeViewModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerRoomsEyeViewModuleLogic *New();
  vtkTypeMacro(vtkSlicerRoomsEyeViewModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
protected:
  vtkSlicerRoomsEyeViewModuleLogic();
  virtual ~vtkSlicerRoomsEyeViewModuleLogic();

private:
  vtkSlicerRoomsEyeViewModuleLogic(const vtkSlicerRoomsEyeViewModuleLogic&); // Not implemented
  void operator=(const vtkSlicerRoomsEyeViewModuleLogic&);            // Not implemented
};

#endif

