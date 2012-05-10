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

// SlicerQt includes
#include "qSlicerDoseAccumulationModuleWidget.h"
#include "ui_qSlicerDoseAccumulationModule.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DoseAccumulation
class qSlicerDoseAccumulationModuleWidgetPrivate: public Ui_qSlicerDoseAccumulationModule
{
public:
  qSlicerDoseAccumulationModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDoseAccumulationModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseAccumulationModuleWidgetPrivate::qSlicerDoseAccumulationModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDoseAccumulationModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDoseAccumulationModuleWidget::qSlicerDoseAccumulationModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDoseAccumulationModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerDoseAccumulationModuleWidget::~qSlicerDoseAccumulationModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::setup()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
}

