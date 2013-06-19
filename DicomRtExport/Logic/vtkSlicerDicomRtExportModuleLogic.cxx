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

// ModuleTemplate includes
#include "vtkSlicerDicomRtExportModuleLogic.h"
#include "vtkSlicerDicomRtWriter.h"
#include "vtkMRMLContourNode.h"
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLDisplayableHierarchyNode.h>

// VTK includes
#include <vtkNew.h>
#include "vtkPolyData.h"
#include <vtkSmartPointer.h>

// ITK includes
#include "itkImage.h"

// STD includes
#include <cassert>

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
  // TODO:
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("SaveDicomRTStudy: MRMLScene not valid!")
    return;
  }

  vtkMRMLScalarVolumeNode* imageNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(imageNodeID));
  if (!imageNode)
  {
    vtkErrorMacro("SaveDicomRTStudy: Must set the primary CT/MR image!")
    return;
  }

  vtkMRMLScalarVolumeNode* doseNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(doseNodeID));
  vtkMRMLDisplayableHierarchyNode* contourHierarchyNode = vtkMRMLDisplayableHierarchyNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(contourHierarchyNodeID));
  if (!doseNode && !contourHierarchyNode)
  {
    vtkErrorMacro("SaveDicomRTStudy: Must set at least the dose or contours!")
    return;
  }

  vtkSmartPointer<vtkSlicerDicomRtWriter> rtWriter = vtkSmartPointer<vtkSlicerDicomRtWriter>::New();

  // Convert input images to the format Plastimatch can use
  itk::Image<short, 3>::Pointer itkImage = itk::Image<short, 3>::New();
  if (SlicerRtCommon::ConvertVolumeNodeToItkImage<short>(imageNode, itkImage) == false)
  {
    vtkErrorMacro("SaveDicomRTStudy: Failed to convert image volumeNode to ITK volume!");
    return;
  }
  rtWriter->SetImage(itkImage);

  if (doseNode)
  {
    itk::Image<float, 3>::Pointer itkDose = itk::Image<float, 3>::New();
    if (SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(doseNode, itkDose) == false)
    {
      vtkErrorMacro("SaveDicomRTStudy: Failed to convert dose volumeNode to ITK volume!");
      return;
    }
    rtWriter->SetDose(itkDose);
  }

  if (contourHierarchyNode)
  {
    vtkSmartPointer<vtkCollection> childContourNodes = vtkSmartPointer<vtkCollection>::New();
    contourHierarchyNode->GetChildrenDisplayableNodes(childContourNodes);
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
      vtkMRMLScalarVolumeNode* labelmapNode = contourNode->GetIndexedLabelmapVolumeNode();
      char *labelmapName = labelmapNode->GetName();
      double labelmapColor[4] = {0.0,0.0,0.0,1.0};
      labelmapNode->GetDisplayNode()->GetColor(labelmapColor);
      itk::Image<unsigned char, 3>::Pointer itkStructure = itk::Image<unsigned char, 3>::New();
      if (SlicerRtCommon::ConvertVolumeNodeToItkImage<unsigned char>(labelmapNode, itkStructure) == false)
      {
        vtkErrorMacro("SaveDicomRTStudy: Failed to convert contour labelmap volumeNode to ITK volume!");
        return;
      }
      rtWriter->AddContour(itkStructure, labelmapName, labelmapColor);
    }
  }

  rtWriter->SetFileName(currentOutputPath);
  rtWriter->Write();
}
