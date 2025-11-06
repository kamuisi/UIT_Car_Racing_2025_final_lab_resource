#ifndef MOTOR_H_
#define MOTOR_H_
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstdint>
#include <string.h>

class Motor
{
    private:
        int _device;
        const char* _device_port;
    public:
        Motor(const char* device_port = "/dev/ttyTHS1") : _device_port(device_port), _device(-1){}
        ~Motor();
        bool Init();
        void SendInitCommand();
        void SendCommand(const char*);
        void SetMode(uint8_t);
        void SetSpeedRad(float Rad);
        void SetSpeedCms(float Cms);
        void SetSpeedPositionRad(float Position, float Rad);
        void SetSpeedPositionCms(float PositionCm, float Cms);
        
};




#endif