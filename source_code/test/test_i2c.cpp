#include "SH1106.h"
#include "Servo.h"

int main()
{
    Servo _servo;
    _servo.Init();
    _servo.SetPWMFreq(50);
    int i = 60;
    bool flag = 0;
    SH1106 _oled;
    _oled.Init();
    while (true)
    {
        _servo.SetAngle(i);
        usleep(500000);
        _oled.clear();
        _oled.drawString(1, 1, std::to_string(i) + "°");
        _oled.display();
        i = flag ? i + 1 : i - 1;
        if(i == 60) flag = 0;
        else if (i == -60) flag = 1;
    }
    return 0;
}