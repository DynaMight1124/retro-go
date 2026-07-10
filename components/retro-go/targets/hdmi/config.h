/****************************************************************************
 * Target definition for SPI2HDMI                                           *
 *                                                                          *
 ****************************************************************************/
#define RG_TARGET_NAME             "HDMI"


/****************************************************************************
 * Status LED                                                               *
 ****************************************************************************/
// #define RG_LED_DRIVER               1   // 1 = GPIO
// #define RG_GPIO_LED                 GPIO_NUM_NC
// #define RG_GPIO_LED_INVERT          // Uncomment if the LED is active LOW


/****************************************************************************
 * I2C / GPIO Extender                                                      *
 ****************************************************************************/
// #define RG_I2C_GPIO_DRIVER          1   // 1 = AW9523, 2 = PCF9539, 3 = MCP23017, 4 = PCF8575, 5 = PCF8574
// #define RG_I2C_GPIO_ADDR            0x00
// #define RG_GPIO_I2C_SDA             GPIO_NUM_NC
// #define RG_GPIO_I2C_SCL             GPIO_NUM_NC


/****************************************************************************
 * Storage                                                                  *
 ****************************************************************************/
#define RG_STORAGE_ROOT              "/sd"
#define RG_STORAGE_SDSPI_HOST        SPI3_HOST
#define RG_STORAGE_SDSPI_SPEED       SDMMC_FREQ_DEFAULT
#define RG_GPIO_SDSPI_MISO           GPIO_NUM_9
#define RG_GPIO_SDSPI_MOSI           GPIO_NUM_11
#define RG_GPIO_SDSPI_CLK            GPIO_NUM_12
#define RG_GPIO_SDSPI_CS             GPIO_NUM_10
// #define RG_STORAGE_SDMMC_HOST       SDMMC_HOST_SLOT_1
// #define RG_STORAGE_SDMMC_SPEED      SDMMC_FREQ_DEFAULT
// #define RG_GPIO_SDMMC_CMD           GPIO_NUM_NC
// #define RG_GPIO_SDMMC_CLK           GPIO_NUM_NC
// #define RG_GPIO_SDMMC_D0            GPIO_NUM_NC
// #define RG_GPIO_SDMMC_D1            GPIO_NUM_NC
// #define RG_GPIO_SDMMC_D2            GPIO_NUM_NC
// #define RG_GPIO_SDMMC_D3            GPIO_NUM_NC
// #define RG_STORAGE_FLASH_PARTITION  "vfs"


/****************************************************************************
 * Audio                                                                    *
 ****************************************************************************/
#define RG_AUDIO_USE_INT_DAC        0   // 0 = Disable, 1 = GPIO25, 2 = GPIO26, 3 = Both
#define RG_AUDIO_USE_EXT_DAC        1   // 0 = Disable, 1 = Enable
#define RG_AUDIO_USE_BUZZER_PIN     0   // See drivers/audio/buzzer.c for details
#define RG_GPIO_SND_I2S_BCK         GPIO_NUM_41
#define RG_GPIO_SND_I2S_WS          GPIO_NUM_42
#define RG_GPIO_SND_I2S_DATA        GPIO_NUM_40
// #define RG_GPIO_SND_AMP_ENABLE   GPIO_NUM_39
// #define RG_GPIO_SND_AMP_ENABLE_INVERT // Uncomment if the mute = HIGH


/****************************************************************************
 * Video                                                                    *
 ****************************************************************************/
#define RG_SCREEN_DRIVER            1   // 1 = ILI9341/ST7789
#define RG_SCREEN_HOST              SPI2_HOST
#define RG_SCREEN_SPEED             SPI_MASTER_FREQ_40M
#define RG_SCREEN_BACKLIGHT         1
#define RG_SCREEN_WIDTH             320
#define RG_SCREEN_HEIGHT            240
#define RG_SCREEN_ROTATION          0   // Possible values are 0-7 (you'll have to experiment)
#define RG_SCREEN_RGB_BGR           0   // Possible values are 0-1 (change if colors are bad)
#define RG_SCREEN_PIXEL_FORMAT      0   // Possible values are 0=565_BE, 1=565_LE
#define RG_SCREEN_VISIBLE_AREA      {0, 0, 0, 0} // Left, Top, Right, Bottom
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0} // Left, Top, Right, Bottom
#define RG_SCREEN_PARTIAL_UPDATES   0
#define RG_SCREEN_INIT()                                                                                         \

#define RG_GPIO_LCD_MISO            GPIO_NUM_NC
#define RG_GPIO_LCD_MOSI            GPIO_NUM_4
#define RG_GPIO_LCD_CLK             GPIO_NUM_6
#define RG_GPIO_LCD_CS              GPIO_NUM_7
#define RG_GPIO_LCD_DC              GPIO_NUM_5
#define RG_GPIO_LCD_BCKL            GPIO_NUM_NC
#define RG_GPIO_LCD_RST             GPIO_NUM_NC
// #define RG_GPIO_LCD_BCKL_INVERT     // Uncomment if the LED is active LOW


/****************************************************************************
 * Input                                                                    *
 ****************************************************************************/
#define RG_GAMEPAD_GPIO_MAP {\
    {RG_KEY_UP,     .num = GPIO_NUM_18,  .pullup = 1, .level = 0},\
    {RG_KEY_RIGHT,  .num = GPIO_NUM_16,  .pullup = 1, .level = 0},\
    {RG_KEY_DOWN,   .num = GPIO_NUM_15,  .pullup = 1, .level = 0},\
    {RG_KEY_LEFT,   .num = GPIO_NUM_17,  .pullup = 1, .level = 0},\
    {RG_KEY_SELECT, .num = GPIO_NUM_1,   .pullup = 1, .level = 0},\
    {RG_KEY_START,  .num = GPIO_NUM_2,   .pullup = 1, .level = 0},\
    {RG_KEY_A,      .num = GPIO_NUM_47,  .pullup = 1, .level = 0},\
    {RG_KEY_B,      .num = GPIO_NUM_48,  .pullup = 1, .level = 0},\
    {RG_KEY_X,      .num = GPIO_NUM_14,  .pullup = 1, .level = 0},\
    {RG_KEY_Y,      .num = GPIO_NUM_21,  .pullup = 1, .level = 0},\
    {RG_KEY_L,      .num = GPIO_NUM_8,   .pullup = 1, .level = 0},\
    {RG_KEY_R,      .num = GPIO_NUM_38,  .pullup = 1, .level = 0},\
}
#define RG_GAMEPAD_VIRT_MAP {\
    {RG_KEY_MENU,   .src = RG_KEY_START | RG_KEY_SELECT},\
    {RG_KEY_OPTION, .src = RG_KEY_START | RG_KEY_B},\
}


/****************************************************************************
 * Battery                                                                  *
 ****************************************************************************/
#define RG_BATTERY_DRIVER           0 // Disabled
#define RG_BATTERY_ADC_UNIT         0
#define RG_BATTERY_ADC_CHANNEL      0
#define RG_BATTERY_CALC_PERCENT(raw) (0)
#define RG_BATTERY_CALC_VOLTAGE(raw) (0)


/****************************************************************************
 * Updater                                                                  *
 ****************************************************************************/
#define RG_UPDATER_ENABLE               1
#define RG_UPDATER_APPLICATION          RG_APP_FACTORY
#define RG_UPDATER_DOWNLOAD_LOCATION    RG_STORAGE_ROOT "/retro-go/updates"


/****************************************************************************
 * Miscellaneous                                                            *
 ****************************************************************************/
#define RG_RECOVERY_BTN                 RG_KEY_SELECT // Keep this button pressed to open the recovery menu

#define RG_CUSTOM_PLATFORM_INIT() \
    /* Arbitrary code executed very early during retro-go init */

// See components/retro-go/config.h for more things you can define here!
