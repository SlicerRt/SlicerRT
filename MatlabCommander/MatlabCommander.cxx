#include "MatlabCommanderCLP.h" 

#include <iostream>
#include <fstream>
#include <math.h>
#include <cstdlib>

#include "igtlOSUtil.h"
#include "igtlStringMessage.h"
#include "igtlClientSocket.h"

const std::string CALL_MATLAB_FUNCTION_ARG="--call-matlab-function";
const std::string MATLAB_DEFAULT_HOST="127.0.0.1";
const int MATLAB_DEFAULT_PORT=4100;

enum ExecuteMatlabCommandStatus
{
  COMMAND_STATUS_FAILED=0,
  COMMAND_STATUS_SUCCESS=1
};

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

ExecuteMatlabCommandStatus ExecuteMatlabCommand(const std::string& hostname, int port, const std::string &cmd, std::string &reply)
{
  //------------------------------------------------------------
  // Establish Connection
  igtl::ClientSocket::Pointer socket;
  socket = igtl::ClientSocket::New();
  int connectErrorCode = socket->ConnectToServer(hostname.c_str(), port);
  if (connectErrorCode != 0)
  {        
    reply="ERROR: Cannot connect to the server";
    return COMMAND_STATUS_FAILED;
  }

  //------------------------------------------------------------
  // Send command
  igtl::StringMessage::Pointer stringMsg;
  stringMsg = igtl::StringMessage::New();
  stringMsg->SetDeviceName("CMD");
  std::cout << "Sending string: " << cmd << std::endl;
  stringMsg->SetString(cmd.c_str());
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
    reply="No reply";
    socket->CloseSocket();
    return COMMAND_STATUS_FAILED;
  }
  if (receivedBytes != headerMsg->GetPackSize())
  {
    reply = "Bad reply";
    socket->CloseSocket();
    return COMMAND_STATUS_FAILED;
  }
  // Deserialize the header
  headerMsg->Unpack();
  if (strcmp(headerMsg->GetDeviceType(), "STRING") != 0)
  {
    reply = std::string("Receiving unsupported message type: ") + headerMsg->GetDeviceType();
    socket->Skip(headerMsg->GetBodySizeToRead(), 0);
    socket->CloseSocket();
    return COMMAND_STATUS_FAILED;
  }
  // Get the reply string
  reply=ReceiveString(socket, headerMsg);

  //------------------------------------------------------------
  // Close connection
  socket->CloseSocket();

  return COMMAND_STATUS_SUCCESS;
}

int CallMatlabFunction(int argc, char * argv [])
{
  std::string functionName=argv[2];
  std::string cmd;
  cmd+=functionName+"({";
  for (int argvIndex=3; argvIndex<argc; argvIndex++)
  {
    std::string arg=argv[argvIndex];
    if ( arg.at(0) == '"' ) 
    {
      // this is a quoted string => remove the quotes (Matlab uses different quotes anyway)
      arg.erase( 0, 1 ); // erase the first character
      arg.erase( arg.size() - 1 ); // erase the last character
    }
    cmd+=std::string("'")+arg+"'";
    if (argvIndex+1<argc)
    {
      // not the last argument, so add a separator
      cmd+=",";
    }
  }
  cmd+="})";

  std::cout << "Command: " << cmd << std::endl;

  std::string reply;
  ExecuteMatlabCommandStatus status=ExecuteMatlabCommand(MATLAB_DEFAULT_HOST, MATLAB_DEFAULT_PORT, cmd, reply);
  if (status!=COMMAND_STATUS_SUCCESS)
  {
    std::cerr << "ERROR: " << reply << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << reply << std::endl;
  return EXIT_SUCCESS;  
}


int CallStandardCli(int argc, char * argv [])
{
  PARSE_ARGS;
  ExecuteMatlabCommandStatus status=ExecuteMatlabCommand(hostname, port, cmd, reply);
  if (status==COMMAND_STATUS_SUCCESS)
  {
    std::cout << reply << std::endl;
    completed=true;
  }
  else
  {
    std::cerr << "ERROR: " << reply << std::endl;
    completed=false;
  }
  SetReturnValues(returnParameterFile,reply.c_str(),completed);    
  return EXIT_SUCCESS; // always return with EXIT_SUCCESS, otherwise Slicer ignores the return values and we cannot show the reply on the module GUI
}

int main (int argc, char * argv [])
{
  if (argc>2 && CALL_MATLAB_FUNCTION_ARG.compare(argv[1])==0)
  {
    // MatlabCommander is called with arguments: --call-matlab-function function_name parameter1 parameter2 ...
    return CallMatlabFunction(argc, argv);
  }
  else
  {
    // MatlabCommander is called as a standard CLI modul
    return CallStandardCli(argc, argv);
  }
} 
