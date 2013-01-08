#include "MatlabCommanderCLP.h" 

#include <iostream>
#include <fstream>
#include <math.h>
#include <cstdlib>

#include "igtlOSUtil.h"
#include "igtlStringMessage.h"
#include "igtlClientSocket.h"

std::string ReceiveString(igtl::Socket * socket, igtl::MessageHeader::Pointer& header)
{
  // Create a message buffer to receive transform data
  igtl::StringMessage::Pointer stringMsg;
  stringMsg = igtl::StringMessage::New();
  stringMsg->SetMessageHeader(header);
  stringMsg->AllocatePack();

  // Receive transform data from the socket
  socket->Receive(stringMsg->GetPackBodyPointer(), stringMsg->GetPackBodySize());

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = stringMsg->Unpack();

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
  {
    return stringMsg->GetString();
  }

  // error
  return "";
}

void SetReturnValues(const std::string &returnParameterFile,const char* reply, bool completed)
{
  // Write out the return parameters in "name = value" form
  std::ofstream rts;
  rts.open(returnParameterFile.c_str() );
  rts << "reply = " << reply << std::endl;
  rts << "completed = " << (completed?"true":"false") << std::endl;
  rts.close(); 
}

int main (int argc, char * argv [])
{
  PARSE_ARGS;
  
  std::cerr << "Executing Matlab command..." << std::endl;

  //------------------------------------------------------------
  // Establish Connection
  igtl::ClientSocket::Pointer socket;
  socket = igtl::ClientSocket::New();
  int connectErrorCode = socket->ConnectToServer(hostname.c_str(), port);
  if (connectErrorCode != 0)
  {    
    std::cerr << "Cannot connect to the server." << std::endl;
    SetReturnValues(returnParameterFile,"ERROR: Cannot connect to the server",0);
    exit(EXIT_SUCCESS);
  }

  //------------------------------------------------------------
  // Send command
  igtl::StringMessage::Pointer stringMsg;
  stringMsg = igtl::StringMessage::New();
  stringMsg->SetDeviceName("MatlabCommandClient");
  std::cout << "Sending string: " << cmd << std::endl;
  stringMsg->SetString(cmd);
  stringMsg->Pack();
  socket->Send(stringMsg->GetPackPointer(), stringMsg->GetPackSize());

  //------------------------------------------------------------
  // Receive reply
  // Create a message buffer to receive header
  igtl::MessageHeader::Pointer headerMsg;
  headerMsg = igtl::MessageHeader::New();
  // Initialize receive buffer
  headerMsg->InitPack();
  // Receive generic header from the socket
  int receivedBytes = socket->Receive(headerMsg->GetPackPointer(), headerMsg->GetPackSize());
  if (receivedBytes == 0)
  {
    std::cerr << "No reply" << std::endl;
    socket->CloseSocket();
    SetReturnValues(returnParameterFile,"ERROR: No reply received",0);
    exit(EXIT_SUCCESS);
  }
  if (receivedBytes != headerMsg->GetPackSize())
  {
    std::cerr << "Bad reply" << std::endl;
    socket->CloseSocket();
    SetReturnValues(returnParameterFile,"ERROR: invalid reply received",0);
    exit(EXIT_SUCCESS);
  }
  // Deserialize the header
  headerMsg->Unpack();
  if (strcmp(headerMsg->GetDeviceType(), "STRING") == 0)
  {
    reply=ReceiveString(socket, headerMsg);
    std::cout << "Result: "<<reply<<std::endl;
    SetReturnValues(returnParameterFile,reply.c_str(),1);
  }
  else
  {
    std::cerr << "Receiving unsupported message type: " << headerMsg->GetDeviceType() << std::endl;
    socket->Skip(headerMsg->GetBodySizeToRead(), 0);
    SetReturnValues(returnParameterFile,"ERROR: invalid reply type received",0);
    exit(EXIT_SUCCESS);
  }

  //------------------------------------------------------------
  // Close connection
  socket->CloseSocket();

  return EXIT_SUCCESS;
} 
