/*==========================================================================

  Copyright (c) Massachusetts General Hospital, Boston, MA, USA. All Rights Reserved.
 
  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Nadya Shusharina, Massachusetts General Hospital
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Natural Sciences and Engineering Research Council
  of Canada.

==========================================================================*/

#ifndef __plmpyMismatchError_h
#define __plmpyMismatchError_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// PlastimatchPy includes
#include "vtkSlicerPlastimatchPyModuleLogicExport.h"

// VTK includes
#include <vtkPoints.h>

class VTK_SLICER_PLASTIMATCHPY_MODULE_LOGIC_EXPORT vtkPlmpyMismatchError 
  : public vtkSlicerModuleLogic
{
public:
  static vtkPlmpyMismatchError *New();
  vtkTypeMacro(vtkPlmpyMismatchError,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  void RunMismatchError();

public:
  vtkSetStringMacro(FixedImageID);
  vtkGetStringMacro(FixedImageID);
  vtkSetStringMacro(MovingImageID);
  vtkGetStringMacro(MovingImageID);
   /// Set the fcsv file name (\sa FixedLandmarksFileName) containing the fixed landmarks.
  vtkSetStringMacro(FixedLandmarksFileName);
  /// Get the fcsv file name (\sa FixedLandmarksFileName) containing the fixed landmarks.
  vtkGetStringMacro(FixedLandmarksFileName);
  /// Set the fcsv file name (\sa MovingLandmarksFileName) containing the moving landmarks.
  vtkSetStringMacro(MovingLandmarksFileName);
  /// Get the fcsv file name (\sa MovingLandmarksFileName) containing the moving landmarks.
  vtkGetStringMacro(MovingLandmarksFileName);
  /// Set the fixed landmarks (\sa FixedLandmarks) using a vtkPoints object.
  vtkSetObjectMacro(FixedLandmarks, vtkPoints);
  /// Get the fixed landmarks (\sa FixedLandmarks) using a vtkPoints object.
  vtkGetObjectMacro(FixedLandmarks, vtkPoints);
  /// Set the moving landmarks (\sa MovingLandmarks) using a vtkPoints object.
  vtkSetObjectMacro(MovingLandmarks, vtkPoints);
  /// Get the moving landmarks (\sa MovingLandmarks) using a vtkPoints object.
  vtkGetObjectMacro(MovingLandmarks, vtkPoints);
  /// Set the warped landmarks (\sa WarpedLandmarks) using a vtkPoints object.
  vtkSetObjectMacro(WarpedLandmarks, vtkPoints);
  /// Get the warped landmarks (\sa WarpedLandmarks) using a vtkPoints object.
  vtkGetObjectMacro(WarpedLandmarks, vtkPoints);
  
  vtkGetStringMacro(AverageString);
  vtkGetStringMacro(VarianceString); 
  vtkGetStringMacro(StdevString);
  vtkGetStringMacro(SeparationString);

protected:
  /// This function sets the vtkPoints as input landmarks for Plastimatch registration
  void SetLandmarksFromSlicer();

  /// This function reads the fcsv files containing the landmarks and sets them as input landmarks for Plastimatch registration
  void SetLandmarksFromFiles();

protected:
  vtkPlmpyMismatchError();
  ~vtkPlmpyMismatchError();

protected:
  char* FixedLandmarksFileName;
  char* MovingLandmarksFileName;
  vtkPoints* FixedLandmarks;
  vtkPoints* MovingLandmarks;
  vtkPoints* WarpedLandmarks;

  float avg;
  float var;
  char* AverageString;
  char* VarianceString;
  char* StdevString;
  char* SeparationString;

private:
  plmpyMismatchError(const plmpyMismatchError&);
  void operator=(const plmpyMismatchError&);
};

  

#endif
