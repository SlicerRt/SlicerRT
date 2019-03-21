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

#include "vtkSlicerRtCommon.h"

// MRML includes
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>

// VTK includes
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkGeneralTransform.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkSmartPointer.h>

// SegmentationCore includes
#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"

// VTK sys tools
#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
// Constant strings
//----------------------------------------------------------------------------

// (the python scripts use the raw strings, those need to be updated when these are changed!)

// SlicerRT constants
const char* vtkSlicerRtCommon::SLICERRT_EXTENSION_NAME = "SlicerRT";
const std::string vtkSlicerRtCommon::STORAGE_NODE_POSTFIX = "_Storage";
const std::string vtkSlicerRtCommon::DISPLAY_NODE_SUFFIX = "_Display";

// Segmentation constants
const char* vtkSlicerRtCommon::SEGMENTATION_RIBBON_MODEL_REPRESENTATION_NAME = "Ribbon model";

const double vtkSlicerRtCommon::COLOR_VALUE_INVALID[4] = {0.5, 0.5, 0.5, 1.0};

// DicomRtImport constants
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX = "DicomRtImport.";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "DoseVolume"; // Identifier
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "DoseUnitName";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "DoseUnitValue";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "SourceAxisDistance";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_GANTRY_ANGLE_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "GantryAngle";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_COUCH_ANGLE_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "CouchAngle";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_COLLIMATOR_ANGLE_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "CollimatorAngle";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_BEAM_JAW_POSITIONS_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "JawPositions";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "BeamNumber";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "RoiReferencedSeriesUid"; // DICOM connection
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "RtImage"; // Identifier
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_RTIMAGE_SID_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "RtImageSid";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_RTIMAGE_POSITION_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "RtImagePosition";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_ISODOSE_MODEL_IDENTIFIER_ATTRIBUTE_NAME = vtkSlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "IsodoseModel"; // Identifier

const std::string vtkSlicerRtCommon::DICOMRTIMPORT_FIDUCIALS_HIERARCHY_NODE_NAME_POSTFIX = "_Fiducials";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_MODEL_HIERARCHY_NODE_NAME_POSTFIX = "_ModelHierarchy";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX = "_Isocenters";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_NO_NAME = "No name";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_NO_STUDY_DESCRIPTION = "No study description";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_SOURCE_HIERARCHY_NODE_NAME_POSTFIX = "_Sources";
const std::string vtkSlicerRtCommon::DICOMRTIMPORT_BEAMMODEL_HIERARCHY_NODE_NAME_POSTFIX = "_Beams";

const char* vtkSlicerRtCommon::DEFAULT_DOSE_COLOR_TABLE_NAME = "Dose_ColorTable";

//----------------------------------------------------------------------------
// Utility functions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
bool vtkSlicerRtCommon::IsStringNullOrEmpty(const char* aString)
{
  if (aString == nullptr)
  {
    return true;
  }
  else if (strcmp(aString, "") == 0)
  {
    return true;
  }
  return false;
}

//---------------------------------------------------------------------------
bool vtkSlicerRtCommon::IsDoseVolumeNode(vtkMRMLNode* node)
{
  if (!node)
  {
    return false;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode"))
  {
    const char* doseVolumeIdentifier = node->GetAttribute(vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());
    if (doseVolumeIdentifier != nullptr)
    {
      return true;
    }
  }

  return false;
}

//---------------------------------------------------------------------------
bool vtkSlicerRtCommon::IsIsodoseModelNode(vtkMRMLNode* node)
{
  if (!node)
  {
    return false;
  }

  if (node->IsA("vtkMRMLModelNode"))
  {
    const char* isodoseModelIdentifier = node->GetAttribute(vtkSlicerRtCommon::DICOMRTIMPORT_ISODOSE_MODEL_IDENTIFIER_ATTRIBUTE_NAME.c_str());
    if (isodoseModelIdentifier != nullptr)
    {
      return true;
    }
  }

  return false;
}

//---------------------------------------------------------------------------
void vtkSlicerRtCommon::StretchDiscreteColorTable(vtkMRMLColorTableNode* inputDiscreteColorTable, vtkMRMLColorTableNode* outputColorTable, unsigned int numberOfColors/*=256*/)
{
  if (!inputDiscreteColorTable || !outputColorTable)
  {
    vtkGenericWarningMacro("vtkSlicerRtCommon::StretchDiscreteColorTable: Invalid input arguments!");
    return;
  }

  if (inputDiscreteColorTable->GetNumberOfColors() >= (int)numberOfColors)
  {
    vtkErrorWithObjectMacro(inputDiscreteColorTable, "vtkSlicerRtCommon::StretchDiscreteColorTable: Input discrete color table should have less colors than the specified number of colors (" << inputDiscreteColorTable->GetNumberOfColors() << " < " << numberOfColors << ")");
    return;
  }

  vtkSmartPointer<vtkDiscretizableColorTransferFunction> colorTransferFunction = vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
  int numberOfIsodoseColors = inputDiscreteColorTable->GetNumberOfColors();
  double step = 1.0 / (double)(numberOfIsodoseColors-1);
  double transferFunctionPosition = 0.0;
  for (int colorIndex=0; colorIndex<numberOfIsodoseColors; ++colorIndex, transferFunctionPosition+=step)
  {
    double color[4] = {0.0, 0.0, 0.0, 0.0};
    inputDiscreteColorTable->GetColor(colorIndex, color);
    colorTransferFunction->AddRGBPoint(transferFunctionPosition, color[0], color[1], color[2]);
  }
  colorTransferFunction->SetNumberOfValues(numberOfColors);
  colorTransferFunction->Build();
  outputColorTable->SetNumberOfColors(numberOfColors);
  outputColorTable->GetLookupTable()->SetTableRange(0,numberOfColors-1);

  step = 1.0 / (double)(numberOfColors-1);
  transferFunctionPosition = 0.0;
  for (int colorIndex=0; colorIndex<outputColorTable->GetNumberOfColors(); ++colorIndex, transferFunctionPosition+=step)
  {
    double color[3] = {0.0, 0.0, 0.0};
    colorTransferFunction->GetColor(transferFunctionPosition, color);
    outputColorTable->SetColor(colorIndex, color[0], color[1], color[2], 1.0);
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerRtCommon::DoVolumeLatticesMatch(vtkMRMLScalarVolumeNode* volume1, vtkMRMLScalarVolumeNode* volume2)
{
  if (!volume1 || !volume2)
  {
    vtkGenericWarningMacro("vtkSlicerRtCommon::DoVolumeLatticesMatch: Invalid (nullptr) argument!");
    return false;
  }

  // Get VTK image data objects
  vtkImageData* imageData1 = volume1->GetImageData();
  vtkImageData* imageData2 = volume2->GetImageData();
  if (!imageData1 || !imageData2)
  {
    vtkErrorWithObjectMacro(volume1, "vtkSlicerRtCommon::DoVolumeLatticesMatch: At least one of the input volume nodes does not have a valid image data!");
    return false;
  }

  // Check parent transforms (they have to be in the same branch)
  if (volume1->GetParentTransformNode() != volume2->GetParentTransformNode())
  {
    vtkDebugWithObjectMacro(volume1, "vtkSlicerRtCommon::DoVolumeLatticesMatch: Parent transform nodes are not the same for the two input volumes");
    return false;
  }

  // Compare IJK to RAS matrices (involves checking the spacing and origin too)
  vtkSmartPointer<vtkMatrix4x4> ijkToRasMatrix1 = vtkSmartPointer<vtkMatrix4x4>::New();
  vtkSmartPointer<vtkMatrix4x4> ijkToRasMatrix2 = vtkSmartPointer<vtkMatrix4x4>::New();
  volume1->GetIJKToRASMatrix(ijkToRasMatrix1);
  volume2->GetIJKToRASMatrix(ijkToRasMatrix2);
  for (int row=0; row<3; ++row)
  {
    for (int col=0; col<3; ++col)
    {
      if ( fabs(ijkToRasMatrix1->GetElement(row, col) - ijkToRasMatrix2->GetElement(row, col)) > EPSILON )
      {
        vtkDebugWithObjectMacro(volume1, "vtkSlicerRtCommon::DoVolumeLatticesMatch: IJK to RAS matrices differ");
        return false;
      }
    }
  }

  // Check image data properties
  int dimensions1[3] = {0,0,0};
  int dimensions2[3] = {0,0,0};
  imageData1->GetDimensions(dimensions1);
  imageData2->GetDimensions(dimensions2);
  if ( dimensions1[0] != dimensions2[0]
    || dimensions1[1] != dimensions2[1]
    || dimensions1[2] != dimensions2[2] )
  {
    vtkDebugWithObjectMacro(volume1, "vtkSlicerRtCommon::DoVolumeLatticesMatch: VTK image data dimensions differ!!");
    return false;
  }

  int extent1[6] = {0,0,0,0,0,0};
  int extent2[6] = {0,0,0,0,0,0};
  imageData1->GetExtent(extent1);
  imageData2->GetExtent(extent2);
  if ( extent1[0] != extent2[0] || extent1[1] != extent2[1]
    || extent1[2] != extent2[2] || extent1[3] != extent2[3]
    || extent1[4] != extent2[4] || extent1[5] != extent2[5] )
  {
    vtkDebugWithObjectMacro(volume1, "vtkSlicerRtCommon::DoVolumeLatticesMatch: VTK image data extents differ!!");
    return false;
  }

  // All of the tests passed, declare the lattices the same
  return true;
}

//---------------------------------------------------------------------------
bool vtkSlicerRtCommon::AreEqualWithTolerance(double a, double b)
{
  return fabs(a - b) < EPSILON;
}

//---------------------------------------------------------------------------
bool vtkSlicerRtCommon::AreExtentsEqual(int extentA[6], int extentB[6])
{
  return 
    extentA[0] == extentB [0] &&
    extentA[1] == extentB [1] &&
    extentA[2] == extentB [2] &&
    extentA[3] == extentB [3] &&
    extentA[4] == extentB [4] &&
    extentA[5] == extentB [5];
}

//---------------------------------------------------------------------------
void vtkSlicerRtCommon::GenerateRandomColor(vtkMRMLColorTableNode* colorNode, double* newColor)
{
  const int MAX_TRIES = 50;

  int currentTry(0);
  while(currentTry < MAX_TRIES )
  {
    newColor[0] = rand() * 1.0 / RAND_MAX;
    newColor[1] = rand() * 1.0 / RAND_MAX;
    newColor[2] = rand() * 1.0 / RAND_MAX;

    bool hasColor(false);
    for (int i = 0; i < colorNode->GetNumberOfColors(); ++i)
    {
      double thisColor[4];
      colorNode->GetColor(i, thisColor);
      if (AreEqualWithTolerance(thisColor[0], newColor[0]) && AreEqualWithTolerance(thisColor[1], newColor[1]) && AreEqualWithTolerance(thisColor[2], newColor[2]))
      {
        hasColor = true;
        break;
      }
    }

    if (!hasColor)
    {
      break;
    }

    currentTry++;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerRtCommon::WriteImageDataToFile(vtkMRMLScene* scene, vtkImageData* imageData, const char* fileName, double dirs[3][3], double spacing[3], double origin[3], bool overwrite)
{
  if (scene == nullptr)
  {
    vtkGenericWarningMacro("vtkSlicerRtCommon::WriteImageDataToFile: Invalid scene");
    return;
  }

  if (fileName == nullptr || strcmp(fileName, "") == 0)
  {
    vtkGenericWarningMacro("vtkSlicerRtCommon::WriteImageDataToFile: Invalid filename");
    return;
  }

  if (vtksys::SystemTools::FileExists(fileName) && overwrite)
  {
    vtksys::SystemTools::RemoveFile(fileName);
  }
  else if (vtksys::SystemTools::FileExists(fileName))
  {
    return;
  }

  vtkSmartPointer<vtkMRMLScalarVolumeNode> volumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  volumeNode->SetIJKToRASDirections(dirs);
  volumeNode->SetOrigin(origin);
  volumeNode->SetSpacing(spacing);
  volumeNode->SetAndObserveImageData(imageData);
  scene->AddNode(volumeNode);

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> storageNode = vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  storageNode->SetFileName(fileName);
  scene->AddNode(storageNode);

  volumeNode->SetAndObserveStorageNodeID(storageNode->GetID());
  if (storageNode->WriteData(volumeNode) == 0)
  {
    vtkGenericWarningMacro("vtkSlicerRtCommon::WriteImageDataToFile: Unable to write image data to file");
  }

  scene->RemoveNode(storageNode);
  scene->RemoveNode(volumeNode);

  return;
}

//---------------------------------------------------------------------------
bool vtkSlicerRtCommon::ConvertVolumeNodeToVtkOrientedImageData(vtkMRMLScalarVolumeNode* inVolumeNode, vtkOrientedImageData* outImageData, bool applyRasToWorldConversion/*=true*/)
{
  if (!inVolumeNode || !inVolumeNode->GetImageData())
  {
    vtkGenericWarningMacro("vtkSlicerRtCommon::ConvertVolumeNodeToVtkOrientedImageData: Invalid volume node!");
    return false;
  }
  if (!outImageData)
  {
    vtkErrorWithObjectMacro(inVolumeNode, "ConvertVolumeNodeToVtkOrientedImageData: Invalid output oriented image data!");
    return false;
  }

  outImageData->vtkImageData::DeepCopy(inVolumeNode->GetImageData());

  vtkSmartPointer<vtkMatrix4x4> ijkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inVolumeNode->GetIJKToRASMatrix(ijkToRasMatrix);
  outImageData->SetGeometryFromImageToWorldMatrix(ijkToRasMatrix);

  // Apply parent transform of the volume node if requested and present
  if (applyRasToWorldConversion)
  {
    // Get world to reference RAS transform
    vtkSmartPointer<vtkGeneralTransform> inVolumeToWorldTransform = vtkSmartPointer<vtkGeneralTransform>::New();
    vtkMRMLTransformNode* parentTransformNode = inVolumeNode->GetParentTransformNode();
    if (parentTransformNode)
    {
      parentTransformNode->GetTransformToWorld(inVolumeToWorldTransform);
    }
    else
    {
      // There is no parent transform for volume, nothing to apply
      return true;
    }

    // Transform oriented image data
    vtkOrientedImageDataResample::TransformOrientedImage(outImageData, inVolumeToWorldTransform);
  }

  return true;
}
