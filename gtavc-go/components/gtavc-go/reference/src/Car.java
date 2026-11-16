/** A vehicle. Positions in mm (1/1024 m); y in 1/64 m. */
final class Car {
    static final int PARKED = 0, TRAFFIC = 1, PLAYER = 2, POLICE = 3;
    static final int ADMIRAL = 0, OCEANIC = 1, TAXI = 2, PCJ600 = 3, POLICE_CAR = 4;
    int x, z, y, yaw, speed;          // speed: mm/s along heading
    int type, color, color2 = 0xE8E8E8, mode, blocked;
    int steer;                        // steering angle in 1/1024 turn
    int hp = 1000;
    int[] partDmg = new int[12];      // accumulated damage per CarModel part id
    int damBits, lostBits;            // part uses its damaged mesh / part fell off
    int wheelRot;                     // wheel spin angle
    int aiTimer, turnDir;
    int vx, vz, vy;                   // velocity mm/s (vy: 1/64 m/s)
    int yawVel, pitch, roll, slopePitch;
    boolean slide, burning;
    int burnT, fxT;
    boolean braking, wrecked;
    Ped driver;
    static final int[] COLORS = { 0xC8302C, 0x2C5FC8, 0xF0D840, 0x2CA05A, 0xE8E8E8, 0x2E2E34, 0xFF8FD0, 0x40C0D0, 0xE87A20, 0x8A2BE2 };
}
