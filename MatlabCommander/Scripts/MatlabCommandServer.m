% OpenIGTLink server that executes the received string commands
function exitValue=MatlabCommandServer(port)

    global OPENIGTLINK_SERVER_SOCKET
    
    import java.net.Socket
    import java.io.*
    import java.net.ServerSocket

    serverSocketInfo.port=4100;
    serverSocketInfo.timeout=1000;

    if (nargin>0)
        serverSocketInfo.port=port;
    end

    disp(['Starting OpenIGTLink command server at port ' num2str(serverSocketInfo.port)]);    

    % Open a TCP Server Port
    if (exist('OPENIGTLINK_SERVER_SOCKET'))
        if (not(isempty(OPENIGTLINK_SERVER_SOCKET)))
          % Socket has not been closed last time
          disp('Socket has not been closed properly last time. Closing it now.');
          OPENIGTLINK_SERVER_SOCKET.close;
          OPENIGTLINK_SERVER_SOCKET=[];
        end
    end

    try
        serverSocketInfo.socket = ServerSocket(serverSocketInfo.port);        
        OPENIGTLINK_SERVER_SOCKET=serverSocketInfo.socket;
    catch 
        error('Failed to open server port. Make sure the port is not open already or blocked by firewall.');
    end        
    serverSocketInfo.socket.setSoTimeout(serverSocketInfo.timeout);

    disp('Waiting for client connections...');
    
    % Handle client connections
    while(true)

        % Wait for client connection
        drawNowCounter=0;
        while(true),            
            try 
              clientSocketInfo.socket = serverSocketInfo.socket.accept;  
              break; 
            catch
            end
            if (drawNowCounter>10)
              drawnow
              drawNowCounter=0;
            end;
            drawNowCounter=drawNowCounter+1;
            pause(0.5);
        end

        % Client connected
        disp('Client connected')
        rehash        
        clientSocketInfo.remoteHost = char(clientSocketInfo.socket.getInetAddress);
        clientSocketInfo.outputStream = clientSocketInfo.socket.getOutputStream;
        clientSocketInfo.inputStream = clientSocketInfo.socket.getInputStream;       
        clientSocketInfo.messageHeaderReceiveTimeoutSec=5;
        clientSocketInfo.messageBodyReceiveTimeoutSec=25;

        % Read command
        receivedMsg=ReadOpenIGTLinkStringMessage(clientSocketInfo);
        if(~isempty(receivedMsg) && length(receivedMsg.string)>0)
            disp(['Data type: [',char(receivedMsg.dataTypeName),']']);
            disp(['Device name: [',char(receivedMsg.deviceName),']']);
            disp(['Message string: [',char(receivedMsg.string),']']);

            cmd=char(receivedMsg.string);
            try
              % try running the command and capture the output
              response=eval(cmd);
            catch
              % failed, so the command is either invalid or does not provide output          
              try
                % retry without expecting an output
                eval(cmd);
                response='OK';
              catch ME
                % failed with and without an output, the command must be invalid
                response=ME.message;
              end          
            end
        else
            response='Failed to receive command';            
        end
        disp(response);
        
        % Send reply
        WriteOpenIGTLinkStringMessage(clientSocketInfo, num2str(response), strcat(char(receivedMsg.deviceName),'Reply'));

        % Close connection
        clientSocketInfo.socket.close;
        clientSocketInfo.socket=[];
        disp('Client connection closed');

    end

    % Close server socket
    serverSocketInfo.socket.close;
    serverSocketInfo.socket=[];

end

function msg=ReadOpenIGTLinkStringMessage(clientSocket)
    msg=ReadOpenIGTLinkMessage(clientSocket);
    if (length(msg.body)<5)
        disp('Error: STRING message received with incomplete contents')
        msg.string='';
        return
    end        
    strMsgEncoding=convertFromUint8VectorToUint16(msg.body(1:2));
    if (strMsgEncoding~=3)
        disp(['Warning: STRING message received with unknown encoding ',num2str(strMsgEncoding)])
    end
    strMsgLength=convertFromUint8VectorToUint16(msg.body(3:4));
    msg.string=char(msg.body(5:4+strMsgLength));
end    

function msg=ReadOpenIGTLinkMessage(clientSocket)
    openIGTLinkHeaderLength=58;
    headerData=ReadWithTimeout(clientSocket, openIGTLinkHeaderLength, clientSocket.messageHeaderReceiveTimeoutSec);
    if (length(headerData)==openIGTLinkHeaderLength)
        msg=ParseOpenIGTLinkMessageHeader(headerData);
        msg.body=ReadWithTimeout(clientSocket, msg.bodySize, clientSocket.messageBodyReceiveTimeoutSec);            
    else
        error('ERROR: Timeout while waiting receiving OpenIGTLink message header')
    end
end    
        
function result=WriteOpenIGTLinkStringMessage(clientSocket, msgString, deviceName)
    msg.dataTypeName='STRING';
    msg.deviceName=deviceName;
    msg.timestamp=0;
    msg.body=[convertFromUint16ToUint8Vector(3),convertFromUint16ToUint8Vector(length(msgString)),uint8(msgString)];
    disp(['Send reply as device ',msg.deviceName,': ',msgString])
    result=WriteOpenIGTLinkMessage(clientSocket, msg);
end

% Returns 1 if successful, 0 if failed
function result=WriteOpenIGTLinkMessage(clientSocket, msg)
    import java.net.Socket
    import java.io.*
    import java.net.ServerSocket
    % Add constant fields values
    msg.versionNumber=1;
    msg.bodySize=length(msg.body);
    msg.bodyCrc=0; % TODO: compute this
    % Pack message
    data=[];
    data=[data, convertFromUint16ToUint8Vector(msg.versionNumber)];
    data=[data, padString(msg.dataTypeName,12)];
    data=[data, padString(msg.deviceName,20)];
    data=[data, convertFromInt64ToUint8Vector(msg.timestamp)];
    data=[data, convertFromInt64ToUint8Vector(msg.bodySize)];
    data=[data, convertFromInt64ToUint8Vector(msg.bodyCrc)];
    data=[data, uint8(msg.body)];    
    result=1;
    try
        DataOutputStream(clientSocket.outputStream).write(uint8(data),0,length(data));
    catch ME
        disp(ME.message)
        result=0;
    end
    try
        DataOutputStream(clientSocket.outputStream).flush;
    catch ME
        disp(ME.message)
        result=0;
    end
    if (result==0)
      disp('Sending OpenIGTLink message failed');
    end
end

function data=ReadWithTimeout(clientSocket, requestedDataLength, timeoutSec)
    import java.net.Socket
    import java.io.*
    import java.net.ServerSocket

    % preallocate to improve performance
    data=zeros(1,requestedDataLength,'uint8');
    signedDataByte=int8(0);
    bytesRead=0;
    while(bytesRead<requestedDataLength)
        bytesToRead=min(clientSocket.inputStream.available, requestedDataLength-bytesRead);
        if (bytesRead==0 && bytesToRead>0)
            % starting to read message header
            tstart=tic;
        end
        for i = bytesRead+1:bytesRead+bytesToRead
            signedDataByte = DataInputStream(clientSocket.inputStream).readByte;
            if signedDataByte>=0
                data(i) = signedDataByte;
            else
                data(i) = bitcmp(-signedDataByte,'uint8')+1;
            end
        end            
        bytesRead=bytesRead+bytesToRead;
        if (bytesRead>0 && bytesRead<requestedDataLength)
            % check if the reading of the header has timed out yet
            timeElapsedSec=toc(tstart);
            if(timeElapsedSec>timeoutSec)
                % timeout, it should not happen
                % remove the unnecessary preallocated elements
                data=data(1:bytesRead);
                break
            end
        end
    end
end

%%  Parse OpenIGTLink messag header
% http://openigtlink.org/protocols/v2_header.html    
function parsedMsg=ParseOpenIGTLinkMessageHeader(rawMsg)
    parsedMsg.versionNumber=convertFromUint8VectorToUint16(rawMsg(1:2));
    parsedMsg.dataTypeName=char(rawMsg(3:14));
    parsedMsg.deviceName=char(rawMsg(15:34));
    parsedMsg.timestamp=convertFromUint8VectorToInt64(rawMsg(35:42));
    parsedMsg.bodySize=convertFromUint8VectorToInt64(rawMsg(43:50));
    parsedMsg.bodyCrc=convertFromUint8VectorToInt64(rawMsg(51:58));
end

function result=convertFromUint8VectorToUint16(uint8Vector)
  result=uint8Vector(1)*256+uint8Vector(2);
end 

function result=convertFromUint16ToUint8Vector(uint16Value)
  result=[uint8(uint16Value/256), uint8(mod(uint16Value,256))];
end 

function result=convertFromUint8VectorToInt64(uint8Vector)
  multipliers = [256^7 256^6 256^5 256^4 256^3 256^2 256^1 1];
  result = sum(int64(uint8Vector).*int64(multipliers));
end 

function result=convertFromInt64ToUint8Vector(int64Value)
  result=zeros(1,8,'uint8');
  result(1)=uint8(int64Value/256^7);
  result(2)=uint8(mod(int64Value/256^6,256));
  result(3)=uint8(mod(int64Value/256^5,256));
  result(4)=uint8(mod(int64Value/256^4,256));
  result(5)=uint8(mod(int64Value/256^3,256));
  result(6)=uint8(mod(int64Value/256^2,256));
  result(7)=uint8(mod(int64Value/256^1,256));
  result(8)=uint8(mod(int64Value,256));
end 

function paddedStr=padString(str,strLen)
  paddedStr=str(1:min(length(str),strLen));
  paddingLength=strLen-length(paddedStr);
  if (paddingLength>0)
      paddedStr=[paddedStr,zeros(1,paddingLength,'uint8')];
  end
end
