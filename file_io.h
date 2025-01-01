
#ifndef FILE_IO_H
#define FILE_IO_H

#include <opencv2/opencv.hpp>


cv::Mat loadImage(const std::string &imageFile);

std::string getNextFileNameRaw(const std::string &directory);

std::string getNextFileNameTif(const std::string &directory);

void writeMatRawData(const cv::Mat &mat, const std::string &filename);

bool writeMatToTif(const cv::Mat &mat, const std::string &filename);



#endif // CAMERA_GRAB_H