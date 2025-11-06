#include "Camera.h"

Camera::~Camera()
{
    if(_cap.isOpened()) _cap.release();
}

bool Camera::Init()
{
    _cap.open(_pipeline, cv::CAP_GSTREAMER);
    if(!_cap.isOpened()) return false;
    return true;
}

void Camera::GetFrame(cv::Mat &frame)
{
    _cap >> frame;
}