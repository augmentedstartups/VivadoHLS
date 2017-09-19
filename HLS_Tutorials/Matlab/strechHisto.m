function [ imgOut ] = strechHisto( imgIn )
%STRECH_HISTO Do the histogram expansion algorithm
yMax = single(255);
xMin = single(min(min(imgIn)));
xMax = single(max(max(imgIn)));

imgOut = imgIn;
for idx=1:numel(imgIn)
    x = single(imgIn(idx));
    imgOut(idx) = ((x - xMin) / (xMax-xMin)) * yMax;
end
end