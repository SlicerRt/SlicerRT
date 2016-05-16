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
};

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
void qSlicerRoomsEyeViewModuleWidget::setup()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
}
