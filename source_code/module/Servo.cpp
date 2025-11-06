#include "Servo.h"

Servo::~Servo()
{
    if (_device != -1) close(_device);
}

bool Servo::Init()
{
    _device = open("/dev/i2c-1", O_RDWR);
    if (ioctl(_device, I2C_SLAVE, _address) != 0)
        return false;
    Reset();
    return true;
}

void Servo::Reset()
{
    i2c_smbus_write_byte_data(_device, MODE1, 0x00 | (1 << 7));
    i2c_smbus_write_byte_data(_device, MODE2, 0x00 | (1 << 2));
}

void Servo::SetPWMFreq(uint16_t Freg)
{
    _freq = Freg;
    _max_pwm = (uint16_t)(2500.0 / (1000000.0 / ((float)_freq * 4096.0))); //  2500 / (T_set / 4096)
    _min_pwm = (uint16_t)(500.0 / (1000000.0 / ((float)_freq * 4096.0)));
    uint8_t prescale_value = INTERNAL_CLOCK / (4096 * Freg) - 1;
    i2c_smbus_write_byte_data(_device, MODE1, 0x00 | (1 << 4));
    i2c_smbus_write_byte_data(_device, PRE_SCALER, prescale_value);
    Reset();
}

void Servo::SetAngle(int16_t Angle)
{
    Angle = std::min(MAXANGLE, std::max(Angle, (int16_t)-MAXANGLE)) + 90; // map theo lượng giác
    uint16_t _value = (uint16_t)(_min_pwm + ((Angle * (_max_pwm - _min_pwm)) / 180));
    SetPWM(15, 0, _value);
}

void Servo::SetPWM(uint8_t channel, uint16_t On_value, uint16_t Off_value)
{
    uint8_t res_addr = LED_BASE_RES + 4 * channel;
    i2c_smbus_write_byte_data(_device, res_addr + 0, On_value & 0xFF);
    i2c_smbus_write_byte_data(_device, res_addr + 1, On_value >> 8);
    i2c_smbus_write_byte_data(_device, res_addr + 2, Off_value & 0xFF);
    i2c_smbus_write_byte_data(_device, res_addr + 3, Off_value >> 8);
}