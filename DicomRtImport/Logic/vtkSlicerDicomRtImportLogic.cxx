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
#include "vtkSlicerDicomRtImportLogic.h"
#include "vtkSlicerDicomRtImportReader.h"

// Slicer includes
#include "vtkSlicerVolumesLogic.h"

// MRML includes
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLAnnotationHierarchyNode.h>
#include <vtkMRMLAnnotationPointDisplayNode.h>
#include <vtkMRMLAnnotationFiducialNode.h>
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkNew.h>
#include "vtkPolyData.h"
#include "vtkPolyDataWriter.h"
#include <vtkSmartPointer.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomRtImportLogic);
vtkCxxSetObjectMacro(vtkSlicerDicomRtImportLogic, VolumesLogic, vtkSlicerVolumesLogic);

//----------------------------------------------------------------------------
vtkSlicerDicomRtImportLogic::vtkSlicerDicomRtImportLogic()
{
  this->VolumesLogic = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtImportLogic::~vtkSlicerDicomRtImportLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtImportLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportLogic::InitializeEventListeners()
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(this->GetMRMLScene(), events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerDicomRtImportLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerDicomRtImportLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
bool vtkSlicerDicomRtImportLogic
::LoadDicomRT(const char *name)
{
	vtkSmartPointer<vtkSlicerDicomRtImportReader> rtReader = vtkSmartPointer<vtkSlicerDicomRtImportReader>::New();
	rtReader->SetFileName(name);
	rtReader->Update();

	int numberOfROI = rtReader->GetNumberOfROI();

	if (!rtReader->GetReadSuccessful())
	{
		return false;
	}

	//vtkSmartPointer<vtkMRMLFiducialListNode> roiPoints = vtkSmartPointer<vtkMRMLFiducialListNode>::New();  
	//roiPoints->SetLocked(true);
	//roiPoints->SetName("TestRtPointSet");
	//roiPoints->SetDescription("RT ROI points");
	//roiPoints->SetColor(1.0,1.0,0);
	//roiPoints->SetSelectedColor(1.0, 0.0, 0.0);
	//roiPoints->SetGlyphType(vtkMRMLFiducialListNode::Sphere3D);
	//roiPoints->SetOpacity(0.7);
	//roiPoints->SetAllFiducialsVisibility(true);
	//roiPoints->SetSymbolScale(5);
	//roiPoints->SetTextScale(5);    
	//this->GetMRMLScene()->AddNode(roiPoints); 

	//int roiPointsModifyOld = roiPoints->StartModify();

	for (int i=0;i<numberOfROI; i++)
	{
		vtkPolyData* poly = rtReader->GetROI(i+1);

    if (poly == NULL)
    {
      continue;
    }

		//vtkPolyDataWriter *polyWriter = vtkPolyDataWriter::New();
		//polyWriter->SetFileName("\\test1.vtk");
		//polyWriter->SetInput(poly);
		//polyWriter->Write();

		//polyWriter->Delete();
		if (poly->GetNumberOfPoints() == 1)
		{
			double p[3];
			double *point_LPS = p;
			point_LPS = poly->GetPoint(0);

			vtkMRMLAnnotationFiducialNode* fnode = vtkMRMLAnnotationFiducialNode::New();
			//vtkSmartPointer<vtkMRMLAnnotationHierarchyNode> hnode = vtkSmartPointer<vtkMRMLAnnotationHierarchyNode>::New();
			//hnode->SetScene(this->GetMRMLScene());
			fnode->SetName(rtReader->GetROIName(i+1)); 			
			fnode->SetScene(this->GetMRMLScene());

			double coord[3] = {-point_LPS[0], -point_LPS[1], point_LPS[2]};
			fnode->AddControlPoint(coord, 0, 1);
			//fnode->CreateAnnotationPointDisplayNode();
			//this->GetMRMLScene()->AddNode(hnode);
			this->GetMRMLScene()->AddNode(fnode);

			vtkMRMLAnnotationPointDisplayNode* dnode = vtkMRMLAnnotationPointDisplayNode::New();
			dnode->SetScene(this->GetMRMLScene());
			dnode->SetGlyphScale(1);
			dnode->SetGlyphType(1);
			this->GetMRMLScene()->AddNode(dnode);
			fnode->SetAndObserveDisplayNodeID(dnode->GetID());
			fnode->SetHideFromEditors(0);
			fnode->SetSelectable(1);

			//int fiducialIndex = roiPoints->AddFiducialWithXYZ(-point_LPS[0], -point_LPS[1], point_LPS[2], false);
			//roiPoints->SetNthFiducialLabelText(fiducialIndex, rtReader->GetROIName(i+1));
			//roiPoints->SetNthFiducialID(fiducialIndex, rtReader->GetROIName(i+1));
			//roiPoints->SetNthFiducialVisibility(fiducialIndex, true);    			

			/*
			vtkMRMLColorTableNode* cnode = 0;
			if (node.contains("color"))
			{
			cnode = vtkMRMLColorTableNode::SafeDownCast(
			q->mrmlScene()->GetNodeByID("vtkMRMLColorTableNodeSPLBrainAtlas"));
			Q_ASSERT(cnode);
			for (int i = 0; i < cnode->GetNumberOfColors(); ++i)
			{
			if (cnode->GetColorName(i) == node["color"])
			{
			dnode->SetColor(cnode->GetLookupTable()->GetTableValue(i));
			}
			}
			}
			*/

			//roiPoints = vtkMRMLFiducialListNode::SafeDownCast(
			//	this->GetMRMLScene()->AddNode(roiPoints));
			////Q_ASSERT(dnode);

			
			//if (!this->ParentID.isEmpty())
			//{
			//	hnode->SetParentNodeID(this->ParentID.toLatin1().data());
			//}

			//this->ParentID = hnode->GetID(); 
		}
		else if (poly->GetNumberOfPoints() > 1)
		{
			vtkSmartPointer<vtkMRMLModelNode> hnode =
				vtkSmartPointer<vtkMRMLModelNode>::New();
			vtkSmartPointer<vtkMRMLModelDisplayNode> dnode =
				vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
			hnode->SetScene(this->GetMRMLScene());
			dnode->SetScene(this->GetMRMLScene()); 
			//dnode->SetVisibility(1);
			hnode->SetAndObservePolyData(poly);
			hnode->SetName(rtReader->GetROIName(i+1));

			//vtkMRMLColorTableNode* cnode = 0;
			/*
			vtkMRMLColorTableNode* cnode = 0;
			if (node.contains("color"))
			{
			cnode = vtkMRMLColorTableNode::SafeDownCast(
			q->mrmlScene()->GetNodeByID("vtkMRMLColorTableNodeSPLBrainAtlas"));
			Q_ASSERT(cnode);
			for (int i = 0; i < cnode->GetNumberOfColors(); ++i)
			{
			if (cnode->GetColorName(i) == node["color"])
			{
			dnode->SetColor(cnode->GetLookupTable()->GetTableValue(i));
			}
			}
			}
			*/
			double *color = rtReader->GetROIDisplayColor(i+1);
			dnode->SetColor(color[0], color[1], color[2]);
			// disable backface culling so the contour can be seen correctly.
			dnode->SetBackfaceCulling(0);

			dnode = vtkMRMLModelDisplayNode::SafeDownCast(
				this->GetMRMLScene()->AddNode(dnode));
			//Q_ASSERT(dnode);
			hnode = vtkMRMLModelNode::SafeDownCast(
				this->GetMRMLScene()->AddNode(hnode));
			//Q_ASSERT(hnode);

			
			//if (!this->ParentID.isEmpty())
			//{
			//	hnode->SetParentNodeID(this->ParentID.toLatin1().data());
			//}

			hnode->SetAndObserveDisplayNodeID(dnode->GetID());
			hnode->SetHideFromEditors(0);
			hnode->SetSelectable(1);

			//this->ParentID = hnode->GetID(); 
		}



	}
	
	//roiPoints->SetVisibility(1);
	//roiPoints->EndModify(roiPointsModifyOld);
	//// StartModify/EndModify discarded vtkMRMLFiducialListNode::FiducialModifiedEvent-s, so we have to resubmit them now
	//roiPoints->InvokeEvent(vtkMRMLFiducialListNode::FiducialModifiedEvent, NULL); 


	return true;
}

//---------------------------------------------------------------------------
vtkMRMLDisplayableNode* vtkSlicerDicomRtImportLogic
::AddArchetypeDICOMObject(const char *filename, const char* name)
{
  std::cout << "Loading series '" << name << "' from file '" << filename << "'" << std::endl;

  // Try to load RT
  if ( LoadDicomRT(filename) )
  {
    return NULL;
  }
  else
  {
    // Try to load Volume
    return this->VolumesLogic->AddArchetypeVolume( filename, name );
  }
}
