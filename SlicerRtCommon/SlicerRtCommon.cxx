/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLColorTableNode.h>

// VTK includes
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkGeneralTransform.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkMatrix4x4.h>
#include <vtkPlane.h>
#include <vtkSmartPointer.h>

//----------------------------------------------------------------------------
// Constant strings
//----------------------------------------------------------------------------

// (the python scripts use the raw strings, those need to be updated when these are changed!)

// SlicerRT constants
const char* SlicerRtCommon::SLICERRT_EXTENSION_NAME = "SlicerRT";
const std::string SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX = "Ref";

// Contour (vtkMRMLContourNode) constants
const std::string SlicerRtCommon::CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX = "_RibbonModel";
const std::string SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX = "_IndexedLabelmap";
const std::string SlicerRtCommon::CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX = "_ClosedSurfaceModel";

const double SlicerRtCommon::DEFAULT_RASTERIZATION_OVERSAMPLING_FACTOR = 2.0;
const double SlicerRtCommon::DEFAULT_DECIMATION_TARGET_REDUCTION_FACTOR = 0.0;

const char* SlicerRtCommon::CONTOUR_ORPHAN_CONTOURS_COLOR_TABLE_NODE_NAME = "OrphanContours_ColorTable";
const char* SlicerRtCommon::COLOR_NAME_BACKGROUND = "Background";
const char* SlicerRtCommon::COLOR_NAME_INVALID = "Invalid";
const char* SlicerRtCommon::COLOR_NAME_REMOVED = "Removed";
const int SlicerRtCommon::COLOR_INDEX_BACKGROUND = 0;
const int SlicerRtCommon::COLOR_INDEX_INVALID = 1;
const double SlicerRtCommon::COLOR_VALUE_INVALID[4] = {0.5, 0.5, 0.5, 1.0};

const std::string SlicerRtCommon::CONTOUR_NEW_CONTOUR_NAME = "NewContour";
const std::string SlicerRtCommon::CONTOURHIERARCHY_NEW_CONTOUR_SET_NAME = "NewContourSet";
const std::string SlicerRtCommon::CONTOURHIERARCHY_DUMMY_ANATOMICAL_VOLUME_NODE_NAME_PREFIX = "DummyAnatomicalVolume_";

// DicomRtImport constants
const std::string SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX = "DicomRtImport.";
const std::string SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "DoseVolume"; // Identifier
const std::string SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "DoseUnitName";
const std::string SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "DoseUnitValue";
const std::string SlicerRtCommon::DICOMRTIMPORT_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "SourceAxisDistance";
const std::string SlicerRtCommon::DICOMRTIMPORT_GANTRY_ANGLE_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "GantryAngle";
const std::string SlicerRtCommon::DICOMRTIMPORT_COUCH_ANGLE_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "CouchAngle";
const std::string SlicerRtCommon::DICOMRTIMPORT_COLLIMATOR_ANGLE_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "CollimatorAngle";
const std::string SlicerRtCommon::DICOMRTIMPORT_BEAM_JAW_POSITIONS_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "JawPositions";
const std::string SlicerRtCommon::DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "BeamNumber";
const std::string SlicerRtCommon::DICOMRTIMPORT_SOP_INSTANCE_UID_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "SopInstanceUid";
const std::string SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "RoiReferencedSeriesUid"; // DICOM connection
const std::string SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "ContourHierarchy"; // Identifier
const std::string SlicerRtCommon::DICOMRTIMPORT_STRUCTURE_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "StructureName";
const std::string SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "RtImage"; // Identifier
const std::string SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_REFERENCED_PLAN_SOP_INSTANCE_UID_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "RtImageReferencedPlanUid"; // DICOM connection
const std::string SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_SID_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "RtImageSid";
const std::string SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_POSITION_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "RtImagePosition";

const std::string SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX = "_ColorTable";
const std::string SlicerRtCommon::DICOMRTIMPORT_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX = "_AllStructures";
const std::string SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX = "_Contour";
const std::string SlicerRtCommon::DICOMRTIMPORT_ROOT_CONTOUR_HIERARCHY_NODE_NAME_POSTFIX = "_AllContours";
const std::string SlicerRtCommon::DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX = "_Isocenters";
const std::string SlicerRtCommon::DICOMRTIMPORT_NO_NAME = "No name";
const std::string SlicerRtCommon::DICOMRTIMPORT_NO_DESCRIPTION = "No description";
const std::string SlicerRtCommon::DICOMRTIMPORT_SOURCE_HIERARCHY_NODE_NAME_POSTFIX = "_Sources";
const std::string SlicerRtCommon::DICOMRTIMPORT_BEAMMODEL_HIERARCHY_NODE_NAME_POSTFIX = "_BeamModels";

const char* SlicerRtCommon::DICOMRTIMPORT_DEFAULT_DOSE_COLOR_TABLE_NAME = "Dose_ColorTable";

// DoseVolumeHistogram constants
const std::string SlicerRtCommon::DVH_ATTRIBUTE_PREFIX = "DoseVolumeHistogram.";
const std::string SlicerRtCommon::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "DVH"; // Identifier
const std::string SlicerRtCommon::DVH_DOSE_VOLUME_NODE_REFERENCE_ROLE = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "doseVolume" + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX; // Reference
const std::string SlicerRtCommon::DVH_CREATED_DVH_NODE_REFERENCE_ROLE = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "createdDvhArray" + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX; // Reference
const std::string SlicerRtCommon::DVH_STRUCTURE_CONTOUR_NODE_REFERENCE_ROLE = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "structureContour" + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX; // Reference
const std::string SlicerRtCommon::DVH_DOSE_VOLUME_OVERSAMPLING_FACTOR_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "DoseVolumeOversamplingFactor";
const std::string SlicerRtCommon::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructureName";
const std::string SlicerRtCommon::DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructureColor";
const std::string SlicerRtCommon::DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructurePlotName";
const std::string SlicerRtCommon::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructurePlotLineStyle";
const std::string SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "DvhMetric_";
const std::string SlicerRtCommon::DVH_METRIC_LIST_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "DvhMetricList";

const char        SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER = '|';
const std::string SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME = "Volume (cc)";
const std::string SlicerRtCommon::DVH_METRIC_MEAN_ATTRIBUTE_NAME_PREFIX = "Mean ";
const std::string SlicerRtCommon::DVH_METRIC_MIN_ATTRIBUTE_NAME_PREFIX = "Min ";
const std::string SlicerRtCommon::DVH_METRIC_MAX_ATTRIBUTE_NAME_PREFIX = "Max ";
const std::string SlicerRtCommon::DVH_METRIC_DOSE_ATTRIBUTE_NAME_POSTFIX = "dose";
const std::string SlicerRtCommon::DVH_METRIC_INTENSITY_ATTRIBUTE_NAME_POSTFIX = "intensity";
const std::string SlicerRtCommon::DVH_METRIC_V_DOSE_ATTRIBUTE_NAME_PREFIX = "V";
const std::string SlicerRtCommon::DVH_ARRAY_NODE_NAME_POSTFIX = "_DvhArray";
const std::string SlicerRtCommon::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE = " Value (% of ";
const std::string SlicerRtCommon::DVH_CSV_HEADER_VOLUME_FIELD_END = " cc)";

// DoseAccumulation constants
const std::string SlicerRtCommon::DOSEACCUMULATION_ATTRIBUTE_PREFIX = "DoseAccumulation.";
const std::string SlicerRtCommon::DOSEACCUMULATION_DOSE_VOLUME_NODE_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DOSEACCUMULATION_ATTRIBUTE_PREFIX + "DoseVolumeNodeName";
const std::string SlicerRtCommon::DOSEACCUMULATION_OUTPUT_BASE_NAME_PREFIX = "Accumulated_";

// Isodose constants
const char* SlicerRtCommon::ISODOSE_DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME = "Isodose_ColorTable.ctbl";
const std::string SlicerRtCommon::ISODOSE_MODEL_NODE_NAME_PREFIX = "IsodoseLevel_";
const std::string SlicerRtCommon::ISODOSE_PARAMETER_SET_BASE_NAME_PREFIX = "IsodoseParameterSet_";
const std::string SlicerRtCommon::ISODOSE_ISODOSE_SURFACES_HIERARCHY_NODE_NAME_POSTFIX = "_IsodoseSurfaces";

// Dose comparison constants
const char* SlicerRtCommon::DOSECOMPARISON_GAMMA_VOLUME_IDENTIFIER_ATTRIBUTE_NAME = "DoseComparison.GammaVolume"; // Identifier
const char* SlicerRtCommon::DOSECOMPARISON_DEFAULT_GAMMA_COLOR_TABLE_FILE_NAME = "Gamma_ColorTable.ctbl";
const std::string SlicerRtCommon::DOSECOMPARISON_OUTPUT_BASE_NAME_PREFIX = "GammaVolume_";

// Beams constants
const std::string SlicerRtCommon::BEAMS_ATTRIBUTE_PREFIX = "Beams.";
const std::string SlicerRtCommon::BEAMS_OUTPUT_ISOCENTER_FIDUCIAL_POSTFIX = "_Isocenter";
const std::string SlicerRtCommon::BEAMS_OUTPUT_SOURCE_FIDUCIAL_POSTFIX = "_Source";
const std::string SlicerRtCommon::BEAMS_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX = "BeamModel_";
const std::string SlicerRtCommon::BEAMS_PARAMETER_SET_BASE_NAME_PREFIX = "BeamParameterSet_";

// PlanarImage constants
const std::string SlicerRtCommon::PLANARIMAGE_MODEL_NODE_NAME_PREFIX = "PlanarImageDisplayedModel_";
const std::string SlicerRtCommon::PLANARIMAGE_TEXTURE_NODE_NAME_PREFIX = "PlanarImageDisplayedModelTexture_";
const std::string SlicerRtCommon::PLANARIMAGE_PARAMETER_SET_BASE_NAME_PREFIX = "PlanarImageParameterSet_";
const std::string SlicerRtCommon::PLANARIMAGE_RT_IMAGE_VOLUME_REFERENCE_ROLE = "planarRtImage" + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX; // Reference
const std::string SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE = "planarImageDisplayedModel" + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX; // Reference
const std::string SlicerRtCommon::PLANARIMAGE_TEXTURE_VOLUME_REFERENCE_ROLE = "planarImageTexture" + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX; // Reference

//----------------------------------------------------------------------------
// Utility functions
//----------------------------------------------------------------------------

void SlicerRtCommon::GetTransformBetweenTransformables(vtkMRMLTransformableNode* fromNode, vtkMRMLTransformableNode* toNode, vtkGeneralTransform* fromNodeToToNodeTransform)
{
  if (!fromNodeToToNodeTransform|| !fromNode || !toNode)
  {
    std::cerr << "SlicerRtCommon::GetTransformBetweenTransformables: Invalid input arguments!" << std::endl;
    return;
  }

  vtkSmartPointer<vtkGeneralTransform> toNodeToWorldTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  vtkMRMLTransformNode* fromNodeTransformNode = fromNode->GetParentTransformNode();
  vtkMRMLTransformNode* toNodeTransformNode = toNode->GetParentTransformNode();

  if (toNodeTransformNode!=NULL)
  {
    // toNode is transformed
    toNodeTransformNode->GetTransformToWorld(toNodeToWorldTransform);    
    if (fromNodeTransformNode!=NULL)
    {
      fromNodeToToNodeTransform->PostMultiply(); // GetTransformToNode assumes PostMultiply
      fromNodeTransformNode->GetTransformToNode(toNodeTransformNode,fromNodeToToNodeTransform);
    }
    else
    {
      // fromNodeTransformNode is NULL => the transform will be computed for the world coordinate frame
      toNodeTransformNode->GetTransformToWorld(fromNodeToToNodeTransform);
      fromNodeToToNodeTransform->Inverse();
    }
  }
  else
  {
    // toNode is not transformed => fromNodeToToNodeTransform = fromNodeToWorldTransformMatrix
    if (fromNodeTransformNode!=NULL)
    {
      // fromNode is transformed
      fromNodeTransformNode->GetTransformToWorld(fromNodeToToNodeTransform);
    }
    else
    {
      // neither fromNode nor toNode is transformed
      fromNodeToToNodeTransform->Identity();
    }
  }
}

//----------------------------------------------------------------------------
bool SlicerRtCommon::IsStringNullOrEmpty(const char* aString)
{
  if (aString == NULL)
  {
    return true;
  }
  else if (strcmp(aString, "") == 0)
  {
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void SlicerRtCommon::GetExtentAndSpacingForOversamplingFactor( vtkMRMLScalarVolumeNode* inputVolumeNode, double oversamplingFactor,
                                                               int outputImageDataExtent[6], double outputImageDataSpacing[3] )
{
  if (!inputVolumeNode || !inputVolumeNode->GetImageData())
  {
    std::cerr << "SlicerRtCommon::GetExtentAndSpacingForOversamplingFactor: Invalid input arguments!" << std::endl;
    return;
  }

  // Sanity check
  if ( oversamplingFactor < 0.01
    || oversamplingFactor > 100.0 )
  {
    std::cerr << "SlicerRtCommon::GetExtentAndSpacingForOversamplingFactor: Oversampling factor" << oversamplingFactor << "seems unreasonable!" << std::endl;
    return;
  }

  int imageDataExtent[6] = {0, 0, 0, 0, 0, 0};
  double imageDataSpacing[3] = {0.0, 0.0, 0.0};
  int extentMin, extentMax;
  inputVolumeNode->GetImageData()->GetWholeExtent(imageDataExtent);
  inputVolumeNode->GetImageData()->GetSpacing(imageDataSpacing);

  for (unsigned int axis=0; axis<3; ++axis)
  {
    extentMin = imageDataExtent[axis*2];
    unsigned int size = imageDataExtent[axis*2+1] - extentMin + 1;

    extentMax = (int)(floor(static_cast<double>(extentMin + size) * oversamplingFactor) - 1);
    extentMin = (int)(ceil(static_cast<double>(extentMin) * oversamplingFactor));

    outputImageDataExtent[axis*2] = extentMin;
    outputImageDataExtent[axis*2+1] = extentMax;
    outputImageDataSpacing[axis] = imageDataSpacing[axis] / oversamplingFactor;
  }
}

//---------------------------------------------------------------------------
bool SlicerRtCommon::IsDoseVolumeNode(vtkMRMLNode* node)
{
  if (!node)
  {
    std::cerr << "SlicerRtCommon::IsDoseVolumeNode: Invalid input arguments!" << std::endl;
    return false;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode"))
  {
    const char* doseVolumeIdentifier = node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());
    if (doseVolumeIdentifier != NULL)
    {
      return true;
    }
  }

  return false;
}

//---------------------------------------------------------------------------
void SlicerRtCommon::StretchDiscreteColorTable(vtkMRMLColorTableNode* inputDiscreteColorTable, vtkMRMLColorTableNode* outputColorTable, unsigned int numberOfColors/*=256*/)
{
  if (!inputDiscreteColorTable || !outputColorTable)
  {
    std::cerr << "SlicerRtCommon::StretchDiscreteColorTable: Invalid input arguments!" << std::endl;
    return;
  }

  if (inputDiscreteColorTable->GetNumberOfColors() >= (int)numberOfColors)
  {
    vtkErrorWithObjectMacro(inputDiscreteColorTable, "SlicerRtCommon::StretchDiscreteColorTable: Input discrete color table should have less colors than the specified number of colors (" << inputDiscreteColorTable->GetNumberOfColors() << " < " << numberOfColors << ")");
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
bool SlicerRtCommon::DoVolumeLatticesMatch(vtkMRMLScalarVolumeNode* volume1, vtkMRMLScalarVolumeNode* volume2)
{
  if (!volume1 || !volume2)
  {
    std::cerr << "SlicerRtCommon::DoVolumeLatticesMatch: Invalid (NULL) argument!" << std::endl;
    return false;
  }

  // Get VTK image data objects
  vtkImageData* imageData1 = volume1->GetImageData();
  vtkImageData* imageData2 = volume2->GetImageData();
  if (!imageData1 || !imageData2)
  {
    vtkErrorWithObjectMacro(volume1, "SlicerRtCommon::DoVolumeLatticesMatch: At least one of the input volume nodes does not have a valid image data!");
    return false;
  }

  // Check parent transforms (they have to be in the same branch)
  if (volume1->GetParentTransformNode() != volume2->GetParentTransformNode())
  {
    vtkDebugWithObjectMacro(volume1, "SlicerRtCommon::DoVolumeLatticesMatch: Parent transform nodes are not the same for the two input volumes");
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
        vtkDebugWithObjectMacro(volume1, "SlicerRtCommon::DoVolumeLatticesMatch: IJK to RAS matrices differ");
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
    vtkDebugWithObjectMacro(volume1, "SlicerRtCommon::DoVolumeLatticesMatch: VTK image data dimensions differ!!");
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
    vtkDebugWithObjectMacro(volume1, "SlicerRtCommon::DoVolumeLatticesMatch: VTK image data extents differ!!");
    return false;
  }

  // All of the tests passed, declare the lattices the same
  return true;
}

//---------------------------------------------------------------------------
bool SlicerRtCommon::AreBoundsEqual(int boundsA[6], int boundsB[6])
{
  return 
    boundsA[0] == boundsB [0] &&
    boundsA[1] == boundsB [1] &&
    boundsA[2] == boundsB [2] &&
    boundsA[3] == boundsB [3] &&
    boundsA[4] == boundsB [4] &&
    boundsA[5] == boundsB [5];
}


//---------------------------------------------------------------------------
bool SlicerRtCommon::OrderPlanesAlongNormal( std::vector<vtkSmartPointer<vtkPlane> > inputPlanes, std::map<double, vtkSmartPointer<vtkPlane> >& outputPlaneOrdering )
{
  std::map<double, vtkVector3<double> > intermediateSortMap;
  // Iterate over each plane
  vtkVector3<double> lastNormal;
  for(  std::vector<vtkSmartPointer<vtkPlane>>::iterator it = inputPlanes.begin(); it != inputPlanes.end(); ++it )
  {
    // Calculate the projection of the origin onto the normal
    vtkVector3<double> origin((*it)->GetOrigin()[0], (*it)->GetOrigin()[1], (*it)->GetOrigin()[2]);
    vtkVector3<double> normal((*it)->GetNormal()[0], (*it)->GetNormal()[1], (*it)->GetNormal()[2]);

    if( it != inputPlanes.begin() && ( normal.X() != lastNormal.X() || normal.Y() != lastNormal.Y() || normal.Z() != lastNormal.Z() ) )
    {
      return false;
    }
    double mag = origin.Dot(normal);
    outputPlaneOrdering[mag] = *it;

    lastNormal.Set(normal.X(), normal.Y(), normal.Z());
  }

  return true;
}
