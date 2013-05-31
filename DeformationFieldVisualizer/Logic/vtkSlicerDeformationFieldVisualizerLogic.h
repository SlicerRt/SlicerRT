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

#ifndef __vtkSlicerDeformationFieldVisualizerLogic_h
#define __vtkSlicerDeformationFieldVisualizerLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkMatrix4x4.h>
#include <vtkImageData.h>

#include "vtkSlicerDeformationFieldVisualizerModuleLogicExport.h"

class vtkMRMLDeformationFieldVisualizerNode;
class vtkMRMLVectorVolumeNode;

/// \ingroup Slicer_QtModules_DeformationFieldVisualizer
class VTK_SLICER_DEFORMATIONFIELDVISUALIZER_MODULE_LOGIC_EXPORT vtkSlicerDeformationFieldVisualizerLogic : 
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDeformationFieldVisualizerLogic *New();
  vtkTypeMacro(vtkSlicerDeformationFieldVisualizerLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum visualizationModes
  {
    VIS_MODE_GLYPH_3D = 1,
    VIS_MODE_GRID_3D,
    VIS_MODE_BLOCK_3D,
    VIS_MODE_CONTOUR_3D,
    VIS_MODE_GLYPH_2D,
    VIS_MODE_GRID_2D
  };
 
  enum glyphSources
  {
    ARROW_3D = 0,
    CONE_3D,
    SPHERE_3D,
  }; 
  
  /*!
   * TODO: Add description of the function itself and its arguments
   * \param option (it is named option in the source but it tells nothing, please rename)
   */
  void CreateVisualization(int visualizationMode);
  
  void GlyphVisualization(vtkImageData*, vtkPolyData*, int);
  void GridVisualization(vtkImageData*, vtkPolyData* output);
  void ContourVisualization(vtkImageData*, vtkPolyData* output);
  void BlockVisualization(vtkImageData*, vtkPolyData* output);
  void GlyphSliceVisualization(vtkImageData*, vtkPolyData* output, vtkSmartPointer<vtkMatrix4x4>); 
  void GridSliceVisualization(vtkImageData*, vtkPolyData* output, vtkSmartPointer<vtkMatrix4x4>);

  /*!
   * Issue when input data is changed without modifying node
   * Will either remake or account for scenario some other way
   */
  void GenerateDeformationField();
  double* GetFieldRange();

public:
  void SetAndObserveDeformationFieldVisualizerNode(vtkMRMLDeformationFieldVisualizerNode *node);
  vtkGetObjectMacro(DeformationFieldVisualizerNode, vtkMRMLDeformationFieldVisualizerNode);

protected:
  vtkSlicerDeformationFieldVisualizerLogic();
  ~vtkSlicerDeformationFieldVisualizerLogic();

  virtual void RegisterNodes();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

protected:
  vtkImageData* deformationField;
  
private:
  vtkSlicerDeformationFieldVisualizerLogic(const vtkSlicerDeformationFieldVisualizerLogic&);// Not implemented
  void operator=(const vtkSlicerDeformationFieldVisualizerLogic&);// Not implemented
  
protected:
  /// Parameter set MRML node
  vtkMRMLDeformationFieldVisualizerNode* DeformationFieldVisualizerNode;
};

#endif
