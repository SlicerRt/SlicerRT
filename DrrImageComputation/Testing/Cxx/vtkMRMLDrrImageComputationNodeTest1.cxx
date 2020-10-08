/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH)
  All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLCoreTestingMacros.h"
#include "vtkMRMLDrrImageComputationNode.h"

int vtkMRMLDrrImageComputationNodeTest1(int , char * [] )
{
  vtkNew< vtkMRMLDrrImageComputationNode > node1;

  EXERCISE_ALL_BASIC_MRML_METHODS(node1.GetPointer());

  TEST_SET_GET_INT(node1.GetPointer(), ImageWindowFlag, 1);
  TEST_SET_GET_INT(node1.GetPointer(), ExponentialMappingFlag, 0);
  TEST_SET_GET_DOUBLE(node1.GetPointer(), IsocenterImagerDistance, 300.0);

  return EXIT_SUCCESS;
}
