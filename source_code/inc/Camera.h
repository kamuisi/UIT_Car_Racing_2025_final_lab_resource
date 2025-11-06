#ifndef CAMERA_H_
#define CAMERA_H_
#include <opencv2/opencv.hpp>
#include <iostream>

class Camera 
{
    private:
        std::string _pipeline;
        cv::VideoCapture _cap;
    public:
        Camera(std::string pipeline = "nvarguscamerasrc sensor-mode=5 wbmode=0 ! "
        "video/x-raw(memory:NVMM),width=1080,height=720,framerate=120/1 ! " 
        "nvvidconv ! video/x-raw,format=BGRx ! " 
        "videoconvert ! video/x-raw,format=BGR ! appsink") : _pipeline(pipeline){}
        ~Camera();
        bool Init();
        void GetFrame(cv::Mat &frame);
};


#endif