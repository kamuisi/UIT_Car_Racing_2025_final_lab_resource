#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include "Motor.h"

int main()
{
    // Motor _motor; //nano
    Motor _motor("/dev/ttyTHS0"); //xaiver
	_motor.Init();
	bool flag = 0;
	int i = -65;
	while(true)
	{
		_motor.SetSpeedCms(i);
		std::cout << i << std::endl << std::flush;
		i = flag == 1 ? i - 1 : i + 1;
		if(i == 65)
		{
			flag = 1;
		}
		else if (i == -65)
		{
			flag = 0;
		}
		usleep(500000);
	}

	// _motor.SetMode(2);
	// _motor.SetSpeedPositionCms(13, 5);

	return 0;
}



