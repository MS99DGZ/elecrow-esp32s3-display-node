
#define LGFX_USE_V1

// === Includes ===
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/touch/Touch_GT911.hpp>
#include <SPI.h>
#include <SD.h>

// === Hardware Configuration ===

// === Backlight control pin ===
static constexpr int PIN_BL = 2;

// === GT911 I2C Pins ===
static constexpr int TP_SDA = 19;
static constexpr int TP_SCL = 20;

// === SD Card SPI Pins ===
static constexpr int SD_MOSI = 11;
static constexpr int SD_MISO = 13;
static constexpr int SD_SCK = 12;
static constexpr int SD_CS = 10;

// === SD Status Flag ===
static bool sdOK = false;

// === Environment Profiles ===

enum class Profile
{
    P1 = 0,
    P2,
    P3,
    P4,
    P5,
    Count
};

struct ProfileSettings
{
    float targetVpdKPa;
    float minTempC;
    float maxTempC;
    float baseHumidityPercent;
};

// --- Profile defaults ---
// Each block is intentionally verbose and commented so it's easy to tweak later.

// Profile 1
static const ProfileSettings PROFILE_P1 =
    {
        0.8f,  // targetVpdKPa
        20.0f, // mindTempC
        24.0f, // maxTempC
        70.0f, // baseHumidityPercent
};

// Profile 2
static const ProfileSettings PROFILE_P2 =
    {
        1.0f,  // targetVpdKPa
        22.0f, // mindTempC
        26.0f, // maxTempC
        65.0f, // baseHumidityPercent
};

// Profile 3
static const ProfileSettings PROFILE_P3 =
    {
        1.2f,  // targetVpdKPa
        22.0f, // mindTempC
        26.0f, // maxTempC
        60.0f, // baseHumidityPercent
};

// Profile 4
static const ProfileSettings PROFILE_P4 =
    {
        1.3f,  // targetVpdKPa
        22.0f, // mindTempC
        26.0f, // maxTempC
        55.0f, // baseHumidityPercent
};

// Profile 5
static const ProfileSettings PROFILE_P5 =
    {
        1.5f,  // targetVpdKPa
        22.0f, // mindTempC
        26.0f, // maxTempC
        50.0f, // baseHumidityPercent
};

// === Table that collects all profiles in order P1–P5 ===

static const ProfileSettings PROFILE_TABLE[static_cast<std::size_t>(Profile::Count)] =
    {
        PROFILE_P1,
        PROFILE_P2,
        PROFILE_P3,
        PROFILE_P4,
        PROFILE_P5};

// === Currently active environment profile ===
static Profile currentProfile = Profile::P1;

// Helper to access the configuration of a given profile.
static const ProfileSettings &getProfileSettings(Profile profile)
{
    const auto index = static_cast<std::size_t>(profile);
    return PROFILE_TABLE[index];
}

// === Page State ===
// Order chosen to match the original project: Main -> Logs -> Modules -> Camera
enum class Page
{
    Main,
    Logs,
    Modules,
    Camera
};

static Page currentPage = Page::Main;
static bool pageInitialized = false;

// === Display Driver Class ===
class LGFX : public lgfx::LGFX_Device
{
public:
    lgfx::Bus_RGB bus;
    lgfx::Panel_RGB panel;
    lgfx::Touch_GT911 touch;

    LGFX()
    {
        // --- Touch configuration ---
        {
            auto cfg = touch.config();
            cfg.i2c_port = 0;
            cfg.pin_sda = TP_SDA;
            cfg.pin_scl = TP_SCL;
            cfg.pin_int = -1;
            cfg.pin_rst = -1;
            touch.config(cfg);
            panel.setTouch(&touch);
        }

        // --- RGB Bus configuration ---
        {
            auto cfg = bus.config();
            cfg.panel = &panel;

            cfg.pin_d0 = GPIO_NUM_8;
            cfg.pin_d1 = GPIO_NUM_3;
            cfg.pin_d2 = GPIO_NUM_46;
            cfg.pin_d3 = GPIO_NUM_9;
            cfg.pin_d4 = GPIO_NUM_1;
            cfg.pin_d5 = GPIO_NUM_5;
            cfg.pin_d6 = GPIO_NUM_6;
            cfg.pin_d7 = GPIO_NUM_7;
            cfg.pin_d8 = GPIO_NUM_15;
            cfg.pin_d9 = GPIO_NUM_16;
            cfg.pin_d10 = GPIO_NUM_4;
            cfg.pin_d11 = GPIO_NUM_45;
            cfg.pin_d12 = GPIO_NUM_48;
            cfg.pin_d13 = GPIO_NUM_47;
            cfg.pin_d14 = GPIO_NUM_21;
            cfg.pin_d15 = GPIO_NUM_14;

            cfg.pin_henable = GPIO_NUM_40;
            cfg.pin_vsync = GPIO_NUM_41;
            cfg.pin_hsync = GPIO_NUM_39;
            cfg.pin_pclk = GPIO_NUM_0;

            cfg.freq_write = 15000000;

            cfg.hsync_front_porch = 8;
            cfg.hsync_pulse_width = 4;
            cfg.hsync_back_porch = 43;

            cfg.vsync_front_porch = 8;
            cfg.vsync_pulse_width = 4;
            cfg.vsync_back_porch = 12;

            bus.config(cfg);
        }

        // --- Panel configuration ---
        {
            auto cfg = panel.config();
            cfg.memory_width = 800;
            cfg.memory_height = 480;
            cfg.panel_width = 800;
            cfg.panel_height = 480;
            panel.config(cfg);
        }

        panel.setBus(&bus);
        setPanel(&panel);
    }
};

// === Global Display instance ===
static LGFX lcd;

// === Forward Declarations ===
// Core init
void initSerial();
void initBacklight();
void initDisplay();
void initSDCard();

// UI / page system
void initPageSystem();
void setPage(Page page);
void drawCurrentPage();

// UI drawing helpers
void drawBootScreen();

// Main page
void drawMainPage();
void drawMainHeader();
void drawProfilePills();

// UI drawing helpers
void drawLogsPagePlaceholder();
void drawModulesPagePlaceholder();
void drawCameraPagePlaceholder();

// === Setup ===
void setup()
{
    initSerial();
    initBacklight();
    initDisplay();
    initSDCard();
    initPageSystem(); // use the page system (bootscreen + first page)
}

void loop()
{
    // Placeholder main loop.
    // Later this will host touch handling and incremental UI updates.
    delay(16);
}

// === Initialization Functions ===

void initSerial()
{
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[BOOT] Display node starting...");
}

void initBacklight()
{
    pinMode(PIN_BL, OUTPUT);
    digitalWrite(PIN_BL, HIGH); // Turn on backlight
}

void initDisplay()
{
    lcd.begin();
    lcd.setBrightness(255); // Set initial brightness (0-255)
    lcd.setRotation(0);
    lcd.fillScreen(TFT_BLACK);
}

void initSDCard()
{
    Serial.println("[SD] Initializing...");

    SPI.begin(SD_SCK, SD_MISO, SD_MOSI);

    if (!SD.begin(SD_CS, SPI, 25000000U))
    {
        Serial.println("[SD] Initialization failed!");
        sdOK = false;
        return;
    }

    Serial.println("[SD] Initialization successful");
    sdOK = true;
}

// === Page System ===

void initPageSystem()
{
    // Boot screen for 2 seconds
    drawBootScreen();
    delay(2000);

    setPage(Page::Main);
}

void setPage(Page page)
{
    // Only skip redraw if we already initialized at least once
    // AND the page is the same. The very first call should always draw.
    if (pageInitialized && currentPage == page)
    {
        return;
    }

    currentPage = page;
    pageInitialized = true;
    drawCurrentPage();
}

void drawCurrentPage()
{
    // Clear screen before drawing a new page
    lcd.fillScreen(TFT_BLACK);

    switch (currentPage)
    {
    case Page::Main:
        drawMainPage();
        break;

    case Page::Logs:
        drawLogsPagePlaceholder();
        break;

    case Page::Modules:
        drawModulesPagePlaceholder();
        break;

    case Page::Camera:
        drawCameraPagePlaceholder();
        break;
    }
}

// === UI Drawing Helpers ===

void drawBootScreen()
{
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextDatum(middle_center);
    lcd.setTextSize(2);

    lcd.drawString("MEC Display Node", lcd.width() / 2, lcd.height() / 2);

    lcd.setTextSize(1);

    if (sdOK)
    {
        lcd.drawString("SD card OK", lcd.width() / 2, lcd.height() / 2 + 24);
    }
    else
    {
        lcd.drawString("SD card not detected", lcd.width() / 2, lcd.height() / 2 + 24);
    }
}

void drawMainHeader()
{
    lcd.setTextDatum(top_left);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextSize(2);

    const int16_t marginX = 16;
    const int16_t marginY = 12;

    lcd.drawString("Main", marginX, marginY);

    lcd.setTextSize(2);

    // === Explain Subtitle ===
    lcd.drawString("Enviroment Controller - Profile overview", marginX, marginY + 28);

    const char *profileLable = nullptr;
    switch (currentProfile)
    {
    case Profile::P1:
        profileLable = "Profile P1";
        break;
    case Profile::P2:
        profileLable = "Profile P2";
        break;
    case Profile::P3:
        profileLable = "Profile P3";
        break;
    case Profile::P4:
        profileLable = "Profile P4";
        break;
    case Profile::P5:
        profileLable = "Profile P5";
        break;
    default:
        profileLable = "Profile ?";
        break;
    }

    lcd.setTextDatum(top_right);
    lcd.drawString(profileLable, lcd.width() - marginX, marginY + 8);

    // Reset datum to a sane default for other drawing routines.
    lcd.setTextDatum(top_left);
}

void drawProfilePills()
{
// Draw five pills (P1–P5) horizontally.
// The currently active profile is highlighted.

const int16_t areaTop = 56;
const int16_t areaHeight = 40;
const int16_t marginX = 16;
const int16_t spacing = 8;

const std::size_t profileCount = static_cast<std::size_t>(Profile::Count);
const int16_t totalWidth = lcd.width() - 2 * marginX;
const int16_t pillWidth = (totalWidth - (profileCount - 1) * spacing) / profileCount;
const int16_t pillHeight = areaHeight;

lcd.setTextSize(1);
lcd.setTextDatum(middle_center);

for(std::size_t i = 0; i < profileCount; ++i)
{
    const int16_t x = marginX + i * (pillWidth + spacing);
    const int16_t y = areaTop;

    const int16_t centerX = x + pillWidth / 2;
    const int16_t centerY = y + pillHeight / 2;

    const Profile profile = static_cast<Profile>(i);
    const bool isActive = (profile == currentProfile);

    // Build label "P1" ... "P5"
    char label[4] = {'P', static_cast<char>('1' + i), '\0', '\0'};

    if (isActive)
    {
        lcd.fillRoundRect(x, y, pillWidth, pillHeight, 8, TFT_WHITE);
        lcd.setTextColor(TFT_BLACK, TFT_WHITE);
        lcd.drawString(label, centerX, centerY);
        lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    else
    {
        lcd.drawRoundRect(x, y, pillWidth, pillHeight, 8, TFT_WHITE);
        lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        lcd.drawString(label, centerX, centerY);
    }

    // Reset datum after drawing.
    lcd.setTextDatum(top_left);
  }
}

void drawMainPage()
{
    drawMainHeader();
    drawProfilePills();
}

void drawLogsPagePlaceholder()
{
    lcd.setTextDatum(top_center);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextSize(2);

    lcd.drawString("Logs Page", lcd.width() / 2, 16);

    lcd.setTextSize(1);
    lcd.drawString("Here the Logs UI will be implemented.", lcd.width() / 2, 48);
}

void drawModulesPagePlaceholder()
{
    lcd.setTextDatum(top_center);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextSize(2);

    lcd.drawString("Modules Page", lcd.width() / 2, 16);

    lcd.setTextSize(1);
    lcd.drawString("Here the Modules UI will be implemented.", lcd.width() / 2, 48);
}

void drawCameraPagePlaceholder()
{
    lcd.setTextDatum(top_center);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextSize(2);

    lcd.drawString("Camera Page", lcd.width() / 2, 16);

    lcd.setTextSize(1);
    lcd.drawString("Here the Camera UI will be implemented.", lcd.width() / 2, 48);
}