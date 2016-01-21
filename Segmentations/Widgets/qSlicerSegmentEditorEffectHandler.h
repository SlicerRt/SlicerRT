/*==============================================================================

  Program: 3D Slicer

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

#ifndef __qSlicerSegmentEditorEffectHandler_h
#define __qSlicerSegmentEditorEffectHandler_h

// SubjectHierarchy includes
#include "qSlicerSegmentationsModuleWidgetsExport.h"

// Qt includes
#include <QObject>
#include <QList>
#include <QString>

class vtkMRMLScene;
class qSlicerSegmentEditorAbstractEffect;
class qSlicerSegmentEditorEffectHandlerCleanup;


/// \ingroup SlicerRt_QtModules_Segmentations
/// \class qSlicerSegmentEditorEffectHandler
/// \brief Singleton class managing segment editor effect plugins
class Q_SLICER_MODULE_SEGMENTATIONS_WIDGETS_EXPORT qSlicerSegmentEditorEffectHandler : public QObject
{
  Q_OBJECT

public:
  Q_PROPERTY(qSlicerSegmentEditorAbstractEffect* activeEffect READ activeEffect WRITE setActiveEffect)

public:
  /// Instance getter for the singleton class
  /// \return Instance object
  Q_INVOKABLE static qSlicerSegmentEditorEffectHandler* instance();

  /// Allows cleanup of the singleton at application exit
  static void setInstance(qSlicerSegmentEditorEffectHandler* instance);

  /// Set MRML scene
  void setScene(vtkMRMLScene* scene);

  /// Get MRML scene
  vtkMRMLScene* scene();

public:
  /// Register a effect
  /// \return True if effect registered successfully, false otherwise
  Q_INVOKABLE bool registerEffect(qSlicerSegmentEditorAbstractEffect* effect);

  /// Get list of registered effects
  Q_INVOKABLE QList<qSlicerSegmentEditorAbstractEffect*> registeredEffects() { return m_RegisteredEffects; };

  /// Get an effect object by name
  /// \return The effect instance if exists, NULL otherwise
  Q_INVOKABLE qSlicerSegmentEditorAbstractEffect* effectByName(QString name);

  /// Return active effect if selected, NULL otherwise
  /// \sa m_ActiveEffect, setActiveEffect()
  qSlicerSegmentEditorAbstractEffect* activeEffect()const;
  /// Set active effect
  /// \sa m_ActiveEffect, activeEffect()
  void setActiveEffect(qSlicerSegmentEditorAbstractEffect* effect);

protected:
  /// List of registered effect instances
  QList<qSlicerSegmentEditorAbstractEffect*> m_RegisteredEffects;

  /// Active effect
  qSlicerSegmentEditorAbstractEffect* m_ActiveEffect;

  /// MRML scene
  vtkMRMLScene* m_Scene;

public:
  /// Private constructor made public to enable python wrapping
  /// IMPORTANT: Should not be used for creating effect handler! Use instance() instead.
  qSlicerSegmentEditorEffectHandler(QObject* parent=NULL);

  /// Private destructor made public to enable python wrapping
  virtual ~qSlicerSegmentEditorEffectHandler();

private:
  Q_DISABLE_COPY(qSlicerSegmentEditorEffectHandler);
  friend class qSlicerSegmentEditorEffectHandlerCleanup;
  friend class qSlicerSubjectHierarchyModule;

private:
  /// Instance of the singleton
  static qSlicerSegmentEditorEffectHandler* m_Instance;
};

#endif
