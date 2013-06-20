function setarg(args,argName,argValue)
% Set a return value

outputFilename=getarg(args,'returnparameterfile')

% open file for appended writing

fid=fopen(outputFilename, 'at+');
if(fid<=0) 
  fprintf('Could not open file: %s\n', outputFilename);
end

fprintf(fid,argName);
fprintf(fid,' = ');
fprintf(fid,num2str(argValue));
fprintf(fid,'\n');

fclose(fid);
