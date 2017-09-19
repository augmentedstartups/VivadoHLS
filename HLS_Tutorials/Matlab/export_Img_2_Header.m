function export_Img_2_Header( img, filename )
%EXPORT_IMG_2_HEADER Summary of this function goes here
%   Detailed explanation goes here
    sizeArray = numel(img);
    fileID = fopen(filename,'w');
    fprintf(fileID,'//Image on header\n');
    fprintf(fileID,'unsigned char img[%d] = {',sizeArray);
    img_sq = reshape(img,[1 sizeArray]);
    for idx=1:numel(img_sq)
        val = img_sq(idx);
        if idx ~= sizeArray
            fprintf(fileID,'%d ,',val);
        else
            fprintf(fileID,'%d',val);
        end
    end
    fprintf(fileID,'};\n');
    fclose(fileID);
end

