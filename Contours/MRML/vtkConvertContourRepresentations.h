/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

// .NAME vtkConvertContourRepresentations - Converts between contour representations
// .SECTION Description

#ifndef __vtkConvertContourRepresentations_h
#define __vtkConvertContourRepresentations_h

// VTK includes
#include "vtkObject.h"

// Contours includes
#include "vtkMRMLContourNode.h"

#include "vtkSlicerContoursModuleMRMLExport.h"

class vtkMRMLModelNode;
class vtkMRMLScalarVolumeNode;
class vtkGeneralTransform;
class vtkImageData;

/// \ingroup SlicerRt_SlicerRtCommon
class VTK_SLICER_CONTOURS_MODULE_MRML_EXPORT vtkConvertContourRepresentations : public vtkObject
{
public:
  static vtkConvertContourRepresentations *New();
  vtkTypeMacro(vtkConvertContourRepresentations, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Convert from a representation to another. Returns true after successful conversion, false otherwise
  bool ConvertToRepresentation(vtkMRMLContourNode::ContourRepresentationType type);

  /// Deletes the existing representation and redoes conversion
  /// Used when conversion parameters have changed
  void ReconvertRepresentation(vtkMRMLContourNode::ContourRepresentationType type);

public:
    vtkGetObjectMacro(ContourNode, vtkMRMLContourNode);
    vtkSetObjectMacro(ContourNode, vtkMRMLContourNode);

    vtkGetMacro(LogSpeedMeasurements, bool);
    vtkSetMacro(LogSpeedMeasurements, bool);
    vtkBooleanMacro(LogSpeedMeasurements, bool);

    vtkGetMacro(LabelmapResamplingThreshold, double);
    vtkSetMacro(LabelmapResamplingThreshold, double);

protected:
  /*!
    Compute transform between a model and a volume IJK coordinate system (to transform the model into the volume voxel space)
    The transform applied to the parent contour node is ignored
    /param fromNode Transformable node from which we want to compute the transform
    /param toVolumeNode Volume node to whose IJK space we want to compute the transform
    /param fromModelToVolumeIjkTransform Output transform
  */
  void GetTransformFromNodeToVolumeIjk(vtkMRMLTransformableNode* fromNode, vtkMRMLScalarVolumeNode* toVolumeNode, vtkGeneralTransform* fromModelToVolumeIjkTransform);

  /// Convert model representation to indexed labelmap
  /// \param type Source model type for the conversion
  vtkMRMLScalarVolumeNode* ConvertFromModelToIndexedLabelmap(vtkMRMLContourNode::ContourRepresentationType type);

  /// Convert indexed labelmap representation to closed surface model
  vtkMRMLModelNode* ConvertFromIndexedLabelmapToClosedSurfaceModel();

  /// Calculate the origin from IJK space to RAS space
  void CalculateOriginInRAS(vtkMRMLScalarVolumeNode* volumeNodeToSet, double originInIJKHomogeneous[4], double* newOriginHomogeneous );

protected:
  // Contour node whose representations to convert
  vtkMRMLContourNode* ContourNode;

  /// Flag telling whether the speed measurements are logged on standard output
  bool LogSpeedMeasurements;

  /// Threshold used to make the resampled labelmap binary again. The voxels having value > threshold * labelValue
  /// are kept as foreground voxels in the output. This value has to be between 0 and 1.
  double LabelmapResamplingThreshold;

protected:
  vtkConvertContourRepresentations();
  virtual ~vtkConvertContourRepresentations();

private:
  vtkConvertContourRepresentations(const vtkConvertContourRepresentations&); // Not implemented
  void operator=(const vtkConvertContourRepresentations&);               // Not implemented
//ETX
};

#endif 