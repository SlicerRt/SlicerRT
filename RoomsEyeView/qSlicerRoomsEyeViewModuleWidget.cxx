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
  vtkSmartPointer<vtkSlicerRoomsEyeViewModuleLogic> logic() const;

  bool ModuleWindowInitialized;
};
//-----------------------------------------------------------------------------
// qSlicerBeamsModuleWidgetPrivate methods

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkSlicerRoomsEyeViewModuleLogic> qSlicerRoomsEyeViewModuleWidgetPrivate::logic() const
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

}
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setParameterNode(vtkMRMLNode *node)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  
  vtkSmartPointer<vtkMRMLRoomsEyeViewNode> paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(node);
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
  this->setMRMLScene(this->mrmlScene());
  connect(d->loadModelButton, SIGNAL(clicked()), this, SLOT(loadModelButtonClicked()));
  connect(d->CollimatorRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(CollimatorRotationSliderValueChanged()));
  connect(d->GantryRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(GantryRotationSliderValueChanged()));
  connect(d->ImagingPanelMovementSlider, SIGNAL(valueChanged(double)), this, SLOT(ImagingPanelMovementSliderValueChanged()));
  connect(d->PatientSupportRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(PatientSupportRotationSliderValueChanged()));
  connect(d->VerticalTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(VerticalTableTopDisplacementSliderValueChanged()));
  connect(d->LongitudinalTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(LongitudinalTableTopDisplacementSliderValueChanged()));
  connect(d->LateralTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(LateralTableTopDisplacementSliderValueChanged()));

}
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::loadModelButtonClicked() //TODO: Rename to loadModelButtonClicked
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
 
  //TODO: No file path names!
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/gantryModel.stl");
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/collimatorModel.stl");
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/leftImagingPanelModel.stl");
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/linacModel.stl");
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/patientModel.stl");
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/patientSupportModel.stl");
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/rightImagingPanelModel.stl");
  d->logic()->LoadLinacModels(this->mrmlScene(), "C:/Users/Vinith/Documents/Third Year/Perk Lab/slicerrt/trunk/SlicerRt/sandbox/RoomsEyeView/Resources/tableTopModel.stl");

  d->logic()->ModelToParentTransforms(this->mrmlScene());
}
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::GantryRotationSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->GantryRotationValueChanged(this->mrmlScene(), d->GantryRotationSlider->value());
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
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::ImagingPanelMovementSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->ImagingPanelMovementValueChanged(this->mrmlScene(), d->ImagingPanelMovementSlider->value());

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
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::CollimatorRotationSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->CollimatorRotationValueChanged(this->mrmlScene(), d->CollimatorRotationSlider->value());

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
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::PatientSupportRotationSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->PatientSupportRotationValueChanged(this->mrmlScene(), d->PatientSupportRotationSlider->value());

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
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::VerticalTableTopDisplacementSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->VerticalDisplacementValueChanged(this->mrmlScene(), d->LateralTableTopDisplacementSlider->value(), d->LongitudinalTableTopDisplacementSlider->value(), d->VerticalTableTopDisplacementSlider->value() );

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
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::LongitudinalTableTopDisplacementSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->LongitudinalDisplacementValueChanged(this->mrmlScene(), d->LateralTableTopDisplacementSlider->value(), d->LongitudinalTableTopDisplacementSlider->value(), d->VerticalTableTopDisplacementSlider->value());

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
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::LateralTableTopDisplacementSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->logic()->LateralDisplacementValueChanged(this->mrmlScene(), d->LateralTableTopDisplacementSlider->value(), d->LongitudinalTableTopDisplacementSlider->value(), d->VerticalTableTopDisplacementSlider->value());

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