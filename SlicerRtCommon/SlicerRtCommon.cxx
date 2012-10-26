#include "SlicerRtCommon.h"

// SlicerRT includes
#include "vtkMRMLContourNode.h"
#include "vtkMRMLContourHierarchyNode.h"

// MRML includes
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLDisplayNode.h>

// VTK includes
#include <vtkGeneralTransform.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>
#include <vtkCollection.h>

//----------------------------------------------------------------------------
// Constant strings
//----------------------------------------------------------------------------

// DicomRtImport constants
const std::string SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX = "DicomRtImport.";
const std::string SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "DoseUnitName";
const std::string SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "DoseUnitValue";
const std::string SlicerRtCommon::DICOMRTIMPORT_SERIES_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "SeriesName";

const std::string SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX = "_ColorTable";
const std::string SlicerRtCommon::DICOMRTIMPORT_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX = "_AllStructures";
const std::string SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX = "_Contour";
const std::string SlicerRtCommon::DICOMRTIMPORT_ROOT_CONTOUR_HIERARCHY_NODE_NAME_POSTFIX = "_AllContours";
const std::string SlicerRtCommon::DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX = "_Isocenters";

// Contour (vtkMRMLContourNode) constants
const std::string SlicerRtCommon::CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX = "_RibbonModel";
const std::string SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX = "_Labelmap";
const std::string SlicerRtCommon::CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX = "_ClosedSurfaceModel";

// DoseVolumeHistogram constants
const std::string SlicerRtCommon::DVH_ATTRIBUTE_PREFIX = "DoseVolumeHistogram.";
const std::string SlicerRtCommon::DVH_TYPE_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "Type";
const std::string SlicerRtCommon::DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "DoseVolumeNodeId";
const std::string SlicerRtCommon::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructureName";
const std::string SlicerRtCommon::DVH_STRUCTURE_CONTOUR_NODE_ID_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructureModelNodeId";
const std::string SlicerRtCommon::DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructureColor";
const std::string SlicerRtCommon::DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructurePlotName";
const std::string SlicerRtCommon::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructurePlotLineStyle";
const std::string SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "DvhMetric_";
const std::string SlicerRtCommon::DVH_METRIC_LIST_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "DvhMetricList";

const std::string SlicerRtCommon::DVH_TYPE_ATTRIBUTE_VALUE = "DVH";
const char        SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER = '|';
const std::string SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME = "Total volume (cc)";
const std::string SlicerRtCommon::DVH_METRIC_MEAN_DOSE_ATTRIBUTE_NAME_PREFIX = "Mean dose";
const std::string SlicerRtCommon::DVH_METRIC_MIN_DOSE_ATTRIBUTE_NAME_PREFIX = "Min dose";
const std::string SlicerRtCommon::DVH_METRIC_MAX_DOSE_ATTRIBUTE_NAME_PREFIX = "Max dose";
const std::string SlicerRtCommon::DVH_METRIC_V_DOSE_ATTRIBUTE_NAME_PREFIX = "V";
const std::string SlicerRtCommon::DVH_ARRAY_NODE_NAME_POSTFIX = "_DvhArray";
const std::string SlicerRtCommon::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE = " Value (% of ";
const std::string SlicerRtCommon::DVH_CSV_HEADER_VOLUME_FIELD_END = " cc)";

// Isodose constants
const std::string SlicerRtCommon::ISODOSE_MODEL_NODE_NAME_PREFIX = "IsodoseLevel_";
const std::string SlicerRtCommon::ISODOSE_COLOR_TABLE_NODE_NAME_POSTFIX = "_IsodoseColor";

//----------------------------------------------------------------------------
// Utility functions
//----------------------------------------------------------------------------

void SlicerRtCommon::GetTransformBetweenDisplayables(vtkMRMLTransformableNode* fromNode, vtkMRMLTransformableNode* toNode, vtkGeneralTransform* fromNodeToToNodeTransform)
{
  if (!fromNodeToToNodeTransform)
  {
    return;
  }
  if (!fromNode || !toNode)
  {
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
void SlicerRtCommon::GetTransformFromModelToVolumeIjk(vtkMRMLModelNode* fromModelNode, vtkMRMLVolumeNode* toVolumeNode, vtkGeneralTransform* fromModelToToVolumeIjkTransform)
{
  if (!fromModelToToVolumeIjkTransform)
  {
    return;
  }
  if (!fromModelNode || !toVolumeNode)
  {
    return;
  }

  vtkSmartPointer<vtkGeneralTransform> fromModelToToVolumeRasTransform=vtkSmartPointer<vtkGeneralTransform>::New();

  SlicerRtCommon::GetTransformBetweenDisplayables(fromModelNode, toVolumeNode, fromModelToToVolumeRasTransform);

  // Create volumeRas to volumeIjk transform
  vtkSmartPointer<vtkMatrix4x4> toVolumeRasToToVolumeIjkTransformMatrix=vtkSmartPointer<vtkMatrix4x4>::New();
  toVolumeNode->GetRASToIJKMatrix( toVolumeRasToToVolumeIjkTransformMatrix );  
  
  // Create model to volumeIjk transform
  fromModelToToVolumeIjkTransform->Identity();
  fromModelToToVolumeIjkTransform->Concatenate(toVolumeRasToToVolumeIjkTransformMatrix);
  fromModelToToVolumeIjkTransform->Concatenate(fromModelToToVolumeRasTransform);
}

//----------------------------------------------------------------------------
void SlicerRtCommon::GetColorIndexForContour(vtkMRMLContourNode* contourNode, vtkMRMLScene* mrmlScene, int &colorIndex, vtkMRMLColorTableNode* &colorNode, vtkMRMLModelNode* referenceModelNode/*=NULL*/)
{
  // Initialize output color index with Gray 'invalid' color
  colorIndex = 1;

  if (!contourNode)
  {
    return;
  }

  // Get hierarchy node
  vtkMRMLContourHierarchyNode* contourHierarchyNode = vtkMRMLContourHierarchyNode::SafeDownCast(
    vtkMRMLDisplayableHierarchyNode::GetDisplayableHierarchyNode(mrmlScene, contourNode->GetID()));
  if (!contourHierarchyNode)
  {
    std::cerr << "Error: No hierarchy node found for structure '" << contourNode->GetName() << "'" << std::endl;
    return;
  }

  // Get color node created for the structure set
  vtkMRMLContourHierarchyNode* parentContourHierarchyNode = vtkMRMLContourHierarchyNode::SafeDownCast(contourHierarchyNode->GetParentNode());

  std::string seriesName = parentContourHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_SERIES_NAME_ATTRIBUTE_NAME.c_str());
  std::string colorNodeName = seriesName + SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;
  vtkCollection* colorNodes = mrmlScene->GetNodesByName(colorNodeName.c_str());
  if (colorNodes->GetNumberOfItems() == 0)
  {
    std::cerr << "Error: No color table found for structure set '" << parentContourHierarchyNode->GetName() << "'" << std::endl;
  }
  colorNodes->InitTraversal();
  colorNode = vtkMRMLColorTableNode::SafeDownCast(colorNodes->GetNextItemAsObject());
  int structureColorIndex = -1;
  while (colorNode)
  {
    int currentColorIndex = -1;
    if ((currentColorIndex = colorNode->GetColorIndexByName(contourNode->GetStructureName())) != -1)
    {
      if (referenceModelNode)
      {
        double modelColor[3];
        double foundColor[4];
        referenceModelNode->GetDisplayNode()->GetColor(modelColor);
        colorNode->GetColor(currentColorIndex, foundColor);
        if ((fabs(modelColor[0]-foundColor[0]) < EPSILON) && (fabs(modelColor[1]-foundColor[1]) < EPSILON) && (fabs(modelColor[2]-foundColor[2])) < EPSILON)
        {
          structureColorIndex = currentColorIndex;
          break;
        }
      }
      else
      {
        structureColorIndex = currentColorIndex;
        break;
      }
    }
    colorNode = vtkMRMLColorTableNode::SafeDownCast(colorNodes->GetNextItemAsObject());
  }

  colorNodes->Delete();

  if (structureColorIndex == -1)
  {
    std::cout << "No matching entry found in the color tables for structure '" << contourNode->GetStructureName() << "'" << std::endl;
    return;
  }

  colorIndex = structureColorIndex;
}
