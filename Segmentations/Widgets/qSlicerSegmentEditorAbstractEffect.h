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

// .NAME qSlicerSegmentEditorAbstractEffect - Logic class for segmentation handling
// .SECTION Description
// TODO

#ifndef __qSlicerSegmentEditorAbstractEffect_h
#define __qSlicerSegmentEditorAbstractEffect_h

// Segmentations Widgets includes
#include "qSlicerSegmentationsModuleWidgetsExport.h"

// CTK includes
#include <ctkPimpl.h>

// Qt includes
#include <QIcon>

class qSlicerSegmentEditorAbstractEffectPrivate;

class vtkMRMLScene;
class vtkMRMLSegmentEditorEffectNode;
class vtkRenderWindowInteractor;
class qMRMLSliceWidget;
class qMRMLThreeDWidget;

/// \ingroup SlicerRt_QtModules_Segmentations
class Q_SLICER_MODULE_SEGMENTATIONS_WIDGETS_EXPORT qSlicerSegmentEditorAbstractEffect : public QObject
{
public:
  Q_OBJECT

public:
  typedef QObject Superclass;
  qSlicerSegmentEditorAbstractEffect(QObject* parent = NULL);
  virtual ~qSlicerSegmentEditorAbstractEffect(); 

// API
public:  
  /// Get name of effect
  virtual QString name() = 0;

  /// Get icon for effect to be displayed in segment editor
  virtual QIcon icon() { return QIcon(); };

  /// Callback function invoked when interaction happens
  virtual void processInteractionEvents(vtkRenderWindowInteractor* callerInteractor, unsigned long eid, qMRMLSliceWidget* sliceWidget, qMRMLThreeDWidget* threeDWidget) { };

  /// Clone editor effect
  virtual qSlicerSegmentEditorAbstractEffect* clone() = 0;

// Effect parameter functions
public:
  /// Set MRML scene
  void setScene(vtkMRMLScene* scene) { m_Scene = scene; };

  /// Get effect parameter set node
  vtkMRMLSegmentEditorEffectNode* parameterSetNode();

  /// Get effect parameter from effect parameter set node
  QString parameter(QString name);

  /// Set effect parameter in effect parameter set node
  void setParameter(QString name, QString value);

// Utility functions
public:
  void cursorOff(qMRMLSliceWidget* sliceWidget);
  void cursorOn(qMRMLSliceWidget* sliceWidget);

protected:
  /// MRML scene
  vtkMRMLScene* m_Scene;
 
protected:
  QScopedPointer<qSlicerSegmentEditorAbstractEffectPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSegmentEditorAbstractEffect);
  Q_DISABLE_COPY(qSlicerSegmentEditorAbstractEffect);
};

#endif
