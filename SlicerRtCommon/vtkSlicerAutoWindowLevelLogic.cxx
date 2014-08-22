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

// AutoWindowLevel Logic includes
#include "vtkSlicerAutoWindowLevelLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeDisplayNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkImageData.h>
#include <vtkImageAccumulate.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerAutoWindowLevelLogic);

//----------------------------------------------------------------------------
vtkSlicerAutoWindowLevelLogic::vtkSlicerAutoWindowLevelLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerAutoWindowLevelLogic::~vtkSlicerAutoWindowLevelLogic()
{
}

//---------------------------------------------------------------------------
void vtkSlicerAutoWindowLevelLogic::ComputeWindowLevel(vtkMRMLScalarVolumeNode* inputScalarVolumeNode)
{
  if (!inputScalarVolumeNode || !inputScalarVolumeNode->GetScalarVolumeDisplayNode() || !inputScalarVolumeNode->GetImageData())
  {
    vtkErrorMacro("ComputeWindowLevel: Invalid input volume node");
    return;
  }

  vtkMRMLScalarVolumeDisplayNode* inputDisplayNode = inputScalarVolumeNode->GetScalarVolumeDisplayNode();
  inputDisplayNode->AutoWindowLevelOff();
  vtkImageData* inputImageData = inputScalarVolumeNode->GetImageData();

  double scalarRange[2] = {0.0,0.0};
  inputDisplayNode->GetDisplayScalarRange(scalarRange);

  int minScalar = scalarRange[0];
  int maxScalar = scalarRange[1];

  // Create the histogram vtkImageData
  vtkSmartPointer<vtkImageData> histogramImageData = vtkSmartPointer<vtkImageData>::New();
  histogramImageData->Initialize();
  histogramImageData->SetExtent(minScalar, maxScalar, 0, 0, 0, 0);
  histogramImageData->SetOrigin(0, 0, 0);
#if (VTK_MAJOR_VERSION <= 5)
  histogramImageData->SetScalarTypeToInt();
  histogramImageData->SetNumberOfScalarComponents(1);
  histogramImageData->AllocateScalars();
#else
  histogramImageData->AllocateScalars(VTK_INT, 1);
#endif

  // Build the histogram for the scalar image values.
  vtkSmartPointer<vtkImageAccumulate> imageAccumulator = vtkSmartPointer<vtkImageAccumulate>::New();
#if(VTK_MAJOR_VERSION <= 5)
  imageAccumulator->SetInput(inputImageData);
#else
  imageAccumulator->SetInputData(inputImageData);
#endif
  imageAccumulator->SetComponentExtent(histogramImageData->GetExtent());
  imageAccumulator->SetComponentOrigin(histogramImageData->GetOrigin());
  imageAccumulator->SetOutput(histogramImageData);
  imageAccumulator->Update();

  int meanScalar = (int)imageAccumulator->GetMean()[0];
  int scalarStandardDeviation = (int)imageAccumulator->GetStandardDeviation()[0];

  // The window width is the standard deviation of the scalar values.
  // The minimum window size is capped at 150.
  int window = (std::max)(scalarStandardDeviation, 150);

  // Find the highest peak to the right of the mean.
  // This will be the level.
  int level = -1;
  double largestBinSize = 0.0;
  double currentBinSize = 0.0;

  // Start looking for the peak in the mean bin.
  // If the mean is too far to the left (because of
  // large negative outliers, move the start bin to
  // the right by one standard deviation
  for (int currentBin = meanScalar; currentBin < maxScalar; currentBin++)
  {
    currentBinSize = histogramImageData->GetScalarComponentAsDouble(currentBin, 0, 0, 0);
    if (largestBinSize <= currentBinSize)
    {
      largestBinSize = currentBinSize;
      level = currentBin;
    }
  }

  // Set the window and level in the display node.
  inputDisplayNode->SetWindow(window);
  inputDisplayNode->SetLevel(level);
}