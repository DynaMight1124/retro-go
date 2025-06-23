// Target definition
#define RG_TARGET_NAME             "CYD"

// Storage
#define RG_STORAGE_ROOT             "/sd"
#define RG_STORAGE_SDSPI_HOST       SPI3_HOST
#define RG_STORAGE_SDSPI_SPEED      SDMMC_FREQ_DEFAULT
// #define RG_STORAGE_SDMMC_HOST       SDMMC_HOST_SLOT_1
// #define RG_STORAGE_SDMMC_SPEED      SDMMC_FREQ_DEFAULT
// #define RG_STORAGE_FLASH_PARTITION  "vfs"

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
#define RG_SCREEN_ROTATE            0
#define RG_SCREEN_MARGIN_TOP        0
#define RG_SCREEN_MARGIN_BOTTOM     0
#define RG_SCREEN_MARGIN_LEFT       0
#define RG_SCREEN_MARGIN_RIGHT      0


#define ST7789_MADCTL 0x36 // Memory Access Control
#define ST7789_MADCTL_MV 0x20
#define ST7789_MADCTL_RGB 0x00
#define ST7789_MADCTL_BGR 0x08


#define RG_SCREEN_INIT()                                                                                         \
    ILI9341_CMD(0xCF, 0x00, 0xc3, 0x30);                                                                         \
    ILI9341_CMD(0xED, 0x64, 0x03, 0x12, 0x81);                                                                   \
    ILI9341_CMD(0xE8, 0x85, 0x00, 0x78);                                                                         \
    ILI9341_CMD(0xCB, 0x39, 0x2c, 0x00, 0x34, 0x02);                                                             \
    ILI9341_CMD(0xF7, 0x20);                                                                                     \
    ILI9341_CMD(0xEA, 0x00, 0x00);                                                                               \
    ILI9341_CMD(0xC0, 0x1B);                 /* Power control   //VRH[5:0] */                                    \
    ILI9341_CMD(0xC1, 0x12);                 /* Power control   //SAP[2:0];BT[3:0] */                            \
    ILI9341_CMD(0xC5, 0x32, 0x3C);           /* VCM control */                                                   \
    ILI9341_CMD(0xC7, 0x91);                 /* VCM control2 */                                                  \
    ILI9341_CMD(ST7789_MADCTL, (ST7789_MADCTL_BGR));                                                             \
    ILI9341_CMD(0x21);                       /* Invert colors */                                                 \
    ILI9341_CMD(0xB1, 0x00, 0x10);           /* Frame Rate Control (1B=70, 1F=61, 10=119) */                     \
    ILI9341_CMD(0xB6, 0x0A, 0xA2);           /* Display Function Control */                                      \
    ILI9341_CMD(0xF6, 0x01, 0x30);                                                                               \
    ILI9341_CMD(0xF2, 0x00); /* 3Gamma Function Disable */                                                       \
    ILI9341_CMD(0xE0, 0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19);       \
    ILI9341_CMD(0xE1, 0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19);   


#define RG_GAMEPAD_I2C_MAP {\
    {RG_KEY_UP,     (1<<2)},\
    {RG_KEY_RIGHT,  (1<<1)},\
    {RG_KEY_DOWN,   (1<<3)},\
    {RG_KEY_LEFT,   (1<<0)},\
    {RG_KEY_SELECT, (1<<6)},\
    {RG_KEY_A,      (1<<4)},\
    {RG_KEY_B,      (1<<5)},\
    {RG_KEY_START,  (1<<7)},\
    {RG_KEY_MENU,   (1<<8)},\
    {RG_KEY_OPTION, (1<<9)},\
}

// #define RG_RECOVERY_BTN 0

// Battery
// #define RG_BATTERY_DRIVER           0
// #define RG_BATTERY_CALC_PERCENT(raw) (99)
// #define RG_BATTERY_CALC_VOLTAGE(raw) (0)

// Status LED
#define RG_GPIO_LED                 GPIO_NUM_4
#define RG_GPIO_LED_ACTIVE_LOW      // setting RG_GPIO_LED low turns on the LED, while high turns it off

// I2C BUS
#define RG_I2C_DRIVER               1   // 0 = AW9523, 1 = PCF8575
#define RG_GPIO_I2C_SDA             GPIO_NUM_22
#define RG_GPIO_I2C_SCL             GPIO_NUM_27

// SPI Display
#define RG_GPIO_LCD_MISO            GPIO_NUM_12
#define RG_GPIO_LCD_MOSI            GPIO_NUM_13
#define RG_GPIO_LCD_CLK             GPIO_NUM_14
#define RG_GPIO_LCD_CS              GPIO_NUM_15
#define RG_GPIO_LCD_DC              GPIO_NUM_2
#define RG_GPIO_LCD_BCKL            GPIO_NUM_21
// #define RG_GPIO_LCD_RST           GPIO_NUM_NC

// SPI SD Card
#define RG_GPIO_SDSPI_MISO          GPIO_NUM_19
#define RG_GPIO_SDSPI_MOSI          GPIO_NUM_23
#define RG_GPIO_SDSPI_CLK           GPIO_NUM_18
#define RG_GPIO_SDSPI_CS            GPIO_NUM_5

// External I2S DAC
// #define RG_GPIO_SND_I2S_BCK         GPIO_NUM_4
// #define RG_GPIO_SND_I2S_WS          GPIO_NUM_12
// #define RG_GPIO_SND_I2S_DATA        GPIO_NUM_15
// #define RG_GPIO_SND_AMP_ENABLE      GPIO_NUM_NC

// Updater
#define RG_UPDATER_ENABLE               0
// #define RG_UPDATER_APPLICATION          RG_APP_FACTORY
// #define RG_UPDATER_DOWNLOAD_LOCATION    RG_STORAGE_ROOT "/cyd/firmware"
