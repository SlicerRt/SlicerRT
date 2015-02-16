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

#include "vtkLabelmapToModelFilter.h"

// VTK includes
#include <vtkVersion.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkMarchingCubes.h>
#include <vtkDecimatePro.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLabelmapToModelFilter);

//----------------------------------------------------------------------------
vtkLabelmapToModelFilter::vtkLabelmapToModelFilter()
{
  this->InputLabelmap = NULL;
  vtkSmartPointer<vtkImageData> inputLabelmap = vtkSmartPointer<vtkImageData>::New();
  this->SetInputLabelmap(inputLabelmap);

  this->OutputModel = NULL;
  vtkSmartPointer<vtkPolyData> outputModel = vtkSmartPointer<vtkPolyData>::New();
  this->SetOutputModel(outputModel);

  this->SetDecimateTargetReduction(0.0);
  this->SetLabelValue(1.0);
}

//----------------------------------------------------------------------------
vtkLabelmapToModelFilter::~vtkLabelmapToModelFilter()
{
  this->SetInputLabelmap(NULL);
  this->SetOutputModel(NULL);
}

//----------------------------------------------------------------------------
void vtkLabelmapToModelFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkLabelmapToModelFilter::GetOutput()
{
  return this->OutputModel;
}

//----------------------------------------------------------------------------
void vtkLabelmapToModelFilter::Update()
{
  if (!this->InputLabelmap || !this->OutputModel)
  {
    vtkErrorMacro("Update: Input labelmap and output poly data have to be initialized!");
    return;
  }

  // Run marching cubes
  vtkSmartPointer<vtkMarchingCubes> marchingCubes = vtkSmartPointer<vtkMarchingCubes>::New();
#if (VTK_MAJOR_VERSION <= 5)
  marchingCubes->SetInput(this->InputLabelmap);
#else
  marchingCubes->SetInputData(this->InputLabelmap);
#endif
  marchingCubes->SetNumberOfContours(1);
  marchingCubes->SetValue(0, this->LabelValue/2.0);
  marchingCubes->ComputeScalarsOff();
  marchingCubes->ComputeGradientsOff();
  marchingCubes->ComputeNormalsOff();
  try
  {
    marchingCubes->Update();
  }
  catch(...)
  {
    vtkErrorMacro("Error while running marching cubes!");
    return;
  }
  if (marchingCubes->GetOutput()->GetNumberOfPolys() == 0)
  {
    vtkErrorMacro("No polygons can be created!");
    return;
  }

  // Decimate
  vtkSmartPointer<vtkDecimatePro> decimator = vtkSmartPointer<vtkDecimatePro>::New();
#if (VTK_MAJOR_VERSION <= 5)
  decimator->SetInput(marchingCubes->GetOutput() );
#else
  decimator->SetInputData(marchingCubes->GetOutput() );
#endif
  decimator->SetFeatureAngle(60);
  decimator->SplittingOff();
  decimator->PreserveTopologyOn();
  decimator->SetMaximumError(1);
  decimator->SetTargetReduction(this->DecimateTargetReduction);
  try
  {
    decimator->Update();
  }
  catch(...)
  {
    vtkErrorMacro("Error decimating model");
    return;
  }

  this->OutputModel->ShallowCopy(decimator->GetOutput());
} 