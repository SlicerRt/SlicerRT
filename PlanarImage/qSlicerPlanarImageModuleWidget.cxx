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
#include "qSlicerPlanarImageModuleWidget.h"
#include "ui_qSlicerPlanarImageModule.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_PlanarImage
class qSlicerPlanarImageModuleWidgetPrivate: public Ui_qSlicerPlanarImageModule
{
  Q_DECLARE_PUBLIC(qSlicerPlanarImageModuleWidget);
protected:
  qSlicerPlanarImageModuleWidget* const q_ptr;
public:
  qSlicerPlanarImageModuleWidgetPrivate(qSlicerPlanarImageModuleWidget& object);
};

//-----------------------------------------------------------------------------
// qSlicerPlanarImageModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerPlanarImageModuleWidgetPrivate::qSlicerPlanarImageModuleWidgetPrivate(qSlicerPlanarImageModuleWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
// qSlicerPlanarImageModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerPlanarImageModuleWidget::qSlicerPlanarImageModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerPlanarImageModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerPlanarImageModuleWidget::~qSlicerPlanarImageModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::setup()
{
  Q_D(qSlicerPlanarImageModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
}
