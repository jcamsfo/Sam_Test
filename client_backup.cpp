#include <iostream>
#include <thread>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <fstream>
#include <chrono>
#include <list>
#include <cstring>
#include <map>
#include <stdlib.h>
#include <string>
#include <ctime>
#include <deque>
#include <filesystem> // C++17 for directory operations

// #include <unistd.h>
// #include <errno.h>
// #include <netdb.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>

#include <opencv2/opencv.hpp>

#include "comms.h"

#include "camera_grab.h"
#include "file_io.h"
#include "client_params.h"

namespace fs = std::filesystem;

void usage()
{
    cout << "usage: MRR_Pi_client_2 [-r repeat_count]  [-f fps] [-p port_number] [-i ip_address] [-p port_number] [-i ip_address] ..." << endl;
    cout << endl;
    cout << "Sample MRR_Pi client code which sends images to one or more MRR_Pi servers." << endl;
    cout << "Each server is described by both a port_number and an ip_address," << endl;
    cout << "so the count of port_numbers must mach the count of ip_addresses," << endl;
    cout << "which are paired by their order in the command line." << endl;
    cout << "If both MRR_Pi_Client_2 and MRR_Pi_server_2 are on the same machine, use 127.0.0.1 as the ip address." << endl;
    cout << "Repeat_count defaults to 0 (loop forever), it is the total number of image files to send to each server, repeatedly picking from the 5 images in the 'raw' folder." << endl;
    cout << "Default fps is 30" << endl;
    cout << endl;

    cout << "sample command line (server is running on default port on localhost): ./MRR_Pi_client_2" << endl;
    cout << "sample command line (specify port and ip_address): ./MRR_Pi_client_2 -i 127.0.0.1 -p 5569" << endl;
    cout << "sample command line (two servers specified): ./MRR_Pi_client_2 -i 127.0.0.1 -p 5569 -i 127.0.0.1 -p 5570" << endl;
    cout << endl;
}

class CommPlus : public Comm
{
public:
    SD blocking_sd;
};

// used to create the subclass CommPlus instead of the default Comm class
Comm *comm_factory()
{
    return new CommPlus();
}

int Display_Test_Images(cv::Mat &Image_1, cv::Mat &Image_2)
{
    static bool displayImage = true;
    static bool displayMotion = true;
    cv::namedWindow("Test Webcam Feed", cv::WINDOW_AUTOSIZE);

    // opencv display stuff
    int key = cv::waitKey(1);
    if (key == 27) // ASCII code for the escape key
        return -1;

    if (key == 'i')
    {
        displayImage = !displayImage; // Toggle the flag
    }

    if (key == 'm')
    {
        displayMotion = !displayMotion; // Toggle the flag
    }

    // Debug Display  // display the image (not needed in final)
    if (displayImage)
    {
        cv::imshow("Test Webcam Feed", Image_1);
    }
    else if (cv::getWindowProperty("Test Webcam Feed", cv::WND_PROP_AUTOSIZE) != -1)
    {
        cv::destroyWindow("Test Webcam Feed"); // Close the window if the image is not displayed
    }

    if (displayMotion)
    {
        cv::imshow("Test frame_Abs_Diff Feed", Image_2);
    }
    else if (cv::getWindowProperty("Test frame_Abs_Diff Feed", cv::WND_PROP_AUTOSIZE) != -1)
    {
        cv::destroyWindow("Test frame_Abs_Diff Feed"); // Close the window if the image is not displayed
    }

    return 1;
}

std::string readFileToString(const std::string &filename)
{
    std::ifstream inputFile(filename);
    if (!inputFile.is_open())
    {
        std::cerr << "Error opening file" << std::endl;
        return "";
    }

    std::ostringstream result;
    std::string line;

    while (std::getline(inputFile, line))
    {
        result << line << " ";
    }

    inputFile.close();

    std::string finalString = result.str();

    if (!finalString.empty())
    {
        finalString.pop_back(); // Remove the trailing space
    }

    return finalString;
}

// Function to read the file and return a deque of strings
std::deque<std::string> readFileToDeque(const std::string &filename)
{
    std::ifstream inputFile(filename);
    if (!inputFile.is_open())
    {
        std::cerr << "Error opening file" << std::endl;
        return {};
    }

    std::deque<std::string> result;
    std::ostringstream section;
    std::string line;

    while (std::getline(inputFile, line))
    {
        if (line.find_first_not_of('-') == std::string::npos && line.size() >= 3)
        {
            // If we encounter a delimiter (all dashes, with length at least as the delimiter), store the current section in the deque
            result.push_back(section.str());
            section.str(""); // Clear the stringstream for the next section
            section.clear(); // Reset any error flags
        }
        else
        {
            section << line << " ";
        }
    }

    // Add the last section after the final boundary (if any)
    if (!section.str().empty())
    {
        result.push_back(section.str());
    }

    inputFile.close();

    // Trim trailing spaces from each string in the deque
    for (auto &str : result)
    {
        if (!str.empty() && str.back() == ' ')
        {
            str.pop_back();
        }
    }

    return result;
}

void on_trackbar(int, void *) {}

void createSliders(const string &windowName, int numSliders)
{
    // Create a window
    cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);

    // Create the specified number of trackbars (sliders) with limits from 0 to 100
    for (int i = 1; i <= numSliders; i++)
    {
        string trackbarName = "Slider" + to_string(i);
        cv::createTrackbar(trackbarName, windowName, nullptr, 100, on_trackbar);
    }
}

// Returns hostname for the local computer
// void checkHostName(int hostname)
// {
//     if (hostname == -1)
//     {
//         perror("gethostname");
//         exit(1);
//     }
// }

// // Returns host information corresponding to host name
// void checkHostEntry(struct hostent * hostentry)
// {
//     if (hostentry == NULL)
//     {
//         perror("gethostbyname");
//         exit(1);
//     }
// }

// // Converts space-delimited IPv4 addresses
// // to dotted-decimal format
// void checkIPbuffer(char *IPbuffer)
// {
//     if (NULL == IPbuffer)
//     {
//         perror("inet_ntoa");
//         exit(1);
//     }
// }

std::string get_random_file(const std::string &dir_path)
{
    std::vector<std::string> files;

    // Read the directory and collect file names
    for (const auto &entry : fs::directory_iterator(dir_path))
    {
        if (fs::is_regular_file(entry.status()))
        {
            files.push_back(entry.path().string());
        }
    }

    if (files.empty())
    {
        return ""; // Return an empty string if no files are found
    }

    // Seed the random number generator
    std::srand(std::time(nullptr));

    // Select a random index from the list of files
    int random_index = std::rand() % files.size();
    return files[random_index];
}

int main(int argc, char *argv[])
{

    const string windowName = "Sliders";
    const int numSliders = 5;

    // Create the sliders
    createSliders(windowName, numSliders);

    // Create a black image to display
    cv::Mat sliders_img = cv::Mat::zeros(300, 512, CV_8UC3);

    /***************************  MY CODE  **************************************/

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    int randomValue = std::rand() % 100;

    bool camera_good;
    auto loopStartTime = std::chrono::steady_clock::now();
    auto loopEndTime = std::chrono::steady_clock::now();
    auto ProcessStartTime = std::chrono::steady_clock::now();
    auto ProcessEndTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> ProcessTime = ProcessEndTime - ProcessStartTime;
    bool displayImage = true;
    bool displayMotion = true;
    bool Image_Motion = false;
    bool New_Frame = true;
    int Image_Status = -1;
    long loopCount = 0;

    Client_Parameters_Main Client_Params;

    Pi_Parameters_Main Pi_Params;

    readParametersFromFile("client_params.txt", Client_Params);

    readPiParametersFromFile("pi_addresses.txt", Pi_Params);

    std::deque<std::string> server_params_read = readFileToDeque("server_params.txt");

    // std::string dir_path = "../tif"; // Replace with your directory
    // ProcessStartTime = std::chrono::steady_clock::now();
    // std::string random_file = get_random_file(dir_path);
    // ProcessEndTime = std::chrono::steady_clock::now();
    // ProcessTime = ProcessEndTime - ProcessStartTime;
    // std::cout << "ProcessTime: " << ProcessTime.count() << std::endl;
    // std::cout << "Randomly selected file: " << random_file << std::endl;
    // exit(0);

    // std::string imageFile = "../../images/image.jpg"; // Path to your image file
    // cv::Mat img = loadImage(imageFile);

    uchar gray_frame_raw[Client_Params.Screen_H_Size * Client_Params.Screen_V_Size];
    cv::Mat gray_frame(Client_Params.Screen_V_Size, Client_Params.Screen_H_Size, CV_8UC1, gray_frame_raw);   // Create an empty cv::Mat with the desired dimensions
    cv::Mat frame_Abs_Diff(Client_Params.Motion_Window_V_Size, Client_Params.Motion_Window_H_Size, CV_8UC1); // Create an empty cv::Mat with the desired dimensions
    std::vector<cv::Mat> Mats_5;

    cv::VideoCapture cap = InitWebCam(camera_good, Client_Params.Cam_H_Size, Client_Params.Cam_V_Size);

    cv::namedWindow("Test Webcam Feed", cv::WINDOW_AUTOSIZE);

    // std::cout << " HERR " << getNextFileNameRaw("../raw/") << std::endl;
    std::cout << " HERR " << getNextFileNameTif("../tif/") << std::endl;

    /***************************  MY CODE  DONE  ********************************/

    cout << CV_VERSION ;

    float fps = .5; // was30  1.1 seconds per image
    long loop_count = 0;

    // std::string connections[5] = {"x", "-i", "127.0.0.1", "-p", "5569"};

    // std::string connections[5] = {"x", "-i", "192.168.42.17", "-p", "5569"};

    // std::string connections[] = {"x", "-i", Pi_Params.i0, "-p", Pi_Params.p0 };

    // std::string connections[] = {"x", "-i", Pi_Params.i0, "-p", Pi_Params.p0, "-i", Pi_Params.i1, "-p", Pi_Params.p1, "-i", Pi_Params.i2, "-p", Pi_Params.p2};

    // std::string connections[] = {"x", "-i", Pi_Params.i0, "-p", Pi_Params.p0, "-i", Pi_Params.i1, "-p", Pi_Params.p1, "-i", Pi_Params.i2, "-p", Pi_Params.p2, "-i", Pi_Params.i3, "-p", Pi_Params.p3};

    // std::string connections[] = {"x", "-i", Pi_Params.i0, "-p", Pi_Params.p0, "-i", Pi_Params.i2, "-p", Pi_Params.p2};

    std::string connections[] = {"x", "-i", Pi_Params.i0, "-p", Pi_Params.p0,
                                 "-i", Pi_Params.i1, "-p", Pi_Params.p1,
                                 "-i", Pi_Params.i2, "-p", Pi_Params.p2,
                                 "-i", Pi_Params.i3, "-p", Pi_Params.p3,
                                 "-i", Pi_Params.i4, "-p", Pi_Params.p4};

    // std::string connections[] = {"x", "-i", Pi_Params.i0, "-p", Pi_Params.p0 };

    //  std::string connections[5] = {"x", "-i", "127.0.0.1", "-p", "5569"};

    int total_args = 21; //21;

    char *argv_file[total_args];

    for (int i = 0; i < total_args; i++)
    {
        argv_file[i] = new char[connections[i].length() + 1];
        strcpy(argv_file[i], connections[i].c_str());
    }
    int argc_file = total_args; // sizeof(connections);

    for (int i = 0; i < total_args; i++)
    {
        std::cout << "argc XX: " << argv_file[i] << endl;
    }

    // exit(0);

    usage();

    auto blocking_send = Comm::NON_BLOCKING;

    // list<Comm *> comms = Comm::start_clients(nullptr, 5, argv_file, comm_factory);
    list<Comm *> comms = Comm::start_clients(nullptr, argc_file, argv_file, comm_factory);
    if (comms.empty())
    {
        return -1;
    }

    for (auto comm : comms)
    {
        comm->send_start_timer();
    }

    SD blocking_sd;
    SD loop_sd;
    long late_count = 0;
    auto begin = SteadyClock::now();
    long unack_count = 0;

    ostringstream string_stream_X;
    string_stream_X.write(reinterpret_cast<const char *>(gray_frame.data), gray_frame.total() * gray_frame.elemSize());
    deque<string> images_to_send_4 = {string_stream_X.str(), string_stream_X.str(), string_stream_X.str(), string_stream_X.str(), string_stream_X.str()};
    deque<string> names_to_send_4;

    string sending_info;

    names_to_send_4 = server_params_read;

    // for (int i = 0; i < 5; i++)
    // {
    // // note limit to 128 characters
    // //  sending_info =  "123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456"  ;
    // // names_to_send_4.push_back("Scrn_H 1024  Scrn_V 768  Noise_Gn 60  In_Gn 100  Out_Gn 180  Gma_Gn 100  Cycle_Tme 68 Fade_Tme 10");
    // names_to_send_4.push_back("xxxx");
    // }
    // cout << names_to_send_4[2] << " size "  << names_to_send_4.size() <<  endl ;
    // exit(0);

    // for (long loop_count = 0; loop_count < ; loop_count++)
    while (true)
    {

        // measure the
        ProcessStartTime = std::chrono::steady_clock::now();

        for (int i = 1; i <= numSliders; i++)
        {
            string trackbarName = "Slider" + to_string(i);
            int sliderValue = cv::getTrackbarPos(trackbarName, windowName);
            //  cout << "Value of " << trackbarName << ": " << sliderValue << endl;
        }

        Image_Status = get_camera_frame(cap,
                                        gray_frame,
                                        frame_Abs_Diff,
                                        Client_Params.Cycle_Time,
                                        Client_Params.Motion_Window_H_Position,
                                        Client_Params.Motion_Window_V_Position,
                                        Client_Params.Noise_Threshold,
                                        Client_Params.Motion_Threshold);

        Image_Motion = (Image_Status == 1);
        New_Frame = (Image_Status >= 0);

        // sets the timing of the images presented and stores the image if it moved
        Sequencer(Image_Motion, gray_frame);

        ostringstream string_stream;

        // store all the images ready to send
        if (Image_Status >= 0)
        {

            randomValue = std::rand() % 100;
            if (randomValue < 70)
            {
                //  convert binary mage to string image
                string_stream.write(reinterpret_cast<const char *>(gray_frame.data), gray_frame.total() * gray_frame.elemSize());
            }
            else
            {
                std::string dir_path = "../tif";
                std::string random_file = get_random_file(dir_path);
                cv::Mat image_read = cv::imread(random_file, cv::IMREAD_UNCHANGED);
                string_stream.write(reinterpret_cast<const char *>(image_read.data), image_read.total() * image_read.elemSize());
                cout << endl;
                cout << "Reading: File: " << random_file << endl;
                cout << endl;
            }

            // put the latest into a a deque so the most recent is always 1st
            images_to_send_4.push_front(string_stream.str());
            if (images_to_send_4.size() > 5)
            {
                images_to_send_4.resize(5);
            }

            // for test viewing
            // Mats_5.push_back(gray_frame);
            // if (Mats_5.size() > 5)
            // {
            //     Mats_5.resize(5);
            // }
        }

        // now send the images
        if (New_Frame)
        {
            int ix = 0;
            for (auto &comm : comms)
            {
                string image_data = images_to_send_4[ix]; // = images_to_send_2.front();
                string send_name = names_to_send_4[ix];   // = names_to_send_2.front();
                comm->send_image(send_name, image_data);
                ix++;
            }
        }

        ProcessEndTime = std::chrono::steady_clock::now();

        ProcessTime = ProcessEndTime - ProcessStartTime;

        // if(true)
        if (ProcessTime.count() > .005)
            std::cout << "ProcessTime: " << ProcessTime.count() << std::endl;

        // sets the timing of a frame  1/30th
        loopEndTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = loopEndTime - loopStartTime;
        while (elapsed_seconds.count() < .0333333)
            elapsed_seconds = std::chrono::steady_clock::now() - loopStartTime;
        loopStartTime = std::chrono::steady_clock::now();

        // imshow(windowName, sliders_img);

        // if (Display_Test_Images(gray_frame, frame_Abs_Diff) == -1)
        //     break;

        loop_count++;
    }

    // give the server time to process the last sends before the connection is dropped
    this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}
