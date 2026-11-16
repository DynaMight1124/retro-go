package javax.microedition.lcdui;
public abstract class Canvas extends Displayable {
    public static final int UP = 1, LEFT = 2, RIGHT = 5, DOWN = 6, FIRE = 8, GAME_A = 9, GAME_B = 10, GAME_C = 11, GAME_D = 12;
    public static final int KEY_NUM0 = 48, KEY_NUM1 = 49, KEY_NUM2 = 50, KEY_NUM3 = 51, KEY_NUM4 = 52, KEY_NUM5 = 53,
            KEY_NUM6 = 54, KEY_NUM7 = 55, KEY_NUM8 = 56, KEY_NUM9 = 57, KEY_STAR = 42, KEY_POUND = 35;
    protected Canvas() {}
    protected abstract void paint(Graphics g);
    protected void keyPressed(int keyCode) {}
    protected void keyReleased(int keyCode) {}
    protected void keyRepeated(int keyCode) {}
    protected void showNotify() {}
    protected void hideNotify() {}
    protected void sizeChanged(int w, int h) {}
    public void setFullScreenMode(boolean mode) {}
    public void repaint() {}
    public void serviceRepaints() {}
    public int getGameAction(int keyCode) {
        switch (keyCode) {
            case -1: case 119: case 50: return UP;     // like Nokia: 2/8/4/6/5 also map to game actions
            case -2: case 115: case 56: return DOWN;
            case -3: case 97: case 52: return LEFT;
            case -4: case 100: case 54: return RIGHT;
            case -5: case 10: case 53: return FIRE;
        }
        return 0;
    }
    public int getKeyCode(int gameAction) { return 0; }
    public boolean hasPointerEvents() { return false; }
    public boolean isDoubleBuffered() { return true; }
    public void shimPaint(Graphics g) { paint(g); }
    public void shimKey(int code, boolean down) { if (down) keyPressed(code); else keyReleased(code); }
}
