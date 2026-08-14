#pragma once

class MOS6526;

class Tape {
public:
    explicit Tape(MOS6526 *cia);

    void Reset();
    void SetMotor(bool on) { motor_on = on; }
    void EmulateCycle() {}
    // TAP recording is not connected in the embedded port, but the
    // cycle-exact CPU still drives the physical write line.
    void WritePulse(unsigned) {}
    bool MotorOn() const { return motor_on; }
    void SetState(bool motor) { motor_on = motor; }

private:
    MOS6526 *the_cia;
    bool motor_on;
};
