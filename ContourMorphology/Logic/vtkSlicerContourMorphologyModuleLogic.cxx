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

// ContourMorpholgyModule Logic includes
#include "vtkSlicerContourMorphologyModuleLogic.h"
#include "vtkMRMLContourMorphologyNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkSlicerContoursModuleLogic.h"
#include "vtkVolumesOrientedResampleUtility.h"

// Core includes
#include <qSlicerSubjectHierarchyAbstractPlugin.h>
#include <qSlicerSubjectHierarchyPluginHandler.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// MRML includes
#include <vtkMRMLContourNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVolumeDisplayNode.h>

// VTK includes
#include <vtkColorTransferFunction.h>
#include <vtkDecimatePro.h>
#include <vtkGeneralTransform.h>
#include <vtkImageAccumulate.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageContinuousDilate3D.h>
#include <vtkImageContinuousErode3D.h>
#include <vtkImageData.h>
#include <vtkImageLogic.h>
#include <vtkImageMarchingCubes.h>
#include <vtkImageReslice.h>
#include <vtkLookupTable.h>
#include <vtkLookupTable.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataNormals.h>
#include <vtkSmartPointer.h>
#include <vtkTriangleFilter.h>
#include <vtkWindowedSincPolyDataFilter.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerContourMorphologyModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerContourMorphologyModuleLogic::vtkSlicerContourMorphologyModuleLogic()
{
  this->ContourMorphologyNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerContourMorphologyModuleLogic::~vtkSlicerContourMorphologyModuleLogic()
{
  this->SetAndObserveContourMorphologyNode(NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::SetAndObserveContourMorphologyNode(vtkMRMLContourMorphologyNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->ContourMorphologyNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourMorphologyNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene!");
    return;
  }

  if (!node || !this->ContourMorphologyNode)
  {
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLContourNode") || node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLContourMorphologyNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeRemoved: Invalid MRML scene!");
    return;
  }

  if (!node || !this->ContourMorphologyNode)
  {
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLContourNode") || node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLContourMorphologyNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLContourMorphologyNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLContourMorphologyNode");
  if (node)
  {
    paramNode = vtkMRMLContourMorphologyNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->ContourMorphologyNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene or input node!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
int vtkSlicerContourMorphologyModuleLogic::SetContourARepresentationToLabelmap()
{
  if (!this->GetMRMLScene() || !this->ContourMorphologyNode)
  {
    vtkErrorMacro("SetContourARepresentationToLabelmap: Invalid MRML scene or parameter set node!");
    return -1;
  }

  vtkMRMLContourNode* inputContourANode = this->ContourMorphologyNode->GetContourANode();

  vtkImageData* inputContourALabelmapData(NULL);
  if ( !inputContourANode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap) )
  {
    vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ContourMorphologyNode->GetReferenceVolumeNode();
    if (!referenceVolumeNode)
    {
      vtkErrorMacro("SetContourARepresentationToLabelmap: Reference Volume is not initialized!")
        return -1;
    }
    inputContourANode->SetAndObserveRasterizationReferenceVolumeNodeId(
      this->ContourMorphologyNode->GetReferenceVolumeNode()->GetID() );

    inputContourANode->SetRasterizationOversamplingFactor(1.0);
  }

  inputContourALabelmapData = inputContourANode->GetLabelmapImageData();
  

  if (!inputContourALabelmapData)
  {
    vtkErrorMacro("SetContourARepresentationToLabelmap: Failed to get indexed labelmap representation from selected contours");
    return -1;
  }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSlicerContourMorphologyModuleLogic::SetContourBRepresentationToLabelmap()
{
  if (!this->GetMRMLScene() || !this->ContourMorphologyNode)
  {
    vtkErrorMacro("SetContourBRepresentationToLabelmap: Invalid MRML scene or parameter set node!");
    return -1;
  }

  vtkMRMLContourNode* inputContourBNode = this->ContourMorphologyNode->GetContourBNode();

  if (!inputContourBNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap))
  {
    vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ContourMorphologyNode->GetReferenceVolumeNode();
    if (!referenceVolumeNode)
    {
      vtkErrorMacro("SetContourBRepresentationToLabelmap: Reference Volume is not initialized!")
        return -1;
    }
    inputContourBNode->SetAndObserveRasterizationReferenceVolumeNodeId(
      this->ContourMorphologyNode->GetReferenceVolumeNode()->GetID() );

      inputContourBNode->SetRasterizationOversamplingFactor(1.0);
  }

  vtkImageData* inputContourBLabelmapData = inputContourBNode->GetLabelmapImageData();

  if (!inputContourBLabelmapData)
  {
    vtkErrorMacro("SetContourBRepresentationToLabelmap: Failed to get indexed labelmap representation from selected contours");
    return -1;
  }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSlicerContourMorphologyModuleLogic::MorphContour()
{
  const int ERROR_RETURN(-1);
  int dimensions[3] = {0, 0, 0};
  double spacingX, spacingY, spacingZ;

  vtkMRMLContourNode* inputContourANode = this->ContourMorphologyNode->GetContourANode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ContourMorphologyNode->GetReferenceVolumeNode();
  vtkMRMLContourNode* outputContourNode = this->ContourMorphologyNode->GetOutputContourNode();

  vtkMRMLContourMorphologyNode::ContourMorphologyOperationType operation = this->ContourMorphologyNode->GetOperation();

  // Make sure inputs are initialized
  if (!this->GetMRMLScene() || !inputContourANode || !referenceVolumeNode )
  {
    vtkErrorMacro("MorphContour: Inputs are not specified!");
    return ERROR_RETURN;
  }

  vtkSmartPointer<vtkMRMLContourNode> tempContourNodeB= vtkSmartPointer<vtkMRMLContourNode>::New();
  vtkMRMLContourNode* inputContourBNode = this->ContourMorphologyNode->GetContourBNode();
  vtkImageData* tempImageB = NULL;
  if (operation == vtkMRMLContourMorphologyNode::Union || operation == vtkMRMLContourMorphologyNode::Intersect || operation == vtkMRMLContourMorphologyNode::Subtract) 
  {
    if (!inputContourBNode)
    {
      vtkErrorMacro("MorphContour: Inputs are not initialized!");
      return ERROR_RETURN;
    }
    if (this->SetContourBRepresentationToLabelmap() != 0)
    {
      vtkErrorMacro("MorphContour: Failed to set contour B representation to labelmap!");
      return ERROR_RETURN;
    }

    vtkMRMLContourNode::ResampleInputContourNodeToReferenceVolumeNode(this->GetMRMLScene(), inputContourBNode, referenceVolumeNode, tempContourNodeB);
    tempImageB = tempContourNodeB->GetLabelmapImageData();
  }

  if (this->SetContourARepresentationToLabelmap() != 0)
  {
    vtkErrorMacro("MorphContour: Failed to set contour A representation to labelmap!");
    return ERROR_RETURN;
  }
  inputContourANode->GetSpacing(spacingX, spacingY, spacingZ);

  int kernelSize[3] = {1,1,1};
  kernelSize[0] = (int)( 2*(this->GetContourMorphologyNode()->GetXSize()/spacingX + 0.5) );
  kernelSize[1] = (int)( 2*(this->GetContourMorphologyNode()->GetYSize()/spacingY + 0.5) );
  kernelSize[2] = (int)( 2*(this->GetContourMorphologyNode()->GetZSize()/spacingZ + 0.5) );

  bool createdNewContour(false);
  if( outputContourNode == NULL )
  {
    createdNewContour = true;
    // Create new contour selected, let's create a descriptive name for the new contour
    std::string newContourName;
    switch(operation)
    {
    case vtkMRMLContourMorphologyNode::Expand:
      {
        std::stringstream ss;
        ss << "Expanded_" << kernelSize[0] << "_" << kernelSize[1] << "_" << kernelSize[2] << "_" << inputContourANode->GetName();
        newContourName = ss.str();
        break;
      }
    case vtkMRMLContourMorphologyNode::Shrink:
      {
        std::stringstream ss;
        ss << "Shrunk_" << kernelSize[0] << "_" << kernelSize[1] << "_" << kernelSize[2] << "_" << inputContourANode->GetName();
        newContourName = ss.str();
        break;
      }
    case vtkMRMLContourMorphologyNode::Intersect:
      {
        std::stringstream ss;
        ss << inputContourANode->GetName() << std::string("_Intersect_") << inputContourBNode->GetName();
        newContourName = ss.str();
        break;
      }
    case vtkMRMLContourMorphologyNode::Union:
      {
        std::stringstream ss;
        ss << inputContourANode->GetName() << std::string("_Union_") << inputContourBNode->GetName();
        newContourName = ss.str();
        break;
      }
    case vtkMRMLContourMorphologyNode::Subtract:
      {
        std::stringstream ss;
        ss << inputContourANode->GetName() << std::string("_Subtract_") << inputContourBNode->GetName();
        newContourName = ss.str();
        break;
      }
    default:
      {
        vtkErrorMacro("Unknown operation detected in vtkSlicerContourMorphologyModuleLogic::MorphContour().");
        return ERROR_RETURN;
      }
    }

    // Let's do that, essentially copy inputContourA settings (size, dimensions, reference volume, etc...)
    outputContourNode = vtkSlicerContoursModuleLogic::CreateEmptyContourFromExistingContour(inputContourANode, newContourName);
    
    // This creates the contour, now let's link it to the SH properly
    vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(inputContourANode));
    if( shNode )
    {
      vtkMRMLSubjectHierarchyNode* parentNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(shNode->GetParentNode());
      if( parentNode )
      {
        qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("ContourSets")->addNodeToSubjectHierarchy( outputContourNode, parentNode );
      }
    }    
  }

  vtkSmartPointer<vtkMRMLContourNode> tempContourNodeA = vtkSmartPointer<vtkMRMLContourNode>::New();
  vtkMRMLContourNode::ResampleInputContourNodeToReferenceVolumeNode(this->GetMRMLScene(), inputContourANode, referenceVolumeNode, tempContourNodeA);
  vtkImageData* tempImageA = tempContourNodeA->GetLabelmapImageData();

  referenceVolumeNode->GetImageData()->GetDimensions(dimensions);

  vtkSmartPointer<vtkImageAccumulate> histogram = vtkSmartPointer<vtkImageAccumulate>::New();
  histogram->SetInput(tempImageA);
  histogram->Update();
  double valueMax = histogram->GetMax()[0];

  vtkSmartPointer<vtkImageData> tempImageOutput = NULL;
  switch (operation) 
  {
  case vtkMRMLContourMorphologyNode::Expand:
    {
      vtkSmartPointer<vtkImageContinuousDilate3D> dilateFilter = vtkSmartPointer<vtkImageContinuousDilate3D>::New();
      dilateFilter->SetInput(tempImageA);
      dilateFilter->SetKernelSize(kernelSize[0], kernelSize[1], kernelSize[2]);
      dilateFilter->Update();
      tempImageOutput = dilateFilter->GetOutput();
      break;
    }
  case vtkMRMLContourMorphologyNode::Shrink:
    {
      vtkSmartPointer<vtkImageContinuousErode3D> erodeFilter = vtkSmartPointer<vtkImageContinuousErode3D>::New();
      erodeFilter->SetInput(tempImageA);
      erodeFilter->SetKernelSize(kernelSize[0], kernelSize[1], kernelSize[2]);
      erodeFilter->Update();
      tempImageOutput = erodeFilter->GetOutput();
    }
    break;
  case vtkMRMLContourMorphologyNode::Union:
    {
      vtkSmartPointer<vtkImageLogic> logicFilter = vtkSmartPointer<vtkImageLogic>::New();
      logicFilter->SetInput1(tempImageA);
      logicFilter->SetInput2(tempImageB);
      logicFilter->SetOperationToOr();
      logicFilter->SetOutputTrueValue(valueMax);
      logicFilter->Update();
      tempImageOutput = logicFilter->GetOutput();
    }
    break;
  case vtkMRMLContourMorphologyNode::Intersect:
    {
      vtkSmartPointer<vtkImageLogic> logicFilter = vtkSmartPointer<vtkImageLogic>::New();
      logicFilter->SetInput1(tempImageA);
      logicFilter->SetInput2(tempImageB);
      logicFilter->SetOperationToAnd();
      logicFilter->SetOutputTrueValue(valueMax);
      logicFilter->Update();
      tempImageOutput = logicFilter->GetOutput();
    }
    break;
  case vtkMRMLContourMorphologyNode::Subtract:
    {
      vtkSmartPointer<vtkImageLogic> logicFilter = vtkSmartPointer<vtkImageLogic>::New();
      logicFilter->SetInput1(tempImageB);
      logicFilter->SetOperationToNot();
      logicFilter->SetOutputTrueValue(valueMax);
      logicFilter->Update();

      vtkSmartPointer<vtkImageLogic> logicFilter2 = vtkSmartPointer<vtkImageLogic>::New();
      logicFilter2->SetInput1(tempImageA);
      logicFilter2->SetInput2(logicFilter->GetOutput());
      logicFilter2->SetOperationToAnd();
      logicFilter2->SetOutputTrueValue(valueMax);
      logicFilter2->Update();
      tempImageOutput = logicFilter2->GetOutput();
    }
    break;
  default:
    vtkErrorMacro("MorphContour: Invalid operation!")
      break;
  }

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  outputContourNode->SetAndObserveLabelmapImageData(tempImageOutput);

  // TODO : 2d vis readdition
  //outputContourNode->GetLabelmapVolumeDisplayNode()->SetInputImageData(tempImageOutput);
  //if (this->GetMRMLScene()->GetNodeByID("vtkMRMLColorTableNodeLabels") != NULL)
  //{
    // Refresh the color table link
    //    outputContourNode->GetLabelmapVolumeDisplayNode()->SetAndObserveColorNodeID("vtkMRMLColorTableNodeLabels");
  //}
  // Show the labelmap if hidden
  //outputContourNode->GetLabelmapVolumeDisplayNode()->SetVisibility(1);

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 

  return 0;
}
