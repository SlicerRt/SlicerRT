function argVal=getarg(args,argName,argIndex)
% Retrieve the value of a command-line argument
% argName: name of the named argument (e.g., if command-line argument is '--threshold' then the argName should be 'threshold')
% argIndex: used only for retrieving unnamed arguments (argName must be set to ''), index of the first argument is 1

% Add an empty argument at the end to make sure we flush all the partially processed arguments
args{length(args)+1}='--';

parsedArgs={};
unnamedArgs={};

curArgName='';
curArgValue='';
for curArgIndex=1:length(args)
    if (args{curArgIndex}(1)=='-')
        % This is an argument name
        if (~isempty(curArgName))
            % There is an argument already, so store it
            parsedArgs.(curArgName)=curArgValue;
            curArgName='';
            curArgValue='';            
        else
            % There is no argument name, only argument value, so it's an
            % unnamed argument
            if (~isempty(curArgValue))
                unnamedArgs{length(unnamedArgs)+1}=curArgValue;
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
        curArgValue=args{curArgIndex};
        if (isempty(curArgName))
            unnamedArgs{length(unnamedArgs)+1}=curArgValue;
            curArgValue='';
        else
            parsedArgs.(curArgName)=curArgValue;
            curArgName='';
            curArgValue='';
        end
    end
end

if (isempty(argName))
    argVal=unnamedArgs{argIndex};
else
    argVal=parsedArgs.(argName);
end
