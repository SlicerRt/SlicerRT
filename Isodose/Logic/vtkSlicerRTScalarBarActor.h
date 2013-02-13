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

// Std includes
#include <string>
#include <vector>

// vtk includes
#include "vtkScalarBarActor.h"

// MRMLLogic includes
#include "vtkSlicerIsodoseModuleLogicExport.h"

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO #210: investigate why the wrapping fails
//BTX

/// \ingroup SlicerRt_Isodose
class VTK_SLICER_ISODOSE_LOGIC_EXPORT vtkSlicerRTScalarBarActor 
  : public vtkScalarBarActor
{
public:
  /// The Usual vtk class functions
  vtkTypeRevisionMacro(vtkSlicerRTScalarBarActor,vtkScalarBarActor);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSlicerRTScalarBarActor *New();

  /// Get/Set for the flag on using color names as label
  vtkGetMacro(UseColorNameAsLabel, int);
  vtkSetMacro(UseColorNameAsLabel, int);
  vtkBooleanMacro(UseColorNameAsLabel, int);

  /// Set the ith color name.
  int SetColorName(int ind, const char *name);

protected:
  vtkSlicerRTScalarBarActor();
  ~vtkSlicerRTScalarBarActor();

  /// overloaded virtual function that adds the color name as label
  virtual void AllocateAndSizeLabels(int *labelSize, int *size,
                                     vtkViewport *viewport, double *range);

  /// A vector of names for the color table elements
  std::vector<std::string> Names;

  /// flag for setting color name as label
  int UseColorNameAsLabel;

private:
  vtkSlicerRTScalarBarActor(const vtkSlicerRTScalarBarActor&);  // Not implemented.
  void operator=(const vtkSlicerRTScalarBarActor&);  // Not implemented.
};

//ETX

#endif

