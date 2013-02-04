/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSlicerRTScalarBarWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSlicerRTScalarBarWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkCoordinate.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSlicerRTScalarBarActor.h"
#include "vtkSlicerRTScalarBarRepresentation.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

vtkStandardNewMacro(vtkSlicerRTScalarBarWidget);

//-------------------------------------------------------------------------
vtkSlicerRTScalarBarWidget::vtkSlicerRTScalarBarWidget()
{
  this->Selectable = 0;
  this->Repositionable = 1;

  // Override the subclasses callback to handle the Repositionable flag.
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSlicerRTScalarBarWidget::MoveAction);
}

//-------------------------------------------------------------------------
vtkSlicerRTScalarBarWidget::~vtkSlicerRTScalarBarWidget()
{
}

//-----------------------------------------------------------------------------
void vtkSlicerRTScalarBarWidget::SetRepresentation(vtkSlicerRTScalarBarRepresentation *rep)
{
  this->SetWidgetRepresentation(rep);
}

//-----------------------------------------------------------------------------
void vtkSlicerRTScalarBarWidget::SetScalarBarActor(vtkSlicerRTScalarBarActor *actor)
{
  vtkSlicerRTScalarBarRepresentation *rep = this->GetScalarBarRepresentation();
  if (!rep)
    {
    this->CreateDefaultRepresentation();
    rep = this->GetScalarBarRepresentation();
    }

  if (rep->GetScalarBarActor() != actor)
    {
    rep->SetScalarBarActor(actor);
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
vtkSlicerRTScalarBarActor *vtkSlicerRTScalarBarWidget::GetScalarBarActor()
{
  vtkSlicerRTScalarBarRepresentation *rep = this->GetScalarBarRepresentation();
  if (!rep)
    {
    this->CreateDefaultRepresentation();
    rep = this->GetScalarBarRepresentation();
    }

  return rep->GetScalarBarActor();
}

//-----------------------------------------------------------------------------
void vtkSlicerRTScalarBarWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
    {
    vtkSlicerRTScalarBarRepresentation *rep = vtkSlicerRTScalarBarRepresentation::New();
    this->SetRepresentation(rep);
    rep->Delete();
    }
}

//-------------------------------------------------------------------------
void vtkSlicerRTScalarBarWidget::SetCursor(int cState)
{
  if (   !this->Repositionable && !this->Selectable
      && cState == vtkBorderRepresentation::Inside)
    {
    // Don't have a special cursor for the inside if we cannot reposition.
    this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
  else
    {
    this->Superclass::SetCursor(cState);
    }
}

//-------------------------------------------------------------------------
void vtkSlicerRTScalarBarWidget::MoveAction(vtkAbstractWidget *w)
{
  // The the superclass handle most stuff.
  vtkSlicerRTScalarBarWidget::Superclass::MoveAction(w);

  vtkSlicerRTScalarBarWidget *self = reinterpret_cast<vtkSlicerRTScalarBarWidget*>(w);
  vtkSlicerRTScalarBarRepresentation *representation=self->GetScalarBarRepresentation();

  // Handle the case where we suppress widget translation.
  if (   !self->Repositionable
      && (   representation->GetInteractionState()
          == vtkBorderRepresentation::Inside ) )
    {
    representation->MovingOff();
    }
}

//-------------------------------------------------------------------------
void vtkSlicerRTScalarBarWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Repositionable: " << this->Repositionable << endl;
}
