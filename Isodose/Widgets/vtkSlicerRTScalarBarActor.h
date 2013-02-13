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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).
  
==============================================================================*/

///  vtkSliceRTScalarBarActor - slicer vtk class for adding color names in scalarbar
///
/// This class enhances the vtkScalarBarActor class by adding color names 
/// in the label display.

#ifndef __vtkSlicerRTScalarBarActor_h
#define __vtkSlicerRTScalarBarActor_h

// VTK includes
#include "vtkScalarBarActor.h"
#include "vtkStringArray.h"

// MRMLLogic includes
#include "vtkSlicerIsodoseModuleWidgetsExport.h"

/// \ingroup SlicerRt_Isodose
class VTK_SLICER_ISODOSE_LOGIC_EXPORT vtkSlicerRTScalarBarActor 
  : public vtkScalarBarActor
{
public:
  /// The Usual vtk class functions
  vtkTypeRevisionMacro(vtkSlicerRTScalarBarActor,vtkScalarBarActor);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSlicerRTScalarBarActor *New();

  /// Get for the flag on using color names as label
  vtkGetMacro(UseColorNameAsLabel, int);
  /// Set for the flag on using color names as label
  vtkSetMacro(UseColorNameAsLabel, int);
  /// Get/Set for the flag on using color names as label
  vtkBooleanMacro(UseColorNameAsLabel, int);

  /// Get color names array
  vtkGetObjectMacro(ColorNames, vtkStringArray);

  /// Set the ith color name.
  int SetColorName(int ind, const char *name);

protected:
  /// Set color names array
  vtkSetObjectMacro(ColorNames, vtkStringArray);

protected:
  vtkSlicerRTScalarBarActor();
  ~vtkSlicerRTScalarBarActor();

  /// overloaded virtual function that adds the color name as label
  virtual void AllocateAndSizeLabels(int *labelSize, int *size,
                                     vtkViewport *viewport, double *range);

  /// A vector of names for the color table elements
  vtkStringArray* ColorNames;

  /// flag for setting color name as label
  int UseColorNameAsLabel;

private:
  vtkSlicerRTScalarBarActor(const vtkSlicerRTScalarBarActor&);  // Not implemented.
  void operator=(const vtkSlicerRTScalarBarActor&);  // Not implemented.
};

#endif
