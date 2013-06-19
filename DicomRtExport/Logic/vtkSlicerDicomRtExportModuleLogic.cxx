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
int vtkSlicerDicomRtExportModuleLogic::SaveDicomRTStudy(char *imageNodeID, char *doseNodeID, char *contourHierarchyNodeID, char *currentOutputPath)
{
  // to do ...
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("Dicom RT Export: MRMLScene not valid!")
    return -1;
  }

  vtkMRMLScalarVolumeNode* imageNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(imageNodeID));
  if (!imageNode)
  {
    vtkErrorMacro("Dicom RT Export: Must set the primary CT/MR image!")
    return -1;
  }

  vtkMRMLScalarVolumeNode* doseNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(doseNodeID));
  vtkMRMLDisplayableHierarchyNode* contourHierarchyNode = vtkMRMLDisplayableHierarchyNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(contourHierarchyNodeID));
  if (!doseNode && !contourHierarchyNode)
  {
    vtkErrorMacro("Dicom RT Export: Must set at least the dose or contours!")
    return -1;
  }

  vtkSmartPointer<vtkSlicerDicomRtWriter> rtWriter = vtkSmartPointer<vtkSlicerDicomRtWriter>::New();

  // Convert input images to the format Plastimatch can use
  itk::Image<short, 3>::Pointer itkImage = itk::Image<short, 3>::New();
  if (SlicerRtCommon::ConvertVolumeNodeToItkImage2<short>(imageNode, itkImage, false) == false)
  {
    vtkErrorMacro(<<"Dicom RT Export: Failed to convert image volumeNode to ITK volume!");
    return -1;
  }
  rtWriter->SetImage(itkImage);

  if (doseNode)
  {
    itk::Image<float, 3>::Pointer itkDose = itk::Image<float, 3>::New();
    if (SlicerRtCommon::ConvertVolumeNodeToItkImage2<float>(doseNode, itkDose, false) == false)
    {
      vtkErrorMacro(<<"Dicom RT Export: Failed to convert dose volumeNode to ITK volume!");
      return -1;
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
      vtkDebugWithObjectMacro(contourHierarchyNode, "Dicom RT Export: Selected contour hierarchy node has no children contour nodes!");
      return -1;
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
      if (SlicerRtCommon::ConvertVolumeNodeToItkImage2<unsigned char>(labelmapNode, itkStructure, false) == false)
      {
        vtkErrorMacro(<<"Dicom RT Export: Failed to convert contour labelmap volumeNode to ITK volume!");
        return -1;
      }
      rtWriter->AddContour(itkStructure, labelmapName, labelmapColor);
    }
  }

  rtWriter->SetFileName(currentOutputPath);
  rtWriter->Write();
}
