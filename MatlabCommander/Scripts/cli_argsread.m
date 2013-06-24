function params=cli_argsread(args)
% Retrieve parameters in a structure from a list of command-line arguments
% The output structure contains all the named arguments (field name is the command-line argument name)
% and an "unnamed" argument containing the list of unnamed arguments in a cell.

% Add an empty argument at the end to make sure we flush all the partially processed arguments
args{length(args)+1}='--';

params={};
params.unnamed={};

curArgName='';
curArgValue='';
for curArgIndex=1:length(args)
    if (args{curArgIndex}(1)=='-')
        % This is an argument name
        if (~isempty(curArgName))
            % There is an argument already, so store it
            params.(curArgName)=curArgValue;
            curArgName='';
            curArgValue='';            
        else
            % There is no argument name, only argument value, so it's an
            % unnamed argument
            if (~isempty(curArgValue))
                params.unnamed{length(params.unnamed)+1}=curArgValue;
                curArgValue='';
            end
        end
        % Save the new argument name
        if (args{curArgIndex}(2)=='-')
            % long flag (--argName)
            curArgName=args{curArgIndex}(3:end);
        else
            % long flag (-argName)
            curArgName=args{curArgIndex}(2:end);
        end
        continue;
    else
        % this is an argument value
        curArgValue=getValueFromString(args{curArgIndex});
        if (isempty(curArgName))
            params.unnamed{length(params.unnamed)+1}=curArgValue;
            curArgValue='';
        else
            params.(curArgName)=curArgValue;
            curArgName='';
            curArgValue='';
        end
    end
end

function paramValue=getValueFromString(paramString)
% Store numerical values as numbers (to don't require the user to call str2num to get a numerical parameter from a string)
  [paramValue paramCount paramError]=sscanf(paramString,'%f,'); % reads one or more comma-separated floating point values
  if (isempty(paramError))
    % No error, so it was a number or vector of numbers
    return;
  end
  % There was an error, so we interpret this value as a string
  paramValue=paramString;
