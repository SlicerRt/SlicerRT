/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkVolumesOrientedResampleUtility.cxx,v $
  Date:      $Date: 2006/01/06 17:56:48 $
  Version:   $Revision: 1.58 $

=========================================================================auto=*/

#include "vtkVolumesOrientedResampleUtility.h"

// MRML nodes includes
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLTransformNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkImageReslice.h>
#include <vtkObjectFactory.h>
#include <vtkTransform.h>

//----------------------------------------------------------------------------
// vtkVolumesOrientedResampleUtility methods

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVolumesOrientedResampleUtility);

//----------------------------------------------------------------------------
vtkVolumesOrientedResampleUtility::vtkVolumesOrientedResampleUtility()
{
}

//----------------------------------------------------------------------------
vtkVolumesOrientedResampleUtility::~vtkVolumesOrientedResampleUtility()
{
}

//----------------------------------------------------------------------------
bool vtkVolumesOrientedResampleUtility
::ResampleInputVolumeNodeToReferenceVolumeNode(vtkMRMLVolumeNode* inVolumeNode, 
                                               vtkMRMLVolumeNode* refVolumeNode, 
                                               vtkMRMLVolumeNode* outVolumeNode)
{
  int dimensions[3] = {0, 0, 0};

  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(refVolumeNode);
  vtkMRMLScalarVolumeNode* inputVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(inVolumeNode);
  vtkMRMLScalarVolumeNode* outputVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(outVolumeNode);
  // Make sure inputs are initialized
  if (!inputVolumeNode || !referenceVolumeNode || !outputVolumeNode)
  {
    // TODO: error message
    return false;
  }
  referenceVolumeNode->GetImageData()->GetDimensions(dimensions);

  vtkSmartPointer<vtkMatrix4x4> inputVolumeIJK2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inputVolumeNode->GetIJKToRASMatrix(inputVolumeIJK2RASMatrix);
  vtkSmartPointer<vtkMatrix4x4> referenceVolumeRAS2IJKMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  referenceVolumeNode->GetRASToIJKMatrix(referenceVolumeRAS2IJKMatrix);

  vtkSmartPointer<vtkTransform> outputResliceTransform = vtkSmartPointer<vtkTransform>::New();
  outputResliceTransform->Identity();
  outputResliceTransform->PostMultiply();
  outputResliceTransform->SetMatrix(inputVolumeIJK2RASMatrix);

  vtkSmartPointer<vtkMRMLTransformNode> inputVolumeNodeTransformNode = vtkMRMLTransformNode::SafeDownCast(
    referenceVolumeNode->GetScene()->GetNodeByID(inputVolumeNode->GetTransformNodeID()));
  vtkSmartPointer<vtkMatrix4x4> inputVolumeRAS2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  if (inputVolumeNodeTransformNode!=NULL)
  {
    inputVolumeNodeTransformNode->GetMatrixTransformToWorld(inputVolumeRAS2RASMatrix);  
    outputResliceTransform->Concatenate(inputVolumeRAS2RASMatrix);
  }
  outputResliceTransform->Concatenate(referenceVolumeRAS2IJKMatrix);
  outputResliceTransform->Inverse();

  vtkSmartPointer<vtkImageReslice> reslice = vtkSmartPointer<vtkImageReslice>::New();
  reslice->SetInput(inputVolumeNode->GetImageData());
  reslice->SetOutputOrigin(0, 0, 0);
  reslice->SetOutputSpacing(1, 1, 1);
  reslice->SetOutputExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  reslice->SetResliceTransform(outputResliceTransform);
  reslice->Update();

  outputVolumeNode->CopyOrientation(referenceVolumeNode);
  outputVolumeNode->SetAndObserveImageData(reslice->GetOutput());
  
  return true;
}

//----------------------------------------------------------------------------
void vtkVolumesOrientedResampleUtility::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


