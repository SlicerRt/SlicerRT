/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include <string.h>
// ModuleTemplate includes
#include "vtkSlicerPlmSlicerBsplineLogic.h"
#include "vtkMRMLPlmSlicerBsplineParametersNode.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageExport.h>

// STD includes
#include <cassert>

// Plastimatch includes
#include "plm_path.h"   // <-- unghf!
#include "plmbase.h"
#include "plmregister.h"

/* JAS 2012.06.22 */
/* We must include plm_path.h because Registration_parms
 * has fixed array members defined using _MAX_PATH.  Without
 * it, array offsets will not match and you'll segfault. */

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlmSlicerBsplineLogic);

//---------------------------------------------------------------------------- 
vtkSlicerPlmSlicerBsplineLogic::vtkSlicerPlmSlicerBsplineLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerPlmSlicerBsplineLogic::~vtkSlicerPlmSlicerBsplineLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerPlmSlicerBsplineLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlmSlicerBsplineLogic::InitializeEventListeners()
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(this->GetMRMLScene(), events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerPlmSlicerBsplineLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerPlmSlicerBsplineLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerPlmSlicerBsplineLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerPlmSlicerBsplineLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//----------------------------------------------------------------------------
void vtkSlicerPlmSlicerBsplineLogic::ConvertVtkImageToItkImage(vtkImageData* inVolume, itk::Image<float, 3>::Pointer outVolume)
{
  if (inVolume == NULL) {
    vtkErrorMacro ("Failed to convert vtk image to itk image - input image is NULL!"); 
    return; 
  }

  if (outVolume.IsNull()) {
    vtkErrorMacro("Failed to convert vtk image to itk image - output image is NULL!"); 
    return; 
  }

  // convert vtkImageData to itkImage 
  vtkSmartPointer<vtkImageExport> imageExport = vtkSmartPointer<vtkImageExport>::New(); 
  imageExport->SetInput (inVolume); 
  imageExport->Update (); 

  int extent[6]={0,0,0,0,0,0}; 
  inVolume->GetExtent (extent); 

  itk::Image<float, 3>::SizeType size;
  size[0] = extent[1] - extent[0] + 1;
  size[1] = extent[3] - extent[2] + 1;
  size[2] = extent[5] - extent[4] + 1;

  itk::Image<float, 3>::IndexType start;
  //double* inputOrigin = inVolume->GetOrigin();
  //start[0]=inputOrigin[0];
  //start[1]=inputOrigin[1];
  //start[2]=inputOrigin[2];
  start[0]=0.0;
  start[1]=0.0;
  start[2]=0.0;

  itk::Image<float, 3>::RegionType region;
  region.SetSize (size);
  region.SetIndex (start);
  outVolume->SetRegions (region);

  //double* inputSpacing = inVolume->GetSpacing();
  //outVolume->SetSpacing(inputSpacing);

  try {
    outVolume->Allocate ();
  }
  catch (itk::ExceptionObject & err) {
    vtkErrorMacro ("Failed to allocate memory for the image conversion: " << err.GetDescription() ); 
    return; 
  }

  imageExport->Export (outVolume->GetBufferPointer()); 
}


//----------------------------------------------------------------------------
//int vtkSlicerPlmSlicerBsplineLogic::Apply()
int vtkSlicerPlmSlicerBsplineLogic::Apply(vtkMRMLPlmSlicerBsplineParametersNode* pnode)
{
  vtkMRMLScene *scene = this->GetMRMLScene();

  vtkMRMLVolumeNode *fixedInputVolume = 
    vtkMRMLVolumeNode::SafeDownCast(scene->GetNodeByID(pnode->GetFixedVolumeNodeID()));
  vtkMRMLVolumeNode *movingInputVolume = 
    vtkMRMLVolumeNode::SafeDownCast(scene->GetNodeByID(pnode->GetMovingVolumeNodeID()));
  vtkMRMLVolumeNode *warpedInputVolume = 
    vtkMRMLVolumeNode::SafeDownCast(scene->GetNodeByID(pnode->GetWarpedVolumeNodeID()));
  vtkMRMLVolumeNode *xformInputVolume = 
    vtkMRMLVolumeNode::SafeDownCast(scene->GetNodeByID(pnode->GetXformVolumeNodeID()));

  if (!fixedInputVolume) {
    std::cerr << "Failed to look up fixed volume!" << std::endl;
    return -1;
  }
  if (!movingInputVolume) {
    std::cerr << "Failed to look up moving volume!" << std::endl;
    return -1;
  }
  if (!warpedInputVolume) {
    std::cerr << "Failed to look up warped volume!" << std::endl;
    return -1;
  }
  if (!xformInputVolume) {
    std::cerr << "Failed to look up xform volume!" << std::endl;
    return -1;
  }

  // Convert input images to the format Plastimatch can use
  itk::Image<float, 3>::Pointer fixedInputVolumeItk = itk::Image<float, 3>::New();
  itk::Image<float, 3>::Pointer movingInputVolumeItk = itk::Image<float, 3>::New();
  itk::Image<float, 3>::Pointer warpedInputVolumeItk = itk::Image<float, 3>::New();
  itk::Image<float, 3>::Pointer xformInputVolumeItk = itk::Image<float, 3>::New();
  Plm_image* fixed  = new Plm_image (fixedInputVolumeItk);
  Plm_image* moving = new Plm_image (movingInputVolumeItk);

  ConvertVtkImageToItkImage (fixedInputVolume->GetImageData(),  fixedInputVolumeItk);
  ConvertVtkImageToItkImage (movingInputVolume->GetImageData(), movingInputVolumeItk);
//  ConvertVtkImageToItkImage (warpedInputVolume->GetImageData(), warpedInputVolumeItk);
//  ConvertVtkImageToItkImage (xformInputVolume->GetImageData(),  xformInputVolumeItk);

  /* Setup the registration parms - single stage only */

  /* JAS 2012.06.22
   *   There seems to be a linking issue somewhere
   *   because writing to members of regp crashes
   *   Slicer.  Oddly, reading back values set by
   *   the regp constructor works fine. o_O */
#if 0
  Registration_parms regp;
  regp.num_stages = 1;
  fprintf (stderr, "num_stages: %i\n", regp.num_stages);
  regp.stages[0] = new Stage_parms ();
  regp.stages[0]->stage_no = 1;
  regp.stages[0]->grid_spac[0] = pnode->GetGridX ();
  regp.stages[0]->grid_spac[1] = pnode->GetGridY ();
  regp.stages[0]->grid_spac[2] = pnode->GetGridZ ();
#endif

  Registration_data regd;
  regd.fixed_image = fixed;
  regd.moving_image = moving;

  fprintf (stderr, "DEBUG: Control Grid: %i %i %i\n", 
                pnode->GetGridX(),
                pnode->GetGridY(),
                pnode->GetGridZ()
  );

  Xform* xf_out = NULL;

//  do_registration_pure (&xf_out, &regd, &regp);

  fprintf (stderr, "DEBUG: Apply () Complete!\n");

  return 0;
}
