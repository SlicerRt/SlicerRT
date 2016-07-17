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

// SlicerRT includes
#include "SlicerRtCommon.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>

// Qt includes
#include <QDebug>

//----------------------------------------------------------------------------
qSlicerMockDoseEngine::qSlicerMockDoseEngine(QObject* parent)
  : qSlicerAbstractDoseEngine(parent)
{
  this->m_Name = QString("Mock random");
}

//----------------------------------------------------------------------------
qSlicerMockDoseEngine::~qSlicerMockDoseEngine()
{
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

  vtkMRMLScene* scene = beamNode->GetScene();

  vtkSmartPointer<vtkClosedSurfaceToBinaryLabelmapConversionRule> converter = 
    vtkSmartPointer<vtkClosedSurfaceToBinaryLabelmapConversionRule>::New();
  converter->SetUseOutputImageDataGeometry(true);
  vtkSmartPointer<vtkSegment> beamSegment = vtkSmartPointer<vtkSegment>::Take(
    vtkSlicerSegmentationsModuleLogic::CreateSegmentFromModelNode(beamNode) );
  vtkPolyData* beamPolyData = vtkPolyData::SafeDownCast(beamSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName()));
  vtkSmartPointer<vtkOrientedImageData> beamImageData = vtkSmartPointer<vtkOrientedImageData>::Take(
    vtkSlicerSegmentationsModuleLogic::CreateOrientedImageDataFromVolumeNode(referenceVolumeNode) );
  converter->Convert(beamPolyData, beamImageData);

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
  double rxDose = parentPlanNode->GetRxDose();
  unsigned char* beamPtr = (unsigned char*)beamImageData->GetScalarPointer();
  float* floatPtr = (float*)protonDoseImageData->GetScalarPointer();
  for (long i=0; i<protonDoseImageData->GetNumberOfPoints(); ++i)
  {
    if ((*beamPtr) > 0)
    {
      (*floatPtr) = rxDose*19.0/20.0 + (float)(rand()%100)*rxDose/1000.0;
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
