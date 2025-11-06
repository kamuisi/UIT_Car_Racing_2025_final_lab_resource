#include "Camera.h"

int main() {
   Camera _cam;
   _cam.Init();

    cv::Mat frame;
    while (true) {
        _cam.GetFrame(frame);
        if (frame.empty()) break;

        cv::imshow("View", frame);
        if (cv::waitKey(1) == (int)'q') break;
    }
    cv::destroyAllWindows();
    return 0;
}
