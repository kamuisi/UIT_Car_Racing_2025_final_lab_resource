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

void MainThread(Camera &_cam, Model &_model, Motor &_motor)
{
    cv::Mat frame;
    float new_speed = 0;
    int8_t new_angle = 0;
    bool save = false;
    int i = 0;
    while (run_flag)
    {
        _cam.GetFrame(frame);
        // auto pred = _model.Predict(frame);
        
        // _model.DrawOverlay(frame, pred);
        cv::imshow("View", frame);

        if(save)
        {
            cv::imwrite(std::to_string(i) + ".jpg", frame);
            i++;
        }
        new_angle = new_angle * 0.8;
        new_speed = new_speed * 0.8;

        int key = cv::waitKey(1);
        switch (key)
        {
            case (int)'w': 
                new_speed = 60;
                break;
            case (int)'s':
                new_speed = -60;
                break;
            case (int)'a':
                new_angle = 30;
                break;
            case (int)'d':
                new_angle = -30;
                break;
            case (int)'q':
                std::cout << "Catched break signal" << std::endl << std::flush;
                run_flag = false;
                break;
            case (int)'f':
                save = !save;
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
    Motor _motor;
    SH1106 _oled;
    Camera _cam("nvarguscamerasrc sensor-mode=5 wbmode=0 ! "
                "video/x-raw(memory:NVMM),width=320,height=180,framerate=120/1 ! " 
                "nvvidconv ! video/x-raw,format=BGRx ! " 
                "videoconvert ! video/x-raw,format=BGR ! appsink");
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