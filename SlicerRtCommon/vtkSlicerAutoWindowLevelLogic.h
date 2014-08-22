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

==============================================================================*/

#ifndef __vtkSlicerAutoWindowLevelLogic_h
#define __vtkSlicerAutoWindowLevelLogic_h

#include "vtkSlicerRtCommonWin32Header.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICERRTCOMMON_EXPORT vtkSlicerAutoWindowLevelLogic : public vtkObject
{
public:
  // This function sets the window and level in the provided vtkMRMLScalarVolumeNode
  // based on the scalar value histogram of the image.
  void ComputeWindowLevel(vtkMRMLScalarVolumeNode* inputScalarVolumeNode);

public:
  static vtkSlicerAutoWindowLevelLogic *New();
  vtkTypeMacro(vtkSlicerAutoWindowLevelLogic, vtkObject);

protected:
  vtkSlicerAutoWindowLevelLogic();
  virtual ~vtkSlicerAutoWindowLevelLogic();
};

#endif
