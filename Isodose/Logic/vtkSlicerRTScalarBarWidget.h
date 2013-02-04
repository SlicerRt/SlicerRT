/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSlicerRTScalarBarWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSlicerRTScalarBarWidget - 2D widget for manipulating a scalar bar
// .SECTION Description
// This class provides support for interactively manipulating the position,
// size, and orientation of a scalar bar. It listens to Left mouse events and
// mouse movement. It also listens to Right mouse events and notifies any 
// observers of Right mouse events on this object when they occur.
// It will change the cursor shape based on its location. If
// the cursor is over an edge of the scalar bar it will change the cursor
// shape to a resize edge shape. If the position of a scalar bar is moved to
// be close to the center of one of the four edges of the viewport, then the
// scalar bar will change its orientation to align with that edge. This
// orientation is sticky in that it will stay that orientation until the
// position is moved close to another edge.

// .SECTION See Also
// vtkInteractorObserver


#ifndef __vtkSlicerRTScalarBarWidget_h
#define __vtkSlicerRTScalarBarWidget_h

#include "vtkBorderWidget.h"

#include "vtkSlicerIsodoseModuleLogicExport.h"

class vtkSlicerRTScalarBarActor;
class vtkSlicerRTScalarBarRepresentation;

class VTK_SLICER_ISODOSE_LOGIC_EXPORT vtkSlicerRTScalarBarWidget : public vtkBorderWidget
{
public:
  static vtkSlicerRTScalarBarWidget *New();
  vtkTypeMacro(vtkSlicerRTScalarBarWidget, vtkBorderWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  virtual void SetRepresentation(vtkSlicerRTScalarBarRepresentation *rep);

  // Description:
  // Return the representation as a vtkScalarBarRepresentation.
  vtkSlicerRTScalarBarRepresentation *GetScalarBarRepresentation()
    { return reinterpret_cast<vtkSlicerRTScalarBarRepresentation *>(this->GetRepresentation()); }

  // Description:
  // Get the ScalarBar used by this Widget. One is created automatically.
  virtual void SetScalarBarActor(vtkSlicerRTScalarBarActor *actor);
  virtual vtkSlicerRTScalarBarActor *GetScalarBarActor();

  // Description:
  // Can the widget be moved. On by default. If off, the widget cannot be moved
  // around.
  //
  // TODO: This functionality should probably be moved to the superclass.
  vtkSetMacro(Repositionable, int);
  vtkGetMacro(Repositionable, int);
  vtkBooleanMacro(Repositionable, int);

  // Description:
  // Create the default widget representation if one is not set. 
  virtual void CreateDefaultRepresentation();

protected:
  vtkSlicerRTScalarBarWidget();
  ~vtkSlicerRTScalarBarWidget();

  int Repositionable;

  // Handle the case of Repositionable == 0
  static void MoveAction(vtkAbstractWidget*);

  // set the cursor to the correct shape based on State argument
  virtual void SetCursor(int State);

private:
  vtkSlicerRTScalarBarWidget(const vtkSlicerRTScalarBarWidget&);  //Not implemented
  void operator=(const vtkSlicerRTScalarBarWidget&);  //Not implemented
};

#endif
