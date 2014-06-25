/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/


// Module includes
#include "vtkSlicerPinnacleDVFReader.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkTransform.h>
#include <vtkVersion.h>

// STD includes
#include <cassert>
#include <iostream>
#include <fstream>
#include <algorithm>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPinnacleDVFReader);

//----------------------------------------------------------------------------
vtkSlicerPinnacleDVFReader::vtkSlicerPinnacleDVFReader()
{
  this->FileName = NULL;

  this->GridOrigin[0] = 0.0;
  this->GridOrigin[1] = 0.0;
  this->GridOrigin[2] = 0.0;
  this->PostDeformationRegistrationMatrix = vtkMatrix4x4::New();
  this->DeformableRegistrationGrid = vtkImageData::New();
  this->DeformableRegistrationGridOrientationMatrix = vtkMatrix4x4::New();

  this->LoadDeformableSpatialRegistrationSuccessful = false;
}

//----------------------------------------------------------------------------
vtkSlicerPinnacleDVFReader::~vtkSlicerPinnacleDVFReader()
{
  this->PostDeformationRegistrationMatrix->Delete();
  this->DeformableRegistrationGrid->Delete();
  this->DeformableRegistrationGridOrientationMatrix->Delete();
}

//----------------------------------------------------------------------------
void vtkSlicerPinnacleDVFReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerPinnacleDVFReader::Update()
{
  this->PostDeformationRegistrationMatrix->Identity();
  this->DeformableRegistrationGridOrientationMatrix->Identity();

  if ((this->FileName != NULL) && (strlen(this->FileName) > 0))
  {
    this->LoadDeformableSpatialRegistration(this->FileName);
  } 
  else 
  {
    vtkDebugMacro("vtkSlicerPinnacleDVFReader::Update: invalid filename: <empty string>");
  }
}

//----------------------------------------------------------------------------
void vtkSlicerPinnacleDVFReader::LoadDeformableSpatialRegistration(char *fileName)
{
  const float MIN_RESOLUTION = 0.004;
  
  /* start coordinates of the bounding box*/
  int fixedBBStartX;
  int fixedBBStartY;
  int fixedBBStartZ;
  /* end coordinates of the bounding box*/
  int fixedBBEndX;
  int fixedBBEndY;
  int fixedBBEndZ;
  /* X, Y and Z extent of the DVF*/
  int dvfSizeX;
  int dvfSizeY;
  int dvfSizeZ;
  /* Voxel spacing in mm along X, Y and Z of the DVF*/
  double xSpacing;
  double ySpacing;
  double zSpacing;

  /* 1  implies that the fixed volume is Non-primary i.e. Secondary in Pinnacle*/
  int isFixedSecondary; 
  /* 1  implies that the fixed volume is Non-primary i.e. Secondary in Pinnacle */
  int isMovingSecondary; 

  /*  stores parameters of the rigid transform estimated by the plug-in*/
  float fixedTx;
  float fixedTy;
  float fixedTz;
  float fixedRx;
  float fixedRy;
  float fixedRz;
  float movingTx;
  float movingTy;
  float movingTz;
  float movingRx;
  float movingRy;
  float movingRz;
  int isLittleEndian;

  this->LoadDeformableSpatialRegistrationSuccessful = false; 
 
  vtkSmartPointer<vtkMatrix4x4> invMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  invMatrix->Identity();
  invMatrix->SetElement(0,0,-1);
  invMatrix->SetElement(1,1,-1);

  ifstream readFileStream;
  readFileStream.open(fileName, std::ios::binary);
  if (readFileStream.fail())
  {
    vtkErrorMacro("LoadPinnacleDVF: The specified file could not be opened.");
    return;
  }

  readFileStream.read ((char *) &isLittleEndian, sizeof(int));
  readFileStream.read ((char *) &isFixedSecondary, sizeof(int));
  readFileStream.read ((char *) &isMovingSecondary, sizeof(int));

  this->PostDeformationRegistrationMatrix->Identity();
  vtkSmartPointer<vtkTransform> tempTransform = vtkSmartPointer<vtkTransform>::New();
  if(isFixedSecondary == 1)
  {
    /* User Selected Fixed Volume is Secondary */
    readFileStream.read ((char *) &fixedTx, sizeof(float));
    readFileStream.read ((char *) &fixedTy, sizeof(float));
    readFileStream.read ((char *) &fixedTz, sizeof(float));
    readFileStream.read ((char *) &fixedRx, sizeof(float));
    readFileStream.read ((char *) &fixedRy, sizeof(float));
    readFileStream.read ((char *) &fixedRz, sizeof(float));
    tempTransform->RotateX(fixedRx);
    tempTransform->RotateY(fixedRy);
    tempTransform->RotateZ(fixedRz);
    tempTransform->Translate(fixedTx, fixedTy, fixedTz);
    this->PostDeformationRegistrationMatrix->DeepCopy(tempTransform->GetMatrix());
  }
  else if(isMovingSecondary == 1)
  {
    /* User Selected Moving Volume is Secondary */
    readFileStream.read ((char *) &movingTx, sizeof(float));
    readFileStream.read ((char *) &movingTy, sizeof(float));
    readFileStream.read ((char *) &movingTz, sizeof(float));
    readFileStream.read ((char *) &movingRx, sizeof(float));
    readFileStream.read ((char *) &movingRy, sizeof(float));
    readFileStream.read ((char *) &movingRz, sizeof(float));
    tempTransform->RotateX(movingRx);
    tempTransform->RotateY(movingRy);
    tempTransform->RotateZ(movingRz);
    tempTransform->Translate(movingTx*10, movingTy*10, movingTz*10);
    this->PostDeformationRegistrationMatrix->DeepCopy(tempTransform->GetMatrix());
  }
  vtkMatrix4x4::Multiply4x4(invMatrix, this->PostDeformationRegistrationMatrix, this->PostDeformationRegistrationMatrix);
  vtkMatrix4x4::Multiply4x4(this->PostDeformationRegistrationMatrix, invMatrix, this->PostDeformationRegistrationMatrix);
  
  readFileStream.read ((char *) &fixedBBStartX, sizeof(int));
  readFileStream.read ((char *) &fixedBBStartY, sizeof(int));
  readFileStream.read ((char *) &fixedBBStartZ, sizeof(int));
  readFileStream.read ((char *) &fixedBBEndX, sizeof(int));
  readFileStream.read ((char *) &fixedBBEndY, sizeof(int));
  readFileStream.read ((char *) &fixedBBEndZ, sizeof(int));
  readFileStream.read ((char *) &dvfSizeX, sizeof(int));
  readFileStream.read ((char *) &dvfSizeY, sizeof(int));
  readFileStream.read ((char *) &dvfSizeZ, sizeof(int));
  readFileStream.read ((char *) &xSpacing, sizeof(double));
  readFileStream.read ((char *) &ySpacing, sizeof(double));
  readFileStream.read ((char *) &zSpacing, sizeof(double));

  long voxelCount = dvfSizeX * dvfSizeY * dvfSizeZ;

  signed char *xBufferHigh = new signed char[voxelCount];
  signed char *yBufferHigh = new signed char[voxelCount];
  signed char *zBufferHigh = new signed char[voxelCount];
  unsigned char *xBufferLow = new unsigned char[voxelCount];
  unsigned char *yBufferLow = new unsigned char[voxelCount];
  unsigned char *zBufferLow = new unsigned char[voxelCount];

  readFileStream.read ((char *) xBufferHigh, voxelCount);
  readFileStream.read ((char *) yBufferHigh, voxelCount);
  readFileStream.read ((char *) zBufferHigh, voxelCount);
  readFileStream.read ((char *) xBufferLow, voxelCount);
  readFileStream.read ((char *) yBufferLow, voxelCount);
  readFileStream.read ((char *) zBufferLow, voxelCount);

  this->DeformableRegistrationGridOrientationMatrix->Identity();
  this->DeformableRegistrationGridOrientationMatrix->SetElement(0,0,-1);
  this->DeformableRegistrationGridOrientationMatrix->SetElement(1,1,-1);
  this->DeformableRegistrationGridOrientationMatrix->SetElement(2,2,-1);

  // Deformable registration grid 
  this->DeformableRegistrationGrid->SetOrigin(this->GridOrigin[0], this->GridOrigin[1], this->GridOrigin[2]);
  this->DeformableRegistrationGrid->SetSpacing(xSpacing, ySpacing, zSpacing);
  this->DeformableRegistrationGrid->SetExtent(0,dvfSizeX-1,0,dvfSizeY-1,0,dvfSizeZ-1);
#if (VTK_MAJOR_VERSION <= 5)
  this->DeformableRegistrationGrid->SetScalarTypeToDouble();
  this->DeformableRegistrationGrid->SetNumberOfScalarComponents(3);
  this->DeformableRegistrationGrid->AllocateScalars();
#else
  this->DeformableRegistrationGrid->AllocateScalars(VTK_DOUBLE, 3);
#endif
  int n = 0;
  for (unsigned int k=0; k<dvfSizeZ; k++)
  {
    for (unsigned int j=0; j<dvfSizeY; j++)
    {
      for (unsigned int i=0; i<dvfSizeX; i++)
      {
        n = i + j*dvfSizeX + k*dvfSizeX*dvfSizeY ;
        this->DeformableRegistrationGrid->SetScalarComponentFromDouble(i,j,k,0, -1*(xBufferHigh[n] + (MIN_RESOLUTION * xBufferLow[n])) );
        this->DeformableRegistrationGrid->SetScalarComponentFromDouble(i,j,k,1, -1*(yBufferHigh[n] + (MIN_RESOLUTION * yBufferLow[n])) );
        this->DeformableRegistrationGrid->SetScalarComponentFromDouble(i,j,k,2,  1*(zBufferHigh[n] + (MIN_RESOLUTION * zBufferLow[n])) );
      }
    }
  }
  readFileStream.close();
  
  if(xBufferHigh)
  {
      delete [] xBufferHigh;
      xBufferHigh = NULL;
  }
  if(yBufferHigh)
  {
      delete [] yBufferHigh;
      yBufferHigh = NULL;
  }
  if(zBufferHigh)
  {
      delete [] zBufferHigh;
      zBufferHigh = NULL;
  }
  if(xBufferLow)
  {
      delete [] xBufferLow;
      xBufferLow = NULL;
  }
  if(yBufferLow)
  {
      delete [] yBufferLow;
      yBufferLow = NULL;
  }
  if(zBufferLow)
  {
      delete [] zBufferLow;
      zBufferLow = NULL;
  }

  this->LoadDeformableSpatialRegistrationSuccessful = true; 
}

//----------------------------------------------------------------------------
vtkMatrix4x4* vtkSlicerPinnacleDVFReader::GetPostDeformationRegistrationMatrix()
{
  return this->PostDeformationRegistrationMatrix;
}

//----------------------------------------------------------------------------
vtkImageData* vtkSlicerPinnacleDVFReader::GetDeformableRegistrationGrid()
{
  return this->DeformableRegistrationGrid;
}

//----------------------------------------------------------------------------
vtkMatrix4x4* vtkSlicerPinnacleDVFReader::GetDeformableRegistrationGridOrientationMatrix()
{
  return this->DeformableRegistrationGridOrientationMatrix;
}
