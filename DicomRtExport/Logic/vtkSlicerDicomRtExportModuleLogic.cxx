/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, RMP, Princess Margaret Hospital
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// DicomRtExport includes
#include "vtkSlicerDicomRtExportModuleLogic.h"
#include "vtkSlicerDicomRtWriter.h"

// SlicerRT includes
#include "PlmCommon.h"
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerContoursModuleLogic.h"

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
void vtkSlicerDicomRtExportModuleLogic::SaveDicomRTStudy(
  const char* patientName,
  const char* patientID,
  const char* imageNodeID, 
  const char* doseNodeID,
  const char* contourHierarchyNodeID, 
  const char* currentOutputPath)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("SaveDicomRTStudy: MRML scene not valid!")
      return;
  }

  // Get nodes for input volumes
  vtkMRMLScalarVolumeNode* imageNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(imageNodeID));
  vtkMRMLScalarVolumeNode* doseNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(doseNodeID));
  vtkMRMLSubjectHierarchyNode* contourHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(contourHierarchyNodeID));

  // Make sure there is an image node.  Don't check for struct / dose, 
  // as those are optional
  if (!imageNode)
  {
    vtkErrorMacro("SaveDicomRTStudy: Must set the primary CT/MR image!")
      return;
  }

  vtkSmartPointer<vtkSlicerDicomRtWriter> rtWriter = vtkSmartPointer<vtkSlicerDicomRtWriter>::New();

  // Set metadata
  rtWriter->SetPatientName (patientName);
  rtWriter->SetPatientID (patientID);

  // Convert input image (CT/MR/etc) to the format Plastimatch can use
  Plm_image::Pointer plm_img = PlmCommon::ConvertVolumeNodeToPlmImage(
    imageNode);
  plm_img->print ();
  rtWriter->SetImage(plm_img);

  // Convert input RTDose to the format Plastimatch can use
  if (doseNode)
  {
    Plm_image::Pointer dose_img = PlmCommon::ConvertVolumeNodeToPlmImage(
      doseNode);
    dose_img->print ();
    rtWriter->SetDose(dose_img);
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
      if (!contourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap))
      {
        contourNode->SetAndObserveRasterizationReferenceVolumeNodeId(imageNodeID);
        contourNode->SetRasterizationOversamplingFactor(1.0);
      }

      if (!contourNode->GetLabelmapImageData())
      {
        vtkErrorMacro("SaveDicomRTStudy: Failed to get indexed labelmap representation from contours");
        return;
      }

      // Make sure the labelmap dimensions match the reference dimensions
      // (the conversion sometimes adds padding to the labelmap to ensure closed surface models)
      int referenceExtents[6] = {0, 0, 0, 0, 0, 0};
      imageNode->GetImageData()->GetExtent(referenceExtents);
      int labelmapExtents[6] = {0, 0, 0, 0, 0, 0};
      contourNode->GetLabelmapImageData()->GetExtent(labelmapExtents);
      if (!SlicerRtCommon::AreBoundsEqual(referenceExtents, labelmapExtents))
      {
        vtkMRMLContourNode::ResampleInputContourNodeToReferenceVolumeNode(this->GetMRMLScene(), contourNode, imageNode, contourNode);
      }

      // TODO : if contours in the core, and plastimatch updated to use contours, remove this
      vtkMRMLScalarVolumeNode* volumeNode = vtkSlicerContoursModuleLogic::ExtractLabelmapFromContour(contourNode);
      Plm_image::Pointer plmStructure = PlmCommon::ConvertVolumeNodeToPlmImage(volumeNode);
      this->GetMRMLScene()->RemoveNode(volumeNode);

      char *labelmapName = contourNode->GetName();
      double labelmapColor[4] = {1.0,0.0,0.0,1.0};
      // TODO : when contour 2d vis is re-added, reconnect this functionality
      int colorIndex;
      vtkMRMLColorTableNode *colorNode;
      contourNode->GetColor(colorIndex,colorNode);
      printf (">>> labelmapName = %s\n", labelmapName);
      printf (">>> colorIndex = %d\n", colorIndex);
      printf (">>> colorNode = %p\n", colorNode);
      if (colorNode) {
        printf (">>> colorNodeType = %d\n", colorNode->GetType());
        printf (">>> colorNodeString = %s\n", colorNode->GetTypeAsString());
        colorNode->GetColor (1, labelmapColor);
        printf (">>> color = %g %g %g %g\n", labelmapColor[0],
          labelmapColor[1], labelmapColor[2], labelmapColor[3]);
      }

      // If no color is assigned to the labelmap node, use the default color table node
      if (labelmapColor[0] == 0.0 && labelmapColor[1] == 0.0 && labelmapColor[2] == 0.0)
      {
        vtkMRMLColorTableNode* defaultTableNode = vtkMRMLColorTableNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID("vtkMRMLColorTableNodeLabels"));
        defaultTableNode->GetColor(i+1, labelmapColor);
      }
      rtWriter->AddContour(plmStructure->itk_uchar(), labelmapName, labelmapColor);
    }
  }

  rtWriter->SetFileName(currentOutputPath);
  rtWriter->Write();
}
