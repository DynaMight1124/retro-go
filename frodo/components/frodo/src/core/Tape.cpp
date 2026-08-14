#include "Tape.h"

Tape::Tape(MOS6526 *cia) : the_cia(cia), motor_on(false)
{
}

void Tape::Reset()
{
    motor_on = false;
}
