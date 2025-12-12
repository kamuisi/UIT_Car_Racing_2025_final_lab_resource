#ifndef SERVO_H_
#define SERVO_H_
#include <linux/i2c-dev.h>
#include <cstdint>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
// #include <cmath>

extern "C" 
{
    #include <i2c/smbus.h>
}

#define MODE1 0x00
#define MODE2 0x01
#define INTERNAL_CLOCK 25 * 1000 * 1000
#define LED_BASE_RES 0x06
#define PRE_SCALER 0xFE
#define MAXANGLE (int16_t)60

class Servo 
{
    private:
        uint8_t _address;
        int _device;
        uint16_t _freq;
        uint16_t _max_pwm, _min_pwm;
    public:
        Servo(uint8_t address = 0x40) : _address(address), _device(-1){}
        ~Servo();
        bool Init();
        void SetPWMFreq(uint16_t Freq);
        void Reset();
        void SetAngle(int16_t Angle);
        void SetPWM(uint8_t channel, uint16_t On_value, uint16_t Off_value);

};


#endif