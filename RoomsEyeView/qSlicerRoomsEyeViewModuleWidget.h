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

#ifndef __qSlicerRoomsEyeViewModuleWidget_h
#define __qSlicerRoomsEyeViewModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerRoomsEyeViewModuleExport.h"

class qSlicerRoomsEyeViewModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup SlicerRt_QtModules_RoomsEyeView
class Q_SLICER_QTMODULES_ROOMSEYEVIEW_EXPORT qSlicerRoomsEyeViewModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerRoomsEyeViewModuleWidget(QWidget *parent=0);
  virtual ~qSlicerRoomsEyeViewModuleWidget();

  virtual void enter();

public slots:
	virtual void setMRMLScene(vtkMRMLScene*);
  void setParameterNode(vtkMRMLNode*);
	void onSceneImportedEvent();

	//void updateWidgetFromMRML();

protected slots:
	void loadModelButtonClicked();
  void SliderWidget_4Clicked();
  void SliderWidgetValueChanged();
  void SliderWidget_2ValueChanged();
  void SliderWidget_5ValueChanged();
  void SliderWidget_6ValueChanged();
  void SliderWidget_7ValueChanged();
  void SliderWidget_8ValueChanged();
//void onLogicModified();


protected:
  QScopedPointer<qSlicerRoomsEyeViewModuleWidgetPrivate> d_ptr;

protected:
  virtual void setup();
  void onEnter();

private:
  Q_DECLARE_PRIVATE(qSlicerRoomsEyeViewModuleWidget);
  Q_DISABLE_COPY(qSlicerRoomsEyeViewModuleWidget);
};

#endif
