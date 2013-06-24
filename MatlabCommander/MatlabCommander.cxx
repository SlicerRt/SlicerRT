#include "MatlabCommanderCLP.h" 

#include <iostream>
#include <fstream>
#include <math.h>
#include <cstdlib>

#include "igtlOSUtil.h"
#include "igtlStringMessage.h"
#include "igtlClientSocket.h"

#include "vtksys/SystemTools.hxx"
#include "vtksys/Process.h"

const std::string CALL_MATLAB_FUNCTION_ARG="--call-matlab-function";
const std::string MATLAB_DEFAULT_HOST="127.0.0.1";
const int MATLAB_DEFAULT_PORT=4100;

const int MAX_MATLAB_STARTUP_TIME_SEC=60; // maximum time allowed for Matlab to start
const int MAX_MATLAB_LAUNCHER_STARTUP_TIME_SEC=30; // maximum time allowed for Matlab launcher to start

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

// Returns true if execution is successful. Matlab start may take an additional minute after this function returns.
bool StartMatlabServer()
{
  const char* matlabExecutablePath=getenv("SLICER_MATLAB_EXECUTABLE_PATH");
  const char* matlabCommandServerScriptPath=getenv("SLICER_MATLAB_COMMAND_SERVER_SCRIPT_PATH");
  
  if ( matlabExecutablePath == NULL )
  {
    std::cerr << "ERROR: The SLICER_MATLAB_EXECUTABLE_PATH environment variable is not set. Cannot start the Matlab command server." << std::endl;
    return false; 
  }
  if ( matlabCommandServerScriptPath == NULL )
  {
    std::cerr << "ERROR: The SLICER_MATLAB_COMMAND_SERVER_SCRIPT_PATH environment variable is not set. Cannot start the Matlab command server." << std::endl;
    return false; 
  }

  if ( !vtksys::SystemTools::FileExists( matlabExecutablePath, true) )
  {
    std::cerr << "ERROR: Unable to find Matlab executable defined in the SLICER_MATLAB_EXECUTABLE_PATH environment variable: " 
      << matlabExecutablePath << std::endl;
    return false; 
  }

  bool success = true; 
  try 
  {
    std::vector<const char*> command;
    command.clear();

    // Add gnuplot command 
    std::cout << "Starting Matlab server: " << matlabExecutablePath; 
    command.push_back(matlabExecutablePath);

    // start in minimized, with text console only
    std::cout << " -automation"; 
    command.push_back("-automation");

    // script directory (-sd) option does not work with the automation option, so need to use the run command to specify full script path

    // run script after startup
    std::cout << " -r"; 
    command.push_back("-r");    
    std::string startupCommand=std::string("run('")+matlabCommandServerScriptPath+"');";
    std::cout << " " << startupCommand; 
    command.push_back(startupCommand.c_str());

    // The array must end with a NULL pointer.
    std::cout << std::endl;
    command.push_back(0); 

    // Create new process 
    vtksysProcess* gp = vtksysProcess_New();

    // Set command
    vtksysProcess_SetCommand(gp, &*command.begin());

    // Hide window
    vtksysProcess_SetOption(gp,vtksysProcess_Option_HideWindow, 1);

    // Run the application
    //std::cout << "Start Matlab process ..." << std::endl; 
    vtksysProcess_Execute(gp); 

    double allowedTimeoutSec=MAX_MATLAB_LAUNCHER_STARTUP_TIME_SEC;
    //std::cout << "Wait for exit (Timeout: " << allowedTimeoutSec << "s) ..." << std::endl; 
    std::string buffer;     
    double timeoutSec = allowedTimeoutSec; 
    char* data = NULL;
    int length=0;

    int waitStatus=vtksysProcess_Pipe_None;
    do
    {
      waitStatus=vtksysProcess_WaitForData(gp,&data,&length,&timeoutSec);
      if (waitStatus==vtksysProcess_Pipe_Timeout)
      {
        std::cerr << "ERROR: Timeout (execution time>"<<allowedTimeoutSec<<"sec) while trying to execute the process. Kill the process." << std::endl;
        vtksysProcess_Kill(gp);
      }
      //std::cerr << "Process start timeout remaining: "<<timeoutSec<<" length="<<length<<std::endl;
      for(int i=0;i<length;i++)
      {
        buffer += data[i];
      }
      length=0;
    }
    while (waitStatus!=vtksysProcess_Pipe_None);         

    if (vtksysProcess_WaitForExit(gp, 0)==0)
    {
      // 0 = Child did not terminate 
      std::cerr << "Process did not terminate within the specified timeout"<<std::endl;
    }
    //std::cout << "Execution time was: " << allowedTimeoutSec - timeoutSec << "sec" << std::endl; 

    int result(0); 
    switch ( vtksysProcess_GetState(gp) )
    {
    case vtksysProcess_State_Exited: 
      {
        result = vtksysProcess_GetExitValue(gp); 
        //std::cout << "Process exited: " << result << std::endl;
        //std::cout << "Program output: " << buffer << std::endl;
      }
      break; 
    case vtksysProcess_State_Error: 
      {
        std::cerr << "ERROR: Error during execution: " << vtksysProcess_GetErrorString(gp) << std::endl;
        std::cout << "Program output: " << buffer << std::endl;
        success=false;
      }
      break;
    case vtksysProcess_State_Exception: 
      {
        std::cerr << "ERROR: Exception during execution: " << vtksysProcess_GetExceptionString(gp) << std::endl;
        std::cout << "Program output: " << buffer << std::endl;
        success=false;
      }
      break;
    case vtksysProcess_State_Starting: 
    case vtksysProcess_State_Executing:
    case vtksysProcess_State_Expired: 
      {
        std::cerr << "ERROR: Unexpected ending state after running execution" << std::endl;
        std::cout << "Program output: " << buffer << std::endl;
        success=false;
      }
      break;
    case vtksysProcess_State_Killed: 
      {
        std::cerr << "ERROR: Program killed" << std::endl;
        std::cout << "Program output: " << buffer << std::endl;
        success=false;
      }
      break;
    }
    
    vtksysProcess_Delete(gp); 
  }
  catch (...)
  {
    std::cerr << "ERROR: Unknown exception while trying to execute command" << std::endl; 
    success=false;; 
  }

  if (success)
  {
    std::cout << "Matlab process start requested successfully" << std::endl;
  }
  else
  {
    std::cerr << "ERROR: Failed to execute Matlab" << std::endl; 
  }

  return success;
}

ExecuteMatlabCommandStatus ExecuteMatlabCommand(const std::string& hostname, int port, const std::string &cmd, std::string &reply)
{
  //------------------------------------------------------------
  // Establish Connection
  igtl::ClientSocket::Pointer socket;
  socket = igtl::ClientSocket::New();

  int connectErrorCode = socket->ConnectToServer(hostname.c_str(), port);
  if (connectErrorCode!=0)
  {
    // Maybe Matlab server has not been started, try to start it
    if (StartMatlabServer())
    {
      // process start requested, try to connect
      for (int retryAttempts=0; retryAttempts<MAX_MATLAB_STARTUP_TIME_SEC; retryAttempts++)
      {
        connectErrorCode = socket->ConnectToServer(hostname.c_str(), port);
        if (connectErrorCode==0)
        {
          // success
          break;
        }
        // Failed to connect, wait some more and retry
        vtksys::SystemTools::Delay(1000); // msec
        std::cerr << "Waiting for Matlab startup ... " << retryAttempts << "sec" << std::endl;
      }
    }
  }
  else
  {
    std::cerr << "Failed to start Matlab process" << std::endl;
  }
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
  socket->SetSendTimeout(5000); // timeout in msec
  if (!socket->Send(stringMsg->GetPackPointer(), stringMsg->GetPackSize()))
  {
    // Failed to send the message
    std::cerr << "Failed to send message to Matlab process" << std::endl;
    socket->CloseSocket();
    return COMMAND_STATUS_FAILED;
  }

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
  // Search for the --returnparameterfile argument. If it is present then arguments shall be returned.
  const std::string returnParameterFileArgName="--returnparameterfile";
  std::string returnParameterFileArgValue;
  for (int argvIndex=3; argvIndex<argc; argvIndex++)
  {
    if (returnParameterFileArgName.compare(argv[argvIndex])==0)
    {
      // found the return parameter file name
      if (argvIndex+1>=argc)
      {
        std::cerr << "ERROR: --returnparameterfile value is not defined" << std::endl;
        break;
      }
      returnParameterFileArgValue=argv[argvIndex+1];
      if ( returnParameterFileArgValue.at(0) == '"' ) 
      {
        // this is a quoted string => remove the quotes (Matlab uses different quotes anyway)
        returnParameterFileArgValue.erase( 0, 1 ); // erase the first character
        returnParameterFileArgValue.erase( returnParameterFileArgValue.size() - 1 ); // erase the last character
      }
      break;
    }
  }
  
  // No return value:
  //   myfunction( cli_argsread({"--paramName1","paramValue1",...}) );
  // With return value:
  //   cli_argswrite( myfunction( cli_argsread({"--paramName1","paramValue1",...}) ) );

  std::string functionName=argv[2];
  std::string cmd;
  if (!returnParameterFileArgValue.empty())
  {
    // with return value
    cmd+="cli_argswrite('"+returnParameterFileArgValue+"',";
  }
  cmd+=functionName+"(cli_argsread({";

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
  cmd+="}))";
  if (!returnParameterFileArgValue.empty())
  {
    // with return value
    cmd+=")";
  }
  cmd+=";";

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
    std::cerr << reply << std::endl;
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
