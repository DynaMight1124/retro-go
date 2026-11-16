/** A pedestrian (also used for the player and for car drivers). Position in mm, y in 1/64 m. */
final class Ped {
    static final int WALK = 0, IDLE = 1, FLEE = 2, DEAD = 3, HIT = 4, JACKED = 5, GETUP = 6;
    int x, z, y, yaw;
    int state, hp = 100, tex, timer, cash;
    int speed;                 // mm/s (signed)
    int turnTimer, targetYaw;
    boolean cop, female;
    int kind;                  // NPC model: 0 cop, 1 construction, 2 hispanic, 3 skater, 4 street
    // animation
    int clip = -1, clipT, clipNext = -1, clipEnd;   // one-shot clip (clipNext: -1 none, -2 hold last frame)
    int loco, locoT;                                // looping locomotion clip
    boolean hitDone;
    Car seat;                                       // seated in this car
    int attackCd;
    int deadT;                                      // ms since death (blood pool grows)
    int bloodX, bloodZ, bloodY;                     // blood pool position
    int closeT;
    boolean angry, brave, gang;                     // fights back
    int aimT;
    boolean air;
    int vy;                                       // cop aiming/shooting                                     // cop: time spent next to the player (arrest)
}
