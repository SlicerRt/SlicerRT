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

// Dose engines includes
#include "qSlicerMockDoseEngine.h"

// Beams includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"

// Segmentations includes
#include "vtkOrientedImageData.h"
#include "vtkSlicerSegmentationsModuleLogic.h"
#include "vtkClosedSurfaceToBinaryLabelmapConversionRule.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>

// Slicer includes
#include <vtkSlicerVersionConfigure.h>

// Qt includes
#include <QDebug>

//----------------------------------------------------------------------------
qSlicerMockDoseEngine::qSlicerMockDoseEngine(QObject* parent)
  : qSlicerAbstractDoseEngine(parent)
{
  this->m_Name = QString("Mock random");
}

//----------------------------------------------------------------------------
qSlicerMockDoseEngine::~qSlicerMockDoseEngine() = default;

//---------------------------------------------------------------------------
void qSlicerMockDoseEngine::defineBeamParameters()
{
  // Noise level parameter
  this->addBeamParameterSpinBox(
    "Mock dose", "NoiseRange", "Noise range (% of Rx):", "Range of noise added to the prescription dose (+- half of the percentage of the Rx dose)",
    0.0, 99.99, 10.0, 1.0, 2 );
}

//---------------------------------------------------------------------------
QString qSlicerMockDoseEngine::calculateDoseUsingEngine(vtkMRMLRTBeamNode* beamNode, vtkMRMLScalarVolumeNode* resultDoseVolumeNode)
{
  if (!beamNode)
  {
    QString errorMessage("Invalid beam node");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }
  vtkMRMLRTPlanNode* parentPlanNode = beamNode->GetParentPlanNode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = parentPlanNode->GetReferenceVolumeNode();
  if (!parentPlanNode || !referenceVolumeNode || !resultDoseVolumeNode)
  {
    QString errorMessage("Unable to access reference volume");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  vtkSmartPointer<vtkClosedSurfaceToBinaryLabelmapConversionRule> converter =
    vtkSmartPointer<vtkClosedSurfaceToBinaryLabelmapConversionRule>::New();
  converter->SetUseOutputImageDataGeometry(true);
  vtkSmartPointer<vtkSegment> beamSegment = vtkSmartPointer<vtkSegment>::Take(
    vtkSlicerSegmentationsModuleLogic::CreateSegmentFromModelNode(beamNode) );
  vtkPolyData* beamPolyData = vtkPolyData::SafeDownCast(beamSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName()));
  vtkSmartPointer<vtkOrientedImageData> beamImageData = vtkSmartPointer<vtkOrientedImageData>::Take(
    vtkSlicerSegmentationsModuleLogic::CreateOrientedImageDataFromVolumeNode(referenceVolumeNode) );
  beamSegment->AddRepresentation(vtkSegmentationConverter::GetBinaryLabelmapRepresentationName(), beamImageData);
#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
  converter->Convert(beamSegment);
  beamImageData = vtkOrientedImageData::SafeDownCast(beamSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()));
#else
  converter->Convert(beamPolyData, beamImageData);
#endif

  if (!beamImageData)
  {
    QString errorMessage("Unable to create beam image!");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  // Create dose image
  vtkSmartPointer<vtkImageData> protonDoseImageData = vtkSmartPointer<vtkImageData>::New();
  protonDoseImageData->SetExtent(referenceVolumeNode->GetImageData()->GetExtent());
  protonDoseImageData->SetSpacing(referenceVolumeNode->GetImageData()->GetSpacing());
  protonDoseImageData->SetOrigin(referenceVolumeNode->GetImageData()->GetOrigin());
  protonDoseImageData->AllocateScalars(VTK_FLOAT, 1);
  if ( beamImageData->GetNumberOfPoints() != protonDoseImageData->GetNumberOfPoints()
    || beamImageData->GetScalarType() != VTK_UNSIGNED_CHAR )
  {
    QString errorMessage("Geometrical discrepancy between beam and dose");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  // Paint voxels touched by beam prescription+noise, all others zero
  float noiseRange = (float)this->doubleParameter(beamNode, "NoiseRange");
  double rxDose = parentPlanNode->GetRxDose();
  unsigned char* beamPtr = (unsigned char*)beamImageData->GetScalarPointer();
  float* floatPtr = (float*)protonDoseImageData->GetScalarPointer();
  for (long i=0; i<protonDoseImageData->GetNumberOfPoints(); ++i)
  {
    if ((*beamPtr) > 0)
    {
      (*floatPtr) = rxDose + (float)((float)rand()/RAND_MAX)*rxDose * noiseRange/100.0 - noiseRange/200.0;
    }
    else
    {
      (*floatPtr) = 0;
    }
    ++floatPtr;
    ++beamPtr;
  }

  resultDoseVolumeNode->SetAndObserveImageData(protonDoseImageData);
  resultDoseVolumeNode->CopyOrientation(referenceVolumeNode);

  std::string randomDoseNodeName = std::string(beamNode->GetName()) + "_MockDose";
  resultDoseVolumeNode->SetName(randomDoseNodeName.c_str());

  return QString();
}
