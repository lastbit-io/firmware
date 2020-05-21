/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __JOYSTICK_H
#define __JOYSTICK_H

#include "main.h"

typedef enum
{
	LEFT,
	RIGHT,
	UP,
	DOWN,
	CENTER,
	NEUTRAL
} GO_JOYSTICK_t;

GO_JOYSTICK_t getJoyStickStatus();

#endif