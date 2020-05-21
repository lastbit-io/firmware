#include "joystick.h"

GO_JOYSTICK_t getJoyStickStatus()
{
    GO_JOYSTICK_t joystick_status = NEUTRAL;
    if (PC1 == 0)
    {
        joystick_status = CENTER;
    }
    if (PC2 == 0)
    {
        joystick_status = UP;
    }
    if (PC3 == 0)
    {
        joystick_status = DOWN;
    }
    if (PC0 == 0)
    {
        joystick_status = RIGHT;
    }
    if (PC4 == 0)
    {
        joystick_status = LEFT;
    }
    return joystick_status;
}