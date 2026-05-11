// Target definition
#define RG_TARGET_NAME             "CYD"

// Storage
#define RG_STORAGE_ROOT             "/sd"
#define RG_STORAGE_SDSPI_HOST       SPI3_HOST
#define RG_STORAGE_SDSPI_SPEED      SDMMC_FREQ_DEFAULT
// #define RG_STORAGE_SDMMC_HOST       SDMMC_HOST_SLOT_1
// #define RG_STORAGE_SDMMC_SPEED      SDMMC_FREQ_DEFAULT
// #define RG_STORAGE_FLASH_PARTITION  "vfs"

// GPIO Extender
#define RG_I2C_GPIO_DRIVER          4   // 1 = AW9523, 2 = PCF9539, 3 = MCP23017, 4 = PCF8575
#define RG_I2C_GPIO_ADDR            0x20

// Audio
#define RG_AUDIO_USE_INT_DAC        2   // 0 = Disable, 1 = GPIO25, 2 = GPIO26, 3 = Both
#define RG_AUDIO_USE_EXT_DAC        0   // 0 = Disable, 1 = Enable

// Video
#define RG_SCREEN_DRIVER            0   // 0 = ILI9341
#define RG_SCREEN_HOST              SPI2_HOST
#define RG_SCREEN_SPEED             SPI_MASTER_FREQ_40M
#define RG_SCREEN_BACKLIGHT         1
#define RG_SCREEN_WIDTH             320
#define RG_SCREEN_HEIGHT            240
#define RG_SCREEN_ROTATION          0
#define RG_SCREEN_RGB_BGR           1
#define RG_SCREEN_VISIBLE_AREA      {0, 0, 0, 0}
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0}
#define RG_SCREEN_INIT()                                                                                         \
    ILI9341_CMD(0x21);                       /* Invert colors */                                                 \
    ILI9341_CMD(0xC0, 0x1B);                 /* Power control   //VRH[5:0] */                                    \
    ILI9341_CMD(0xC1, 0x12);                 /* Power control   //SAP[2:0];BT[3:0] */                            \
    ILI9341_CMD(0xC5, 0x32, 0x3C);           /* VCM control */                                                   \
    ILI9341_CMD(0xC7, 0x91);                 /* VCM control2 */                                                  \
    ILI9341_CMD(0xB2, 0x0C, 0x0C, 0x00, 0x33, 0x33);  /* Porch Setting (0x0C, 0x0C=Std or 0x0F, 0x0F=Slow */     \
    ILI9341_CMD(0xC6, 0x03);          /* ST7789 Frame Rate Control (0F=60, 07 to 00=75 to 119, 6Hz steps) */     \
    ILI9341_CMD(0xB6, 0x0A, 0x82);           /* Gate Scan Direction (82=Std, A2=Inv, 22=Alt) */                  \
    ILI9341_CMD(0xF6, 0x01, 0x00);           /* Interface Control (01=Std, 21=Interleave */                      \
    ILI9341_CMD(0xE0, 0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19);       \
    ILI9341_CMD(0xE1, 0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19);       \


#define RG_GAMEPAD_I2C_MAP {\
    {RG_KEY_UP,     .num = 2, .pullup = 0, .level = 0},\
    {RG_KEY_RIGHT,  .num = 1, .pullup = 0, .level = 0},\
    {RG_KEY_DOWN,   .num = 3, .pullup = 0, .level = 0},\
    {RG_KEY_LEFT,   .num = 0, .pullup = 0, .level = 0},\
    {RG_KEY_SELECT, .num = 6, .pullup = 0, .level = 0},\
    {RG_KEY_A,      .num = 4, .pullup = 0, .level = 0},\
    {RG_KEY_B,      .num = 5, .pullup = 0, .level = 0},\
    {RG_KEY_START,  .num = 7, .pullup = 0, .level = 0},\
    {RG_KEY_MENU,   .num = 8, .pullup = 0, .level = 0},\
    {RG_KEY_OPTION, .num = 9, .pullup = 0, .level = 0},\
}


// Battery
#define RG_BATTERY_DRIVER           0
// #define RG_BATTERY_CALC_PERCENT(raw) (99)
// #define RG_BATTERY_CALC_VOLTAGE(raw) (0)

// Status LED
#define RG_GPIO_LED                 GPIO_NUM_4
#define RG_GPIO_LED_ACTIVE_LOW      // setting RG_GPIO_LED low turns on the LED, while high turns it off

// I2C BUS
#define RG_GPIO_I2C_SDA             GPIO_NUM_22
#define RG_GPIO_I2C_SCL             GPIO_NUM_27

// SPI Display
#define RG_GPIO_LCD_MISO            GPIO_NUM_12
#define RG_GPIO_LCD_MOSI            GPIO_NUM_13
#define RG_GPIO_LCD_CLK             GPIO_NUM_14
#define RG_GPIO_LCD_CS              GPIO_NUM_15
#define RG_GPIO_LCD_DC              GPIO_NUM_2
#define RG_GPIO_LCD_BCKL            GPIO_NUM_21

// SPI SD Card
#define RG_GPIO_SDSPI_MISO          GPIO_NUM_19
#define RG_GPIO_SDSPI_MOSI          GPIO_NUM_23
#define RG_GPIO_SDSPI_CLK           GPIO_NUM_18
#define RG_GPIO_SDSPI_CS            GPIO_NUM_5

// Updater
#define RG_UPDATER_ENABLE               1
#define RG_UPDATER_APPLICATION          RG_APP_FACTORY
#define RG_UPDATER_DOWNLOAD_LOCATION    RG_STORAGE_ROOT "/retro-go/updates"
