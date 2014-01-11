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

==============================================================================*/

// DicomRtExport includes
#include "vtkSlicerDicomRtExportModuleLogic.h"
#include "vtkSlicerDicomRtWriter.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkVolumesOrientedResampleUtility.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLColorTableNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkImageShiftScale.h>
#include <vtkObjectFactory.h>

// ITK includes
#include <itkImage.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomRtExportModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDicomRtExportModuleLogic::vtkSlicerDicomRtExportModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtExportModuleLogic::~vtkSlicerDicomRtExportModuleLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtExportModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtExportModuleLogic::SaveDicomRTStudy(const char* imageNodeID, const char* doseNodeID, const char* contourHierarchyNodeID, const char* currentOutputPath)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("SaveDicomRTStudy: MRML scene not valid!")
    return;
  }

  vtkMRMLScalarVolumeNode* imageNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(imageNodeID));
  if (!imageNode)
  {
    vtkErrorMacro("SaveDicomRTStudy: Must set the primary CT/MR image!")
    return;
  }

  // Cast the input CT/MR image to Short in case it is read in as Int
  vtkImageData* imageData = imageNode->GetImageData();
  vtkSmartPointer<vtkImageShiftScale> caster = vtkSmartPointer<vtkImageShiftScale>::New();
  caster->SetInput(imageData);
  caster->SetShift(0);
  caster->SetScale(1);
  caster->SetOutputScalarTypeToShort();
  caster->Update();

  vtkSmartPointer<vtkImageData> newImageData = caster->GetOutput();
  imageNode->SetAndObserveImageData(newImageData);

  // check if there is at least one RTDose or RTSTRUCT is included
  vtkMRMLScalarVolumeNode* doseNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(doseNodeID));
  vtkMRMLSubjectHierarchyNode* contourHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(contourHierarchyNodeID));
  if (!doseNode && !contourHierarchyNode)
  {
    vtkErrorMacro("SaveDicomRTStudy: Must set at least the dose or contours!")
    return;
  }

  vtkSmartPointer<vtkSlicerDicomRtWriter> rtWriter = vtkSmartPointer<vtkSlicerDicomRtWriter>::New();

  // Convert input CT/MR image to the format Plastimatch can use
  itk::Image<short, 3>::Pointer itkImage = itk::Image<short, 3>::New();
  if (SlicerRtCommon::ConvertVolumeNodeToItkImage<short>(imageNode, itkImage, true) == false)
  {
    vtkErrorMacro("SaveDicomRTStudy: Failed to convert image volumeNode to ITK volume!");
    return;
  }
  rtWriter->SetImage(itkImage);

  // Convert input RTDose to the format Plastimatch can use
  if (doseNode)
  {
    itk::Image<float, 3>::Pointer itkDose = itk::Image<float, 3>::New();
    if (SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(doseNode, itkDose, true) == false)
    {
      vtkErrorMacro("SaveDicomRTStudy: Failed to convert dose volumeNode to ITK volume!");
      return;
    }
    rtWriter->SetDose(itkDose);
  }

  // Convert input contours to the format Plastimatch can use
  if (contourHierarchyNode)
  {
    vtkSmartPointer<vtkCollection> childContourNodes = vtkSmartPointer<vtkCollection>::New();
    contourHierarchyNode->GetAssociatedChildrenNodes(childContourNodes);
    childContourNodes->InitTraversal();
    if (childContourNodes->GetNumberOfItems() < 1)
    {
      vtkDebugMacro("SaveDicomRTStudy: Selected contour hierarchy node has no children contour nodes!");
      return;
    }

    // Collect contour nodes in the hierarchy and determine whether their active representation types are the same
    for (int i=0; i<childContourNodes->GetNumberOfItems(); ++i)
    {
      vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(childContourNodes->GetItemAsObject(i));
      if (!contourNode)
      {
        continue; // There is a color table in the contour hierarchy
      }
      if (contourNode->GetIndexedLabelmapVolumeNodeId() == NULL)
      {
        contourNode->SetAndObserveRasterizationReferenceVolumeNodeId(imageNodeID);
        contourNode->SetRasterizationOversamplingFactor(1.0);
      }
      vtkMRMLScalarVolumeNode* labelmapNode = contourNode->GetIndexedLabelmapVolumeNode();
      if (!labelmapNode)
      {
        vtkErrorMacro("SaveDicomRTStudy: Failed to get indexed labelmap representation from contours");
        return;
      }

      // Make sure the labelmap dimensions match the reference dimensions
      // (the conversion sometimes adds padding to the labelmap to ensure closed surface models)
      int referenceExtents[6] = {0, 0, 0, 0, 0, 0};
      imageNode->GetImageData()->GetExtent(referenceExtents);
      int labelmapExtents[6] = {0, 0, 0, 0, 0, 0};
      labelmapNode->GetImageData()->GetExtent(labelmapExtents);
      if (!SlicerRtCommon::AreBoundsEqual(referenceExtents, labelmapExtents))
      {
        vtkVolumesOrientedResampleUtility::ResampleInputVolumeNodeToReferenceVolumeNode(labelmapNode, imageNode, labelmapNode);
      }

      itk::Image<unsigned char, 3>::Pointer itkStructure = itk::Image<unsigned char, 3>::New();
      if (SlicerRtCommon::ConvertVolumeNodeToItkImage<unsigned char>(labelmapNode, itkStructure, true) == false)
      {
        vtkErrorMacro("SaveDicomRTStudy: Failed to convert contour labelmap volumeNode to ITK volume!");
        return;
      }
      char *labelmapName = labelmapNode->GetName();
      double labelmapColor[4] = {0.0,0.0,0.0,1.0};
      labelmapNode->GetDisplayNode()->GetColor(labelmapColor);

	    // If no color is assigned to the labelmap node, use the default color table node
	    if (labelmapColor[0] == 0.0 && labelmapColor[1] == 0.0 && labelmapColor[2] == 0.0)
	    {
	      vtkMRMLColorTableNode* defaultTableNode = vtkMRMLColorTableNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID("vtkMRMLColorTableNodeLabels"));
		    defaultTableNode->GetColor(i+1, labelmapColor);
	    }

      rtWriter->AddContour(itkStructure, labelmapName, labelmapColor);
    }
  }

  rtWriter->SetFileName(currentOutputPath);
  rtWriter->Write();
}
