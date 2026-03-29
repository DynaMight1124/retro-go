# Celeste for ESP32-S3 / BreezyBox (fork of ccleste)

This is a port of [lemon32767/ccleste](https://github.com/lemon32767/ccleste) with a merged scrollable map from [Scrolleste](https://www.lexaloffle.com/bbs/?tid=41751) to ESP32-S3 running [BreezyBox](https://github.com/valdanylchuk/breezybox) and [breezy_rgb_lcd](https://github.com/valdanylchuk/breezy_rgb_lcd). See my [demo project](https://github.com/valdanylchuk/breezydemo) for a fully integrated example and a demo video. It also still supports SDL target on your PC, for easier testing and development.

## Installing / building

This is a hacky hobby project. Do not expect a plug-and-play experience, or much in the way of customer support. Basically you probably need to be at least hobbyist-level embedded developer to get it running.

### A. If you run a compatible configuration of BreezyBox:

This downloads and installs and ELF binary from the latest github release:

```
eget valdanylchuk/ccleste

celeste
```

Tested only on my board from that demo project. For anything else, you will likely need to fork/clone and modify the source code.

### B. Building for ESP32

```
./buildelf.sh
```

You will need xtensa-esp32s3-elf-gcc from the xtensa-esp-elf toolkit. [Espressif IDF Tools documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/tools/idf-tools.html) provides a good starting point on where to get and how to install that.

### C. SDL PC version

```
make
```

You need SDL 2.0 installed, for example via homebrew.

## License

This is free software, licensed under CC-BY-SA-4.0 (see [LICENSE](LICENSE) file).

All credit for the original game goes to the original developers (Maddy Thorson & Noel Berry).
