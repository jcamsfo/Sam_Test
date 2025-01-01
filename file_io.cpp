
#include <filesystem>
#include <regex>
#include <fstream>

#include "file_io.h"


namespace fs = std::filesystem;



// Load a grayscale image
cv::Mat loadImage(const std::string &imageFile)
{
    cv::Mat img = cv::imread(imageFile, cv::IMREAD_GRAYSCALE);
    if (img.empty())
    {
        std::cerr << "Couldn't open image " << imageFile << ".\n";
        exit(-1);
    }
    return img;
}


std::string getNextFileNameRaw(const std::string &directory)
{
    std::regex filePattern(R"((\d+)\.raw)");

    int maxNumber = -1;

    // Iterate through the files in the directory
    for (const auto &entry : fs::directory_iterator(directory))
    {
        if (fs::is_regular_file(entry))
        {
            std::string filename = entry.path().filename().string();
            std::smatch match;
            if (std::regex_match(filename, match, filePattern))
            {
                int number = std::stoi(match[1]);
                if (number > maxNumber)
                {
                    maxNumber = number;
                }
            }
        }
    }

    // Determine the new file name
    int newFileNumber = maxNumber + 1;
    std::string newFilename = std::to_string(newFileNumber).insert(0, 6 - std::to_string(newFileNumber).length(), '0') + ".raw";

    return directory + newFilename;
}


std::string getNextFileNameTif(const std::string &directory)
{
    std::regex filePattern(R"((\d+)\.tif)");

    int maxNumber = -1;

    // Iterate through the files in the directory
    for (const auto &entry : fs::directory_iterator(directory))
    {
        if (fs::is_regular_file(entry))
        {
            std::string filename = entry.path().filename().string();
            std::smatch match;
            if (std::regex_match(filename, match, filePattern))
            {
                int number = std::stoi(match[1]);
                if (number > maxNumber)
                {
                    maxNumber = number;
                }
            }
        }
    }

    // Determine the new file name
    int newFileNumber = maxNumber + 1;
    std::string newFilename = std::to_string(newFileNumber).insert(0, 6 - std::to_string(newFileNumber).length(), '0') + ".tif";

    return directory + newFilename;
}


void writeMatRawData(const cv::Mat &mat, const std::string &filename)
{
    // Open a binary file for writing
    std::ofstream outFile(filename, std::ios::binary);

    // Check if the file is opened successfully
    if (!outFile)
    {
        std::cerr << "Error opening file for writing!" << std::endl;
        return;
    }

    // Write matrix data (no additional metadata)
    outFile.write(reinterpret_cast<const char *>(mat.data), mat.total() * mat.elemSize());

    // Close the file
    outFile.close();
}


bool writeMatToTif(const cv::Mat &mat, const std::string &filename)
{

    // Set the TIFF compression parameter to none (1)
    std::vector<int> compression_params = {cv::IMWRITE_TIFF_COMPRESSION, 1}; // No compression

    // Write the image to an uncompressed TIFF file
    if (cv::imwrite(filename, mat, compression_params))
    {
        std::cout << "Image saved as '" << filename << "' successfully." << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Failed to save the image." << std::endl;
        return false;
    }
}