#include <iostream>
#include <thread>
#include <mutex>
#include <cstdint>
#include <chrono>
#include <atomic>
#include "Camera.h"
#include "Servo.h"
#include "SH1106.h"
#include "Motor.h"
#include "Model.h"
#include <optional>
#include <utility>
#include <algorithm>

#define BODY_LENGTH 19 // 10 cm
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::mutex mtx_speed;
std::mutex mtx_angle;
std::atomic<bool> run_flag(true);
float speed = 0;
int8_t angle = 0;

void ServoThread(Servo &_servo)
{
    int8_t new_angle;
    while (run_flag)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_angle);
            new_angle = angle;
        }
        _servo.SetAngle(new_angle);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    _servo.SetAngle(0);
}

void MotorThread(Motor &_motor)
{
    float new_speed;
    while (run_flag)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_speed);
            new_speed = speed;
        }
        _motor.SetSpeedCms(new_speed);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    _motor.SetSpeedCms(0);
}

void OLEDThread(SH1106 &_oled)
{
    int8_t new_angle;
    float new_speed;
    while (run_flag)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_speed);
            new_speed = speed;
        }
        {
            std::lock_guard<std::mutex> lock(mtx_angle);
            new_angle = angle;
        }
        _oled.clear();
        _oled.drawString(1, 1, std::to_string(new_speed) + "Cm/s " + std::to_string(new_angle) + "°");
        _oled.display();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

std::optional<std::pair<int, int>> find_min_max_y(const cv::Mat &mask, int &class_id)
{
    int height = mask.rows;
    int width = mask.cols;

    int max_y = -1;
    int min_y = height - 1;

    for (uint8_t y = 0; y < height; y++)
    {
        const uint8_t *row = mask.ptr<uint8_t>(y);
        for (uint16_t x = 0; x < width; x++)
        {
            if (row[x] == class_id)
            {
                if (y > max_y)
                    max_y = y;
                if (y < min_y)
                    min_y = y;
                break;
            }
        }
    }

    if (max_y == -1)
        return std::nullopt;

    return std::make_pair(min_y + 5, max_y - 11);
}

std::optional<int> find_middle_x(const cv::Mat &mask, int class_id, int y)
{
    int width = mask.cols;

    const uint8_t *row = mask.ptr<uint8_t>(y);

    int sum_x = 0;
    int count = 0;

    for (int x = 0; x < width; x++)
    {
        if (row[x] == class_id)
        {
            sum_x += x;
            count++;
        }
    }

    if (count == 0)
        return std::nullopt;

    return (sum_x / count);
}

std::optional<std::pair<int, int>> find_target_point(const cv::Mat &mask, int class_id, float &speed)
{
    const float k_dd = 2.0f;
    auto min_max_y = find_min_max_y(mask, class_id);

    if (!min_max_y.has_value())
        return std::nullopt;

    int height = mask.rows - 1;

    int max_y = height - min_max_y.value().first;
    int min_y = height - min_max_y.value().second;

    // std::cout << "Min Y: " << min_y << " Max Y: " << max_y << std::endl << std::flush;

    float lookahead_dist = k_dd * speed;
    float clamped_dist = std::clamp(lookahead_dist, (float)min_y, (float)max_y);
    int ld = height - static_cast<int>(clamped_dist);

    auto x_median = find_middle_x(mask, class_id, ld);
    int middle_width = mask.cols / 2;

    if (!x_median.has_value())
        return std::nullopt;

    int x_center = x_median.value() - middle_width;
    ld = height - ld;

    return std::make_optional(std::make_pair(x_center, ld));
}

int GetTrafficSign(const cv::Mat &mask)
{
    int height = mask.rows * 0.5;
    int width = mask.cols * 0.5;

    cv::Rect roiRect(mask.cols - width, 0, width, height);

    cv::Mat roiMat = mask(roiRect);

    cv::Mat binaryMask = (roiMat != 0) & (roiMat != 9);

    cv::Mat stats, centroids, labels;
    int numLabels = cv::connectedComponentsWithStats(binaryMask, labels, stats, centroids);

    if (numLabels <= 1)
        return -1;

    int maxArea = 0;
    int maxLabelIdx = -1;

    for (int i = 1; i < numLabels; i++)
    {
        int area = stats.at<int>(i, cv::CC_STAT_AREA);
        if (area > maxArea && area > 200)
        {
            maxArea = area;
            maxLabelIdx = i;
        }
    }

    if (maxLabelIdx == -1)
        return -1;

    int cX = centroids.at<double>(maxLabelIdx, 0);
    int cY = centroids.at<double>(maxLabelIdx, 1);

    int classId = (int)roiMat.at<uint8_t>(cY, cX);

    return classId;
}

void MainThread(Camera &_cam, Model &_model, Motor &_motor)
{
    cv::Mat frame;
    float new_speed = 0;
    int8_t new_angle = 0;
    int8_t action_angle = 0;
    // bool save = false;
    // int i = 21000;
    int sign_cout = 0;
    bool park = false;
    bool first_loop = false;
    int pre_traffic_sign = -1;
    bool sign_active = false;
    auto start = std::chrono::steady_clock::time_point{};
    bool self_drive = true;
    int x_tp = 0;
    int y_tp = 0;
    while (run_flag)
    {
        _cam.GetFrame(frame);
        if (park)
        {
            new_speed = 35;
            new_angle = 50;
            auto eslapsed = std::chrono::steady_clock::now() - start;
            if(eslapsed < std::chrono::duration<double>(1))
            {
                new_angle = 0;
            }
            else if(eslapsed >= std::chrono::duration<double>(2.3) && eslapsed < std::chrono::duration<double>(3.5))
            {
                new_angle = -25;
            }
            else if (eslapsed >= std::chrono::duration<double>(3.5))
            {
                std::cout << "Parking completed" << std::endl
                          << std::flush;
                new_speed = 0;
                new_angle = 0;
                park = false;
            }
        }
        else
        {
            auto pred = _model.Predict(frame);
            cv::Mat rawMask(pred.size(0), pred.size(1), CV_8UC1, pred.data_ptr<uint8_t>());
            int traffic_sign = GetTrafficSign(rawMask);
            // std::cout << "Traffic Sign: " << traffic_sign << std::endl << std::flush;

            if (traffic_sign == -2)
            {
                {
                    std::lock_guard<std::mutex> lock(mtx_speed);
                    speed = 0;
                }
                {
                    std::lock_guard<std::mutex> lock(mtx_angle);
                    angle = 0;
                }
                first_loop = true;
                std::cout << "Stop Sign Detected - Stopping for 4 seconds" << std::endl
                          << std::flush;
                std::this_thread::sleep_for(std::chrono::seconds(4));
            }
            else
            {

                if (first_loop)
                {
                    std::cout << "Run" << std::endl
                              << std::flush;
                    first_loop = false;
                    new_speed = 35;
                }

                // if (traffic_sign != -1)
                // {
                //     pre_traffic_sign = traffic_sign;
                //     if(traffic_sign == pre_traffic_sign)
                //     {
                //         sign_cout++;
                //         std::cout << "Sign count: " << sign_cout << std::endl << std::flush;
                //     }
                //     else
                //     {
                //         sign_cout = 0;
                //     }
                //     sign_active = false;
                //     start = {};
                // }
                // else if (pre_traffic_sign != -1 && !sign_active && sign_cout >= 5)
                // {
                //     sign_cout = 0;
                //     start = std::chrono::steady_clock::now();
                //     sign_active = true;
                //     switch (pre_traffic_sign)
                //     {
                //     case 3:
                //     case 4:
                //     case 5:
                //     case 6:
                //     case 8:
                //         std::cout << "Turn Left Sign Detected" << std::endl
                //                   << std::flush;
                //         action_angle = 50;
                //         break;
                //         // std::cout << "Turn Right Sign Detected" << std::endl
                //         //           << std::flush;
                //         // action_angle = -50;
                //         // break;
                //     case 7:
                //         start = std::chrono::steady_clock::now();
                //         park = true;
                //         break;
                //     default:
                //         pre_traffic_sign = -1;
                //         sign_active = false;
                //         start = {};
                //         break;
                //     }
                // }

                if (sign_active)
                {
                    auto eslapsed = std::chrono::steady_clock::now() - start;

                    new_angle = action_angle * 0.5;

                    if (eslapsed >= std::chrono::duration<double>(0.5))
                    {
                        std::cout << "Push push push" << std::endl
                                  << std::flush;
                        new_angle = action_angle;
                    }

                    if (eslapsed >= std::chrono::duration<double>(2))
                    {
                        std::cout << "Turn action finished" << std::endl
                                  << std::flush;
                        pre_traffic_sign = -1;
                        sign_active = false;
                        start = {};
                    }
                }

                else
                {

                    switch (traffic_sign)
                    {
                    // case 10:
                    //     new_speed = 0;
                    //     new_angle = 0;
                    //     break;
                    // case 2:
                    //     // new_speed = 35;
                    default:
                        auto target_point = find_target_point(rawMask, 9, new_speed);
                        if (target_point.has_value())
                        {
                            x_tp = target_point.value().first;
                            y_tp = target_point.value().second;

                            // std::cout << "Target Point: (" << x_tp << ", " << y_tp << ")" << std::endl << std::flush;

                            float alpha = std::atan2(x_tp, y_tp);

                            float raw_angle = std::atan2(2 * BODY_LENGTH * std::sin(alpha), y_tp) * 180.0f / M_PI;
                            int calculated_angle = -static_cast<int8_t>(std::clamp(raw_angle, -60.0f, 60.0f));
                            if (self_drive)
                            {
                                new_angle = calculated_angle;
                            }
                            // std::cout << "Steering Angle: " << static_cast<int>(calculated_angle)
                            //           << "°" << " Raw Angle: " << raw_angle << std::endl << std::flush;
                            x_tp += pred.size(1) / 2;
                            y_tp = pred.size(0) - y_tp;
                        }
                        break;
                    }
                }
            }
            if (!park)
            {
                _model.DrawOverlay(frame, rawMask, x_tp, y_tp);
            }
        }
        cv::imshow("View", frame);

        // if(save)
        // {
        //     cv::imwrite("../train/" + std::to_string(i) + ".jpg", frame);
        //     i++;
        // }

        if (!self_drive)
        {
            new_angle = new_angle * 0.99;
        }

        int key = cv::waitKey(1);
        switch (key)
        {
        case (int)'w':
            new_speed = 35;
            break;
        case (int)'s':
            new_speed = -30;
            break;
        case (int)'a':
            new_angle = 60;
            break;
        case (int)'d':
            new_angle = -60;
            break;
        case (int)'q':
            std::cout << "Catched break signal" << std::endl
                    << std::flush;
            run_flag = false;
            break;
        // case (int)'f':
        //     save = !save;
        //     std::cout << save << std::endl << std::flush;
        //     break;
        case (int)'r':
            self_drive = !self_drive;
            std::cout << "Self Drive: " << self_drive << std::endl
                    << std::flush;
            break;
        case (int)'t':
            _motor.AutoTunePid();
            break;
        case (int)'c':
            new_angle = 0;
            new_speed = 0;
            break;
        default:
            break;
        }

        {
            std::lock_guard<std::mutex> lock(mtx_speed);
            speed = new_speed;
        }
        {
            std::lock_guard<std::mutex> lock(mtx_angle);
            angle = new_angle;
        }
    }
}

int main()
{
    Servo _servo;
    Motor _motor; // nano
    // Motor _motor("/dev/ttyTHS1"); //xaiver
    SH1106 _oled;
    // Camera _cam("nvarguscamerasrc sensor-mode=5 wbmode=0 gainrange=\"1 8\" saturation=0.9 ! "
    //             "video/x-raw(memory:NVMM),width=320,height=180,framerate=120/1 ! "
    //             "nvvidconv ! video/x-raw,format=BGRx ! "
    //             "videoconvert ! video/x-raw,format=BGR ! appsink max-buffers=1 drop=True"); //nano
    Camera _cam("nvarguscamerasrc sensor-mode=5 wbmode=0 ! "
                "video/x-raw(memory:NVMM),width=320,height=180,framerate=120/1 ! "
                "nvvidconv ! video/x-raw,format=BGRx ! "
                "videoconvert ! video/x-raw,format=BGR ! appsink max-buffers=1 drop=True"); // nano
    // Camera _cam("nvarguscamerasrc sensor-mode=4 wbmode=0 ! "
    //         "video/x-raw(memory:NVMM),width=320,height=180,framerate=60/1 ! "
    //         "nvvidconv ! video/x-raw,format=BGRx ! "
    //         "videoconvert ! video/x-raw,format=BGR ! appsink max-buffers=1 drop=True"); //xaiver
    Model _model("../model/fast_scnn_model.ts");

    _servo.Init();
    _motor.Init();
    _oled.Init();
    _cam.Init();
    // _model.LoadModel();

    _servo.SetPWMFreq(50);

    std::thread servo_thread(ServoThread, std::ref(_servo));
    std::thread motor_thread(MotorThread, std::ref(_motor));
    std::thread oled_thread(OLEDThread, std::ref(_oled));
    std::thread main_thread(MainThread, std::ref(_cam), std::ref(_model), std::ref(_motor));

    main_thread.join();
    servo_thread.join();
    motor_thread.join();
    oled_thread.join();

    return 0;
}