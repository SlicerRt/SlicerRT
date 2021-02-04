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

// Qt includes
#include <QDebug>

// qSlicer includes
#include "qSlicerDrrImageComputationModuleWidget.h"
#include "ui_qSlicerDrrImageComputationModuleWidget.h"

// Slicer MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// SlicerRT MRML Beams includes
#include <vtkMRMLRTBeamNode.h>
#include <vtkMRMLRTIonBeamNode.h>

// SlicerRT MRML DrrImageComputation includes
#include "vtkMRMLDrrImageComputationNode.h"

// Logic includes
#include "vtkSlicerDrrImageComputationLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DrrImageComputation
class qSlicerDrrImageComputationModuleWidgetPrivate: public Ui_qSlicerDrrImageComputationModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerDrrImageComputationModuleWidget);
protected:
  qSlicerDrrImageComputationModuleWidget* const q_ptr;
public:
  qSlicerDrrImageComputationModuleWidgetPrivate(qSlicerDrrImageComputationModuleWidget &object);
  virtual ~qSlicerDrrImageComputationModuleWidgetPrivate();
  vtkSlicerDrrImageComputationLogic* logic() const;

  bool ModuleWindowInitialized;
};

//-----------------------------------------------------------------------------
// qSlicerDrrImageComputationModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDrrImageComputationModuleWidgetPrivate::qSlicerDrrImageComputationModuleWidgetPrivate(qSlicerDrrImageComputationModuleWidget &object)
  :
  q_ptr(&object),
  ModuleWindowInitialized(false)
{
}

qSlicerDrrImageComputationModuleWidgetPrivate::~qSlicerDrrImageComputationModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerDrrImageComputationLogic* qSlicerDrrImageComputationModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerDrrImageComputationModuleWidget);
  return vtkSlicerDrrImageComputationLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
// qSlicerDrrImageComputationModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDrrImageComputationModuleWidget::qSlicerDrrImageComputationModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDrrImageComputationModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerDrrImageComputationModuleWidget::~qSlicerDrrImageComputationModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::setup()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Nodes
  connect( d->MRMLNodeComboBox_RtBeam, SIGNAL(currentNodeChanged(vtkMRMLNode*)), 
    this, SLOT(onRTBeamNodeChanged(vtkMRMLNode*)));
  connect( d->MRMLNodeComboBox_CtVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), 
    this, SLOT(onCtVolumeNodeChanged(vtkMRMLNode*)));
  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), 
    this, SLOT(onParameterNodeChanged(vtkMRMLNode*)));

  // Sliders
  connect( d->SliderWidget_IsocenterImagerDistance, SIGNAL(valueChanged(double)), 
    this, SLOT(onIsocenterImagerDistanceValueChanged(double)));

  // Coordinates widgets
  connect( d->CoordinatesWidget_ImagerResolution, SIGNAL(coordinatesChanged(double*)), 
    this, SLOT(onImagerResolutionChanged(double*)));
  connect( d->CoordinatesWidget_ImagerSpacing, SIGNAL(coordinatesChanged(double*)), 
    this, SLOT(onImagerSpacingChanged(double*)));
  connect( d->RangeWidget_ImageWindowColumns, SIGNAL(valuesChanged( double, double)), 
    this, SLOT(onImageWindowColumnsValuesChanged( double, double)));
  connect( d->RangeWidget_ImageWindowRows, SIGNAL(valuesChanged( double, double)), 
    this, SLOT(onImageWindowRowsValuesChanged( double, double)));

  // Buttons
  connect( d->PushButton_ComputeDrr, SIGNAL(clicked()), this, SLOT(onComputeDrrClicked()));
  connect( d->CheckBox_ShowDrrMarkups, SIGNAL(toggled(bool)), this, SLOT(onShowMarkupsToggled(bool)));
  connect( d->GroupBox_ImageWindowParameters, SIGNAL(toggled(bool)), this, SLOT(onUseImageWindowToggled(bool)));

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT(onLogicModified()));
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()));
  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndCloseEvent, this, SLOT(onSceneClosedEvent()));

  // Find parameters node or create it if there is none in the scene
  if (scene)
  {
    if (d->MRMLNodeComboBox_ParameterSet->currentNode())
    {
      this->setParameterNode(d->MRMLNodeComboBox_ParameterSet->currentNode());
    }
    else if (vtkMRMLNode* node = scene->GetNthNodeByClass( 0, "vtkMRMLDrrImageComputationNode"))
    {
      this->setParameterNode(node);
    }
    else
    {
      vtkNew<vtkMRMLDrrImageComputationNode> newNode;
      this->mrmlScene()->AddNode(newNode);
      this->setParameterNode(newNode);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::setParameterNode(vtkMRMLNode *node)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(node);

  // Make sure the parameter set node is selected (in case the function was not called by the selector combobox signal)
  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(node);

  // Set parameter node to children widgets (PlastimatchParameters)
  d->PlastimatchParametersWidget->setParameterNode(node);
 
  // Each time the node is modified, the UI widgets are updated
  qvtkReconnect( parameterNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  // Set selected MRML nodes in comboboxes in the parameter set if it was nullptr there
  // (then in the meantime the comboboxes selected the first one from the scene and we have to set that)
  if (parameterNode)
  {
    if (!parameterNode->GetBeamNode())
    {
      vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
//      qvtkConnect( beamNode, vtkMRMLRTBeamNode::BeamTransformModified, this, SLOT(updateNormalAndVupVectors()));
      parameterNode->SetAndObserveBeamNode(beamNode);
      parameterNode->Modified();
    }
  }
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  // Enable widgets
  d->CheckBox_ShowDrrMarkups->setEnabled(parameterNode);
  d->CollapsibleButton_ReferenceInput->setEnabled(parameterNode);
  d->CollapsibleButton_GeometryBasicParameters->setEnabled(parameterNode);
  d->PlastimatchParametersWidget->setEnabled(parameterNode);
  d->PushButton_ComputeDrr->setEnabled(parameterNode);
  
  if (!parameterNode || !d->ModuleWindowInitialized)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node, or module window isn't initialized";
    return;
  }

  if (!parameterNode->GetBeamNode())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid referenced parameter's beam node";
    return;
  }

  vtkMRMLScalarVolumeNode* ctVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_CtVolume->currentNode());
  if (!ctVolumeNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid referenced volume node";
    return;
  }

  // Update widgets info from parameter node
  d->MRMLNodeComboBox_RtBeam->setCurrentNode(parameterNode->GetBeamNode());
  d->SliderWidget_IsocenterImagerDistance->setValue(parameterNode->GetIsocenterImagerDistance());

  int imagerResolution[2] = {};
  double imagerRes[2] = {};
  parameterNode->GetImagerResolution(imagerResolution);
  imagerRes[0] = static_cast<double>(imagerResolution[0]);
  imagerRes[1] = static_cast<double>(imagerResolution[1]);
  d->CoordinatesWidget_ImagerResolution->setCoordinates(imagerRes);
  d->CoordinatesWidget_ImagerSpacing->setCoordinates(parameterNode->GetImagerSpacing());

  d->RangeWidget_ImageWindowColumns->setMinimum(0.);
  d->RangeWidget_ImageWindowColumns->setMaximum(double(imagerResolution[0] - 1));
  d->RangeWidget_ImageWindowRows->setMinimum(0.);
  d->RangeWidget_ImageWindowRows->setMaximum(double(imagerResolution[1] - 1));

  bool useImageWindow = parameterNode->GetImageWindowFlag();
  int imageWindow[4] = {};
  parameterNode->GetImageWindow(imageWindow);

  d->GroupBox_ImageWindowParameters->setChecked(useImageWindow);
  if (!useImageWindow)
  {
    d->RangeWidget_ImageWindowColumns->setValues( 0., double(imagerResolution[0] - 1));
    d->RangeWidget_ImageWindowRows->setValues( 0., double(imagerResolution[1] - 1));
  }
  else
  {
    d->RangeWidget_ImageWindowColumns->setValues( 
      static_cast<double>(std::max<int>( 0, imageWindow[0])),
      static_cast<double>(std::min<int>( imagerResolution[0] - 1, imageWindow[2])));
    d->RangeWidget_ImageWindowRows->setValues( 
      static_cast<double>(std::max<int>( 0, imageWindow[1])), 
      static_cast<double>(std::min<int>( imagerResolution[1] - 1, imageWindow[3])));
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onSceneClosedEvent()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  this->updateWidgetFromMRML();
}

/// RTBeam Node (RTBeam or RTIonBeam) changed
void qSlicerDrrImageComputationModuleWidget::onRTBeamNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

//  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
//  if (!shNode)
//  {
//    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy";
//    return;
//  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(node);
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node";
    return;
  }

  vtkMRMLRTIonBeamNode* ionBeamNode = vtkMRMLRTIonBeamNode::SafeDownCast(node);
  Q_UNUSED(ionBeamNode);

  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  parameterNode->SetAndObserveBeamNode(beamNode);
  parameterNode->Modified(); // Update imager and image markups, DRR arguments in logic
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::enter()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  this->Superclass::enter();
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::exit()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

  this->Superclass::exit();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onCtVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (!volumeNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid reference CT volume node";
    return;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onParameterNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(node);

  if (!parameterNode || !d->ModuleWindowInitialized)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node, or module window isn't initialized";
    return;
  }

  this->setParameterNode(parameterNode);
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onEnter()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic";
    return;
  }

  vtkMRMLDrrImageComputationNode* parameterNode = nullptr; 
  // Try to find one in the scene
  if (vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass( 0, "vtkMRMLDrrImageComputationNode"))
  {
    parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(node);
  }

  if (parameterNode && parameterNode->GetBeamNode())
  {
    // First thing first: update normal and vup vectors for parameter node
    // in case observed beam node transformation has been modified
    d->logic()->UpdateNormalAndVupVectors(parameterNode);
  }

  // Create or update DRR markups nodes
  d->logic()->CreateMarkupsNodes(parameterNode);

  this->updateWidgetFromMRML();

  // All required data for GUI is initiated
  d->ModuleWindowInitialized = true;
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onIsocenterImagerDistanceValueChanged(double value)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  parameterNode->SetIsocenterImagerDistance(value);
  parameterNode->Modified(); // Update imager and image markups, DRR arguments
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onImagerSpacingChanged(double* spacing)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  double s[2] = { spacing[0], spacing[1] }; // columns, rows
  parameterNode->SetImagerSpacing(s);
  parameterNode->Modified(); // Update imager and image markups, DRR arguments
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onShowMarkupsToggled(bool toggled)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);

  // Update imager and image markups, DRR arguments
  d->logic()->ShowMarkupsNodes(toggled);
}

/// @brief Setup imager resolution (dimention)
/// @param dimention: dimention[0] = columns, dimention[1] = rows
void qSlicerDrrImageComputationModuleWidget::onImagerResolutionChanged(double* res)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  int imagerResolution[2] = { static_cast<int>(res[0]), static_cast<int>(res[1]) }; // x, y

  parameterNode->SetImagerResolution(imagerResolution);
  parameterNode->Modified(); // Update imager and image markups, DRR arguments
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onImageWindowColumnsValuesChanged(double start_column, double end_column)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  int imageWindow[4];
  parameterNode->GetImageWindow(imageWindow);
  
  imageWindow[0] = start_column;
  imageWindow[2] = end_column;

  parameterNode->SetImageWindow(imageWindow);
  parameterNode->Modified(); // Update imager and image markups, DRR arguments
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onImageWindowRowsValuesChanged( double start_row, double end_row)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  int imageWindow[4];
  parameterNode->GetImageWindow(imageWindow);
  
  imageWindow[1] = start_row;
  imageWindow[3] = end_row;

  parameterNode->SetImageWindow(imageWindow);
  parameterNode->Modified(); // Update imager and image markups, DRR arguments
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onUseImageWindowToggled(bool value)
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  if (value)
  {
    int imagerResolution[2] = {};
    int imageWindow[4] = {};
    parameterNode->GetImagerResolution(imagerResolution);

    double columns[2], rows[2];
    d->RangeWidget_ImageWindowColumns->values( columns[0], columns[1]);
    d->RangeWidget_ImageWindowRows->values( rows[0], rows[1]);

    imageWindow[0] = static_cast<int>(columns[0]); // c1 = x1
    imageWindow[1] = static_cast<int>(rows[0]); // r1 = y1
    imageWindow[2] = static_cast<int>(columns[1]); // c2 = x2
    imageWindow[3] = static_cast<int>(rows[1]); // r2 = y2

    parameterNode->SetImageWindow(imageWindow);
  }
  else
  {
//    const double* window = d->CoordinatesWidget_ImagerResolution->coordinates();
//    imageWindow[0] = 0; // c1 = x1
//    imageWindow[1] = 0; // r1 = y1
//    imageWindow[2] = static_cast<int>(window[0] - 1.); // c2 = x2
//    imageWindow[3] = static_cast<int>(window[1] - 1.); // r2 = y2
  }

  parameterNode->SetImageWindowFlag(value);
  parameterNode->Modified(); // Update imager and image markups, DRR arguments
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModuleWidget::onComputeDrrClicked()
{
  Q_D(qSlicerDrrImageComputationModuleWidget);
  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  vtkMRMLScalarVolumeNode* ctVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_CtVolume->currentNode());

  if (!parameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  if (!parameterNode->GetBeamNode())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid referenced parameter's beam node";
    return;
  }

  if (!ctVolumeNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid referenced volume node";
    return;
  }
  
  bool result = d->logic()->ComputePlastimatchDRR( parameterNode, ctVolumeNode);
  if (result)
  {
    return;
  }
}
