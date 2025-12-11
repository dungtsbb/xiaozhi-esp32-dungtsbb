# Custom Board Guide

This guide explains how to create a new board initialization for the Xiaozhi AI voice chatbot project. Xiaozhi AI supports more than 70 ESP32-family boards, and each board’s initialization code lives in its own directory.

## Important Notice

> **Warning**: For custom boards, if your IO mapping differs from an existing board, do not overwrite that board’s config and build firmware directly. Create a new board type, or use different `name` and `sdkconfig` macros in `config.json`’s `builds` section. Use `python scripts/release.py [board_dir_name]` to build and package firmware.
>
> If you overwrite an existing config, future OTA upgrades may replace your custom firmware with the standard firmware for that board, breaking your device. Each board has a unique identifier and firmware upgrade channel—keep the identifier unique.

## Directory Layout

Each board directory typically contains:

- `xxx_board.cc` - Main board initialization code and board-specific logic
- `config.h` - Board config, defining pin mappings and other settings
- `config.json` - Build config, specifying target chip and special build options
- `README.md` - Documentation for the board

## Steps to Create a Custom Board

### 1. Create a new board directory

In `boards/`, create a new directory named `[brand]-[board-type]`, for example `m5stack-tab5`:

```bash
mkdir main/boards/my-custom-board
```

### 2. Create config files

#### config.h

Define all hardware settings in `config.h`, including:

- Audio sample rate and I2S pin mapping
- Audio codec address and I2C pin mapping
- Button and LED pins
- Display parameters and pins

Reference example (from lichuang-c3-dev):

```c
#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// Audio settings
#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_10
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_12
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_8
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_7
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_11

#define AUDIO_CODEC_PA_PIN       GPIO_NUM_13
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_0
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_1
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR

// Button settings
#define BOOT_BUTTON_GPIO        GPIO_NUM_9

// Display settings
#define DISPLAY_SPI_SCK_PIN     GPIO_NUM_3
#define DISPLAY_SPI_MOSI_PIN    GPIO_NUM_5
#define DISPLAY_DC_PIN          GPIO_NUM_6
#define DISPLAY_SPI_CS_PIN      GPIO_NUM_4

#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY true

#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0

#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_2
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT true

#endif // _BOARD_CONFIG_H_
```

#### config.json

Define build settings in `config.json`. This file is used by `scripts/release.py` for automated builds:

```json
{
    "target": "esp32s3",  // Target chip model: esp32, esp32s3, esp32c3, esp32c6, esp32p4, etc.
    "builds": [
        {
            "name": "my-custom-board",  // Board name, used for firmware packaging
            "sdkconfig_append": [
                // Special flash size config
                "CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y",
                // Custom partition table
                "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=\"partitions/v2/8m.csv\""
            ]
        }
    ]
}
```

**Config item notes:**
- `target`: Chip model; must match your hardware
- `name`: Firmware package name; usually matches the directory name
- `sdkconfig_append`: Extra sdkconfig entries appended to defaults

**Common `sdkconfig_append` entries:**
```json
// Flash size
"CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y"   // 4MB flash
"CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y"   // 8MB flash
"CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y"  // 16MB flash

// Partition table
"CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=\"partitions/v2/4m.csv\""  // 4MB partition table
"CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=\"partitions/v2/8m.csv\""  // 8MB partition table
"CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=\"partitions/v2/16m.csv\"" // 16MB partition table

// Language
"CONFIG_LANGUAGE_EN_US=y"  // English
"CONFIG_LANGUAGE_ZH_CN=y"  // Simplified Chinese

// Wake word / AEC
"CONFIG_USE_DEVICE_AEC=y"          // Enable on-device AEC
"CONFIG_WAKE_WORD_DISABLED=y"      // Disable wake word
```

### 3. Write board initialization code

Create `my_custom_board.cc` and implement all board initialization.

A basic board class includes:

1. **Class definition**: Inherit from `WifiBoard` or `Ml307Board`
2. **Init functions**: Initialize I2C, display, buttons, IoT, etc.
3. **Virtual overrides**: e.g. `GetAudioCodec()`, `GetDisplay()`, `GetBacklight()`
3. **Register the board**: Use `DECLARE_BOARD`

```cpp
#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>

#define TAG "MyCustomBoard"

class MyCustomBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t codec_i2c_bus_;
    Button boot_button_;
    LcdDisplay* display_;

    // I2C init
    void InitializeI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_));
    }

    // SPI init (for display)
    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_SPI_SCK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    // Button init
    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

    // Display init (ST7789 example)
    void InitializeDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;
        
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = 2;
        io_config.pclk_hz = 80 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io));

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = GPIO_NUM_NC;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));
        
        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, true);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        
        // Create display object
        display_ = new SpiLcdDisplay(panel_io, panel,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT, 
                                    DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, 
                                    DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    // MCP tools init
    void InitializeTools() {
        // See MCP docs
    }

public:
    // Constructor
    MyCustomBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeI2c();
        InitializeSpi();
        InitializeDisplay();
        InitializeButtons();
        InitializeTools();
        GetBacklight()->SetBrightness(100);
    }

    // Get audio codec
    virtual AudioCodec* GetAudioCodec() override {
        static Es8311AudioCodec audio_codec(
            codec_i2c_bus_, 
            I2C_NUM_0, 
            AUDIO_INPUT_SAMPLE_RATE, 
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, 
            AUDIO_I2S_GPIO_BCLK, 
            AUDIO_I2S_GPIO_WS, 
            AUDIO_I2S_GPIO_DOUT, 
            AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN, 
            AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }

    // Get display
    virtual Display* GetDisplay() override {
        return display_;
    }
    
    // Get backlight control
    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }
};

// Register board
DECLARE_BOARD(MyCustomBoard);
```

### 4. Add build-system config

#### Add board option to Kconfig.projbuild

Open `main/Kconfig.projbuild` and add a new board option under `choice BOARD_TYPE`:

```kconfig
choice BOARD_TYPE
    prompt "Board Type"
    default BOARD_TYPE_BREAD_COMPACT_WIFI
    help
        Board type. Select your board
    
    # ... other board options ...
    
    config BOARD_TYPE_MY_CUSTOM_BOARD
        bool "My Custom Board (custom board)"
        depends on IDF_TARGET_ESP32S3  # Adjust for your target chip
endchoice
```

**Notes:**
- `BOARD_TYPE_MY_CUSTOM_BOARD` must be uppercase with underscores
- `depends on` should match the target chip (e.g. `IDF_TARGET_ESP32S3`, `IDF_TARGET_ESP32C3`, etc.)
- Description can be bilingual if needed

#### Add board config to CMakeLists.txt

Open `main/CMakeLists.txt` and extend the board selection chain:

```cmake
# Add your board config to the elseif chain
elseif(CONFIG_BOARD_TYPE_MY_CUSTOM_BOARD)
    set(BOARD_TYPE "my-custom-board")  # Match the directory name
    set(BUILTIN_TEXT_FONT font_puhui_basic_20_4)  # Choose font size based on screen size
    set(BUILTIN_ICON_FONT font_awesome_20_4)
    set(DEFAULT_EMOJI_COLLECTION twemoji_64)  # Optional if you need emoji
endif()
```

**Fonts and emoji hints:**

Choose fonts by screen resolution:
- Small screens (128x64 OLED): `font_puhui_basic_14_1` / `font_awesome_14_1`
- Small/medium (240x240): `font_puhui_basic_16_4` / `font_awesome_16_4`
- Medium (240x320): `font_puhui_basic_20_4` / `font_awesome_20_4`
- Large (480x320+): `font_puhui_basic_30_4` / `font_awesome_30_4`

Emoji set options:
- `twemoji_32` - 32x32 emojis (small screens)
- `twemoji_64` - 64x64 emojis (large screens)

### 5. Configure and build

#### Option 1: Manual config with idf.py

1. **Set target chip** (first time or when changing chips):
   ```bash
   # For ESP32-S3
   idf.py set-target esp32s3
   
   # For ESP32-C3
   idf.py set-target esp32c3
   
   # For ESP32
   idf.py set-target esp32
   ```

2. **Clean old config**:
   ```bash
   idf.py fullclean
   ```

3. **Open menuconfig**:
   ```bash
   idf.py menuconfig
   ```
   
   Navigate to `Xiaozhi Assistant` -> `Board Type` and choose your custom board.

4. **Build and flash**:
   ```bash
   idf.py build
   idf.py flash monitor
   ```

#### Option 2: Use release.py (recommended)

If your board directory has `config.json`, you can automate config and build:

```bash
python scripts/release.py my-custom-board
```

The script will:
- Read `target` from `config.json` and set the chip
- Apply `sdkconfig_append` options
- Build and package firmware

### 6. Create README.md

Document board features, hardware requirements, build, and flash steps in `README.md`.


## Common board components

### 1. Displays

Supported display drivers include:
- ST7789 (SPI)
- ILI9341 (SPI)
- SH8601 (QSPI)
- etc.

### 2. Audio codecs

Supported codecs:
- ES8311 (common)
- ES7210 (mic array)
- AW88298 (power amplifier)
- etc.

### 3. Power management

Some boards use PMICs:
- AXP2101
- Other supported PMICs

### 4. MCP devices

You can add MCP tools for AI control:
- Speaker (speaker control)
- Screen (screen brightness)
- Battery (battery reading)
- Light (lighting control)
- etc.

## Board class inheritance

- `Board` - Base class
  - `WifiBoard` - Wi-Fi boards
  - `Ml307Board` - Boards using 4G module
  - `DualNetworkBoard` - Boards supporting Wi-Fi/4G switching

## Tips

1. **Reference similar boards**: Reuse patterns from boards that resemble yours
2. **Iterate in steps**: Bring up basics (e.g. display) before adding complex features (e.g. audio)
3. **Pin mapping**: Double-check pin definitions in `config.h`
4. **Compatibility**: Confirm all chips and drivers are supported

## Possible issues

1. **Display issues**: Check SPI setup, mirror settings, and color inversion
2. **No audio**: Verify I2S config, PA enable pin, and codec address
3. **Cannot connect to network**: Check Wi-Fi credentials and network config
4. **Cannot talk to server**: Check MQTT or WebSocket settings

## References

- ESP-IDF docs: https://docs.espressif.com/projects/esp-idf/
- LVGL docs: https://docs.lvgl.io/
- ESP-SR docs: https://github.com/espressif/esp-sr 
