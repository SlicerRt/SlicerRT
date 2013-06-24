function argswrite(returnParameterFilename, args)
% Write return values specified in the args structure to the return parameter text file

% open file for writing text file (create new)
fid=fopen(returnParameterFilename, 'wt+');
assert(fid > 0, ['Could not open output file:' returnParameterFilename]);

argsCellArr = struct2cell(args);

% Get string names for each field in the meta data
fields = fieldnames(args);

% Print the header data to the output file
for i=1:numel(fields)
  fprintf(fid,fields{i});
  fprintf(fid,' = ');
  fprintf(fid,num2str(argsCellArr{i}));
  fprintf(fid,'\n');
end

fclose(fid);
