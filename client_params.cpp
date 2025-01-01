
#include <chrono>
#include <ctime>
#include <fstream>

#include <opencv2/opencv.hpp>

#include "client_params.h"
#include "file_io.h"

void readParametersFromFile(const std::string &filename, Client_Parameters_Main &params)
{
    // Open the file containing the parameters
    std::ifstream infile(filename);
    if (!infile)
    {
        std::cerr << "Error opening file" << std::endl;
        return;
    }

    // Read the file line by line
    std::string line;
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        std::string name;
        std::string value;

        if (!(iss >> name >> value))
        {
            continue; // Skip lines that don't match the expected format
        }

        // Update the corresponding parameter in the structure
        if (name == "Cam_H_Size")
        {
            params.Cam_H_Size = std::stoi(value); // Convert string to integer
        }
        else if (name == "Cam_V_Size")
        {
            params.Cam_V_Size = std::stoi(value); // Convert string to integer
        }

        else if (name == "Screen_H_Size")
        {
            params.Screen_H_Size = std::stoi(value); // Convert string to integer
        }
        else if (name == "Screen_V_Size")
        {
            params.Screen_V_Size = std::stoi(value); // Convert string to integer}
        }

        else if (name == "Motion_Window_H_Size_Multiplier")
        {
            params.Motion_Window_H_Size_Multiplier = std::stoi(value); // Convert string to integer
        }
        else if (name == "Motion_Window_V_Size_Multiplier")
        {
            params.Motion_Window_V_Size_Multiplier = std::stoi(value); // Convert string to integer
        }
        else if (name == "Cycle_Time")
        {
            params.Cycle_Time = std::stof(value); // Convert string to integer
        }
        else if (name == "Noise_Threshold")
        {
            params.Noise_Threshold = std::stoi(value); // Convert string to integer
        }
        else if (name == "Motion_Threshold")
        {
            params.Motion_Threshold = std::stoi(value); // Convert string to integer
        }


        params.Motion_Window_H_Size = (params.Screen_H_Size * params.Motion_Window_H_Size_Multiplier) / 100;
        params.Motion_Window_V_Size = (params.Screen_V_Size * params.Motion_Window_V_Size_Multiplier) / 100;

        params.Motion_Window_H_Position = (params.Screen_H_Size - params.Motion_Window_H_Size) / 2;
        params.Motion_Window_V_Position = (params.Screen_V_Size - params.Motion_Window_V_Size) / 2;
    }

    std::cout << "Parameter 0: " << params.Cam_H_Size << std::endl;
    std::cout << "Parameter 1: " << params.Cam_V_Size << std::endl;
    std::cout << "Parameter 2: " << params.Screen_H_Size << std::endl;
    std::cout << "Parameter 3: " << params.Screen_V_Size << std::endl;
    std::cout << "Parameter 4: " << params.Motion_Window_H_Size << std::endl;
    std::cout << "Parameter 5: " << params.Motion_Window_V_Size << std::endl;
    std::cout << "Parameter 6: " << params.Motion_Window_H_Position << std::endl;
    std::cout << "Parameter 7: " << params.Motion_Window_V_Position << std::endl;
    std::cout << "Parameter 8: " << params.Cycle_Time << std::endl;
    std::cout << "Parameter 9: " << params.Noise_Threshold << std::endl;    
    std::cout << "Parameter 10: " << params.Motion_Threshold << std::endl;        
};



void readPiParametersFromFile(const std::string &filename, Pi_Parameters_Main &params)
{
    // Open the file containing the parameters
    std::ifstream infile(filename);
    if (!infile)
    {
        std::cerr << "Error opening file" << std::endl;
        return;
    }

    // Read the file line by line
    std::string line;
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        std::string name;
        std::string value;

        if (!(iss >> name >> value))
        {
            continue; // Skip lines that don't match the expected format
        }

        // Update the corresponding parameter in the structure
        if (name == "p0")
        {
            params.p0 = value;
        }
        if (name == "i0")
        {
            params.i0 = value;
        }

        if (name == "p1")
        {
            params.p1 = value;
        }
        if (name == "i1")
        {
            params.i1 = value;
        }

        if (name == "p2")
        {
            params.p2 = value;
        }
        if (name == "i2")
        {
            params.i2 = value;
        }

        if (name == "p3")
        {
            params.p3 = value;
        }
        if (name == "i3")
        {
            params.i3 = value;
        }

        if (name == "p4")
        {
            params.p4 = value;
        }
        if (name == "i4")
        {
            params.i4 = value;
        }
    }

    std::cout << "Parameter 0: " << params.p0 << std::endl;
    std::cout << "Parameter 1: " << params.i0 << std::endl;
    std::cout << "Parameter 2: " << params.p1 << std::endl;
    std::cout << "Parameter 3: " << params.i1 << std::endl;
    std::cout << "Parameter 4: " << params.p2 << std::endl;
    std::cout << "Parameter 5: " << params.i2 << std::endl;
    std::cout << "Parameter 6: " << params.p3 << std::endl;
    std::cout << "Parameter 7: " << params.i3 << std::endl;
    std::cout << "Parameter 8: " << params.p4 << std::endl;
    std::cout << "Parameter 9: " << params.i4 << std::endl;
}




void Sequencer(const bool Image_Motion, const cv::Mat &gray_frame_local)
{
    static std::time_t currentTime = std::time(nullptr);
    static std::tm *localTime = std::localtime(&currentTime);
    static unsigned long last_image_stored = currentTime;
    static unsigned long time_since_last_iage_stored = 0;
    std::string nextFileName;

    // std::cout << "Current time in 24-hour format: "
    //           << std::put_time(localTime, "%H:%M:%S")
    //           << " "  << currentTime <<  std::endl;

    currentTime = std::time(nullptr);
    time_since_last_iage_stored = currentTime - last_image_stored;
    if ((time_since_last_iage_stored > 600) && Image_Motion)   //  in seconds 10 minutes
    {
        last_image_stored = currentTime;

        auto loopStartTime = std::chrono::steady_clock::now();

        // nextFileName = getNextFileNameRaw("./raw/");
        // writeMatRawData(gray_frame_local, nextFileName);

        nextFileName = getNextFileNameTif("../tif/");
        writeMatToTif(gray_frame_local, nextFileName);

        auto loopEndTime = std::chrono::steady_clock::now();

        std::chrono::duration<double> elapsed_seconds = loopEndTime - loopStartTime;
        // std::cout << "Loop duration: " << elapsed_seconds.count() << "s\n";
        std::cout << std::endl;
        std::cout << "WRITING next filename " << nextFileName << "  time  " << elapsed_seconds.count() << std::endl;
        std::cout << std::endl;        
    }
}
