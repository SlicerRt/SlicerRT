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

#ifndef __qSlicerBeamsModuleWidget_h
#define __qSlicerBeamsModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerBeamsModuleExport.h"

class qSlicerBeamsModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup SlicerRt_QtModules_Beams
class Q_SLICER_QTMODULES_BEAMS_EXPORT qSlicerBeamsModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerBeamsModuleWidget(QWidget *parent=0);
  virtual ~qSlicerBeamsModuleWidget();

protected:
  QScopedPointer<qSlicerBeamsModuleWidgetPrivate> d_ptr;

protected:
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerBeamsModuleWidget);
  Q_DISABLE_COPY(qSlicerBeamsModuleWidget);
};

#endif
