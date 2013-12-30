/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

// Qt includes
#include <QDebug>

// SlicerQt includes
#include "qSlicerRTHierarchyModuleWidget.h"
#include "ui_qSlicerRTHierarchyModuleWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RtHierarchy
class qSlicerRTHierarchyModuleWidgetPrivate: public Ui_qSlicerRTHierarchyModuleWidget
{
public:
  qSlicerRTHierarchyModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerRTHierarchyModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerRTHierarchyModuleWidgetPrivate::qSlicerRTHierarchyModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerRTHierarchyModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerRTHierarchyModuleWidget::qSlicerRTHierarchyModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerRTHierarchyModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerRTHierarchyModuleWidget::~qSlicerRTHierarchyModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerRTHierarchyModuleWidget::setup()
{
  Q_D(qSlicerRTHierarchyModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
}

