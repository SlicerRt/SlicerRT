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

==============================================================================*/


// Beams includes
#include <vtkMRMLRTBeamNode.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLCameraNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>

#include "vtkMRMLDrrImageComputationNode.h"

// STD include
#include <cstring>

//------------------------------------------------------------------------------
namespace
{

const char* BEAM_REFERENCE_ROLE = "beamRef";
const char* CAMERA_REFERENCE_ROLE = "cameraRef";

} // namespace

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLDrrImageComputationNode);

//----------------------------------------------------------------------------
vtkMRMLDrrImageComputationNode::vtkMRMLDrrImageComputationNode()
{
  NormalVector[0] = 0.;
  NormalVector[1] = 0.;
  NormalVector[2] = 1.;
  ViewUpVector[0] = -1.;
  ViewUpVector[1] = 0.;
  ViewUpVector[2] = 0.;
  ImagerCenterOffset[0] = 0.; // columns = x
  ImagerCenterOffset[1] = 0.; // rows = y
  ImagerResolution[0] = 2000; // columns = x
  ImagerResolution[1] = 2000; // rows = y

  ImagerSpacing[0] = 0.25; // 250 um (columns = x)
  ImagerSpacing[1] = 0.25; // 250 um (rows = y)

  // default image window is whole imager
  ImageWindowFlag = false;
  ImageWindow[0] = 0; // c1 = x0 (start column) 
  ImageWindow[1] = 0; // r1 = y0 (start row)
  ImageWindow[2] = ImagerResolution[0] - 1; // c2 = x1 (end column)
  ImageWindow[3] = ImagerResolution[1] - 1; // r2 = y1 (end row)

  ImageCenter[0] = ImageWindow[0] + (ImageWindow[2] - ImageWindow[0]) / 2.f; // column
  ImageCenter[1] = ImageWindow[1] + (ImageWindow[3] - ImageWindow[1]) / 2.f; // row

  AlgorithmReconstuction = Uniform;
  HUConversion = Preprocess;
  Threading = CPU;
  ExponentialMappingFlag = true;
  AutoscaleFlag = true;
  AutoscaleRange[0] = 0.;
  AutoscaleRange[1] = 255.;
  InvertIntensityFlag = true;

  IsocenterImagerDistance = 300.;
  HUThresholdBelow = -1000;

  // Observe RTBeam node events (like change of transform or geometry)
  vtkNew<vtkIntArray> nodeEvents;
  nodeEvents->InsertNextValue(vtkCommand::ModifiedEvent);
  nodeEvents->InsertNextValue(vtkMRMLRTBeamNode::BeamGeometryModified);
  nodeEvents->InsertNextValue(vtkMRMLRTBeamNode::BeamTransformModified);
  this->AddNodeReferenceRole(BEAM_REFERENCE_ROLE, nullptr, nodeEvents);
}

//----------------------------------------------------------------------------
vtkMRMLDrrImageComputationNode::~vtkMRMLDrrImageComputationNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLDrrImageComputationNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLVectorMacro(NormalVector, NormalVector, double, 3);
  vtkMRMLWriteXMLVectorMacro(ViewUpVector, ViewUpVector, double, 3);
  vtkMRMLWriteXMLFloatMacro(IsocenterImagerDistance, IsocenterImagerDistance);
  vtkMRMLWriteXMLVectorMacro(ImagerCenterOffset, ImagerCenterOffset, double, 2);
  vtkMRMLWriteXMLVectorMacro(ImagerResolution, ImagerResolution, int, 2);
  vtkMRMLWriteXMLVectorMacro(ImagerSpacing, ImagerSpacing, double, 2);
  vtkMRMLWriteXMLVectorMacro(ImageCenter, ImageCenter, int, 2);
  vtkMRMLWriteXMLBooleanMacro(ImageWindowFlag, ImageWindowFlag);
  vtkMRMLWriteXMLVectorMacro(ImageWindow, ImageWindow, int, 4);
  vtkMRMLWriteXMLBooleanMacro(ExponentialMappingFlag, ExponentialMappingFlag);
  vtkMRMLWriteXMLBooleanMacro(AutoscaleFlag, AutoscaleFlag);
  vtkMRMLWriteXMLBooleanMacro(InvertIntensityFlag, InvertIntensityFlag);
  vtkMRMLWriteXMLVectorMacro(AutoscaleRange, AutoscaleRange, float, 2); 
  vtkMRMLWriteXMLIntMacro(AlgorithmReconstuction, AlgorithmReconstuction);
  vtkMRMLWriteXMLIntMacro(HUConversion, HUConversion);
  vtkMRMLWriteXMLIntMacro(HUThresholdBelow, HUThresholdBelow);
  vtkMRMLWriteXMLIntMacro(Threading, Threading);
  // add new parameters here
  vtkMRMLWriteXMLEndMacro(); 
}

//----------------------------------------------------------------------------
void vtkMRMLDrrImageComputationNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  vtkMRMLNode::ReadXMLAttributes(atts);

  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLVectorMacro(NormalVector, NormalVector, double, 3);
  vtkMRMLReadXMLVectorMacro(ViewUpVector, ViewUpVector, double, 3);
  vtkMRMLReadXMLFloatMacro(IsocenterImagerDistance, IsocenterImagerDistance);
  vtkMRMLReadXMLVectorMacro(ImagerCenterOffset, ImagerCenterOffset, double, 2);
  vtkMRMLReadXMLVectorMacro(ImagerResolution, ImagerResolution, int, 2);
  vtkMRMLReadXMLVectorMacro(ImagerSpacing, ImagerSpacing, double, 2);
  vtkMRMLReadXMLVectorMacro(ImageCenter, ImageCenter, int, 2);
  vtkMRMLReadXMLBooleanMacro(ImageWindowFlag, ImageWindowFlag);
  vtkMRMLReadXMLVectorMacro(ImageWindow, ImageWindow, int, 4);
  vtkMRMLReadXMLBooleanMacro(ExponentialMappingFlag, ExponentialMappingFlag);
  vtkMRMLReadXMLBooleanMacro(AutoscaleFlag, AutoscaleFlag);
  vtkMRMLReadXMLBooleanMacro(InvertIntensityFlag, InvertIntensityFlag);
  vtkMRMLReadXMLVectorMacro(AutoscaleRange, AutoscaleRange, float, 2);
  vtkMRMLReadXMLIntMacro(AlgorithmReconstuction, AlgorithmReconstuction);
  vtkMRMLReadXMLIntMacro(HUConversion, HUConversion);
  vtkMRMLReadXMLIntMacro(HUThresholdBelow, HUThresholdBelow);
  vtkMRMLReadXMLIntMacro(Threading, Threading);
  // add new parameters here
  vtkMRMLReadXMLEndMacro();

  this->EndModify(disabledModify);

  // Note: ReportString is not read from XML, it is a strictly temporary value
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
void vtkMRMLDrrImageComputationNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);

  vtkMRMLDrrImageComputationNode* node = vtkMRMLDrrImageComputationNode::SafeDownCast(anode);
  if (!node)
  {
    return;
  }

  // Copy beam parameters
  this->DisableModifiedEventOn();
  vtkMRMLCopyBeginMacro(node);
  vtkMRMLCopyVectorMacro(NormalVector, double, 3);
  vtkMRMLCopyVectorMacro(ViewUpVector, double, 3);
  vtkMRMLCopyFloatMacro(IsocenterImagerDistance);
  vtkMRMLCopyVectorMacro(ImagerCenterOffset, double, 2);
  vtkMRMLCopyVectorMacro(ImagerResolution, int, 2);
  vtkMRMLCopyVectorMacro(ImagerSpacing, double, 2);
  vtkMRMLCopyVectorMacro(ImageCenter, int, 2);
  vtkMRMLCopyBooleanMacro(ImageWindowFlag);
  vtkMRMLCopyVectorMacro(ImageWindow, int, 4);
  vtkMRMLCopyBooleanMacro(ExponentialMappingFlag);
  vtkMRMLCopyBooleanMacro(AutoscaleFlag);
  vtkMRMLCopyBooleanMacro(InvertIntensityFlag);
  vtkMRMLCopyVectorMacro(AutoscaleRange, float, 2);
  vtkMRMLCopyIntMacro(AlgorithmReconstuction);
  vtkMRMLCopyIntMacro(HUConversion);
  vtkMRMLCopyIntMacro(HUThresholdBelow);
  vtkMRMLCopyIntMacro(Threading);
  // add new parameters here
  vtkMRMLCopyEndMacro(); 

  this->EndModify(disabledModify);

  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLDrrImageComputationNode::CopyContent(vtkMRMLNode *anode, bool deepCopy/*=true*/)
{
  MRMLNodeModifyBlocker blocker(this);
  Superclass::CopyContent(anode, deepCopy);

  vtkMRMLDrrImageComputationNode* node = vtkMRMLDrrImageComputationNode::SafeDownCast(anode);
  if (!node)
  {
    return;
  }

  vtkMRMLCopyBeginMacro(node);
  vtkMRMLCopyVectorMacro(NormalVector, double, 3);
  vtkMRMLCopyVectorMacro(ViewUpVector, double, 3);
  vtkMRMLCopyFloatMacro(IsocenterImagerDistance);
  vtkMRMLCopyVectorMacro(ImagerCenterOffset, double, 2);
  vtkMRMLCopyVectorMacro(ImagerResolution, int, 2);
  vtkMRMLCopyVectorMacro(ImagerSpacing, double, 2);
  vtkMRMLCopyVectorMacro(ImageCenter, int, 2);
  vtkMRMLCopyBooleanMacro(ImageWindowFlag);
  vtkMRMLCopyVectorMacro(ImageWindow, int, 4);
  vtkMRMLCopyBooleanMacro(ExponentialMappingFlag);
  vtkMRMLCopyBooleanMacro(AutoscaleFlag);
  vtkMRMLCopyBooleanMacro(InvertIntensityFlag);
  vtkMRMLCopyVectorMacro(AutoscaleRange, float, 2);
  vtkMRMLCopyIntMacro(AlgorithmReconstuction);
  vtkMRMLCopyIntMacro(HUConversion);
  vtkMRMLCopyIntMacro(HUThresholdBelow);
  vtkMRMLCopyIntMacro(Threading);
  // add new parameters here
  vtkMRMLCopyEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLDrrImageComputationNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintVectorMacro(NormalVector, double, 3);
  vtkMRMLPrintVectorMacro(ViewUpVector, double, 3);
  vtkMRMLPrintFloatMacro(IsocenterImagerDistance);
  vtkMRMLPrintVectorMacro(ImagerCenterOffset, double, 2);
  vtkMRMLPrintVectorMacro(ImagerResolution, int, 2);
  vtkMRMLPrintVectorMacro(ImagerSpacing, double, 2);
  vtkMRMLPrintVectorMacro(ImageCenter, int, 2);
  vtkMRMLPrintBooleanMacro(ImageWindowFlag);
  vtkMRMLPrintVectorMacro(ImageWindow, int, 4);
  vtkMRMLPrintBooleanMacro(ExponentialMappingFlag);
  vtkMRMLPrintBooleanMacro(AutoscaleFlag);
  vtkMRMLPrintBooleanMacro(InvertIntensityFlag);
  vtkMRMLPrintVectorMacro(AutoscaleRange, float, 2);
  vtkMRMLPrintIntMacro(AlgorithmReconstuction);
  vtkMRMLPrintIntMacro(HUConversion);
  vtkMRMLPrintIntMacro(HUThresholdBelow);
  vtkMRMLPrintIntMacro(Threading);
  // add new parameters here
  vtkMRMLPrintEndMacro(); 
}

//----------------------------------------------------------------------------
void vtkMRMLDrrImageComputationNode::ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, eventID, callData);

  if (!this->Scene)
  {
    vtkErrorMacro("ProcessMRMLEvents: Invalid MRML scene");
    return;
  }
  if (this->Scene->IsBatchProcessing())
  {
    return;
  }

  // Update the DRR View-Up and normal vectors, if beam geometry or transform was changed
  switch (eventID)
  {
  case vtkMRMLRTBeamNode::BeamGeometryModified:
  case vtkMRMLRTBeamNode::BeamTransformModified:
    this->Modified();
    break;
  default:
    break;
  }
}

//----------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkMRMLDrrImageComputationNode::GetBeamNode()
{
  return vtkMRMLRTBeamNode::SafeDownCast( this->GetNodeReference(BEAM_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLDrrImageComputationNode::SetAndObserveBeamNode(vtkMRMLRTBeamNode* node)
{
  if (node && this->Scene != node->GetScene())
  {
    vtkErrorMacro("SetAndObserveBeamNode: Cannot set reference, the referenced and referencing node are not in the same scene");
    return;
  }

  this->SetNodeReferenceID(BEAM_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//----------------------------------------------------------------------------
vtkMRMLCameraNode* vtkMRMLDrrImageComputationNode::GetCameraNode()
{
  return vtkMRMLCameraNode::SafeDownCast( this->GetNodeReference(CAMERA_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLDrrImageComputationNode::SetAndObserveCameraNode(vtkMRMLCameraNode* node)
{
  if (node && this->Scene != node->GetScene())
  {
    vtkErrorMacro("SetAndObserveCameraNode: Cannot set reference, the referenced and referencing node are not in the same scene");
    return;
  }

  this->SetNodeReferenceID(CAMERA_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//----------------------------------------------------------------------------
void vtkMRMLDrrImageComputationNode::GetRTImagePosition(double position[2])
{
  position[0] = 0.;
  position[1] = 0.;
  double offsetX = double(ImagerResolution[1]) * ImagerSpacing[1] / 2.;
  double offsetY = double(ImagerResolution[0]) * ImagerSpacing[0] / 2.;

  position[0] -= offsetX;
  position[1] -= offsetY;

  if (ImageWindowFlag)
  {
    position[0] += ImagerSpacing[1] * double(ImageWindow[1]); // columns (X)
    position[1] += ImagerSpacing[0] * double(ImageWindow[0]); // rows (Y)
  }
}

//----------------------------------------------------------------------------
void vtkMRMLDrrImageComputationNode::SetAlgorithmReconstuction(int algorithmReconstuction)
{
  switch (algorithmReconstuction)
  {
    case 1:
      SetAlgorithmReconstuction(PlastimatchAlgorithmReconstuctionType::Uniform);
      break;
    case 0:
    default:
      SetAlgorithmReconstuction(PlastimatchAlgorithmReconstuctionType::Exact);
      break;
  };
}

//----------------------------------------------------------------------------
void vtkMRMLDrrImageComputationNode::SetHUConversion(int huConvension)
{
  switch (huConvension)
  {
    case 1:
      SetHUConversion(PlastimatchHounsfieldUnitsConversionType::Inline);
      break;
    case 2:
      SetHUConversion(PlastimatchHounsfieldUnitsConversionType::None);
      break;
    case 0:
    default:
      SetHUConversion(PlastimatchHounsfieldUnitsConversionType::Preprocess);
      break;
  };
}

//----------------------------------------------------------------------------
void vtkMRMLDrrImageComputationNode::SetThreading(int threading)
{
  switch (threading)
  {
    case 1:
      SetThreading(PlastimatchThreadingType::CUDA);
      break;
    case 2:
      SetThreading(PlastimatchThreadingType::OpenCL);
      break;
    case 0:
    default:
      SetThreading(PlastimatchThreadingType::CPU);
      break;
  };
}

//----------------------------------------------------------------------------
void vtkMRMLDrrImageComputationNode::GetIsocenterPositionLPS(double isocenterPosition[3])
{
  vtkMRMLRTBeamNode* beamNode = this->GetBeamNode();
  if (!beamNode)
  {
    vtkErrorMacro("GetIsocenterPositionLPS: RT Beam node is invalid");
    return;
  }

  // Isocenter RAS position, for plastimatch isocenter MUST BE in LPS system
  beamNode->GetPlanIsocenterPosition(isocenterPosition);
  // Position from RAS to LPS
  isocenterPosition[0] *= -1.;
  isocenterPosition[1] *= -1.;
}

//----------------------------------------------------------------------------
void vtkMRMLDrrImageComputationNode::GetImageCenter(double imageCenter[2])
{
  int image_window[4] = {};
  int image_center[2] = {};
  if (this->ImageWindowFlag)
  {
    image_window[0] = std::max<int>( 0, this->ImageWindow[0]); // start column
    image_window[1] = std::min<int>( this->ImagerResolution[0] - 1, this->ImageWindow[2]); // end column
    image_window[2] = std::max<int>( 0, this->ImageWindow[1]); // start row
    image_window[3] = std::min<int>( this->ImagerResolution[1] - 1, this->ImageWindow[3]); // end row
  }
  else
  {
    image_window[0] = 0; // start column
    image_window[1] = this->ImagerResolution[0] - 1; // end column
    image_window[2] = 0; // start row
    image_window[3] = this->ImagerResolution[1] - 1; // end row
  }
  imageCenter[0] = image_window[0] + (image_window[1] - image_window[0]) / 2.; // column
  imageCenter[1] = image_window[2] + (image_window[3] - image_window[2]) / 2.; // row
  image_center[0] = static_cast<int>(imageCenter[0]);
  image_center[1] = static_cast<int>(imageCenter[1]);
  this->SetImageCenter(image_center);
}
