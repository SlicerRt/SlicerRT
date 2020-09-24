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

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>

// STD includes
#include <sstream>

#include "vtkMRMLPlmDrrNode.h"

//------------------------------------------------------------------------------
namespace
{

const char* BEAM_REFERENCE_ROLE = "beamRef";

} // namespace

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlmDrrNode);

//----------------------------------------------------------------------------
vtkMRMLPlmDrrNode::vtkMRMLPlmDrrNode()
{
  ImagerCenterOffset[0] = 0.;
  ImagerCenterOffset[1] = 0.;
  ImageDimention[0] = 2000; // columns = x
  ImageDimention[1] = 2000; // rows = y

  ImageSpacing[0] = 0.25; // 250 um (columns = x)
  ImageSpacing[1] = 0.25; // 250 um (rows = y)

  // default image window is whole imager
  ImageWindowFlag = false;
  ImageWindow[0] = 0; // c1 = x0 (start column) 
  ImageWindow[1] = 0; // r1 = y0 (start row)
  ImageWindow[2] = ImageDimention[0] - 1; // c2 = x1 (end column)
  ImageWindow[3] = ImageDimention[1] - 1; // r2 = y1 (end row)

  ImageCenter[0] = double(ImageWindow[2]) / 2.; // columns = x
  ImageCenter[1] = double(ImageWindow[3]) / 2.; // rows = y

  AlgorithmReconstuction = EXACT;
  HUConversion = PREPROCESS;
  Threading = CPU;
  ExponentialMappingFlag = true;
  AutoscaleFlag = false;
  AutoscaleRange[0] = 0;
  AutoscaleRange[1] = 255;

  IsocenterImagerDistance = 300.;
  RotateX = 0.;
  RotateY = 0.;
  RotateZ = 0.;

  this->SetSingletonTag("DRR");
}

//----------------------------------------------------------------------------
vtkMRMLPlmDrrNode::~vtkMRMLPlmDrrNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLPlmDrrNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLVectorMacro(NormalVector, NormalVector, double, 4);
  vtkMRMLWriteXMLVectorMacro(ViewUpVector, ViewUpVector, double, 4);
  vtkMRMLWriteXMLFloatMacro(IsocenterImagerDistance, IsocenterImagerDistance);
  vtkMRMLWriteXMLVectorMacro(ImagerCenterOffset, ImagerCenterOffset, double, 2);
  vtkMRMLWriteXMLVectorMacro(ImageDimention, ImageDimention, int, 2);
  vtkMRMLWriteXMLVectorMacro(ImageSpacing, ImageSpacing, double, 2);
  vtkMRMLWriteXMLVectorMacro(ImageCenter, ImageCenter, int, 2);
  vtkMRMLWriteXMLBooleanMacro(ImageWindowFlag, ImageWindowFlag);
  vtkMRMLWriteXMLVectorMacro(ImageWindow, ImageWindow, int, 4);
  vtkMRMLWriteXMLBooleanMacro(ExponentialMappingFlag, ExponentialMappingFlag);
  vtkMRMLWriteXMLBooleanMacro(AutoscaleFlag, AutoscaleFlag);
  vtkMRMLWriteXMLVectorMacro(AutoscaleRange, AutoscaleRange, signed long int, 2); 
  vtkMRMLWriteXMLIntMacro(AlgorithmReconstuction, AlgorithmReconstuction);
  vtkMRMLWriteXMLIntMacro(HUConversion, HUConversion);
  vtkMRMLWriteXMLIntMacro(Threading, Threading);
  vtkMRMLWriteXMLFloatMacro(RotateX, RotateX);
  vtkMRMLWriteXMLFloatMacro(RotateY, RotateY);
  vtkMRMLWriteXMLFloatMacro(RotateZ, RotateZ);
  // add new parameters here
  vtkMRMLWriteXMLEndMacro(); 
}

//----------------------------------------------------------------------------
void vtkMRMLPlmDrrNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  vtkMRMLNode::ReadXMLAttributes(atts);

  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLVectorMacro(NormalVector, NormalVector, double, 4);
  vtkMRMLReadXMLVectorMacro(ViewUpVector, ViewUpVector, double, 4);
  vtkMRMLReadXMLFloatMacro(IsocenterImagerDistance, IsocenterImagerDistance);
  vtkMRMLReadXMLVectorMacro(ImagerCenterOffset, ImagerCenterOffset, double, 2);
  vtkMRMLReadXMLVectorMacro(ImageDimention, ImageDimention, int, 2);
  vtkMRMLReadXMLVectorMacro(ImageSpacing, ImageSpacing, double, 2);
  vtkMRMLReadXMLVectorMacro(ImageCenter, ImageCenter, int, 2);
  vtkMRMLReadXMLBooleanMacro(ImageWindowFlag, ImageWindowFlag);
  vtkMRMLReadXMLVectorMacro(ImageWindow, ImageWindow, int, 4);
  vtkMRMLReadXMLBooleanMacro(ExponentialMappingFlag, ExponentialMappingFlag);
  vtkMRMLReadXMLBooleanMacro(AutoscaleFlag, AutoscaleFlag);
  vtkMRMLReadXMLVectorMacro(AutoscaleRange, AutoscaleRange, signed long int, 2);
  vtkMRMLReadXMLIntMacro(AlgorithmReconstuction, AlgorithmReconstuction);
  vtkMRMLReadXMLIntMacro(HUConversion, HUConversion);
  vtkMRMLReadXMLIntMacro(Threading, Threading);
  vtkMRMLReadXMLFloatMacro(RotateX, RotateX);
  vtkMRMLReadXMLFloatMacro(RotateY, RotateY);
  vtkMRMLReadXMLFloatMacro(RotateZ, RotateZ);
  // add new parameters here
  vtkMRMLReadXMLEndMacro();

  this->EndModify(disabledModify);

  // Note: ReportString is not read from XML, it is a strictly temporary value
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
void vtkMRMLPlmDrrNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);

  vtkMRMLPlmDrrNode* node = vtkMRMLPlmDrrNode::SafeDownCast(anode);
  if (!node)
  {
    return;
  }

  // Copy beam parameters
  this->DisableModifiedEventOn();
  vtkMRMLCopyBeginMacro(node);
  vtkMRMLCopyVectorMacro(NormalVector, double, 4);
  vtkMRMLCopyVectorMacro(ViewUpVector, double, 4);
  vtkMRMLCopyFloatMacro(IsocenterImagerDistance);
  vtkMRMLCopyVectorMacro(ImagerCenterOffset, double, 2);
  vtkMRMLCopyVectorMacro(ImageDimention, int, 2);
  vtkMRMLCopyVectorMacro(ImageSpacing, double, 2);
  vtkMRMLCopyVectorMacro(ImageCenter, int, 2);
  vtkMRMLCopyBooleanMacro(ImageWindowFlag);
  vtkMRMLCopyVectorMacro(ImageWindow, int, 4);
  vtkMRMLCopyBooleanMacro(ExponentialMappingFlag);
  vtkMRMLCopyBooleanMacro(AutoscaleFlag);
  vtkMRMLCopyVectorMacro(AutoscaleRange, signed long int, 2);
  vtkMRMLCopyIntMacro(AlgorithmReconstuction);
  vtkMRMLCopyIntMacro(HUConversion);
  vtkMRMLCopyIntMacro(Threading);
  vtkMRMLCopyFloatMacro(RotateX);
  vtkMRMLCopyFloatMacro(RotateY);
  vtkMRMLCopyFloatMacro(RotateZ);
  // add new parameters here
  vtkMRMLCopyEndMacro(); 

  this->EndModify(disabledModify);

  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLPlmDrrNode::CopyContent(vtkMRMLNode *anode, bool deepCopy/*=true*/)
{
  MRMLNodeModifyBlocker blocker(this);
  Superclass::CopyContent(anode, deepCopy);

  vtkMRMLPlmDrrNode* node = vtkMRMLPlmDrrNode::SafeDownCast(anode);
  if (!node)
  {
    return;
  }

  vtkMRMLCopyBeginMacro(node);
  vtkMRMLCopyVectorMacro(NormalVector, double, 4);
  vtkMRMLCopyVectorMacro(ViewUpVector, double, 4);
  vtkMRMLCopyFloatMacro(IsocenterImagerDistance);
  vtkMRMLCopyVectorMacro(ImagerCenterOffset, double, 2);
  vtkMRMLCopyVectorMacro(ImageDimention, int, 2);
  vtkMRMLCopyVectorMacro(ImageSpacing, double, 2);
  vtkMRMLCopyVectorMacro(ImageCenter, int, 2);
  vtkMRMLCopyBooleanMacro(ImageWindowFlag);
  vtkMRMLCopyVectorMacro(ImageWindow, int, 4);
  vtkMRMLCopyBooleanMacro(ExponentialMappingFlag);
  vtkMRMLCopyBooleanMacro(AutoscaleFlag);
  vtkMRMLCopyVectorMacro(AutoscaleRange, signed long int, 2);
  vtkMRMLCopyIntMacro(AlgorithmReconstuction);
  vtkMRMLCopyIntMacro(HUConversion);
  vtkMRMLCopyIntMacro(Threading);
  vtkMRMLCopyFloatMacro(RotateX);
  vtkMRMLCopyFloatMacro(RotateY);
  vtkMRMLCopyFloatMacro(RotateZ);
  // add new parameters here
  vtkMRMLCopyEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLPlmDrrNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintVectorMacro(NormalVector, double, 4);
  vtkMRMLPrintVectorMacro(ViewUpVector, double, 4);
  vtkMRMLPrintFloatMacro(IsocenterImagerDistance);
  vtkMRMLPrintVectorMacro(ImagerCenterOffset, double, 2);
  vtkMRMLPrintVectorMacro(ImageDimention, int, 2);
  vtkMRMLPrintVectorMacro(ImageSpacing, double, 2);
  vtkMRMLPrintVectorMacro(ImageCenter, int, 2);
  vtkMRMLPrintBooleanMacro(ImageWindowFlag);
  vtkMRMLPrintVectorMacro(ImageWindow, int, 4);
  vtkMRMLPrintBooleanMacro(ExponentialMappingFlag);
  vtkMRMLPrintBooleanMacro(AutoscaleFlag);
  vtkMRMLPrintVectorMacro(AutoscaleRange, signed long int, 2);
  vtkMRMLPrintIntMacro(AlgorithmReconstuction);
  vtkMRMLPrintIntMacro(HUConversion);
  vtkMRMLPrintIntMacro(Threading);
  vtkMRMLPrintFloatMacro(RotateX);
  vtkMRMLPrintFloatMacro(RotateY);
  vtkMRMLPrintFloatMacro(RotateZ);
  // add new parameters here
  vtkMRMLPrintEndMacro(); 
}

//----------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkMRMLPlmDrrNode::GetBeamNode()
{
  return vtkMRMLRTBeamNode::SafeDownCast( this->GetNodeReference(BEAM_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLPlmDrrNode::SetAndObserveBeamNode(vtkMRMLRTBeamNode* node)
{
  if (node && this->Scene != node->GetScene())
  {
    vtkErrorMacro("SetAndObserveBeamNode: Cannot set reference, the referenced and referencing node are not in the same scene");
    return;
  }

  this->SetNodeReferenceID(BEAM_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//----------------------------------------------------------------------------
void vtkMRMLPlmDrrNode::GetRTImagePosition(double position[2])
{
  double offsetX = double(ImageWindow[0]) * ImageSpacing[0];
  double offsetY = double(ImageWindow[1]) * ImageSpacing[1];

  position[0] = offsetX - ImageSpacing[0] * ImageDimention[0] / 2.; // columns (X)
  position[1] = offsetY - ImageSpacing[1] * ImageDimention[1] / 2.; // rows (Y)
}

//----------------------------------------------------------------------------
void vtkMRMLPlmDrrNode::SetAlgorithmReconstuction(int algorithmReconstuction)
{
  switch (algorithmReconstuction)
  {
    case 1:
      SetAlgorithmReconstuction(AlgorithmReconstuctionType::UNIFORM);
      break;
    case 0:
    default:
      SetAlgorithmReconstuction(AlgorithmReconstuctionType::EXACT);
      break;
  };
}

//----------------------------------------------------------------------------
void vtkMRMLPlmDrrNode::SetHUConversion(int huConvension)
{
  switch (huConvension)
  {
    case 1:
      SetHUConversion(HounsfieldUnitsConversionType::INLINE);
      break;
    case 2:
      SetHUConversion(HounsfieldUnitsConversionType::NONE);
      break;
    case 0:
    default:
      SetHUConversion(HounsfieldUnitsConversionType::PREPROCESS);
      break;
  };
}

//----------------------------------------------------------------------------
void vtkMRMLPlmDrrNode::SetThreading(int threading)
{
  switch (threading)
  {
    case 1:
      SetThreading(ThreadingType::CUDA);
      break;
    case 2:
      SetThreading(ThreadingType::OPENCL);
      break;
    case 0:
    default:
      SetThreading(ThreadingType::CPU);
      break;
  };
}

//----------------------------------------------------------------------------
std::string vtkMRMLPlmDrrNode::GenerateArguments(std::list< std::string >& plastimatchArguments)
{
  vtkMRMLRTBeamNode* beamNode = this->GetBeamNode();

  if (!beamNode)
  {
    vtkErrorMacro("GenerateArguments: Observed beam node is invalid");
    return std::string();
  }

  vtkMRMLTransformNode* beamTransformNode = beamNode->GetParentTransformNode();
  vtkTransform* beamTransform = nullptr;
  vtkNew<vtkMatrix4x4> mat; // DICOM beam transform matrix
  mat->Identity();

  if (beamTransformNode)
  {
    beamTransform = vtkTransform::SafeDownCast(beamTransformNode->GetTransformToParent());

    vtkNew<vtkTransform> rasToLpsTransform;
    rasToLpsTransform->Identity();
    rasToLpsTransform->RotateZ(180.0);
    
    vtkNew<vtkTransform> dicomBeamTransform;
    dicomBeamTransform->Identity();
    dicomBeamTransform->PreMultiply();
    dicomBeamTransform->Concatenate(rasToLpsTransform);
    dicomBeamTransform->Concatenate(beamTransform);

    dicomBeamTransform->GetMatrix(mat);
  }
  else
  {
    vtkWarningMacro("GenerateArguments: Beam transform node is invalid, identity matrix will be used instead");
  }

  double n[4], vup[4];
  double normalVector[4] = { 0., 0., 1., 0. }; // beam positive Z-axis
  double viewUpVector[4] = { -1., 0., 0., 0. }; // beam negative X-axis

  mat->MultiplyPoint( normalVector, n);
  mat->MultiplyPoint( viewUpVector, vup);

  this->SetNormalVector(n);
  this->SetViewUpVector(vup);

  // Isocenter RAS position, for plastimatch isocenter MUST BE in LPS position
  double isocenter[3];
  beamNode->GetPlanIsocenterPosition(isocenter);

  // image center
  double imageCenterX = double(this->ImageDimention[0] - 1) / 2.;
  double imageCenterY = double(this->ImageDimention[1] - 1) / 2.;
  if (this->ImageWindowFlag) // image window center
  {
    imageCenterX = double(this->ImageWindow[0]) + double(this->ImageWindow[2] - this->ImageWindow[0] - 1) / 2.;
    imageCenterY = double(this->ImageWindow[1]) + double(this->ImageWindow[3] - this->ImageWindow[1] - 1) / 2.;
  }

  std::ostringstream command;
  command << "plastimatch drr ";
  switch (this->Threading)
  {
    case ThreadingType::CPU:
      command << "-A cpu \\\n";
      break;
    case ThreadingType::CUDA:
      command << "-A cuda \\\n";
      break;
    case ThreadingType::OPENCL:
      command << "-A opencl \\\n";
      break;
    default:
      break;
  }

  command << "\t--nrm" << " \"" << n[0] << " " << n[1] << " " << n[2] << "\" \\" << "\n";
  command << "\t--vup" << " \"" << vup[0] << " " << vup[1] << " " << vup[2] << "\" \\" << "\n";

  command << "\t--sad " << beamNode->GetSAD() << " --sid " \
    << beamNode->GetSAD() + this->IsocenterImagerDistance << " \\" << "\n";
  command << "\t-r" << " \"" << this->ImageDimention[1] << " " \
    << this->ImageDimention[0] << "\" \\" << "\n";
  command << "\t-z" << " \"" << this->ImageDimention[1] * this->ImageSpacing[1] << " " \
    << this->ImageDimention[0] * this->ImageSpacing[0] << "\" \\" << "\n";
  command << "\t-c" << " \"" << imageCenterY << " " << imageCenterX << "\" \\" << "\n";

  // Isocenter LPS position (-isocenter[0], -isocenter[1], isocenter[2])
  command << "\t-o" << " \"" << -1. * isocenter[0] << " " \
    << -1. * isocenter[1] << " " << isocenter[2] << "\" \\" << "\n";
  if (this->ImageWindowFlag)
  {
    command << "\t-w" << " \"" << this->ImageWindow[1] << " " << this->ImageWindow[3] << " " \
      << this->ImageWindow[0] << " " << this->ImageWindow[2] << "\" \\" << "\n";
  }

  if (this->ExponentialMappingFlag)
  {
    command << "\t-e ";
  }
  else
  {
    command << "\t ";
  }

  if (this->AutoscaleFlag)
  {
    command << " --autoscale ";
  }

  switch (this->AlgorithmReconstuction)
  {
    case AlgorithmReconstuctionType::EXACT:
      command << "-i exact ";
      break;
    case AlgorithmReconstuctionType::UNIFORM:
      command << "-i uniform ";
      break;
    default:
      break;
  }

  switch (this->HUConversion)
  {
    case HounsfieldUnitsConversionType::NONE:
      command << "-P none ";
      break;
    case HounsfieldUnitsConversionType::PREPROCESS:
      command << "-P preprocess ";
      break;
    case HounsfieldUnitsConversionType::INLINE:
      command << "-P inline ";
      break;
    default:
      break;
  }

  command << "-O Out -t raw";

  plastimatchArguments.clear();
  plastimatchArguments.push_back("drr");
  plastimatchArguments.push_back("--nrm");
  std::ostringstream drrStream;
  drrStream << n[0] << " " << n[1] << " " << n[2];
  plastimatchArguments.push_back(drrStream.str());
  
  plastimatchArguments.push_back("--vup");
  std::ostringstream vupStream;
  vupStream << vup[0] << " " << vup[1] << " " << vup[2];
  plastimatchArguments.push_back(vupStream.str());

  plastimatchArguments.push_back("--sad");
  plastimatchArguments.push_back(std::to_string(beamNode->GetSAD()));
  plastimatchArguments.push_back("--sid");
  plastimatchArguments.push_back(std::to_string(beamNode->GetSAD() + this->IsocenterImagerDistance));

  plastimatchArguments.push_back("-r");
  std::ostringstream imagerResolutionStream;
  imagerResolutionStream << this->ImageDimention[1] << " " << this->ImageDimention[0];
  plastimatchArguments.push_back(imagerResolutionStream.str());

  plastimatchArguments.push_back("-z");
  std::ostringstream imagerSizeStream;
  imagerSizeStream << this->ImageDimention[1] * this->ImageSpacing[1] << " " \
    << this->ImageDimention[0] * this->ImageSpacing[0];
  plastimatchArguments.push_back(imagerSizeStream.str());

  plastimatchArguments.push_back("-c");
  std::ostringstream imagerCenterPositionStream;
  imagerCenterPositionStream << imageCenterY << " " << imageCenterX;
  plastimatchArguments.push_back(imagerCenterPositionStream.str());

  // Isocenter LPS position (-isocenter[0], -isocenter[1], isocenter[2])
  plastimatchArguments.push_back("-o");
  std::ostringstream isocenterStream;
  isocenterStream << -1. * isocenter[0] << " " << -1. * isocenter[1] << " " << isocenter[2];
  plastimatchArguments.push_back(isocenterStream.str());

  if (this->ImageWindowFlag)
  {
    plastimatchArguments.push_back("-w");
    std::ostringstream imageWindowStream;
    imageWindowStream << this->ImageWindow[1] << " " << this->ImageWindow[3] << " " \
      << this->ImageWindow[0] << " " << this->ImageWindow[2];
    plastimatchArguments.push_back(imageWindowStream.str());
  }

  if (this->ExponentialMappingFlag)
  {
    plastimatchArguments.push_back("-e");
  }

  if (this->AutoscaleFlag)
  {
    plastimatchArguments.push_back("--autoscale");
  }

  plastimatchArguments.push_back("-A");
  switch (this->Threading)
  {
    case ThreadingType::CPU:
      plastimatchArguments.push_back("cpu");
      break;
    case ThreadingType::CUDA:
      plastimatchArguments.push_back("cuda");
      break;
    case ThreadingType::OPENCL:
      plastimatchArguments.push_back("opencl");
      break;
    default:
      break;
  }

  plastimatchArguments.push_back("-i");
  switch (this->AlgorithmReconstuction)
  {
    case AlgorithmReconstuctionType::EXACT:
      plastimatchArguments.push_back("exact");
      break;
    case AlgorithmReconstuctionType::UNIFORM:
      plastimatchArguments.push_back("uniform");
      break;
    default:
      break;
  }

  plastimatchArguments.push_back("-P");
  switch (this->HUConversion)
  {
    case HounsfieldUnitsConversionType::NONE:
      plastimatchArguments.push_back("none");
      break;
    case HounsfieldUnitsConversionType::PREPROCESS:
      plastimatchArguments.push_back("preprocess");
      break;
    case HounsfieldUnitsConversionType::INLINE:
      plastimatchArguments.push_back("inline");
      break;
    default:
      break;
  }

  plastimatchArguments.push_back("-O");
  plastimatchArguments.push_back("Out");
  plastimatchArguments.push_back("-t");
  plastimatchArguments.push_back("raw");

  return command.str();
}
