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

==============================================================================*/

#ifndef __vtkPlanarContourToClosedSurfaceByRasterizationConversionRule_h
#define __vtkPlanarContourToClosedSurfaceByRasterizationConversionRule_h

// Slicer include
#include <vtkSlicerVersionConfigureMinimal.h>

// SegmentationCore includes
#include "vtkSegmentationConverterRule.h"
#include "vtkSegmentationConverter.h"

#include "vtkSlicerDicomRtImportExportConversionRulesExport.h"

class vtkPolyData;
class vtkMatrix4x4;

/// \ingroup DicomRtImportImportExportConversionRules
/// \brief Convert planar contour representation to closed surface representation
///   using a rasterization-based approach.
///
/// This rule rasterizes each contour into a 2D binary slice, builds a 3D volume
/// with nearest-neighbor fill, applies a Gaussian low-pass filter, and extracts
/// the closed surface using marching cubes.  This approach is useful when the
/// direct triangulation method produces artifacts (e.g. for complex or highly
/// variable contour data).
///
/// The conversion cost is set higher than the default triangulation rule so that
/// this rule is not chosen automatically unless explicitly preferred.
///
class VTK_SLICER_DICOMRTIMPORTEXPORT_CONVERSIONRULES_EXPORT vtkPlanarContourToClosedSurfaceByRasterizationConversionRule
  : public vtkSegmentationConverterRule
{
public:
  static vtkPlanarContourToClosedSurfaceByRasterizationConversionRule* New();
  vtkTypeMacro(vtkPlanarContourToClosedSurfaceByRasterizationConversionRule, vtkSegmentationConverterRule);
  vtkSegmentationConverterRule* CreateRuleInstance() override;

  /// Constructs representation object from representation name
  vtkDataObject* ConstructRepresentationObjectByRepresentation(std::string representationName) override;

  /// Constructs representation object from class name
  vtkDataObject* ConstructRepresentationObjectByClass(std::string className) override;

  /// Update the target representation based on the source representation
#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
  bool Convert(vtkSegment* segment) override;
#else
  bool Convert(vtkDataObject* sourceRepresentation, vtkDataObject* targetRepresentation) override;
#endif

  /// Get the cost of the conversion.
  /// Higher than the default triangulation rule (700) so this is not chosen by default.
  unsigned int GetConversionCost(vtkDataObject* sourceRepresentation = nullptr, vtkDataObject* targetRepresentation = nullptr) override;

  /// Human-readable name of the converter rule
  const char* GetName() override { return "Planar contour to closed surface (rasterization)"; };

  /// Human-readable name of the source representation
  const char* GetSourceRepresentationName() override { return vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName(); };

  /// Human-readable name of the target representation
  const char* GetTargetRepresentationName() override { return vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName(); };

protected:
  vtkPlanarContourToClosedSurfaceByRasterizationConversionRule();
  ~vtkPlanarContourToClosedSurfaceByRasterizationConversionRule() override;

  /// Calculate the spacing between the lines in the polydata.
  /// WARNING: This function requires that the normal vector of all contours is aligned with the Z-axis.
  double GetSpacingBetweenLines(vtkPolyData* inputROIPoints);

  /// Find the transform to align the contour normals with the Z-axis.
  void CalculateContourTransform(vtkPolyData* inputPolyData, vtkMatrix4x4* contourToRAS);

  /// Find the normal of all of the contours in the polydata.
  void CalculateContourNormal(vtkPolyData* inputPolyData, double outputNormal[3], int minimumContourSize);

private:
  vtkPlanarContourToClosedSurfaceByRasterizationConversionRule(const vtkPlanarContourToClosedSurfaceByRasterizationConversionRule&) = delete;
  void operator=(const vtkPlanarContourToClosedSurfaceByRasterizationConversionRule&) = delete;
};

#endif
