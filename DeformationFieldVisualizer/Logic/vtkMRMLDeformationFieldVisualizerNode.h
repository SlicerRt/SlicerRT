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
  
  This file was originally developed by Franklin King, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#pragma once
#ifndef __vtkMRMLDeformationFieldVisualizerNode_h
#define __vtkMRMLDeformationFieldVisualizerNode_h

#include <vtkMRML.h>
#include <vtkMRMLNode.h>

#include "vtkSlicerDeformationFieldVisualizerModuleLogicExport.h"

/// \ingroup Slicer_QtModules_DeformationFieldVisualizer
class VTK_SLICER_DEFORMATIONFIELDVISUALIZER_MODULE_LOGIC_EXPORT vtkMRMLDeformationFieldVisualizerNode : 
  public vtkMRMLNode
{
public:   

  static vtkMRMLDeformationFieldVisualizerNode *New();
  vtkTypeMacro(vtkMRMLDeformationFieldVisualizerNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();
  virtual void ReadXMLAttributes( const char** atts);
  virtual void WriteXML(ostream& of, int indent);
  virtual void Copy(vtkMRMLNode *node);
  virtual const char* GetNodeTagName() {return "DeformationFieldVisualizerParameters";};
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

public:
  vtkSetStringMacro(InputVolumeNodeID);
  vtkGetStringMacro(InputVolumeNodeID);
  void SetAndObserveInputVolumeNodeID(const char* id);

  vtkSetStringMacro(ReferenceVolumeNodeID);
  vtkGetStringMacro(ReferenceVolumeNodeID);
  void SetAndObserveReferenceVolumeNodeID(const char* id);

  vtkSetStringMacro(OutputModelNodeID);
  vtkGetStringMacro(OutputModelNodeID);
  void SetAndObserveOutputModelNodeID(const char* id);

  // Glyph Parameters
  vtkSetMacro(GlyphPointMax, int);
  vtkGetMacro(GlyphPointMax, int);
  vtkSetMacro(GlyphScale, float);
  vtkGetMacro(GlyphScale, float);
  vtkSetMacro(GlyphScaleDirectional, bool);
  vtkGetMacro(GlyphScaleDirectional, bool);
  vtkSetMacro(GlyphScaleIsotropic, bool);
  vtkGetMacro(GlyphScaleIsotropic, bool);
  vtkSetMacro(GlyphThresholdMax, double);
  vtkGetMacro(GlyphThresholdMax, double);
  vtkSetMacro(GlyphThresholdMin, double);
  vtkGetMacro(GlyphThresholdMin, double);  
  vtkSetMacro(GlyphSeed, int);
  vtkGetMacro(GlyphSeed, int);
  vtkSetMacro(GlyphSourceOption, int);
  vtkGetMacro(GlyphSourceOption, int);
  // Arrow Parameters
  vtkSetMacro(GlyphArrowTipLength, float);
  vtkGetMacro(GlyphArrowTipLength, float);
  vtkSetMacro(GlyphArrowTipRadius, float);
  vtkGetMacro(GlyphArrowTipRadius, float);
  vtkSetMacro(GlyphArrowShaftRadius, float);
  vtkGetMacro(GlyphArrowShaftRadius, float);
  vtkSetMacro(GlyphArrowResolution, int);
  vtkGetMacro(GlyphArrowResolution, int);
  // Cone Parameters
  vtkSetMacro(GlyphConeHeight, float);
  vtkGetMacro(GlyphConeHeight, float);
  vtkSetMacro(GlyphConeRadius, float);
  vtkGetMacro(GlyphConeRadius, float);
  vtkSetMacro(GlyphConeResolution, int);
  vtkGetMacro(GlyphConeResolution, int);
  // Sphere Parameters
  vtkSetMacro(GlyphSphereResolution, float);
  vtkGetMacro(GlyphSphereResolution, float);
    
  // Grid Parameters
  vtkSetMacro(GridScale, float);
  vtkGetMacro(GridScale, float);
  vtkSetMacro(GridSpacingMM, int);
  vtkGetMacro(GridSpacingMM, int);
  
  // Block Parameters
  vtkSetMacro(BlockScale, float);
  vtkGetMacro(BlockScale, float);
  vtkSetMacro(BlockDisplacementCheck, int);
  vtkGetMacro(BlockDisplacementCheck, int);  
    
  // Contour Parameters
  vtkSetMacro(ContourNumber, int);
  vtkGetMacro(ContourNumber, int);
  vtkSetMacro(ContourMin, float);
  vtkGetMacro(ContourMin, float);
  vtkSetMacro(ContourMax, float);
  vtkGetMacro(ContourMax, float);
  vtkSetMacro(ContourDecimation, float);
  vtkGetMacro(ContourDecimation, float); 
  
  // Glyph Slice Parameters
  vtkSetStringMacro(GlyphSliceNodeID);
  vtkGetStringMacro(GlyphSliceNodeID);
  void SetAndObserveGlyphSliceNodeID(const char* id);
  vtkSetMacro(GlyphSlicePointMax, int);
  vtkGetMacro(GlyphSlicePointMax, int);
  vtkSetMacro(GlyphSliceThresholdMax, double);
  vtkGetMacro(GlyphSliceThresholdMax, double);
  vtkSetMacro(GlyphSliceThresholdMin, double);
  vtkGetMacro(GlyphSliceThresholdMin, double);    
  vtkSetMacro(GlyphSliceScale, float);
  vtkGetMacro(GlyphSliceScale, float);
  vtkSetMacro(GlyphSliceSeed, int);
  vtkGetMacro(GlyphSliceSeed, int);
  
  //Grid Slice Parameters
  vtkSetStringMacro(GridSliceNodeID);
  vtkGetStringMacro(GridSliceNodeID);
  void SetAndObserveGridSliceNodeID(const char* id);
  vtkSetMacro(GridSliceScale, float);
  vtkGetMacro(GridSliceScale, float);
  vtkSetMacro(GridSliceSpacingMM, int);
  vtkGetMacro(GridSliceSpacingMM, int);
  
protected:
  vtkMRMLDeformationFieldVisualizerNode();
  ~vtkMRMLDeformationFieldVisualizerNode();

  vtkMRMLDeformationFieldVisualizerNode(const vtkMRMLDeformationFieldVisualizerNode&);
  void operator=(const vtkMRMLDeformationFieldVisualizerNode&);
  
  char* InputVolumeNodeID;
  char* ReferenceVolumeNodeID;
  char* OutputModelNodeID;

//Parameters
protected:
  // Glyph Parameters
  int GlyphPointMax;
  //TODO: Need to change the UI into float too
  float GlyphScale;
  bool GlyphScaleDirectional;
  bool GlyphScaleIsotropic;
  double GlyphThresholdMax;
  double GlyphThresholdMin;
  int GlyphSeed;
  int GlyphSourceOption; //0 - Arrow, 1 - Cone, 2 - Sphere
  // Arrow Parameters
  float GlyphArrowTipLength;
  float GlyphArrowTipRadius;
  float GlyphArrowShaftRadius;
  int GlyphArrowResolution;
  
  // Cone Parameters
  float GlyphConeHeight;
  float GlyphConeRadius;
  int GlyphConeResolution;
  
  // Sphere Parameters
  float GlyphSphereResolution;

  // Grid Parameters
  float GridScale;
  int GridSpacingMM;
  
  // Block Parameters
  float BlockScale;
  int BlockDisplacementCheck;
    
  // Contour Parameters
  int ContourNumber;
  float ContourMin;
  float ContourMax;
  float ContourDecimation;

  // Glyph Slice Parameters
  char* GlyphSliceNodeID;
  int GlyphSlicePointMax;
  double GlyphSliceThresholdMax;
  double GlyphSliceThresholdMin;
  float GlyphSliceScale;
  int GlyphSliceSeed;
  
  // Grid Slice Parameters
  char* GridSliceNodeID;
  float GridSliceScale;
  int GridSliceSpacingMM;
};

#endif
