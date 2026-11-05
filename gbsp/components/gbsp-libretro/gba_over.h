#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
static const ini_t gbaover[] = {
   {
      // 007 - Nightfire (U)
      "A7OE",                      /* gamepak_code         */
      0,                           /* flags                */
#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
static const ini_t gbaover[] = {
   {
      // Sonic Advance (U) -- esp32-emu-turbo: VBlank spin at 0x800096c (31% of the level's instructions)
      "SONICADVANCE",              /* gamepak_title        */
      "ASOE",                      /* gamepak_code         */
      "78",                        /* gamepak_maker        */
      0,                           /* flags                */
      0x800096c,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
      // 007 - Nightfire (U)
      "NIGHTFIRE",                 /* gamepak_title        */
      "A7OE",                      /* gamepak_code         */
      "69",                        /* gamepak_maker        */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
static const ini_t gbaover[] = {
   {
      // 007 - Nightfire (U)
      "NIGHTFIRE",                 /* gamepak_title        */
      "A7OE",                      /* gamepak_code         */
      "69",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80031d6,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Advance Wars (E)
      "AWRP",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_SERIAL_AW1,     /* flags        */
      0x803880a,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Advance Wars (E)
      "ADVANCEWARSP",              /* gamepak_title        */
      "AWRP",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
      0x803880a,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Advance Wars (U)
      "AWRE",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_SERIAL_AW1,     /* flags        */
      0x803880a,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Advance Wars (U)
      "ADVANCEWARS",               /* gamepak_title        */
      "AWRE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
      0x803880a,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Advance Wars 2: Black Hole Rising (E)
      "AW2P",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_SERIAL_AW2,     /* flags        */
      0x8036e2a,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Advance Wars 2: Black Hole Rising (E)
      "ADVANCEWARS2",              /* gamepak_title        */
      "AW2P",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
      0x8036e2a,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Advance Wars 2: Black Hole Rising (U)
      "AW2E",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_SERIAL_AW2,     /* flags        */
      0x8036e2a,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Advance Wars 2: Black Hole Rising (U)
      "ADVANCEWARS2",              /* gamepak_title        */
      "AW2E",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
      0x8036e2a,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Animal Yokochou Dokidoki Shinkyuu Shiken (J)
      "BAXJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Animal Yokochou Dokidoki Shinkyuu Shiken (J)
      "ANIYOKO2",                  /* gamepak_title        */
      "BAXJ",                      /* gamepak_code         */
      "A4",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0x3000f1c,                   /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Another World (Homebrew)
      "Home",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Another World (Homebrew)
      "FoxAnWorld",                /* gamepak_title        */
      "Home",                      /* gamepak_code         */
      "00",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0x3000f1c,                   /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Asterix & Obelix XXL (E)
      "BLXP",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Asterix & Obelix XXL (E)
      "ASTERIX",                   /* gamepak_title        */
      "BLXP",                      /* gamepak_code         */
      "70",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x846d060,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Battle Network Rockman EXE (J)
      "AREJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Battle Network RockMan EXE (J)
      "ROCKMAN_EXE",               /* gamepak_title        */
      "AREJ",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000338,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Bleach Advance (J)
      "BLEJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Bleach Advance (J)
      "BLEACH ADV1",               /* gamepak_title        */
      "BLEJ",                      /* gamepak_code         */
      "6L",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Shin Bokura no Taiyou: Gyakushuu no Sabata (J)
      "U33J",                      /* gamepak_code         */
      FLAGS_RTC,                   /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Shin Bokura no Taiyou: Gyakushuu no Sabata (J)
      "BOKTAI3",                   /* gamepak_title        */
      "U33J",                      /* gamepak_code         */
      "A4",                        /* gamepak_maker        */
      FLAGS_RTC,                   /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Bomberman Jetters Game Collection (J)
      "BOMJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Bomberman Jetters Game Collection (J)
      "BOMBERMANJGC",              /* gamepak_title        */
      "BOMJ",                      /* gamepak_code         */
      "18",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Bomberman Tournament (U)
      "ABSE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Bomberman Tournament (U)
      "BOMSTORYUSA",               /* gamepak_title        */
      "ABSE",                      /* gamepak_code         */
      "52",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000526,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Bookworm (U)
      "BKWE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Bookworm (U)
      "BOOKWORM",                  /* gamepak_title        */
      "BKWE",                      /* gamepak_code         */
      "5G",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x800397c,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Broken Sword - The Shadow of the Templars (U)
      "ABJE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Broken Sword - The Shadow of the Templars (U)
      "BROKENSWORD",               /* gamepak_title        */
      "ABJE",                      /* gamepak_code         */
      "6L",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000a26,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Calciobit (J)
      "BIXJ",                      /* gamepak_code         */
      FLAGS_FLASH_128KB,           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Calciobit (J)
      "CALCIOBIT01",               /* gamepak_title        */
      "BIXJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB,           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // No No No Puzzle Chailien (J)
      "BKRJ",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // No No No Puzzle Chailien (J)
      "CHAILIEN NO ",              /* gamepak_title        */
      "BKRJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Castlevania (E)
      "AAMP",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Castlevania (E)
      "DRACULA AGB1",              /* gamepak_title        */
      "AAMP",                      /* gamepak_code         */
      "A4",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80003d2,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Castlevania Circle Of The Moon (U)
      "AAME",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Castlevania Circle Of The Moon (U)
      "DRACULA AGB1",              /* gamepak_title        */
      "AAME",                      /* gamepak_code         */
      "A4",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80003d2,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Chicken Little (J)
      "BCHJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Chicken Little (J)
      "CLITTLEJ",                  /* gamepak_title        */
      "BCHJ",                      /* gamepak_code         */
      "G9",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Defender Of The Crown (U)
      "ADHE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Defender Of The Crown (U)
      "DOTC",                      /* gamepak_title        */
      "ADHE",                      /* gamepak_code         */
      "5N",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80007ec,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Digimon Battle Spirit (U)
      "A8SE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Digimon Battle Spirit (U)
      "DIGIMON BTSP",              /* gamepak_title        */
      "A8SE",                      /* gamepak_code         */
      "B2",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8011208,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Digimon Battle Spirit 2 (U)
      "BDSE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Digimon Battle Spirit 2 (U)
      "DIGIMON BS2",               /* gamepak_title        */
      "BDSE",                      /* gamepak_code         */
      "B2",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8010eb0,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Digimon Racing (U)
      "BDGE",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Digimon Racing (U)
      "DIGIMONRACIN",              /* gamepak_title        */
      "BDGE",                      /* gamepak_code         */
      "B2",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Digimon Racing (E)
      "BDGP",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Digimon Racing (E)
      "DIGIMONRACIN",              /* gamepak_title        */
      "BDGP",                      /* gamepak_code         */
      "B2",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Disney's Aladdin (J)
      "AJ6J",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Disney's Aladdin (J)
      "ALADDIN",                   /* gamepak_title        */
      "AJ6J",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Disney's Aladdin (E)
      "BADP",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Disney's Aladdin (E)
      "ALADDIN",                   /* gamepak_title        */
      "BADP",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Disney's Aladdin (U)
      "BADE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Disney's Aladdin (U)
      "ALADDIN",                   /* gamepak_title        */
      "BADE",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Doom 2 (U)
      "A9DE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Doom 2 (U)
      "DOOM II",                   /* gamepak_title        */
      "A9DE",                      /* gamepak_code         */
      "52",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0x30041c8,                   /* translation_gate_target_1 */
      0x3004fa0,                   /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Donald Duck Advance (U)
      "ADKE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Donald Duck Advance (U)
      "DISNEY'S DON",              /* gamepak_title        */
      "ADKE",                      /* gamepak_code         */
      "41",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8002f30,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Dragon Ball Z - Buu's Fury (U)
      "BG3E",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Dragon Ball Z - Buu's Fury (U)
      "DBZBUUSFURY",               /* gamepak_title        */
      "BG3E",                      /* gamepak_code         */
      "70",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Dragon Ball Z - The Legacy of Goku (E)
      "ALGP",                      /* gamepak_code         */
      FLAGS_EEPROM,                /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Dragon Ball Z - The Legacy of Goku (E)
      "DRAGONBALL Z",              /* gamepak_title        */
      "ALGP",                      /* gamepak_code         */
      "70",                        /* gamepak_maker        */
      FLAGS_EEPROM,                /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Dragon Ball Z - The Legacy of Goku II (U)
      "ALFE",                      /* gamepak_code         */
      FLAGS_EEPROM,                /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Dragon Ball Z - The Legacy of Goku II (U)
      "DBZLGCYGOKU2",              /* gamepak_title        */
      "ALFE",                      /* gamepak_code         */
      "70",                        /* gamepak_maker        */
      FLAGS_EEPROM,                /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Dragon Ball Z - The Legacy of Goku II (E)
      "ALFP",                      /* gamepak_code         */
      FLAGS_EEPROM,                /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Dragon Ball Z - The Legacy of Goku II (E)
      "DBZLGCYGOKU2",              /* gamepak_title        */
      "ALFP",                      /* gamepak_code         */
      "70",                        /* gamepak_maker        */
      FLAGS_EEPROM,                /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Dragon Ball Z - Taiketsu (U)
      "BDBE",                      /* gamepak_code         */
      FLAGS_EEPROM,                /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Dragon Ball Z - Taiketsu (U)
      "DBZ TAIKETSU",              /* gamepak_title        */
      "BDBE",                      /* gamepak_code         */
      "70",                        /* gamepak_maker        */
      FLAGS_EEPROM,                /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Dragon Ball Z - Taiketsu (E)
      "BDBP",                      /* gamepak_code         */
      FLAGS_EEPROM,                /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Dragon Ball Z - Taiketsu (E)
      "DBZ TAIKETSU",              /* gamepak_title        */
      "BDBP",                      /* gamepak_code         */
      "70",                        /* gamepak_maker        */
      FLAGS_EEPROM,                /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Drill Dozer (U)
      "V49E",                      /* gamepak_code         */
      FLAGS_RUMBLE,                /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Drill Dozer (U)
      "DRILL DOZER",               /* gamepak_title        */
      "V49E",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RUMBLE,                /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80006c2,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // F-Zero for Game Boy Advance (J)
      "AFZJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // F-Zero for Game Boy Advance (J)
      "F-ZERO ADVAN",              /* gamepak_title        */
      "AFZJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000c2e,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // F-Zero - Maximum Velocity (U)
      "AFZE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // F-Zero - Maximum Velocity (U)
      "F-ZERO ADVAN",              /* gamepak_title        */
      "AFZE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000c2e,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // F-Zero: Climax (U)
      "BFTJ",                      /* gamepak_code         */
      FLAGS_FLASH_128KB,           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // F-Zero: Climax (U)
      "F-ZEROCLIMAX",              /* gamepak_title        */
      "BFTJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB,           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Final Fantasy IV Advance (U)
      "BZ4E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Final Fantasy IV Advance (U)
      "FF4ADVANCE",                /* gamepak_title        */
      "BZ4E",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x800fabe,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Final Fantasy Tactics Advance (U)
      "AFXE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Final Fantasy Tactics Advance (U)
      "FFTA_USVER.",               /* gamepak_title        */
      "AFXE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x800041e,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Final Fight One (E)
      "AFFP",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Final Fight One (E)
      "FINAL FIGHT",               /* gamepak_title        */
      "AFFP",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x800b428,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Final Fight One (U)
      "AFFE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Final Fight One (U)
      "FINAL FIGHT",               /* gamepak_title        */
      "AFFE",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x800b428,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Game Boy Wars Advance 1+2 (J)
      "BGWJ",                      /* gamepak_code         */
      FLAGS_FLASH_128KB,           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Game Boy Wars Advance 1+2 (J)
      "GBWARS1+2",                 /* gamepak_title        */
      "BGWJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB,           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Goodboy Galaxy (World)
      "2GBP",                      /* gamepak_code         */
      FLAGS_RUMBLE,                /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Goodboy Galaxy (World)
      "GOODBOY",                   /* gamepak_title        */
      "2GBP",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RUMBLE,                /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Golden Sun (U)
      "AGSE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Golden Sun (U)
      "Golden_Sun_A",              /* gamepak_title        */
      "AGSE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0x3000820,                   /* translation_gate_target_1 */
      0x30009ac,                   /* translation_gate_target_2 */
      0x3007dac,                   /* translation_gate_target_3 */
   },
   {
      /* Probably the most horrifically coded GBA game in existence. */
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Golden Sun: The Lost Age (U)
      "AGFE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Golden Sun: The Lost Age (U)
      "GOLDEN_SUN_B",              /* gamepak_title        */
      "AGFE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x08013542,                  /* idle_loop_target_pc  */
      0x30009ac,                   /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
      /* Badly coded game with several idle loops. This one works for level
       * one at least.
       */
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Gradius Galaxies (U)
      "AGAE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Gradius Galaxies (U)
      "GRADIUSGALAX",              /* gamepak_title        */
      "AGAE",                      /* gamepak_code         */
      "A4",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8013844,                   /* idle_loop_target_pc  */
      0x30009ac,                   /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Hoshi no Kirby: Yume no Izumi Deluxe (J)
      "A7KJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Hoshi no Kirby: Yume no Izumi Deluxe (J)
      "AGB KIRBY DX",              /* gamepak_title        */
      "A7KJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000f92,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // J-League Pro Soccer Club wo Tsukurou Advance (J)
      "AC2J",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // J-League Pro Soccer Club wo Tsukurou Advance (J)
      "SAKATUKUADV",               /* gamepak_title        */
      "AC2J",                      /* gamepak_code         */
      "8P",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Kim Possible 2 (U)
      "BKME",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Kim Possible 2 (U)
      "KIMPOSSIBLE2",              /* gamepak_title        */
      "BKME",                      /* gamepak_code         */
      "4Q",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80008a2,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Kim Possible 2 (E)
      "BKMP",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Kim Possible 2 (E)
      "KIMPOSSIBLE2",              /* gamepak_title        */
      "BKMP",                      /* gamepak_code         */
      "4Q",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80008a2,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Kirby: Nightmare in Dream Land (E)
      "A7KP",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Kirby: Nightmare in Dreamland (E)
      "AGB KIRBY DX",              /* gamepak_title        */
      "A7KP",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000fae,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Kirby: Nightmare in Dream Land (U)
      "A7KE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Kirby: Nightmare in Dreamland (U)
      "AGB KIRBY DX",              /* gamepak_title        */
      "A7KE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000fae,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Koinu-Chan no Hajimete no Osanpo (J)
      "BISJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Koinu-Chan no Hajimete no Osanpo (J)
      "OSANPO",                    /* gamepak_title        */
      "BISJ",                      /* gamepak_code         */
      "IQ",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Legendz - Sign of Nekuromu (J)
      "BLVJ",                      /* gamepak_code         */
      FLAGS_RTC,                   /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Legendz - Sign of Nekuromu (J)
      "LEGENDZSON__",              /* gamepak_title        */
      "BLVJ",                      /* gamepak_code         */
      "B2",                        /* gamepak_maker        */
      FLAGS_RTC,                   /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Legendz - Yomigaeru Shiren no Shima (J)
      "BLJJ",                      /* gamepak_code         */
      FLAGS_RTC,                   /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Legendz - Yomigaeru Shiren no Shima (J)
      "LEGENDZSHIMA",              /* gamepak_title        */
      "BLJJ",                      /* gamepak_code         */
      "B2",                        /* gamepak_maker        */
      FLAGS_RTC,                   /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Legendz - Buhwarhaneun Siryeonyi Seom (K)
      "BLJK",                      /* gamepak_code         */
      FLAGS_RTC,                   /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Legendz - Buhwarhaneun Siryeonyi Seom (K)
      "LEGENDZSHIMA",              /* gamepak_title        */
      "BLJK",                      /* gamepak_code         */
      "B2",                        /* gamepak_maker        */
      FLAGS_RTC,                   /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Magical Houshin (J)
      "AJOJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Magical Houshin (J)
      "M HOUSHIN",                 /* gamepak_title        */
      "AJOJ",                      /* gamepak_code         */
      "C8",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Magical Quest 2 Starring Mickey & Minnie (U)
      "AQME",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Magical Quest 2 Starring Mickey & Minnie (U)
      "M&M MAGICAL2",              /* gamepak_title        */
      "AQME",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x801d340,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Magical Quest 3 Starring Mickey & Donald (U)
      "BMQE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Magical Quest 3 Starring Mickey & Donald (U)
      "M&D MAGICAL3",              /* gamepak_title        */
      "BMQE",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8016064,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mario & Luigi: Superstar Saga (E)
      "A88P",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_GBA_PLAYER,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Mario & Luigi - Super Star Saga (E)
      "MARIO&LUIGIP",              /* gamepak_title        */
      "A88P",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_GBA_PLAYER,            /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mario & Luigi: Superstar Saga (U)
      "A88E",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_GBA_PLAYER,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Mario & Luigi - Super Star Saga (U)
      "MARIO&LUIGIU",              /* gamepak_title        */
      "A88E",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_GBA_PLAYER,            /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mario & Luigi RPG (J)
      "A88J",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_GBA_PLAYER,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Mario & Luigi - Super Star Saga (J)
      "MARIO&LUIGIJ",              /* gamepak_title        */
      "A88J",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_GBA_PLAYER,            /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (U)
      "BMGE",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (U)
      "MARIOGOLFGBA",              /* gamepak_title        */
      "BMGE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0x3000d00,                   /* translation_gate_target_1 */
      0x3000a30,                   /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mario Golf Advance (J)
      "BMGJ",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Mario Golf: Advance Tour (J)
      "MARIOGOLFGBA",              /* gamepak_title        */
      "BMGJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0x3000d00,                   /* translation_gate_target_1 */
      0x3000a30,                   /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (E)
      "BMGP",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (E)
      "MARIOGOLFGBA",              /* gamepak_title        */
      "BMGP",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0x3000d00,                   /* translation_gate_target_1 */
      0x3000a30,                   /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (S)
      "BMGS",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (S)
      "MARIOGOLFGBA",              /* gamepak_title        */
      "BMGS",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0x3000d00,                   /* translation_gate_target_1 */
      0x3000a30,                   /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (F)
      "BMGF",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (F)
      "MARIOGOLFGBA",              /* gamepak_title        */
      "BMGF",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0x3000d00,                   /* translation_gate_target_1 */
      0x3000a30,                   /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (I)
      "BMGI",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (I)
      "MARIOGOLFGBA",              /* gamepak_title        */
      "BMGI",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0x3000d00,                   /* translation_gate_target_1 */
      0x3000a30,                   /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (G)
      "BMGD",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (G)
      "MARIOGOLFGBA",              /* gamepak_title        */
      "BMGD",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0x3000d00,                   /* translation_gate_target_1 */
      0x3000a30,                   /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (A)
      "BMGU",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Mario Golf: Advance Tour (A)
      "MARIOGOLFGBA",              /* gamepak_title        */
      "BMGU",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0x8014e0a,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0x3000d00,                   /* translation_gate_target_1 */
      0x3000a30,                   /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mario vs. Donkey Kong (U)
      "BM5E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Mario Vs Donkey Kong (U)
      "MARIOVSDK",                 /* gamepak_title        */
      "BM5E",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8033eec,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Mario Party Advance (U)
      "B8ME",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Mario Party Advance (U)
      "MARIOPARTYUS",              /* gamepak_title        */
      "B8ME",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Mario Party Advance (J)
      "B8MJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Mario Party Advance (J)
      "MARIOPARTYJA",              /* gamepak_title        */
      "B8MJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Mario Party Advance (E)
      "B8MP",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Mario Party Advance (E)
      "MARIOPARTYEU",              /* gamepak_title        */
      "B8MP",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mario Power Tennis (E)
      "BTMP",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
      // Mario Tennis Advance (J)
      "BTMJ",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
      // Mario Tennis: Power Tour (U/A)
      "BTME",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Mario Tenis (E)
      "MARIOTENNISA",              /* gamepak_title        */
      "BTME",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Medal of Honor (J)
      "BMHJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Medal of Honor (J)
      "INFILTRATOR",               /* gamepak_title        */
      "BMHJ",                      /* gamepak_code         */
      "13",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Battle Network (E)
      "AREP",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Battle Network (E)
      "MEGAMANEXEBN",              /* gamepak_title        */
      "AREP",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000338,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Battle Network (U)
      "AREE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Battle Network (U)
      "MEGAMAN_BN",                /* gamepak_title        */
      "AREE",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000338,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Battle Network 2 (U)
      "AE2E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Battle Network 2 (U)
      "MEGAMAN_EXE2",              /* gamepak_title        */
      "AE2E",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000358,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Battle Network 3 White (U)
      "A6BE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Battle Network 3 White (U)
      "MEGA_EXE3_WH",              /* gamepak_title        */
      "A6BE",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x800036c,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Battle Network 3 Blue (U)
      "A3XE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Battle Network 3 Blue (U)
      "MEGA_EXE3_BL",              /* gamepak_title        */
      "A3XE",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x800036c,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Battle Network 4 Red Sun (U)
      "B4WE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Battle Network 4 Red Sun (U)
      "MEGAMANBN4RS",              /* gamepak_title        */
      "B4WE",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80003a6,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Battle Network 4 Blue Moon (U)
      "B4BE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Battle Network 4 Blue Moon (U)
      "MEGAMANBN4BM",              /* gamepak_title        */
      "B4BE",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80003a6,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Battle Network 5 Team Protoman (U)
      "BRBE",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0x80003ca,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Battle Network 5 Team Protoman (U)
      "MEGAMAN5_TP_",              /* gamepak_title        */
      "BRBE",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0x80003ca,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Battle Network 5 Team Colonel (U)
      "BRKE",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0x80003ca,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Battle Network 5 Team Colonel (U)
      "MEGAMAN5_TC_",              /* gamepak_title        */
      "BRKE",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0x80003ca,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Battle Network 6 Cybeast Gregar (U)
      "BR5E",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0x80003da,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Battle Network 6 Cybeast Gregar (U)
      "MEGAMAN6_GXX",              /* gamepak_title        */
      "BR5E",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0x80003da,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Battle Network 6 Cybeast Falzar (U)
      "BR6E",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0x80003da,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Battle Network 6 Cybeast Falzar (U)
      "MEGAMAN6_FXX",              /* gamepak_title        */
      "BR6E",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0x80003da,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Battle Chip Challenge (U)
      "A89E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Battle Chip Challenge (U)
      "BATTLECHIPGP",              /* gamepak_title        */
      "A89E",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000544,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Zero (E/U)
      "AZCE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Zero (E/U)
      "MEGAMAN ZERO",              /* gamepak_title        */
      "AZCE",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80004ee,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Zero 2 (U)
      "A62E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Zero 2 (U)
      "MEGAMANZERO2",              /* gamepak_title        */
      "A62E",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000664,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Zero 3 (U)
      "BZ3E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Zero 3 (U)
      "MEGAMANZERO3",              /* gamepak_title        */
      "BZ3E",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8001a08,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mega Man Zero 4 (U)
      "B4ZP",                      /* gamepak_code         */
      0,                           /* flags                */
      0x800090c,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Zero 4 (U)
      "MEGAMANZERO4",              /* gamepak_title        */
      "B4ZP",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
      0x800090c,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
      // Metal Slug Advance (E) -- esp32-emu-turbo: same loop as (U), found with the PC histogram
      "AGB-BSMP",                  /* gamepak_title        */
      "BSMP",                      /* gamepak_code         */
      "7U",                        /* gamepak_maker        */
      0,                           /* flags                */
      0x8000298,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Megaman Zero 4 (U)
      "MEGAMANZERO4",              /* gamepak_title        */
      "B4ZP",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
      0x800090c,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Metal Slug Advance (U)
      "BSME",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Metal Slug Advance (U)
      "METAL SLUG",                /* gamepak_title        */
      "BSME",                      /* gamepak_code         */
      "B7",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000298,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Momotarou Densetsu G - Gold Deck wo Tsukure! (J)
      "BM2J",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Momotarou Densetsu G - Gold Deck wo Tsukure! (J)
      "MOMOTETSU-G",              /* gamepak_title        */
      "BM2J",                      /* gamepak_code         */
      "18",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Monster Force (U)
      "AM8E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Monster Force (U)
      "MONSTERFORCE",              /* gamepak_title        */
      "AM8E",                      /* gamepak_code         */
      "7D",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000b00,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Monster Rancher Advance (U)
      "AMFE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Monster Rancher Advance (U)
      "MONSRANCHERA",              /* gamepak_title        */
      "AMFE",                      /* gamepak_code         */
      "9B",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x809f394,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Monster Rancher Advance 2 (U)
      "A2QE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Monster Rancher Advance 2 (U)
      "MONSTERRANC2",              /* gamepak_title        */
      "A2QE",                      /* gamepak_code         */
      "9B",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x81c7290,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Motoracer Advance (E)
      "A9MP",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Motoracer Advance (E)
      "MOTORACERADV",              /* gamepak_title        */
      "A9MP",                      /* gamepak_code         */
      "41",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x3001cf8,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Narnia Chronicles (U)
      "B2WE",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Narnia Chronicles (U)
      "NARNIA LWW",                /* gamepak_title        */
      "B2WE",                      /* gamepak_code         */
      "4Q",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Ninja Cop (E)
      "ANXP",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Ninja Cop (E)
      "NINJA COP",              /* gamepak_title        */
      "ANXP",                      /* gamepak_code         */
      "A4",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Ninja Five-0 (U)
      "ANXE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Ninja Five-0 (U)
      "NINJA FIVE 0",              /* gamepak_title        */
      "ANXE",                      /* gamepak_code         */
      "A4",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Nishihara Rieko no Dendou Mahjong (J)
      "ASMJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Nishihara Rieko no Dendou Mahjong (J)
      "DENDOHMAJANG",              /* gamepak_title        */
      "ASMJ",                      /* gamepak_code         */
      "AG",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Open Season (U)
      "BOAE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Open Season (U)
      "OPEN SEASON",               /* gamepak_title        */
      "BOAE",                      /* gamepak_code         */
      "41",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8066378,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Oriental Blue - Ao no Tengai (J)
      "AORJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Oriental Blue - Ao no Tengai (J)
      "ORIENTALBLUE",              /* gamepak_title        */
      "AORJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x3001cf8,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pinball Challenge Deluxe (E)
      "APLP",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Pinball Challenge Deluxe (E)
      "PINBALL CHAL",              /* gamepak_title        */
      "APLP",                      /* gamepak_code         */
      "41",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80075a6,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Emerald (E/U)
      "BPEE",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_RFU,
      0x80008ce,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Emerald (E/U)
      "POKEMON EMER",              /* gamepak_title        */
      "BPEE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_RFU,  /* flags */
      0x80008ce,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Emerald (J)
      "BPEJ",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_RFU,
      0x80008ce,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Emerald (J)
      "POKEMON EMER",              /* gamepak_title        */
      "BPEJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_RFU,  /* flags */
      0x80008ce,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Emerald (G)
      "BPED",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_RFU,
      0x80008ce,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Emerald (G)
      "POKEMON EMER",              /* gamepak_title        */
      "BPED",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_RFU,  /* flags */
      0x80008ce,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Emerald (F)
      "BPEF",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_RFU,
      0x80008ce,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Emerald (F)
      "POKEMON EMER",              /* gamepak_title        */
      "BPEF",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_RFU,  /* flags */
      0x80008ce,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Emerald (S)
      "BPES",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_RFU,
      0x80008ce,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Emerald (S)
      "POKEMON EMER",              /* gamepak_title        */
      "BPES",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_RFU,  /* flags */
      0x80008ce,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Emerald (I)
      "BPEI",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_RFU,
      0x80008ce,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Emerald (I)
      "POKEMON EMER",              /* gamepak_title        */
      "BPEI",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_RFU,  /* flags */
      0x80008ce,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Sapphire (U)
      "AXPE",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_SERIAL_POKE,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Sapphire (U)
      "POKEMON SAPP",              /* gamepak_title        */
      "AXPE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Sapphire (J)
      "AXPJ",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_SERIAL_POKE,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Sapphire (J)
      "POKEMON SAPP",              /* gamepak_title        */
      "AXPJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Sapphire (G)
      "AXPD",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_SERIAL_POKE,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Sapphire (G)
      "POKEMON SAPP",              /* gamepak_title        */
      "AXPD",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Sapphire (I)
      "AXPI",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_SERIAL_POKE,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Sapphire (I)
      "POKEMON SAPP",              /* gamepak_title        */
      "AXPI",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Sapphire (S)
      "AXPS",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_SERIAL_POKE,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Sapphire (S)
      "POKEMON SAPP",              /* gamepak_title        */
      "AXPS",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Sapphire (F)
      "AXPF",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_SERIAL_POKE,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Sapphire (F)
      "POKEMON SAPP",              /* gamepak_title        */
      "AXPF",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Ruby (U)
      "AXVE",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_SERIAL_POKE,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Ruby (U)
      "POKEMON RUBY",              /* gamepak_title        */
      "AXVE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Ruby (J)
      "AXVJ",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_SERIAL_POKE,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Ruby (J)
      "POKEMON RUBY",              /* gamepak_title        */
      "AXVJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Ruby (G)
      "AXVD",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_SERIAL_POKE,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Ruby (G)
      "POKEMON RUBY",              /* gamepak_title        */
      "AXVD",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Ruby (I)
      "AXVI",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_SERIAL_POKE,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Ruby (I)
      "POKEMON RUBY",              /* gamepak_title        */
      "AXVI",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Ruby (S)
      "AXVS",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_SERIAL_POKE,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Ruby (S)
      "POKEMON RUBY",              /* gamepak_title        */
      "AXVS",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Pokemon Ruby (F)
      "AXVF",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC | FLAGS_SERIAL | FLAGS_SERIAL_POKE,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Pokemon Ruby (F)
      "POKEMON RUBY",              /* gamepak_title        */
      "AXVF",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Fire Red (J)
      "BPRJ",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_RFU,
      0x80008b2,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Fire Red (J)
      "POKEMON FIRE",              /* gamepak_title        */
      "BPRJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RFU,  /* flags             */
      0x80008b2,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Fire Red (E/U)
      "BPRE",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_RFU,
      0x80008b2,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Fire Red (E/U)
      "POKEMON FIRE",              /* gamepak_title        */
      "BPRE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RFU,  /* flags             */
      0x80008b2,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Fire Red (S)
      "BPRS",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_RFU,
      0x80008b2,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Fire Red (S)
      "POKEMON FIRE",              /* gamepak_title        */
      "BPRS",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RFU,  /* flags             */
      0x80008b2,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Fire Red (G)
      "BPRD",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_RFU,
      0x80008b2,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Fire Red (G)
      "POKEMON FIRE",              /* gamepak_title        */
      "BPRD",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RFU,  /* flags             */
      0x80008b2,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Fire Red (I)
      "BPRI",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_RFU,
      0x80008b2,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Fire Red (I)
      "POKEMON FIRE",              /* gamepak_title        */
      "BPRI",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RFU,  /* flags             */
      0x80008b2,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Fire Red (F)
      "BPRF",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_RFU,
      0x80008b2,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Fire Red (F)
      "POKEMON FIRE",              /* gamepak_title        */
      "BPRF",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RFU,  /* flags             */
      0x80008b2,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      /* Hey, this one is the same as Fire Red, who'd have thought? :B */
      // Pokemon Leaf Green (E/U)
      "BPGE",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_RFU,
      0x80008b2,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      /* Hey, this one is the same as Fire Red, who'd have thought? :B */
      // Pokemon: Leaf Green (E/U)
      "POKEMON LEAF",              /* gamepak_title        */
      "BPGE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RFU,  /* flags             */
      0x80008b2,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Leaf Green (S)
      "BPGS",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_RFU,
      0x80008b6,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Leaf Green (S)
      "POKEMON LEAF",              /* gamepak_title        */
      "BPGS",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RFU,  /* flags             */
      0x80008b6,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Leaf Green (G)
      "BPGD",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_RFU,
      0x80008b6,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Leaf Green (G)
      "POKEMON LEAF",              /* gamepak_title        */
      "BPGD",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RFU,  /* flags             */
      0x80008b6,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Leaf Green (I)
      "BPGI",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_RFU,
      0x80008b6,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Leaf Green (I)
      "POKEMON LEAF",              /* gamepak_title        */
      "BPGI",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RFU,  /* flags             */
      0x80008b6,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Leaf Green (F)
      "BPGF",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_RFU,
      0x80008b6,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Leaf Green (F)
      "POKEMON LEAF",              /* gamepak_title        */
      "BPGF",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RFU,  /* flags             */
      0x80008b6,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Leaf Green (J)
      "BPGJ",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_RFU,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Leaf Green (J)
      "POKEMON LEAF",              /* gamepak_title        */
      "BPGJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RFU,  /* flags             */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Pinball: Ruby & Sapphire (U)
      "BPPE",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_GBA_PLAYER,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Pinball (U)
      "POKEPIN R/S",               /* gamepak_title        */
      "BPPE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_GBA_PLAYER,            /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Pinball: Ruby & Sapphire (E)
      "BPPP",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_GBA_PLAYER,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Pinball (E)
      "POKEPIN R/S",               /* gamepak_title        */
      "BPPP",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_GBA_PLAYER,            /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Pinball: Ruby & Sapphire (J)
      "BPPJ",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_GBA_PLAYER,
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Pinball (J)
      "POKEPIN R/S",               /* gamepak_title        */
      "BPPJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_GBA_PLAYER,            /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Fushigi no Dungeon: Aka no Kyuujotai (J)
      "B24J",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Fushigi no Dungeon Aka no Kyuujotai (J)
      "POKE DUNGEON",              /* gamepak_title        */
      "B24J",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Mystery Dungeon: Red Rescue Team (U)
      "B24E",                      /* gamepak_code         */
      FLAGS_FLASH_128KB,           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Red Rescue Team (U)
      "POKE DUNGEON",              /* gamepak_title        */
      "B24E",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB,           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon Mystery Dungeon: Red Rescue Team (E)
      "B24P",                      /* gamepak_code         */
      FLAGS_FLASH_128KB,           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Pokemon: Red Rescue Team (E)
      "POKE DUNGEON",              /* gamepak_title        */
      "B24P",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB,           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Prince of Persia - The Sands of Time (U)
      "BPYE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Prince of Persia - The Sands of Time (U)
      "PRINCEPERSIA",              /* gamepak_title        */
      "BPYE",                      /* gamepak_code         */
      "41",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x808ff3a,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   
   {
      /* Badly coded game with several idle loops. I don't think any are
       * even close to dominant, and it jumps around too much when things
       * matter....
       */
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Rebelstar Tactical Command (U)
      "BRLE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Rebelstar Tactical Command (U)
      "REBELSTAR",                 /* gamepak_title        */
      "BRLE",                      /* gamepak_code         */
      "AF",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x800041a,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // River City Ransom EX (J)
      "BDTJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // River City Ransom EX (J)
      "DOWNTOWN",                  /* gamepak_title        */
      "BDTJ",                      /* gamepak_code         */
      "EB",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x800065a,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // River City Ransom EX (U)
      "BDTE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // River City Ransom EX (U)
      "RIVERCRANSOM",              /* gamepak_title        */
      "BDTE",                      /* gamepak_code         */
      "EB",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x800065a,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Rhythm Tengoku (J)
      "BRIJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Rhythm Tengoku (J)
      "RHYTHMTENGOK",              /* gamepak_title        */
      "BRIJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80013d4,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Rockman Zero (J)
      "ARZJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Rockman Zero (J)
      "ROCKMAN ZERO",              /* gamepak_title        */
      "ARZJ",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80004ee,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Robopon 2 Cross Version (U)
      "ACVE",                      /* gamepak_code         */
      /* Has a 512K flash */
#else /* GBSP_BACKEND_VARIANT */
      // Robopon 2 Cross Version (U)
      "ROBOPON2CROS",              /* gamepak_title        */
      "ACVE",                      /* gamepak_code         */
      "EB",                        /* gamepak_maker        */
      /* Has a 512K flash */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* flags                */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Robopon 2 Ring Version (U)
      "ARPE",                      /* gamepak_code         */
      /* Has a 512K flash */
#else /* GBSP_BACKEND_VARIANT */
      // Robopon 2 Ring Version (U)
      "ROBOPON2RING",              /* gamepak_title        */
      "ARPE",                      /* gamepak_code         */
      "EB",                        /* gamepak_maker        */
      /* Has a 512K flash */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* flags                */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Robot Ponkotto 2 - Cross Version (J)
      "ACVJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Robot Ponkotto 2 - Cross Version (J)
      "ROBOPON2CROS",              /* gamepak_title        */
      "ACVJ",                      /* gamepak_code         */
      "18",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Robot Ponkotto 2 - Ring Version (J)
      "ARPJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Robot Ponkotto 2 - Ring Version (J)
      "ROBOPON2RING",              /* gamepak_title        */
      "ARPJ",                      /* gamepak_code         */
      "18",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Rockman EXE 4.5 - Real Operation (J)
      "BR4J",                      /* gamepak_code         */
      FLAGS_RTC,                   /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // RockMan EXE 4.5 - Real Operation (J)
      "ROCKEXE4.5RO",              /* gamepak_title        */
      "BR4J",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      FLAGS_RTC,                   /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Sennen Kazoku (J)
      "BKAJ",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
#else /* GBSP_BACKEND_VARIANT */
      // Sennen Kazoku (J)
      "SENNENKAZOKU",              /* gamepak_title        */
      "BKAJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_RTC,  /* flags             */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Shining Soul (J)
      "AHUJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Shining Soul (J)
      "SHINING SOUL",              /* gamepak_title        */
      "AHUJ",                      /* gamepak_code         */
      "8P",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Shining Soul (U)
      "AHUE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Shining Soul (U)
      "SHINING SOUL",              /* gamepak_title        */
      "AHUE",                      /* gamepak_code         */
      "EB",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Shining Soul (E)
      "AHUP",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Shining Soul (E)
      "SHINING SOUL",              /* gamepak_title        */
      "AHUP",                      /* gamepak_code         */
      "8P",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Shining Soul 2 (J)
      "AU2J",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Shining Soul 2 (J)
      "SHININGSOUL2",              /* gamepak_title        */
      "AU2J",                      /* gamepak_code         */
      "8P",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Shining Soul 2 (U)
      "AU2E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Shining Soul 2 (U)
      "SHININGSOUL2",              /* gamepak_title        */
      "AU2E",                      /* gamepak_code         */
      "EB",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Shining Soul 2 (E)
      "AU2P",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Shining Soul 2 (E)
      "SHININGSOUL2",              /* gamepak_title        */
      "AU2P",                      /* gamepak_code         */
      "EB",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance (Demo/Kiosk)
      "AMZE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance (Demo/Kiosk)
      "SUPER MARIOA",              /* gamepak_title        */
      "AMZE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8001cf2,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance (E/U)
      "AMAE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance (E/U)
      "SUPER MARIOA",              /* gamepak_title        */
      "AMAE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8001cf2,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 2 (E)
      "AA2P",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 2 (E)
      "SUPER MARIOB",              /* gamepak_title        */
      "AA2P",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000534,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 2 (U)
      "AA2E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 2 (U)
      "SUPER MARIOB",              /* gamepak_title        */
      "AA2E",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000534,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 3 (J)
      "A3AJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 3 (J)
      "SUPER MARIOC",              /* gamepak_title        */
      "A3AJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8002ba4,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 3 (U)
      "A3AE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 3 (U)
      "SUPER MARIOC",              /* gamepak_title        */
      "A3AE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8002ba4,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 4 (J)
      "AX4J",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_GBA_PLAYER,
      0x8000732,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 4 (J)
      "SUPER MARIOD",              /* gamepak_title        */
      "AX4J",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_GBA_PLAYER,  /* flags      */
      0x8000732,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 4 (E)
      "AX4P",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_GBA_PLAYER,
      0x8000732,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 4 (E)
      "SUPER MARIOD",              /* gamepak_title        */
      "AX4P",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_GBA_PLAYER,  /* flags      */
      0x8000732,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 4 (U)
      "AX4E",                      /* gamepak_code         */
      FLAGS_FLASH_128KB | FLAGS_SERIAL | FLAGS_GBA_PLAYER,
      0x8000732,                   /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Super Mario Advance 4 (U)
      "SUPER MARIOD",              /* gamepak_title        */
      "AX4E",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_FLASH_128KB | FLAGS_GBA_PLAYER,  /* flags      */
      0x8000732,                   /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Shrek Super Slam (U)
      "B4UE",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Shrek Super Slam (U)
      "SUPER SLAM",                /* gamepak_title        */
      "B4UE",                      /* gamepak_code         */
      "52",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags      */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Shrek Super Slam (E)
      "B4UP",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Shrek Super Slam (E)
      "SUPER SLAM",                /* gamepak_title        */
      "B4UP",                      /* gamepak_code         */
      "52",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags      */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Super Puzzle Fighter II Turbo (E)
      "AZ8P",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Super Puzzle Fighter II Turbo (E)
      "PUZZLEFIGHT2",              /* gamepak_title        */
      "AZ8P",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8002b5e,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Super Puzzle Fighter II Turbo (U)
      "AZ8E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Super Puzzle Fighter II Turbo (U)
      "PUZZLEFIGHT2",              /* gamepak_title        */
      "AZ8E",                      /* gamepak_code         */
      "08",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8002b5e,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Tales of Phantasia (U)
      "AN8E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Tales of Phantasia (U)
      "PHANTASIA",                 /* gamepak_title        */
      "AN8E",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Tales of Phantasia (J)
      "AN8J",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Tales of Phantasia (J)
      "PHANTASIA",                 /* gamepak_title        */
      "AN8J",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Tales of Phantasia (E)
      "AN8P",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Tales of Phantasia (E)
      "PHANTASIA",                 /* gamepak_title        */
      "AN8P",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // The Pinball of The Dead
      "APDE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // The Pinball of The Dead
      "PINBALL DEAD",              /* gamepak_title        */
      "APDE",                      /* gamepak_code         */
      "78",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x800030,                    /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Tom Clancy's Splinter Cell (U)
      "A04E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Tom Clancy's Splinter Cell (U)
      "SPLINTERCELL",              /* gamepak_title        */
      "A04E",                      /* gamepak_code         */
      "41",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x807a0c4,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Lord of the Rings: the third Age (U)
      "B3AE",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Lord of the Rings: the third Age (U)
      "THIRD AGE",                 /* gamepak_title        */
      "B3AE",                      /* gamepak_code         */
      "69",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Tom Clancy's Splinter Cell: Pandora Tomorrow (U)
      "BSLE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Tom Clancy's Splinter Cell: Pandora Tomorrow (U)
      "TOM CLANCY'S",              /* gamepak_title        */
      "BSLE",                      /* gamepak_code         */
      "41",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x807785e,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Tottoko Hamutaro Hamuhamu Sports (J/U)
      "B85A",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Tottoko Hamutaro Hamuhamu Sports (J/U)
      "HAMSPORTS",                 /* gamepak_title        */
      "B85A",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Hamtaro Ham Ham Games (E)
      "B85P",                      /* gamepak_code         */
      FLAGS_SERIAL | FLAGS_RFU,    /* flags                */
      0,                           /* idle_loop_target_pc  */
#else /* GBSP_BACKEND_VARIANT */
      // Hamtaro Ham Ham Games (E)
      "HAMSPORTS",                 /* gamepak_title        */
      "B85P",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RFU,                   /* flags                */
      0,                           /* idle_loop_target_pc  */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Tringo (U)
      "BTJE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Tringo (U)
      "TRINGO",                    /* gamepak_title        */
      "BTJE",                      /* gamepak_code         */
      "42",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80009a4,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Ueki no Housoku Jingi Sakuretsu! Nouryokusya Battle (J)
      "BUHJ",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Ueki no Housoku Jingi Sakuretsu! Nouryokusya Battle (J)
      "UEKIJINGIBTL",              /* gamepak_title        */
      "BUHJ",                      /* gamepak_code         */
      "D9",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // V-Rally 3 (E)
      "AVRP",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // V-Rally 3 (E)
      "V_RALLY 3",                 /* gamepak_title        */
      "AVRP",                      /* gamepak_code         */
      "70",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80aa920,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Virtual Kasparov (U)
      "AVKE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Virtual Kasparov (U)
      "VIRTKASPAROV",              /* gamepak_title        */
      "AVKE",                      /* gamepak_code         */
      "60",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x800093a,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // WarioWare, Inc.: Mega Microgames (U)
      "AZWE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Wario Ware, Inc. Mega Microgames (U)
      "WARIOWAREINC",              /* gamepak_title        */
      "AZWE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8000f66,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // WarioWare: Twisted (U)
      "RZWE",                      /* gamepak_code         */
      FLAGS_RUMBLE,                /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Wario Ware, Twisted (U)
      "WARIOTWISTED",              /* gamepak_title        */
      "RZWE",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RUMBLE,                /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // WarioWare: Twisted (E)
      "RZWP",                      /* gamepak_code         */
      FLAGS_RUMBLE,                /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Wario Ware, Twisted (E)
      "WARIOTWISTED",              /* gamepak_title        */
      "RZWP",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RUMBLE,                /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
   {
      // Mawaru Made in Wario (J)
      "RZWJ",                      /* gamepak_code         */
      FLAGS_RUMBLE,                /* flags                */
#else /* GBSP_BACKEND_VARIANT */
   {
      // Wario Ware, Twisted (J)
      "MAWARUWARIO",               /* gamepak_title        */
      "RZWJ",                      /* gamepak_code         */
      "01",                        /* gamepak_maker        */
      FLAGS_RUMBLE,                /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Yu-Gi-Oh! - Dungeon Dice Monsters (U)
      "AYDE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Yu-Gi-Oh! - Dungeon Dice Monsters (U)
      "YU-GI-OH DDM",              /* gamepak_title        */
      "AYDE",                      /* gamepak_code         */
      "A4",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x802cc6a,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Yu-Gi-Oh! - The Eternal Duelist Soul (U)
      "AY5E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Yu-Gi-Oh! - The Eternal Duelist Soul (U)
      "YU-GI-OH!EDS",              /* gamepak_title        */
      "AY5E",                      /* gamepak_code         */
      "A4",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8075d96,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Yu-Gi-Oh! - The Sacred Cards (U)
      "AY7E",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Yu-Gi-Oh! - The Sacred Cards (U)
      "YUGIOH DM7",                /* gamepak_title        */
      "AY7E",                      /* gamepak_code         */
      "A4",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8003bd6,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Yu-Gi-Oh! - World Championship Tournament 2004 (U)
      "BYWE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Yu-Gi-Oh! - World Championship Tournament 2004 (U)
      "YWCT2004USA",               /* gamepak_title        */
      "BYWE",                      /* gamepak_code         */
      "A4",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x80831da,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Yu-Gi-Oh! - Worldwide Edition - Stairway to the Destined Duel (U)
      "AYWE",                      /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Yu-Gi-Oh! - Worldwide Edition - Stairway to the Destined Duel (U)
      "YUGIOHWWE",                 /* gamepak_title        */
      "AYWE",                      /* gamepak_code         */
      "A4",                        /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0x8089792,                   /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#if 0
   {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      // Sample
      "",                          /* gamepak_code         */
      0,                           /* flags                */
#else /* GBSP_BACKEND_VARIANT */
      // Sample
      "",                          /* gamepak_title        */
      "",                          /* gamepak_code         */
      "",                          /* gamepak_maker        */
      0,                           /* flags                */
#endif /* GBSP_BACKEND_VARIANT */
      0,                           /* idle_loop_target_pc  */
      0,                           /* translation_gate_target_1 */
      0,                           /* translation_gate_target_2 */
      0,                           /* translation_gate_target_3 */
   },
#endif
};
