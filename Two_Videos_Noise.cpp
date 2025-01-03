#include <iostream>
#include <thread>
#include <iomanip>
#include <sstream>
#include <ctime>
// #include <condition_variable>
// #include <mutex>
#include <cstring>
// #include <deque>
// #include <fstream>
// #include <unordered_map>
// #include <csignal>
#include <limits>
#include <string>

#include <opencv2/opencv.hpp>

#include "OpenCVGtkWindowController.hpp"

#include "mixer_processor.h"

using namespace std;

typedef std::chrono::steady_clock SteadyClock;
typedef std::chrono::duration<double> Seconds;

#define APPLY_LOW_PASS_FILTER true // low pass filter the noise Set to false to disable low-pass filtering

#define NUM_OF_NOISE_FRAMES 30

#define FULLSCREEN_MODE false // Set to false for windowed mode

#define SHOW_TIMING true // Show timing on screen

// #define NOISE_WEIGHT .6 // 0.5 // Adjust this value as needed (e.g., 0.33 for 1/3)

// #define OUTPUT_GAIN 1.8 // Adjust this value for output gain of the final image

// #define FADE_TIMER_TC 64 // Adjust this value for lenngth of fade

// #define FADE_TIME 38 // Adjust this value for lenngth of fade  nominal 38 frames

struct Server_Parameters_Main
{
    int Screen_H_Size;      //  in pixels
    int Screen_V_Size;      //  in pixels
    int Noise_Gain;         // in percent
    int Input_Gain;         // in percent
    int Output_Gain;        // in percent
    int Gamma_Gain;         // in percent
    int Cycle_Time;         // cycle time determined by the images coming in so not really used
    int Fade_Time;          // in frames 1/30 of a second
    int Full_Screen_Enable; // in frames 1/30 of a second

    // Constructor to initialize default values
    Server_Parameters_Main() : Screen_H_Size(1024), Screen_V_Size(768),
                               Noise_Gain(60), Input_Gain(75), Output_Gain(180), Gamma_Gain(100), Cycle_Time(64), Fade_Time(38), Full_Screen_Enable(0) {}
};

int main(int argc, char *argv[])
{

    Server_Parameters_Main Server_Params;
    std::string control_params_in;

    int Fade_Timer = 0;
    // int Fade_Timer_TC = 64; //  at  30 fps  64/30 seconds
    // int Fade_Time = 38;
    bool New_Image = false;
    float Fade_Val = 0;

    float avg_sum = 0;
    int average_cnter = 0;

    // create a gradient for test image using a pointer
    int width = 1024;
    int height = 768;
    int size = width * height;
    uchar *dataX = new uchar[size];
    for (int i = 0; i < size; ++i)
    {
        dataX[i] = i / 3072; // Example: gradient effect
    }

    // use memcopy to convert Jonathan's container to an opencv Mat   // had ame offset reults
    cv::Mat image1(height, width, CV_8UC1); // Create an empty cv::Mat with the desired dimensions
    cv::Mat image2(height, width, CV_8UC1); // Create an empty cv::Mat with the desired dimensions
    cv::Mat image3(height, width, CV_8UC1); // Create an empty cv::Mat with the desired dimensions

    cv::Mat transformedImgLeft(height, width, CV_8UC1);  // Create an empty cv::Mat with the desired dimensions
    cv::Mat transformedImgRight(height, width, CV_8UC1); // Create an empty cv::Mat with the desired dimensions

    // for timing various things
    auto start_check = std::chrono::high_resolution_clock::now();
    auto end_check = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_check - start_check;

    auto start_check_2 = std::chrono::high_resolution_clock::now();
    auto end_check_2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_2 = end_check_2 - start_check_2;

    double fps = 30;

    auto begin = SteadyClock::now();

    long max_loop = std::numeric_limits<long>::max();

    // generate noise
    std::vector<cv::Mat> noiseFrames = generateNoiseFrames(image1.cols, image1.rows, NUM_OF_NOISE_FRAMES, APPLY_LOW_PASS_FILTER);
    //  Create the parabolic lookup table for gamma correction
    cv::Mat lut = createParabolicLUT();

    cv::namedWindow("Grayscale Image Left", cv::WINDOW_NORMAL);
    cv::namedWindow("Grayscale Image Right", cv::WINDOW_NORMAL);

    // low-level window controls to allow for cursor hiding and monitor assignment
    OpenCVGtkWindowController L_win_ctrls { "Grayscale Image Left" };
    OpenCVGtkWindowController R_win_ctrls { "Grayscale Image Right" };

    // begin in fullscreen with hidden cursor, one window per monitor
    // (assumes monitor 1 (index 0) is primary and monitor 2 is to right)
    L_win_ctrls.hide_cursor();
    L_win_ctrls.enable_auto_hiding_cursor();
    L_win_ctrls.fullscreen_on_monitor(0);
    R_win_ctrls.hide_cursor();
    R_win_ctrls.enable_auto_hiding_cursor();
    R_win_ctrls.fullscreen_on_monitor(1);

    cv::Mat image_Read_1 = cv::imread("004.tif", cv::IMREAD_UNCHANGED);
    cv::Mat image_Read_2 = cv::imread("169.tif", cv::IMREAD_UNCHANGED);
    cv::Mat image_Read_3 = cv::imread("043.tif", cv::IMREAD_UNCHANGED);

    image1 = image_Read_1.clone();
    image2 = image_Read_2.clone();
    image3 = image_Read_3.clone();

    Server_Params.Screen_H_Size = 1024;
    Server_Params.Screen_V_Size = 768;
    Server_Params.Noise_Gain = 50;
    Server_Params.Input_Gain = 80;
    Server_Params.Output_Gain = 90;
    Server_Params.Gamma_Gain = 70;
    Server_Params.Fade_Time = 45;
    Server_Params.Full_Screen_Enable = 0;
    Server_Params.Cycle_Time = 90;

    int tog = 0;

    for (long loop_count = 0; loop_count < max_loop; loop_count++)
    {
        if (loop_count % 90 == 89)
        {
            image3 = image2.clone();
            image2 = image1.clone();
            if (tog == 0)
                image1 = image_Read_1.clone();
            else if (tog == 1)
                image1 = image_Read_2.clone();
            else
                image1 = image_Read_3.clone();

            Fade_Timer = 0;

            Fade_Val = 0;
            tog++;
            if (tog >= 3)
                tog = 0;
        }
        else if (Fade_Timer < Server_Params.Cycle_Time)
        {
            Fade_Timer++;
            Fade_Val = (float)(Fade_Timer <= Server_Params.Fade_Time ? Fade_Timer : Server_Params.Fade_Time) / (float)Server_Params.Fade_Time;
        }



        blendImagesAndNoise(image1, image2, noiseFrames, transformedImgLeft, lut, Fade_Val,
                            (float)Server_Params.Input_Gain / 100,
                            (float)Server_Params.Noise_Gain / 100,
                            (float)Server_Params.Gamma_Gain / 100,
                            (float)Server_Params.Output_Gain / 100);

        blendImagesAndNoise(image2, image3, noiseFrames, transformedImgRight, lut, Fade_Val,
                            (float)Server_Params.Input_Gain / 100,
                            (float)Server_Params.Noise_Gain / 100,
                            (float)Server_Params.Gamma_Gain / 100,
                            (float)Server_Params.Output_Gain / 100);

        end_check_2 = std::chrono::high_resolution_clock::now();
        elapsed_2 = end_check_2 - start_check_2;
        average_cnter++;
        if (average_cnter >= 30)
        {
            // std::cout << "elapsed_2: " << elapsed_2.count() << std::endl;
            std::cout << "elapsed_2: " << avg_sum / 30 << std::endl;
            avg_sum = elapsed_2.count();
            average_cnter = 0;
        }
        else
            avg_sum += elapsed_2.count();

        // Loop Timer to set frame rate
        double goal = (loop_count + 1) / fps;
        Seconds elapsed = SteadyClock::now() - begin;
        if (elapsed.count() < goal)
        {
            this_thread::sleep_for(std::chrono::duration<double>(goal - elapsed.count()));
        }

        start_check_2 = std::chrono::high_resolution_clock::now();

        cv::moveWindow("Grayscale Image Left", 100, 100);
        cv::moveWindow("Grayscale Image Right", 1124, 100);
        // Display the image
        cv::imshow("Grayscale Image Left", transformedImgLeft);
        cv::imshow("Grayscale Image Right", transformedImgRight);

        // needed for opencv loop
        int key = cv::waitKey(1);
        if (key == 27)
        { // ASCII code for the escape key
            break;
        }
        else if (key == 'f')
        {
            L_win_ctrls.toggle_fullscreen();
            R_win_ctrls.toggle_fullscreen();
        }

        L_win_ctrls.decrement_frames_until_hiding_cursor();
        R_win_ctrls.decrement_frames_until_hiding_cursor();

        // check for long frame times
        end_check = std::chrono::high_resolution_clock::now();
        elapsed = end_check - start_check;
        start_check = std::chrono::high_resolution_clock::now();
        if (elapsed.count() > .04)
            std::cout << "XXXXXXXXXXXXXXXXXX  " << elapsed.count() << std::endl;
    }

    return 0;
}
