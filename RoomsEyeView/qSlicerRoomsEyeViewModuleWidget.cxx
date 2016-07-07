/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

// SlicerQt includes
#include "qSlicerRoomsEyeViewModuleWidget.h"
#include "ui_qSlicerRoomsEyeViewModule.h"

#include "vtkSlicerRoomsEyeViewModuleLogic.h"
#include "vtkMRMLRoomsEyeViewNode.h"

// Slicer includes
#include <qSlicerApplication.h>
#include <qSlicerSubjectHierarchyAbstractPlugin.h>
#include <qSlicerIO.h>
#include <qSlicerCoreIOManager.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkSlicerModelsLogic.h>
#include <vtkMRMLModelNode.h>


// SlicerRT includes
#include "SlicerRtCommon.h"

// Qt includes
#include <QDebug>
#include <QTableWidgetItem>
#include <ctkSliderWidget.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RoomsEyeView
class qSlicerRoomsEyeViewModuleWidgetPrivate: public Ui_qSlicerRoomsEyeViewModule
{
  Q_DECLARE_PUBLIC(qSlicerRoomsEyeViewModuleWidget);
protected:
  qSlicerRoomsEyeViewModuleWidget* const q_ptr;
public:
  qSlicerRoomsEyeViewModuleWidgetPrivate(qSlicerRoomsEyeViewModuleWidget& object): q_ptr(&object) { };
  ~qSlicerRoomsEyeViewModuleWidgetPrivate() { };
  vtkSlicerRoomsEyeViewModuleLogic* logic() const;

  bool ModuleWindowInitialized;
};
//-----------------------------------------------------------------------------
// qSlicerBeamsModuleWidgetPrivate methods

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkSlicerRoomsEyeViewModuleLogic* qSlicerRoomsEyeViewModuleWidgetPrivate::logic() const
{
	Q_Q(const qSlicerRoomsEyeViewModuleWidget);
	return vtkSlicerRoomsEyeViewModuleLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
// qSlicerRoomsEyeViewModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerRoomsEyeViewModuleWidget::qSlicerRoomsEyeViewModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerRoomsEyeViewModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerRoomsEyeViewModuleWidget::~qSlicerRoomsEyeViewModuleWidget()
{
  
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
	Q_D(qSlicerRoomsEyeViewModuleWidget);
	this->Superclass::setMRMLScene(scene);
	qvtkReconnect(d->logic(), scene, vtkMRMLScene::EndImportEvent,this, SLOT(onSceneImportedEvent())	);

  if (scene)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLRoomsEyeViewNode");
    if (node)
    {
      this->setParameterNode(node);
    }
    else
    {
      vtkSmartPointer<vtkMRMLRoomsEyeViewNode> newNode = vtkSmartPointer<vtkMRMLRoomsEyeViewNode>::New();
      this->mrmlScene()->AddNode(newNode);
      this->setParameterNode(newNode);
    }
  }
}
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onSceneImportedEvent()
{
	this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::enter()
{
	this->onEnter();
	this->Superclass::enter();
}
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onEnter()
{
	if (!this->mrmlScene())
	{
		qCritical() << Q_FUNC_INFO << ": Invalid scene!";
		return;
	}

	Q_D(qSlicerRoomsEyeViewModuleWidget);
	
	// First check the logic if it has a parameter node
	if (!d->logic())
	{
		qCritical() << Q_FUNC_INFO << ": Invalid logic!";
		return;
	}

	d->ModuleWindowInitialized = true;

	//this->updateWidgetFromMRML();

}
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setParameterNode(vtkMRMLNode *node)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  
  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(node);
  //qvtkReconnect(paramNode, vtkCommand::ModifiedEvent,this,SLOT(updateWidgetFromMRML())  );
  /*
  if (paramNode)
  {
    if (!paramNode->GetGantryToFixedReferenceTransformNode())
    {
      paramNode->SetAndObserveGantryToFixedReferenceTransformNode(vtkMRMLLinearTransformNode::SafeDownCast(d->));
    }
  }*/
}
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setup()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
  connect(d->loadModelButton, SIGNAL(clicked()), this, SLOT(loadModelButtonClicked()));
  connect(d->SliderWidget_4, SIGNAL(valueChanged(double)), this, SLOT(SliderWidget_4Clicked()));
  connect(d->SliderWidget, SIGNAL(valueChanged(double)), this, SLOT(SliderWidgetValueChanged()));
  connect(d->SliderWidget_2, SIGNAL(valueChanged(double)), this, SLOT(SliderWidget_2ValueChanged()));
  connect(d->SliderWidget_5, SIGNAL(valueChanged(double)), this, SLOT(SliderWidget_5ValueChanged()));
  connect(d->SliderWidget_6, SIGNAL(valueChanged(double)), this, SLOT(SliderWidget_6ValueChanged()));
  connect(d->SliderWidget_7, SIGNAL(valueChanged(double)), this, SLOT(SliderWidget_7ValueChanged()));
  connect(d->SliderWidget_8, SIGNAL(valueChanged(double)), this, SLOT(SliderWidget_8ValueChanged()));
}
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::loadModelButtonClicked() //TODO: Rename to loadModelButtonClicked
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/gantryModel.stl");
  //TODO: No file path names!
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/collimatorModel.stl");
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/leftImagingPanelModel.stl");
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/linacModel.stl");
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/patientModel.stl");
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/patientSupportModel.stl");
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/rightImagingPanelModel.stl");
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/tableTopModel.stl");

  d->logic()->ModelToParentTransforms(this->mrmlScene());
  
 
}
void qSlicerRoomsEyeViewModuleWidget::SliderWidgetValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->GantryRotationValueChanged(this->mrmlScene(), d->SliderWidget->value());
  std::string collisionString = d->logic()->CheckForCollisions();
  
  if (collisionString.length() > 0)
  {
    d->CollisionsDetected->setText(QString::fromStdString(collisionString));
    d->CollisionsDetected->setStyleSheet("color: red");

  }

  else
  {
    d->CollisionsDetected->setText(QString::fromStdString("No collisions detected"));
    d->CollisionsDetected->setStyleSheet("color: green");
  }
}

void qSlicerRoomsEyeViewModuleWidget::SliderWidget_2ValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->ImagingPanelMovementValueChanged(this->mrmlScene(), d->SliderWidget_2->value());

  std::string collisionString = d->logic()->CheckForCollisions();

  if (collisionString.length() > 0)
  {
    d->CollisionsDetected->setText(QString::fromStdString(collisionString));
    d->CollisionsDetected->setStyleSheet("color: red");

  }

  else
  {
    d->CollisionsDetected->setText(QString::fromStdString("No collisions detected"));
    d->CollisionsDetected->setStyleSheet("color: green");
  }
}

void qSlicerRoomsEyeViewModuleWidget::SliderWidget_4Clicked()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->CollimatorRotationValueChanged(this->mrmlScene(), d->SliderWidget_4->value());

  std::string collisionString = d->logic()->CheckForCollisions();

  if (collisionString.length() > 0)
  {
    d->CollisionsDetected->setText(QString::fromStdString(collisionString));
    d->CollisionsDetected->setStyleSheet("color: red");

  }

  else
  {
    d->CollisionsDetected->setText(QString::fromStdString("No collisions detected"));
    d->CollisionsDetected->setStyleSheet("color: green");
  }
}

void qSlicerRoomsEyeViewModuleWidget::SliderWidget_5ValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->PatientSupportRotationValueChanged(this->mrmlScene(), d->SliderWidget_5->value());

  std::string collisionString = d->logic()->CheckForCollisions();

  if (collisionString.length() > 0)
  {
    d->CollisionsDetected->setText(QString::fromStdString(collisionString));
    d->CollisionsDetected->setStyleSheet("color: red");

  }

  else
  {
    d->CollisionsDetected->setText(QString::fromStdString("No collisions detected"));
    d->CollisionsDetected->setStyleSheet("color: green");
  }

}

void qSlicerRoomsEyeViewModuleWidget::SliderWidget_6ValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->VerticalDisplacementValueChanged(this->mrmlScene(), d->SliderWidget_8->value(), d->SliderWidget_7->value(), d->SliderWidget_6->value() );

  std::string collisionString = d->logic()->CheckForCollisions();

  if (collisionString.length() > 0)
  {
    d->CollisionsDetected->setText(QString::fromStdString(collisionString));
    d->CollisionsDetected->setStyleSheet("color: red");

  }

  else
  {
    d->CollisionsDetected->setText(QString::fromStdString("No collisions detected"));
    d->CollisionsDetected->setStyleSheet("color: green");
  }

}

void qSlicerRoomsEyeViewModuleWidget::SliderWidget_7ValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->LongitudinalDisplacementValueChanged(this->mrmlScene(), d->SliderWidget_8->value(), d->SliderWidget_7->value(), d->SliderWidget_6->value());

  std::string collisionString = d->logic()->CheckForCollisions();

  if (collisionString.length() > 0)
  {
    d->CollisionsDetected->setText(QString::fromStdString(collisionString));
    d->CollisionsDetected->setStyleSheet("color: red");

  }

  else
  {
    d->CollisionsDetected->setText(QString::fromStdString("No collisions detected"));
    d->CollisionsDetected->setStyleSheet("color: green");
  }

}

void qSlicerRoomsEyeViewModuleWidget::SliderWidget_8ValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->LateralDisplacementValueChanged(this->mrmlScene(), d->SliderWidget_8->value(), d->SliderWidget_7->value(), d->SliderWidget_6->value());

  std::string collisionString = d->logic()->CheckForCollisions();

  if (collisionString.length() > 0)
  {
    d->CollisionsDetected->setText(QString::fromStdString(collisionString));
    d->CollisionsDetected->setStyleSheet("color: red");

  }

  else
  {
    d->CollisionsDetected->setText(QString::fromStdString("No collisions detected"));
    d->CollisionsDetected->setStyleSheet("color: green");
  }

}