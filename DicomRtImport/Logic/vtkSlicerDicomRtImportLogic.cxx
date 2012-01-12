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

// MRML includes
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLFiducial.h>

// VTK includes
#include <vtkNew.h>
#include "vtkPolyData.h"
#include "vtkPolyDataWriter.h"
#include <vtkSmartPointer.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomRtImportLogic);

//----------------------------------------------------------------------------
vtkSlicerDicomRtImportLogic::vtkSlicerDicomRtImportLogic()
{
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

void vtkSlicerDicomRtImportLogic
::LoadDicomRT(char *name)
{
	vtkSlicerDicomRtImportReader* rtReader = vtkSlicerDicomRtImportReader::New();
	rtReader->SetFileName(name);
	rtReader->Update();

	int numberOfROI = rtReader->GetNumberOfROI();
	for(int i=0;i<numberOfROI; i++)
	{
		vtkPolyData* poly = rtReader->GetROI(i+1);

		//vtkPolyDataWriter *polyWriter = vtkPolyDataWriter::New();
		//polyWriter->SetFileName("\\test1.vtk");
		//polyWriter->SetInput(poly);
		//polyWriter->Write();

		//polyWriter->Delete();
		if (poly->GetNumberOfPoints() == 1)
		{
			//vtkSmartPointer<vtkMRMLFiducial> fnode =
			//	vtkSmartPointer<vtkMRMLFiducial>::New();
			//fnode->SetVisibility(1);
			//double point[3];
			//point = poly->GetPoint(0);
			//fnode->SetXYZ(point);
			//fnode->SetID(rtReader->GetROIName(i+1));
			//fnode->SetLabelText(rtReader->GetROIName(i+1));

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

			//fnode = vtkMRMLFiducial::SafeDownCast(
			//	this->GetMRMLScene()->AddNode(fnode));
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
	
	rtReader->Delete();
}
