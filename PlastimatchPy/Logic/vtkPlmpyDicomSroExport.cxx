/*==========================================================================

  Copyright (c) Massachusetts General Hospital, Boston, MA, USA. All Rights Reserved.
 
  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Gregory C. Sharp, Massachusetts General Hospital
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Natural Sciences and Engineering Research Council
  of Canada.

==========================================================================*/

#include "vtkPlmpyDicomSroExport.h"

// SlicerRT includes
#include "PlmCommon.h"

// vtkAddon includes
#include "vtkOrientedBSplineTransform.h"
#include "vtkOrientedGridTransform.h"

/// This workaround prevents a compilation fail with ITK-5.3
/// Something defines POSIX in preprocessor, and it conflicts
/// with ITK KWSys POSIX enumeration constant
#if (ITK_VERSION_MAJOR == 5) && (ITK_VERSION_MINOR > 2) && defined(POSIX)
#define PLMPOSIX_TMP (POSIX)
#undef POSIX
#endif

// MRML includes
#include <vtkITKTransformConverter.h>

#ifdef PLMPOSIX_TMP
#define POSIX (PLMPOSIX_TMP)
#undef PLMPOSIX_TMP
#endif

#include <vtkMRMLGridTransformNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>

// ITK includes
#include <itkAffineTransform.h>
#include <itkCastImageFilter.h>
#include <itkImage.h>

// Plastimatch include
#include "dicom_sro_save.h"
#include "plm_image.h"

//------------------------------------------------------------------------------

vtkStandardNewMacro(vtkPlmpyDicomSroExport);

//----------------------------------------------------------------------------
vtkPlmpyDicomSroExport::vtkPlmpyDicomSroExport()
{
  this->FixedImageID = nullptr;
  this->MovingImageID = nullptr;
  this->XformID = nullptr;
  this->OutputDirectory = nullptr;
  this->TransformsLogic = nullptr;
}

//----------------------------------------------------------------------------
vtkPlmpyDicomSroExport::~vtkPlmpyDicomSroExport()
{
  this->SetFixedImageID(nullptr);
  this->SetMovingImageID(nullptr);
  this->SetXformID(nullptr);
  this->SetOutputDirectory(nullptr);
  this->SetTransformsLogic(nullptr);
}

//----------------------------------------------------------------------------
void vtkPlmpyDicomSroExport::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

#if defined (commentout)
  os << indent << "Value: "  << this->Value;
  os << indent << "Radius: " << this->Radius;
  os << indent << "Shape: "  << this->Shape;

  // vtkSetObjectMacro
  os << indent << "Points: ";
  if (this->Points)
    {
    this->Points->PrintSelf(os << "\n" ,indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
#endif
}

//---------------------------------------------------------------------------
void vtkPlmpyDicomSroExport::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkPlmpyDicomSroExport::RegisterNodes()
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("RegisterNodes: Invalid MRML Scene!");
    return;
    }
}

//---------------------------------------------------------------------------
void vtkPlmpyDicomSroExport::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML Scene!");
    return;
    }
}

//----------------------------------------------------------------------------
int vtkPlmpyDicomSroExport::DoExport()
{
  if ( this->FixedImageID == nullptr
    || this->MovingImageID == nullptr
    || this->XformID == nullptr
    || this->OutputDirectory == nullptr)
  {
    vtkErrorMacro("DoExport: Invalid inputs");
    return 1;
  }

  if (this->GetMRMLScene() == nullptr)
  {
    vtkErrorMacro("DoExport: Invalid MRML scene");
    return 1;
  }

  // Convert input CT/MR image to the format Plastimatch can use
  vtkMRMLScalarVolumeNode* fixedVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->FixedImageID));
  Plm_image::Pointer fixedImage = PlmCommon::ConvertVolumeNodeToPlmImage(fixedVolumeNode, true);

  vtkMRMLScalarVolumeNode* movingVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->MovingImageID));
  Plm_image::Pointer movingImage = PlmCommon::ConvertVolumeNodeToPlmImage(movingVolumeNode, false);

  // Convert xform into a form that Plastimatch can use
  vtkMRMLTransformNode *xformNode = vtkMRMLTransformNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->XformID));
  if (!xformNode)
  {
    vtkErrorMacro("DoExport: Transform node was not actually a transform");
    return 1;
  }
  
  // Convert any deformable transform to Grid transform, because DICOM only supports vector fields
  bool removeTemporaryTransformNode = false;
  if (!xformNode->IsLinear())
  {
    if (!this->TransformsLogic)
    {
      vtkErrorMacro("DoExport: Failed to access transforms logic. Please set it using SetTransformsLogic");
      return 1;
    }
    vtkMRMLTransformNode* gridTransformNode = this->TransformsLogic->ConvertToGridTransform(xformNode, fixedVolumeNode);
    if (!gridTransformNode)
    {
      vtkErrorMacro("DoExport: Failed to convert transform '" << xformNode->GetName() << "' to grid transform");
      return 1;
    }
    else
    {
      vtkDebugMacro("DoExport: Converted transform '" << xformNode->GetName() << "' to grid transform");
      removeTemporaryTransformNode = true;
      xformNode = gridTransformNode;
    }
  }

  vtkAbstractTransform* transformVtk = xformNode->GetTransformToParent();
  itk::Object::Pointer transformItkObj;
  itk::Object::Pointer secondaryTransformItk; // only used for ITKv3 compatibility
  transformItkObj = vtkITKTransformConverter::CreateITKTransformFromVTK(this, transformVtk, secondaryTransformItk, 0);
  if (transformItkObj.IsNull())
  {
    vtkErrorMacro("DoExport: Cannot to convert VTK transform to ITK transform");
    return 1;
  }

  Xform::Pointer xform = Xform::New();
  if (xformNode->IsLinear())
  {
    itk::AffineTransform<double, 3>::Pointer affineTransformItk =
      dynamic_cast<itk::AffineTransform<double, 3> *>(transformItkObj.GetPointer());
    xform->set_aff(affineTransformItk);
  }
  else
  {
    // Get vector field as ITK image
    itk::DisplacementFieldTransform<double, 3>::Pointer gridTransformItk =
      dynamic_cast<itk::DisplacementFieldTransform<double, 3>*>(transformItkObj.GetPointer());
    itk::Image<itk::Vector<double, 3>, 3>::Pointer gridTransformImage = gridTransformItk->GetDisplacementField();

    // Cast double vector image to float vector image
    using FilterType = itk::CastImageFilter< itk::Image<itk::Vector<double, 3>, 3>, itk::Image<itk::Vector<float, 3>, 3> >;
    FilterType::Pointer castImageFilter = FilterType::New();
    castImageFilter->SetInput(gridTransformImage);
    castImageFilter->Update();

    xform->set_itk_vf(castImageFilter->GetOutput());
  }

  // Run exporter
  Dicom_sro_save dss;
  dss.set_fixed_image (fixedImage);
  dss.set_moving_image (movingImage);
  dss.set_xform (xform);
  dss.set_output_dir (this->OutputDirectory);
  dss.run (); //TODO: Error handling in exporter

  if (removeTemporaryTransformNode)
  {
    this->GetMRMLScene()->RemoveNode(xformNode);
  }

  return 0;
}
