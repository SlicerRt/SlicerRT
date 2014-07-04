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


// Module includes
#include "vtkSlicerDoseCalculationEngine.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>

// STD includes
#include <cassert>

vtkStandardNewMacro(vtkSlicerDoseCalculationEngine);

//----------------------------------------------------------------------------
vtkSlicerDoseCalculationEngine::vtkSlicerDoseCalculationEngine()
{
  this->FileName = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerDoseCalculationEngine::~vtkSlicerDoseCalculationEngine()
{
}

//----------------------------------------------------------------------------
void vtkSlicerDoseCalculationEngine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseCalculationEngine::CalculateDose()
{
}

