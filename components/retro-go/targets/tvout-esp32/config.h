/****************************************************************************
 * Target definition for TVOUT (Composite Video) on ESP32                   *
 * Build: python rg_tool.py --target tvout-esp32 release --no-networking    *
 ****************************************************************************/
#define RG_TARGET_NAME             "TVOUT-ESP32"


/****************************************************************************
 * Status LED                                                               *
 ****************************************************************************/
// #define RG_LED_DRIVER            1   // 1 = GPIO
// #define RG_GPIO_LED              GPIO_NUM_2


/****************************************************************************
 * Storage (SDMMC Slot 1)                                                   *
 ****************************************************************************/
#define RG_STORAGE_ROOT             "/sd"
#define RG_STORAGE_SDSPI_HOST       SPI2_HOST
#define RG_STORAGE_SDSPI_SPEED      SDMMC_FREQ_DEFAULT
#define RG_GPIO_SDSPI_MISO          GPIO_NUM_19
#define RG_GPIO_SDSPI_MOSI          GPIO_NUM_23
#define RG_GPIO_SDSPI_CLK           GPIO_NUM_21
#define RG_GPIO_SDSPI_CS            GPIO_NUM_22


/****************************************************************************
 * Audio                                                                    *
 ****************************************************************************/
// TVout uses I2S0 + DAC_CHANNEL_1 (GPIO25).
// Use Sigma-Delta modulation on GPIO18 for audio.
#define RG_AUDIO_USE_INT_DAC        0
#define RG_AUDIO_USE_PWM_SND        1
#define RG_GPIO_AUDIO_OUT           GPIO_NUM_18


/****************************************************************************
 * Video                                                                    *
 ****************************************************************************/
#define RG_SCREEN_DRIVER            3   // 3 = TVOUT
#define RG_TVOUT_STANDARD           0   // 0 = NTSC, 1 = PAL
#define RG_SCREEN_WIDTH             256
#define RG_SCREEN_HEIGHT            240
#define RG_SCREEN_ROTATION          0
#define RG_SCREEN_PIXEL_FORMAT      0   // 0=565_BE, 1=565_LE
#define RG_SCREEN_VISIBLE_AREA      {10, 8, 4, 6} // Left, Top, Right, Bottom
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0} // Left, Top, Right, Bottom
#define RG_SCREEN_PARTIAL_UPDATES   0


/****************************************************************************
 * Input                                                                    *
 ****************************************************************************/
#define RG_GAMEPAD_GPIO_MAP {\
    {RG_KEY_UP,     .num = GPIO_NUM_13,  .pullup = 1, .level = 0},\
    {RG_KEY_RIGHT,  .num = GPIO_NUM_32,  .pullup = 1, .level = 0},\
    {RG_KEY_DOWN,   .num = GPIO_NUM_33,  .pullup = 1, .level = 0},\
    {RG_KEY_LEFT,   .num = GPIO_NUM_27,  .pullup = 1, .level = 0},\
    {RG_KEY_SELECT, .num = GPIO_NUM_14,  .pullup = 1, .level = 0},\
    {RG_KEY_START,  .num = GPIO_NUM_4,   .pullup = 1, .level = 0},\
    {RG_KEY_A,      .num = GPIO_NUM_39,  .pullup = 1, .level = 0},\
    {RG_KEY_B,      .num = GPIO_NUM_36,  .pullup = 1, .level = 0},\
    {RG_KEY_X,      .num = GPIO_NUM_34,  .pullup = 1, .level = 0},\
    {RG_KEY_Y,      .num = GPIO_NUM_35,  .pullup = 1, .level = 0},\
}
#define RG_GAMEPAD_VIRT_MAP {\
    {RG_KEY_MENU, .src = RG_KEY_START | RG_KEY_SELECT},\
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
 * Miscellaneous                                                            *
 ****************************************************************************/
#define RG_RECOVERY_BTN                 RG_KEY_START

#define RG_CUSTOM_PLATFORM_INIT() \
    /* Arbitrary code executed very early during retro-go init */
