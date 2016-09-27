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

  This file was originally developed by Kyle Sunderland, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

  This file is a modified version of vtkPolyDataToImageStencil.h from VTK

==============================================================================*/

#ifndef vtkPolyDataToFractionalLabelMap_h
#define vtkPolyDataToFractionalLabelMap_h

#include "vtkSlicerRtCommonWin32Header.h"

// VTK includes
#include <vtkPolyDataToImageStencil.h>
#include <vtkSmartPointer.h>
#include <vtkCellArray.h>
#include <vtkSetGet.h>
#include <vtkMatrix4x4.h>
#include <vtkCellLocator.h>

//
#include <vtkOrientedImageData.h>

#include <map>

class VTK_SLICERRTCOMMON_EXPORT vtkPolyDataToFractionalLabelMap :
  public vtkPolyDataToImageStencil
{
private:
  std::map<double, vtkSmartPointer<vtkCellArray> > LinesCache;
  std::map<double, vtkSmartPointer<vtkPolyData> > SliceCache;
  std::map<double, vtkIdType*> PointIdsCache;
  std::map<double, vtkIdType> NptsCache;
  std::map<double,  vtkSmartPointer<vtkIdTypeArray> > PointNeighborCountsCache;

  vtkCellLocator* CellLocator;

  vtkMatrix4x4* OutputImageToWorldMatrix;
  int NumberOfOffsets;

public:
  static vtkPolyDataToFractionalLabelMap* New();
  vtkTypeMacro(vtkPolyDataToFractionalLabelMap, vtkPolyDataToImageStencil);

  virtual vtkOrientedImageData* GetOutput();
  virtual void SetOutput(vtkOrientedImageData* output);

  /// This method deletes the currently stored cache variables
  void DeleteCache();

  vtkSetObjectMacro(OutputImageToWorldMatrix, vtkMatrix4x4);
  vtkGetObjectMacro(OutputImageToWorldMatrix, vtkMatrix4x4);

  vtkSetMacro(NumberOfOffsets, int);
  vtkGetMacro(NumberOfOffsets, int);

protected:
  vtkPolyDataToFractionalLabelMap();
  ~vtkPolyDataToFractionalLabelMap();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  vtkOrientedImageData *AllocateOutputData(vtkDataObject *out, int* updateExt);
  virtual int FillOutputPortInformation(int, vtkInformation*);

  /// Create a binary image stencil for the closed surface within the current extent
  /// This method is a modified version of vtkPolyDataToImageStencil::ThreadedExecute
  /// \param output Output stencil data
  /// \param closedSurface The input surface to be converted
  /// \param extent The extent region that is being converted
  void FillImageStencilData(vtkImageStencilData *output, vtkPolyData* closedSurface, int extent[6]);

  /// Add the values of the binary labelmap to the fractional labelmap.
  /// \param binaryLabelMap Binary labelmap that will be added to the fractional labelmap
  /// \param fractionalLabelMap The fractional labelmap that the binary labelmap is added to
  void AddBinaryLabelMapToFractionalLabelMap(vtkImageData* binaryLabelMap, vtkImageData* fractionalLabelMap);

  /// Clip the polydata at the specified z coordinate to create a planar contour.
  /// This method is a modified version of vtkPolyDataToImageStencil::PolyDataCutter to decrease execution time
  /// \param input The closed surface that is being cut
  /// \param output Polydata containing the contour lines
  /// \param z The z coordinate for the cutting plane
  void PolyDataCutter(vtkPolyData *input, vtkPolyData *output,
                             double z);

private:
  vtkPolyDataToFractionalLabelMap(const vtkPolyDataToFractionalLabelMap&);  // Not implemented.
  void operator=(const vtkPolyDataToFractionalLabelMap&);  // Not implemented.
};

#endif