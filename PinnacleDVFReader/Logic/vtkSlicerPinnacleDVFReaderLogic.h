/*==========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

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

==========================================================================*/

#ifndef __vtkSlicerPinnacleDVFReaderLogic_h
#define __vtkSlicerPinnacleDVFReaderLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// STD includes
#include <vector>

// PinnacleDVFReader includes
#include "vtkSlicerPinnacleDVFReaderLogicExport.h"

/// \ingroup SlicerRt_QtModules_PinnacleDVFReader
class VTK_SLICER_PINNACLEDVFREADER_LOGIC_EXPORT vtkSlicerPinnacleDVFReaderLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerPinnacleDVFReaderLogic *New();
  vtkTypeMacro(vtkSlicerPinnacleDVFReaderLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Load DVF from file
  /// \param filename Path and filename of the DVF file
  /// \param 
  void LoadPinnacleDVF(char *filename, double gridOriginX, double gridOriginY, double gridOriginZ);

protected:
  vtkSlicerPinnacleDVFReaderLogic();
  virtual ~vtkSlicerPinnacleDVFReaderLogic();

private:
  vtkSlicerPinnacleDVFReaderLogic(const vtkSlicerPinnacleDVFReaderLogic&); // Not implemented
  void operator=(const vtkSlicerPinnacleDVFReaderLogic&);               // Not implemented
};

#endif