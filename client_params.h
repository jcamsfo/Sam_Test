
#ifndef CLIENT_PARAMS_H
#define CLIENT_PARAMS_H




struct Client_Parameters_Main
{
    int Cam_H_Size;
    int Cam_V_Size;
    int Screen_H_Size;
    int Screen_V_Size;

    int Motion_Window_H_Size_Multiplier;
    int Motion_Window_V_Size_Multiplier;

    float Cycle_Time;

    int Noise_Threshold;
    int Motion_Threshold;

    // Constructor to initialize default values
    Client_Parameters_Main() : Cam_H_Size(800), Cam_V_Size(600),
                               Screen_H_Size(1024), Screen_V_Size(768),
                               Motion_Window_H_Size_Multiplier(75), Motion_Window_V_Size_Multiplier(75), Cycle_Time(1.2), Noise_Threshold(5), Motion_Threshold(5000) {}

    int Motion_Window_H_Size = (Screen_H_Size * Motion_Window_H_Size_Multiplier) / 100;
    int Motion_Window_V_Size = (Screen_V_Size * Motion_Window_V_Size_Multiplier) / 100;

    int Motion_Window_H_Position = (Screen_H_Size - Motion_Window_H_Size) / 2;
    int Motion_Window_V_Position = (Screen_V_Size - Motion_Window_V_Size) / 2;
};


void readParametersFromFile(const std::string &filename, Client_Parameters_Main &params);



struct Pi_Parameters_Main
{
    std::string p0;
    std::string i0;  

    std::string p1;
    std::string i1;    

    std::string p2;
    std::string i2;    

    std::string p3;
    std::string i3;    

    std::string p4;
    std::string i4;    

    // Constructor to initialize default values
    Pi_Parameters_Main() : p0("5569"), i0("192.168.42.17"),    p1("5570"), i1("192.168.42.32"),   p2("5571"), i2("192.168.42.37"),   p3("5585"), i3("192.168.42.43"),   p4("5573"), i4("192.168.42.45") {}
};


void readPiParametersFromFile(const std::string &filename, Pi_Parameters_Main &params);






void Sequencer(const bool Image_Motion, const cv::Mat &gray_frame_local);



#endif // CLIENT_PARAMS_H