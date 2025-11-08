#include "Motor.h"

Motor::~Motor()
{
    if (_device != -1)
        close(_device);
}

bool Motor::Init()
{
    _device = open(_device_port, O_RDWR | O_NOCTTY);
    termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(_device, &tty) != 0)
    { // load cau hinh hien tai
        std::cerr << "Error getting termios attrs\n";
        return false;
    }
    cfmakeraw(&tty);            // set raw mode aka khong cho chinh sua lung tung byte gui
    cfsetospeed(&tty, B115200); // set baud rate
    cfsetispeed(&tty, B115200);
    tcsetattr(_device, TCSANOW, &tty); // nap cau hinh moi
    SendInitCommand();
    return true;
}

static void SaveAndResetDriver()
{
    SendCommand("N1 $101=1 \n");
    usleep(200000); // cho reset
    SendCommand("N1 O K0 \n");
    SendCommand("N1 O U C r \n");
}

void Motor::SendInitCommand()
{
    SendCommand("N1 O D1 \n");
    SendCommand("N1 $008=0 \n");
    SendCommand("N1 $002=330 \n");
    SendCommand("N1 O U C r \n");
    SetMode(1);
    SaveAndResetDriver();
}

void Motor::SendCommand(const char *cmd)
{
    // std::cout << cmd << std::flush;
    write(_device, cmd, strlen(cmd));
}

void Motor::SetMode(uint8_t mode)
{
    if (mode != 1 && mode != 2)
        return;
    std::string cmd = "N1 O M" + std::to_string(mode + 2) + " \n";
    SendCommand(cmd.c_str());
}

void Motor::AutoTunePid()
{
    SendCommand("N1 O T \n");
    usleep(5000000);
    SetMode(1);
    SaveAndResetDriver();
}

void Motor::SetSpeedRad(float Rad)
{
    std::string cmd = "N1 v" + std::to_string(Rad) + " \n";
    SendCommand(cmd.c_str());
}

void Motor::SetSpeedCms(float Cms)
{
    std::string cmd = "N1 v" + std::to_string((Cms * 2.0) / 6.5) + " \n"; // Cm/s / chu vi = số vòng / s
    SendCommand(cmd.c_str());                                             // (số vòng / s) * 2pi = rad / s
}                                                                         // Tổng quát (Cm/s * 2pi) / (D * pi) = (Cm/s * 2) / D

void Motor::SetSpeedPositionRad(float Position, float Rad)
{
    std::string cmd = "N1 p" + std::to_string(Position) + " v" + std::to_string(Rad) + " \n";
    SendCommand(cmd.c_str());
}

void Motor::SetSpeedPositionCms(float PositionCm, float Cms)
{
    std::string cmd = "N1 p" + std::to_string((PositionCm * 2.0) / 6.5) + " v" + std::to_string((Cms * 2.0) / 6.5) + " \n";
    SendCommand(cmd.c_str());
}