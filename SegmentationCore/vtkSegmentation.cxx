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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// SegmentationCore includes
#include "vtkSegmentation.h"

#include "vtkSegmentationConverterRule.h"
#include "vtkSegmentationConverterFactory.h"

#include "vtkOrientedImageData.h"
#include "vtkCalculateOversamplingFactor.h"

// MRML includes
#include <vtkEventBroker.h> // Only using the event broker mechanism from MRML, otherwise no dependence

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkMath.h>
#include <vtkAbstractTransform.h>
#include <vtkCallbackCommand.h>
#include <vtkStringArray.h>

// STD includes
#include <sstream>
#include <algorithm>
#include <functional>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSegmentation);

//----------------------------------------------------------------------------
template<class T>
struct MapValueCompare : public std::binary_function<typename T::value_type, typename T::mapped_type, bool>
{
public:
  bool operator() (typename T::value_type &pair, typename T::mapped_type value) const
  {
    return pair.second == value;
  }
};

//----------------------------------------------------------------------------
vtkSegmentation::vtkSegmentation()
{
  this->MasterRepresentationName = NULL;
  this->Converter = vtkSegmentationConverter::New();

  this->SegmentCallbackCommand = vtkCallbackCommand::New();
  this->SegmentCallbackCommand->SetClientData( reinterpret_cast<void *>(this) );
  this->SegmentCallbackCommand->SetCallback( vtkSegmentation::OnSegmentModified );

  this->MasterRepresentationCallbackCommand = vtkCallbackCommand::New();
  this->MasterRepresentationCallbackCommand->SetClientData( reinterpret_cast<void *>(this) );
  this->MasterRepresentationCallbackCommand->SetCallback( vtkSegmentation::OnMasterRepresentationModified );
}

//----------------------------------------------------------------------------
vtkSegmentation::~vtkSegmentation()
{
  // Properly remove all segments
  std::vector<std::string> segmentIds;
  this->GetSegmentIDs(segmentIds);
  for (std::vector<std::string>::iterator segmentIt = segmentIds.begin(); segmentIt != segmentIds.end(); ++segmentIt)
  {
    this->RemoveSegment(*segmentIt);
  }
  this->Segments.clear();

  this->Converter->Delete();

  if (this->SegmentCallbackCommand)
  {
    this->SegmentCallbackCommand->SetClientData(NULL);
    this->SegmentCallbackCommand->Delete();
    this->SegmentCallbackCommand = NULL;
  }

  if (this->MasterRepresentationCallbackCommand)
  {
    this->MasterRepresentationCallbackCommand->SetClientData(NULL);
    this->MasterRepresentationCallbackCommand->Delete();
    this->MasterRepresentationCallbackCommand = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkSegmentation::WriteXML(ostream& of, int nIndent)
{
  vtkIndent indent(nIndent);

  of << indent << "MasterRepresentationName:  " << (this->MasterRepresentationName ? this->MasterRepresentationName : "NULL") << "\n";

  //TODO: Write segment info? Reading that out will probably need to be managed by the storage node instead.
}

//----------------------------------------------------------------------------
void vtkSegmentation::ReadXMLAttributes(const char** atts)
{
  const char* attName;
  const char* attValue;
  while (*atts != NULL)
  {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "MasterRepresentationName")) 
    {
      std::stringstream ss;
      ss << attValue;
      this->SetMasterRepresentationName(ss.str().c_str());
    }
  }
}

//----------------------------------------------------------------------------
void vtkSegmentation::DeepCopy(vtkSegmentation* aSegmentation)
{
  if (!aSegmentation)
  {
    return;
  }

  // Copy properties
  this->SetMasterRepresentationName(aSegmentation->GetMasterRepresentationName());

  // Copy conversion parameters
  this->Converter->DeepCopy(aSegmentation->Converter);

  // Deep copy segments list
  for (SegmentMap::iterator it = aSegmentation->Segments.begin(); it != aSegmentation->Segments.end(); ++it)
  {
    vtkSmartPointer<vtkSegment> segment = vtkSmartPointer<vtkSegment>::New();
    segment->DeepCopy(it->second);
    this->AddSegment(segment);
  }
}

//----------------------------------------------------------------------------
void vtkSegmentation::CopyConversionParameters(vtkSegmentation* aSegmentation)
{
  this->Converter->DeepCopy(aSegmentation->Converter);
}

//----------------------------------------------------------------------------
void vtkSegmentation::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  for (SegmentMap::iterator it = this->Segments.begin(); it != this->Segments.end(); ++it)
  {
    os << indent << "Segment:   " << it->first << "\n";
    vtkSegment* segment = it->second;
    segment->PrintSelf(os, indent.GetNextIndent());
  }
}

//---------------------------------------------------------------------------
// (Xmin, Xmax, Ymin, Ymax, Zmin, Zmax)
//---------------------------------------------------------------------------
void vtkSegmentation::GetBounds(double bounds[6])
{
  vtkMath::UninitializeBounds(bounds);

  for (SegmentMap::iterator it = this->Segments.begin(); it != this->Segments.end(); ++it)
  {
    double segmentBounds[6];
    vtkMath::UninitializeBounds(segmentBounds);

    vtkSegment* segment = it->second;
    segment->GetBounds(segmentBounds);

    vtkSegment::ExtendBounds(segmentBounds, bounds);
  }
}

//---------------------------------------------------------------------------
bool vtkSegmentation::GetModifiedSinceRead()
{
  for (SegmentMap::iterator it = this->Segments.begin(); it != this->Segments.end(); ++it)
  {
    vtkSegment* segment = it->second;
    if (segment->GetModifiedSinceRead(this->MTime))
    {
      return true;
    }
  }

  return false;
}

//---------------------------------------------------------------------------
void vtkSegmentation::SetMasterRepresentationName(const char* representationName)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting MasterRepresentationName to " << (representationName?representationName:"(null)") );
  if ( this->MasterRepresentationName == NULL && representationName == NULL) { return;}
  if ( this->MasterRepresentationName && representationName && (!strcmp(this->MasterRepresentationName,representationName))) { return;}
  delete [] this->MasterRepresentationName;
  if (representationName)
  {
    size_t n = strlen(representationName) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (representationName);
    this->MasterRepresentationName = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
  }
  else
  {
    this->MasterRepresentationName = NULL;
  }
  this->Modified();
  this->InvokeEvent(vtkSegmentation::MasterRepresentationModified, this);
}

//---------------------------------------------------------------------------
std::string vtkSegmentation::GenerateUniqueSegmentId(std::string id)
{
  // If input ID string is empty then set it to default "SegmentN", where N is the number of segments
  if (id.empty())
  {
    std::stringstream idStream;
    idStream << "Segment_" << this->GetNumberOfSegments();
    id = this->GenerateUniqueSegmentId(idStream.str());
  }

  // If ID already exists then postfix it with "_1"
  SegmentMap::iterator segmentIt = this->Segments.find(id);
  if (segmentIt != this->Segments.end())
  {
    id.append("_1");

    // Make sure the postfixed ID is unique, too
    id = this->GenerateUniqueSegmentId(id);
  }

  return id;
}

//---------------------------------------------------------------------------
bool vtkSegmentation::AddSegment(vtkSegment* segment, std::string segmentId/*=""*/)
{
  if (!segment)
  {
    vtkErrorMacro("AddSegment: Invalid segment!");
    return false;
  }
  if (!this->MasterRepresentationName)
  {
    vtkErrorMacro("AddSegment: Invalid master representation name!");
    return false;
  }

  // Observe segment underlying data for changes
  vtkEventBroker::GetInstance()->AddObservation(
    segment, vtkCommand::ModifiedEvent, this, this->SegmentCallbackCommand );

  // Get representation names contained by the added segment
  std::vector<std::string> containedRepresentationNamesInAddedSegment;
  segment->GetContainedRepresentationNames(containedRepresentationNamesInAddedSegment);

  // Perform necessary conversions if needed on the added segment:
  // 1. If the segment can be added, and it does not contain the master representation,
  // then the master representation is converted using the cheapest available path.
  if (!segment->GetRepresentation(this->MasterRepresentationName))
  {
    // Collect all available paths to master representation
    vtkSegmentationConverter::ConversionPathAndCostListType allPathsToMaster;
    for (std::vector<std::string>::iterator reprIt = containedRepresentationNamesInAddedSegment.begin();
      reprIt != containedRepresentationNamesInAddedSegment.end(); ++reprIt)
    {
      vtkSegmentationConverter::ConversionPathAndCostListType pathsFromCurrentRepresentationToMaster;
      this->Converter->GetPossibleConversions((*reprIt), this->MasterRepresentationName, pathsFromCurrentRepresentationToMaster);
      // Append paths from current representation to master to all found paths to master
      allPathsToMaster.insert( allPathsToMaster.end(),
        pathsFromCurrentRepresentationToMaster.begin(), pathsFromCurrentRepresentationToMaster.end() );
    }
    // Get cheapest path from any representation to master and try to convert
    vtkSegmentationConverter::ConversionPathType cheapestPath =
      vtkSegmentationConverter::GetCheapestPath(allPathsToMaster);
    if (cheapestPath.empty() || !this->ConvertSegmentUsingPath(segment, cheapestPath))
    {
      // Return if cannot convert to master representation
      vtkErrorMacro("AddSegment: Unable to create master representation!");
      return false;
    }
  }

  /// 2. Make sure that the segment contains the same types of representations that are
  /// present in the existing segments of the segmentation (because we expect all segments
  /// in a segmentation to contain the same types of representations).
  if (this->GetNumberOfSegments() > 0)
  {
    vtkSegment* firstSegment = this->Segments.begin()->second;
    std::vector<std::string> containedRepresentationNamesInFirstSegment;
    firstSegment->GetContainedRepresentationNames(containedRepresentationNamesInFirstSegment);

    // Convert to representations that exist in this segmentation
    for (std::vector<std::string>::iterator reprIt = containedRepresentationNamesInFirstSegment.begin();
      reprIt != containedRepresentationNamesInFirstSegment.end(); ++reprIt)
    {
      // If representation exists then there is nothing to do
      if (segment->GetRepresentation(*reprIt))
      {
        continue;
      }

      // Convert using the cheapest available path
      vtkSegmentationConverter::ConversionPathAndCostListType pathsToCurrentRepresentation;
      this->Converter->GetPossibleConversions(this->MasterRepresentationName, (*reprIt), pathsToCurrentRepresentation);
      vtkSegmentationConverter::ConversionPathType cheapestPath =
        vtkSegmentationConverter::GetCheapestPath(pathsToCurrentRepresentation);
      if (cheapestPath.empty())
      {
        vtkErrorMacro("AddSegment: Unable to perform conversion!"); // Sanity check, it should never happen
        return false;
      }
      // Perform conversion
      this->ConvertSegmentUsingPath(segment, cheapestPath);
    }

    // Remove representations that do not exist in this segmentation
    for (std::vector<std::string>::iterator reprIt = containedRepresentationNamesInAddedSegment.begin();
      reprIt != containedRepresentationNamesInAddedSegment.end(); ++reprIt)
    {
      if (!firstSegment->GetRepresentation(*reprIt))
      {
        segment->RemoveRepresentation(*reprIt);
      }
    }
  }

  // Add to list. If segmentId is empty, then segment name becomes the ID
  std::string key = segmentId;
  if (key.empty())
  {
    key = segment->GetName();
    key = this->GenerateUniqueSegmentId(key);
  }
  this->Segments[key] = segment;

  // Fire segment added event
  const char* segmentIdChars = key.c_str();
  this->InvokeEvent(vtkSegmentation::SegmentAdded, (void*)segmentIdChars);

  this->Modified();

  return true;
}

//---------------------------------------------------------------------------
void vtkSegmentation::RemoveSegment(std::string segmentId)
{
  SegmentMap::iterator segmentIt = this->Segments.find(segmentId);
  if (segmentIt == this->Segments.end())
  {
    vtkWarningMacro("RemoveSegment: Segment to remove cannot be found!");
    return;
  }

  // Remove segment
  this->RemoveSegment(segmentIt);
}

//---------------------------------------------------------------------------
void vtkSegmentation::RemoveSegment(vtkSegment* segment)
{
  if (!segment)
  {
    vtkErrorMacro("RemoveSegment: Invalid segment!");
    return;
  }

  SegmentMap::iterator segmentIt = std::find_if(
    this->Segments.begin(), this->Segments.end(), std::bind2nd(MapValueCompare<SegmentMap>(), segment) );
  if (segmentIt == this->Segments.end())
  {
    vtkWarningMacro("RemoveSegment: Segment to remove cannot be found!");
    return;
  }

  // Remove segment
  this->RemoveSegment(segmentIt);
}

//---------------------------------------------------------------------------
void vtkSegmentation::RemoveSegment(SegmentMap::iterator segmentIt)
{
  if (segmentIt == this->Segments.end())
  {
    return;
  }

  std::string segmentId(segmentIt->first);

  // Remove observations
  vtkEventBroker::GetInstance()->RemoveObservations(
    segmentIt->second.GetPointer(), vtkCommand::ModifiedEvent, this, this->SegmentCallbackCommand );

  // Remove segment
  this->Segments.erase(segmentIt);

  // If the segmentation became empty then clear master representation
  //TODO: Any bad consequences? Otherwise the representation table will show a master representation with no data underneath
  if (this->Segments.empty())
  {
    this->SetMasterRepresentationName(NULL);
  }
  
  // Fire segment removed event
  this->InvokeEvent(vtkSegmentation::SegmentRemoved, (void*)segmentId.c_str());

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSegmentation::OnSegmentModified(vtkObject* caller,
                                        unsigned long vtkNotUsed(eid),
                                        void* clientData,
                                        void* vtkNotUsed(callData))
{
  vtkSegmentation* self = reinterpret_cast<vtkSegmentation*>(clientData);
  vtkSegment* callerSegment = reinterpret_cast<vtkSegment*>(caller);
  if (!self || !callerSegment)
  {
    return;
  }
  if (!self->MasterRepresentationName)
  {
    vtkErrorWithObjectMacro(self, "vtkSegmentation::OnSegmentModified: Master representation not specified!");
    return;
  }

  // Get master representation of caller segment to renew its observation
  vtkDataObject* masterRepresentation = callerSegment->GetRepresentation(self->MasterRepresentationName);
  if (masterRepresentation)
  {
    // Remove observation from segment's master representation (in case it has changed)
    vtkEventBroker::GetInstance()->RemoveObservations(
      masterRepresentation, vtkCommand::ModifiedEvent, self, self->MasterRepresentationCallbackCommand );
    masterRepresentation->UnRegister(self);

    // Observe segment's master representation
    vtkEventBroker::GetInstance()->AddObservation(
      masterRepresentation, vtkCommand::ModifiedEvent, self, self->MasterRepresentationCallbackCommand );
    masterRepresentation->Register(self);
  }

  // Invoke segment modified event, but do not invoke general modified event
  std::string segmentId = self->GetSegmentIdBySegment(callerSegment);
  if (segmentId.empty())
  {
    // Segment is modified before actually having been added to the segmentation (within AddSegment)
    return;
  }
  const char* segmentIdChars = segmentId.c_str();
  self->InvokeEvent(vtkSegmentation::SegmentModified, (void*)(segmentIdChars));
}

//---------------------------------------------------------------------------
void vtkSegmentation::OnMasterRepresentationModified(vtkObject* vtkNotUsed(caller),
                                                     unsigned long vtkNotUsed(eid),
                                                     void* clientData,
                                                     void* vtkNotUsed(callData))
{
  vtkSegmentation* self = reinterpret_cast<vtkSegmentation*>(clientData);
  if (!self)
  {
    return;
  }

  self->InvokeEvent(vtkSegmentation::MasterRepresentationModified, self);

  self->Modified();
}

//---------------------------------------------------------------------------
vtkSegment* vtkSegmentation::GetSegment(std::string segmentId)
{
  SegmentMap::iterator segmentIt = this->Segments.find(segmentId);
  if (segmentIt == this->Segments.end())
  {
    return NULL;
  }

  return segmentIt->second;
}

//---------------------------------------------------------------------------
int vtkSegmentation::GetNumberOfSegments() const
{
  return this->Segments.size();
}

//---------------------------------------------------------------------------
std::string vtkSegmentation::GetSegmentIdBySegment(vtkSegment* segment)
{
  if (!segment)
  {
    vtkErrorMacro("GetSegmentIdBySegment: Invalid segment!");
    return "";
  }

  SegmentMap::iterator segmentIt = std::find_if(
    this->Segments.begin(), this->Segments.end(), std::bind2nd(MapValueCompare<SegmentMap>(), segment) );
  if (segmentIt == this->Segments.end())
  {
    vtkDebugMacro("GetSegmentIdBySegment: Segment cannot be found!");
    return "";
  }

  return segmentIt->first;
}

//---------------------------------------------------------------------------
std::vector<vtkSegment*> vtkSegmentation::GetSegmentsByTag(std::string tag)
{
  //TODO:
  vtkErrorMacro("Not implemented!");
  return std::vector<vtkSegment*>();
}


//---------------------------------------------------------------------------
void vtkSegmentation::GetSegmentIDs(std::vector<std::string> &segmentIds)
{
  segmentIds.clear();
  for (SegmentMap::iterator segmentIt = this->Segments.begin(); segmentIt != this->Segments.end(); ++segmentIt)
  {
    segmentIds.push_back(segmentIt->first);
  }
}

//---------------------------------------------------------------------------
void vtkSegmentation::GetSegmentIDs(vtkStringArray* segmentIds)
{
  if (!segmentIds)
  {
    return;
  }
  segmentIds->Initialize();
  for (SegmentMap::iterator segmentIt = this->Segments.begin(); segmentIt != this->Segments.end(); ++segmentIt)
  {
    segmentIds->InsertNextValue(segmentIt->first.c_str());
  }
}

//---------------------------------------------------------------------------
void vtkSegmentation::ApplyLinearTransform(vtkAbstractTransform* transform)
{
  // Apply linear transform for each segment:
  // Harden transform on poly data, apply to directions on oriented image data
  for (SegmentMap::iterator it = this->Segments.begin(); it != this->Segments.end(); ++it)
  {
    it->second->ApplyLinearTransform(transform);
  }
}

//---------------------------------------------------------------------------
void vtkSegmentation::ApplyNonLinearTransform(vtkAbstractTransform* transform)
{
  // Harden transform on both image data and poly data for each segment individually
  for (SegmentMap::iterator it = this->Segments.begin(); it != this->Segments.end(); ++it)
  {
    it->second->ApplyNonLinearTransform(transform);
  }
}

//-----------------------------------------------------------------------------
bool vtkSegmentation::ConvertSegmentUsingPath(vtkSegment* segment, vtkSegmentationConverter::ConversionPathType path, bool overwriteExisting/*=false*/)
{
  // Execute each conversion step in the selected path
  vtkSegmentationConverter::ConversionPathType::iterator pathIt;
  for (pathIt = path.begin(); pathIt != path.end(); ++pathIt)
  {
    vtkSegmentationConverterRule* currentConversionRule = (*pathIt);
    if (!currentConversionRule)
    {
      vtkErrorMacro("ConvertSegmentUsingPath: Invalid converter rule!");
      return false;
    }

    // Get source representation from segment. It is expected to exist
    vtkDataObject* sourceRepresentation = segment->GetRepresentation(
      currentConversionRule->GetSourceRepresentationName() );
    if (!currentConversionRule)
    {
      vtkErrorMacro("ConvertSegmentUsingPath: Source representation does not exist!");
      return false;
    }

    // Get target representation
    vtkSmartPointer<vtkDataObject> targetRepresentation = segment->GetRepresentation(
      currentConversionRule->GetTargetRepresentationName() );
    // If target representation exists and we do not overwrite existing representations,
    // then no conversion is necessary with this conversion rule
    if (targetRepresentation.GetPointer() && !overwriteExisting)
    {
      continue;
    }
    // Create an empty target representation if it does not exist
    if (!targetRepresentation.GetPointer())
    {
      targetRepresentation = vtkSmartPointer<vtkDataObject>::Take(
        currentConversionRule->ConstructRepresentationObjectByRepresentation(currentConversionRule->GetTargetRepresentationName()) );
    }

    // Perform conversion step
    currentConversionRule->Convert(sourceRepresentation, targetRepresentation);

    // Add representation to segment
    segment->AddRepresentation(currentConversionRule->GetTargetRepresentationName(), targetRepresentation);
  }

  return true;
}

//---------------------------------------------------------------------------
bool vtkSegmentation::CreateRepresentation(const std::string& targetRepresentationName, bool alwaysConvert/*=false*/)
{
  if (!this->Converter)
  {
    vtkErrorMacro("CreateRepresentation: Invalid converter!");
    return false;
  }
  if (!this->MasterRepresentationName)
  {
    vtkErrorMacro("CreateRepresentation: Master representation not specified!");
    return false;
  }

  // Simply return success if the target representation exists
  if (!alwaysConvert)
  {
    bool representationExists = true;
    for (SegmentMap::iterator segmentIt = this->Segments.begin(); segmentIt != this->Segments.end(); ++segmentIt)
    {
      if (!segmentIt->second->GetRepresentation(targetRepresentationName))
      {
        // All segments should have the same representation configuration,
        // so checking each segment is mostly a safety measure
        representationExists = false;
        break;
      }
    }
    if (representationExists)
    {
      return true;
    }
  }

  // Get conversion path with lowest cost.
  // If always convert, then only consider conversions from master, otherwise consider all available representations
  vtkSegmentationConverter::ConversionPathAndCostListType pathCosts;
  if (alwaysConvert)
  {
    this->Converter->GetPossibleConversions(this->MasterRepresentationName, targetRepresentationName, pathCosts);
  }
  else
  {
    vtkSegmentationConverter::ConversionPathAndCostListType currentPathCosts;
    std::vector<std::string> representationNames;
    this->GetContainedRepresentationNames(representationNames);
    for (std::vector<std::string>::iterator reprIt=representationNames.begin(); reprIt!=representationNames.end(); ++reprIt)
    {
      this->Converter->GetPossibleConversions((*reprIt), targetRepresentationName, currentPathCosts);
      for (vtkSegmentationConverter::ConversionPathAndCostListType::const_iterator pathIt = currentPathCosts.begin(); pathIt != currentPathCosts.end(); ++pathIt)
      {
        pathCosts.push_back(*pathIt);
      }
    }
  }
  // Get cheapest path from found conversion paths
  vtkSegmentationConverter::ConversionPathType cheapestPath = vtkSegmentationConverter::GetCheapestPath(pathCosts);
  if (cheapestPath.empty())
  {
    return false;
  }

  // Perform conversion on all segments (no overwrites)
  for (SegmentMap::iterator segmentIt = this->Segments.begin(); segmentIt != this->Segments.end(); ++segmentIt)
  {
    if (!this->ConvertSegmentUsingPath(segmentIt->second, cheapestPath, alwaysConvert))
    {
      vtkErrorMacro("CreateRepresentation: Conversion failed!");
      return false;
    }
  }

  const char* targetRepresentationNameChars = targetRepresentationName.c_str();
  this->InvokeEvent(vtkSegmentation::RepresentationCreated, (void*)targetRepresentationNameChars);
  return true;
}

//---------------------------------------------------------------------------
bool vtkSegmentation::CreateRepresentation(const std::string& targetRepresentationName,
                                           vtkSegmentationConverter::ConversionPathType path,
                                           vtkSegmentationConverterRule::ConversionParameterListType parameters)
{
  if (!this->Converter)
  {
    vtkErrorMacro("CreateRepresentation: Invalid converter!");
    return false;
  }
  if (!this->MasterRepresentationName)
  {
    vtkErrorMacro("CreateRepresentation: Master representation not specified!");
    return false;
  }
  if (path.empty())
  {
    return false;
  }

  // Set conversion parameters
  this->Converter->SetConversionParameters(parameters);

  // Perform conversion on all segments (do overwrites)
  for (SegmentMap::iterator segmentIt = this->Segments.begin(); segmentIt != this->Segments.end(); ++segmentIt)
  {
    if (!this->ConvertSegmentUsingPath(segmentIt->second, path, true))
    {
      vtkErrorMacro("CreateRepresentation: Conversion failed!");
      return false;
    }
  }

  const char* targetRepresentationNameChars = targetRepresentationName.c_str();
  this->InvokeEvent(vtkSegmentation::RepresentationCreated, (void*)targetRepresentationNameChars);
  return true;
}

//---------------------------------------------------------------------------
vtkDataObject* vtkSegmentation::GetSegmentRepresentation(std::string segmentId, std::string representationName)
{
  vtkSegment* segment = this->GetSegment(segmentId);
  if (!segment)
  {
    return NULL;
  }

  return segment->GetRepresentation(representationName);
}

//---------------------------------------------------------------------------
void vtkSegmentation::InvalidateNonMasterRepresentations()
{
  if (!this->MasterRepresentationName)
  {
    vtkErrorMacro("InvalidateNonMasterRepresentations: Master representation not specified!");
    return;
  }

  // Iterate through all segments and remove all representations that are not the master representation
  for (SegmentMap::iterator segmentIt = this->Segments.begin(); segmentIt != this->Segments.end(); ++segmentIt)
  {
    segmentIt->second->RemoveAllRepresentations(this->MasterRepresentationName);
  }
}

//---------------------------------------------------------------------------
void vtkSegmentation::GetContainedRepresentationNames(std::vector<std::string>& representationNames)
{
  if (this->Segments.empty())
  {
    return;
  }

  vtkSegment* firstSegment = this->Segments.begin()->second;
  firstSegment->GetContainedRepresentationNames(representationNames);
}

//---------------------------------------------------------------------------
bool vtkSegmentation::ContainsRepresentation(std::string representationName)
{
  if (this->Segments.empty())
  {
    return false;
  }

  std::vector<std::string> containedRepresentationNames;
  this->GetContainedRepresentationNames(containedRepresentationNames);
  std::vector<std::string>::iterator reprIt = std::find(
    containedRepresentationNames.begin(), containedRepresentationNames.end(), representationName);

  return (reprIt != containedRepresentationNames.end());
}

//-----------------------------------------------------------------------------
bool vtkSegmentation::CanAcceptRepresentation(std::string representationName)
{
  if (representationName.empty() || !this->MasterRepresentationName)
  {
    return false;
  }

  // If representation is the master representation then it can be accepted
  if (!representationName.compare(this->MasterRepresentationName))
  {
    return true;
  }

  // Otherwise if the representation can be converted to the master representation, then
  // it can be accepted, if cannot be converted then not.
  vtkSegmentationConverter::ConversionPathAndCostListType pathCosts;
  this->Converter->GetPossibleConversions(representationName, this->MasterRepresentationName, pathCosts);
  return !pathCosts.empty();
}

//-----------------------------------------------------------------------------
bool vtkSegmentation::CanAcceptSegment(vtkSegment* segment)
{
  if (!segment)
  {
    return false;
  }

  // Can accept any segment if there segmentation is empty
  if (this->Segments.size() == 0)
  {
    return true;
  }

  // Check if segmentation can accept any of the segment's representations
  std::vector<std::string> containedRepresentationNames;
  segment->GetContainedRepresentationNames(containedRepresentationNames);
  for (std::vector<std::string>::iterator reprIt = containedRepresentationNames.begin();
    reprIt != containedRepresentationNames.end(); ++reprIt)
  {
    if (this->CanAcceptRepresentation(*reprIt))
    {
      return true;
    }
  }

  // If no representation in the segment is acceptable by this segmentation then the
  // segment is unacceptable.
  return false;
}

//-----------------------------------------------------------------------------
bool vtkSegmentation::AddEmptySegment(std::string segmentId/*=""*/)
{
  vtkSmartPointer<vtkSegment> segment = vtkSmartPointer<vtkSegment>::New();
  segment->SetDefaultColor(vtkSegment::SEGMENT_COLOR_VALUE_INVALID[0], vtkSegment::SEGMENT_COLOR_VALUE_INVALID[1],
                           vtkSegment::SEGMENT_COLOR_VALUE_INVALID[2] );

  // Segment ID will be segment name by default
  segmentId = this->GenerateUniqueSegmentId(segmentId);
  segment->SetName(segmentId.c_str());
  
  // If there are no segments in segmentation then just create a master representation.
  if (this->GetNumberOfSegments() == 0)
  {
    // If there is no master representation then set it to binary labelmap
    if (this->MasterRepresentationName == NULL)
    {
      this->SetMasterRepresentationName(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
    }

    vtkSmartPointer<vtkDataObject> emptyMasterRepresentation = vtkSmartPointer<vtkDataObject>::Take(
      vtkSegmentationConverterFactory::GetInstance()->ConstructRepresentationObjectByRepresentation(this->MasterRepresentationName) );
    if (!emptyMasterRepresentation)
    {
      vtkErrorMacro("AddEmptySegment: Unable to construct empty master representation type '" << this->MasterRepresentationName << "'");
      return false;
    }
    segment->AddRepresentation(this->MasterRepresentationName, emptyMasterRepresentation);
  }
  // Create empty representations for all types that are present in this segmentation
  // (the representation configuration in all segments needs to match in a segmentation)
  else
  {
    vtkSegment* firstSegment = this->Segments.begin()->second;
    std::vector<std::string> containedRepresentationNamesInFirstSegment;
    firstSegment->GetContainedRepresentationNames(containedRepresentationNamesInFirstSegment);

    for (std::vector<std::string>::iterator reprIt = containedRepresentationNamesInFirstSegment.begin();
      reprIt != containedRepresentationNamesInFirstSegment.end(); ++reprIt)
    {
      vtkSmartPointer<vtkDataObject> emptyRepresentation = vtkSmartPointer<vtkDataObject>::Take(
        vtkSegmentationConverterFactory::GetInstance()->ConstructRepresentationObjectByRepresentation(*reprIt) );
      if (!emptyRepresentation)
      {
        vtkErrorMacro("AddEmptySegment: Unable to construct empty representation type '" << (*reprIt) << "'");
        return false;
      }
      segment->AddRepresentation((*reprIt), emptyRepresentation);
    }
  }

  return this->AddSegment(segment);
}

//-----------------------------------------------------------------------------
bool vtkSegmentation::CopySegmentFromSegmentation(vtkSegmentation* fromSegmentation, std::string segmentId, bool removeFromSource/*=false*/)
{
  if (!fromSegmentation || segmentId.empty())
  {
    return false;
  }

  // If segment with the same ID is present in the target (this instance), then do not copy
  if (this->GetSegment(segmentId))
  {
    vtkWarningMacro("CopySegmentFromSegmentation: Segment with the same ID as the copied one (" << segmentId << ") already exists in the target segmentation");
    return false;
  }

  // Get segment from source
  vtkSegment* segment = fromSegmentation->GetSegment(segmentId);
  if (!segment)
  {
    vtkErrorMacro("CopySegmentFromSegmentation: Failed to get segment!");
    return false;
  }

  // If source segmentation contains reference image geometry conversion parameter,
  // but target segmentation does not, then, then copy that parameter from the source segmentation
  // TODO: Do this with all parameters? (so those which have non-default values are replaced)
  std::string referenceImageGeometryParameter = this->GetConversionParameter(vtkSegmentationConverter::GetReferenceImageGeometryParameterName());
  std::string fromReferenceImageGeometryParameter = fromSegmentation->GetConversionParameter(vtkSegmentationConverter::GetReferenceImageGeometryParameterName());
  if (referenceImageGeometryParameter.empty() && !fromReferenceImageGeometryParameter.empty())
  {
    this->SetConversionParameter(vtkSegmentationConverter::GetReferenceImageGeometryParameterName(), fromReferenceImageGeometryParameter);
  }

  // If copy, then duplicate segment and add it to the target segmentation
  if (!removeFromSource)
  {
    vtkSmartPointer<vtkSegment> segmentCopy = vtkSmartPointer<vtkSegment>::New();
    segmentCopy->DeepCopy(segment);
    if (!this->AddSegment(segmentCopy, segmentId))
    {
      vtkErrorMacro("CopySegmentFromSegmentation: Failed to add segment '" << segmentId << "' to segmentation!");
      return false;
    }
  }
  // If move, then just add segment to target and remove from source (ownership is transferred)
  else
  {
    if (!this->AddSegment(segment, segmentId))
    {
      vtkErrorMacro("CopySegmentFromSegmentation: Failed to add segment '" << segmentId << "' to segmentation!");
      return false;
    }
    fromSegmentation->RemoveSegment(segmentId);
  }

  return true;
}

//-----------------------------------------------------------------------------
std::string vtkSegmentation::DetermineCommonLabelmapGeometry(const std::vector<std::string>& segmentIDs/*=std::vector<std::string>()*/)
{
  // If segment IDs list is empty then include all segments
  std::vector<std::string> mergedSegmentIDs;
  if (segmentIDs.empty())
  {
    vtkSegmentation::SegmentMap segmentMap = this->GetSegments();
    for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
    {
      mergedSegmentIDs.push_back(segmentIt->first);
    }
  }
  else
  {
    mergedSegmentIDs = segmentIDs;
  }

  // Get highest resolution reference geometry available in segments
  vtkOrientedImageData* highestResolutionLabelmap = NULL;
  double lowestSpacing[3] = {pow(VTK_DOUBLE_MAX,0.3), pow(VTK_DOUBLE_MAX,0.3), pow(VTK_DOUBLE_MAX,0.3)}; // We'll multiply the spacings together to get the voxel size
  for (std::vector<std::string>::iterator segmentIt = mergedSegmentIDs.begin(); segmentIt != mergedSegmentIDs.end(); ++segmentIt)
  {
    vtkSegment* currentSegment = this->GetSegment(*segmentIt);
    if (!currentSegment)
    {
      vtkWarningMacro("GenerateMergedLabelmap: Segment ID " << (*segmentIt) << " not found in segmentation");
      continue;
    }
    vtkOrientedImageData* currentBinaryLabelmap = vtkOrientedImageData::SafeDownCast(
      currentSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) );
    double currentSpacing[3] = {0.0,0.0,0.0};
    currentBinaryLabelmap->GetSpacing(currentSpacing);
    if (currentSpacing[0]*currentSpacing[1]*currentSpacing[2] < lowestSpacing[0]*lowestSpacing[1]*lowestSpacing[2])
    {
      lowestSpacing[0] = currentSpacing[0];
      lowestSpacing[1] = currentSpacing[1];
      lowestSpacing[2] = currentSpacing[2];
      highestResolutionLabelmap = currentBinaryLabelmap;
    }
  }
  if (!highestResolutionLabelmap)
  {
    vtkErrorMacro("GenerateMergedLabelmap: Unable to find highest resolution labelmap!");
    return false;
  }
  
  // Get reference image geometry conversion parameter
  std::string referenceGeometryString = this->GetConversionParameter(vtkSegmentationConverter::GetReferenceImageGeometryParameterName());
  if (referenceGeometryString.empty())
  {
    // Reference image geometry might be missing because segmentation was created from labelmaps.
    // Set reference image geometry from largest segment labelmap
    double largestExtentMm3 = 0;
    for (std::vector<std::string>::iterator segmentIt = mergedSegmentIDs.begin(); segmentIt != mergedSegmentIDs.end(); ++segmentIt)
    {
      vtkSegment* currentSegment = this->GetSegment(*segmentIt);
      if (!currentSegment)
      {
        vtkWarningMacro("GenerateMergedLabelmap: Segment ID " << (*segmentIt) << " not found in segmentation");
        continue;
      }
      vtkOrientedImageData* currentBinaryLabelmap = vtkOrientedImageData::SafeDownCast(
        currentSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) );
      // Calculate extent in mm
      int extent[6] = {0,-1,0,-1,0,-1};
      currentBinaryLabelmap->GetExtent(extent);
      double spacing[3] = {0.0,0.0,0.0};
      currentBinaryLabelmap->GetSpacing(spacing);
      double extentMm3 = ((extent[1]-extent[0]+1) * spacing[0]) * ((extent[3]-extent[2]+1) * spacing[1]) * ((extent[5]-extent[4]+1) * spacing[2]);
      if (extentMm3 > largestExtentMm3)
      {
        largestExtentMm3 = extentMm3;
      }
    }
    if (!highestResolutionLabelmap)
    {
      vtkErrorMacro("GenerateMergedLabelmap: Unable to find largest extent labelmap to define reference image geometry!");
      return false;
    }
    referenceGeometryString = vtkSegmentationConverter::SerializeImageGeometry(highestResolutionLabelmap);
  }

  // Oversample reference image geometry to match highest resolution labelmap's spacing
  vtkSmartPointer<vtkOrientedImageData> commonGeometryImage = vtkSmartPointer<vtkOrientedImageData>::New();
  vtkSegmentationConverter::DeserializeImageGeometry(referenceGeometryString, commonGeometryImage);
  double referenceSpacing[3] = {0.0,0.0,0.0};
  commonGeometryImage->GetSpacing(referenceSpacing);
  double voxelSizeRatio = ((referenceSpacing[0]*referenceSpacing[1]*referenceSpacing[2]) / (lowestSpacing[0]*lowestSpacing[1]*lowestSpacing[2]));
  // Round oversampling to the nearest integer
  // Note: We need to round to some degree, because e.g. pow(64,1/3) is not exactly 4. It may be debated whether to round to integer or to a certain number of decimals
  double oversamplingFactor = vtkMath::Round( pow( voxelSizeRatio, 1.0/3.0 ) );
  vtkCalculateOversamplingFactor::ApplyOversamplingOnImageGeometry(commonGeometryImage, oversamplingFactor);

  // Serialize common geometry and return it
  return vtkSegmentationConverter::SerializeImageGeometry(commonGeometryImage);
}
