#ifndef INTERFACE_CORE_TITLEBAR_H_
#define INTERFACE_CORE_TITLEBAR_H_

#include "commonControls.h"
#include "displayStructs.h"

class Interface;
class Battery;
class Synth;

class TitleBar
{
  public:
    void init(Synth* synth, Battery* battery);
    void initControls();
    void refreshColors();

    void show(bool showRightLabel = false);
    void hide();

    void setTitle(const char* text);

    // Width and font of the title label, for code that fits text into the title.
    static constexpr uint16_t kTitleWidth = 270;
    const strFont* getTitleFont() const;
    // Text too wide for widthLimit is cut and ends in kCutMarker. Returns how many characters of
    // text to keep: strlen(text) when it fits, else the length before the marker (no space left
    // before it).
    static constexpr const char* kCutMarker = "...";
    static uint16_t lengthBeforeCut(const strFont* font, const char* text, uint16_t widthLimit);
    void setRightLabel(const char* text);
    // picked: the current instrument is the one the user picked, shown with a "-> " prefix
    void showCurrentInstrumentOnRightLabel(bool picked = false);
    void setBatteryValue(int valuePercent, bool charging);

    void showRightLabel(bool show);
    void showBattery(bool show);

    void refresh();

    friend Interface;

  private:
    static constexpr uint32_t kTitleStyle =
      controlStyleCenterY | controlStyleFont4 | controlStyleLimitTextWidth;

    static constexpr int kPerformance1Treshold = 85;
    static constexpr int kPerformance2Treshold = 93;
    static constexpr int kPerformance3Treshold = 99;
    static constexpr int kBoxHideDelayLoopCount = 3;

    static constexpr int kBatteryNotPresent = -1;
    bool visible = false;

    hControl titleLabel = -1;
    hControl rightLabel = -1;
    hControl bar = -1;
    hControl battery = -1;
    hControl performBox = -1;

    strLabelData barLabelData;

    char timerText[50];

    Battery* batteryDriver;
    Synth* synth;

    bool lastBatteryCharging = false;
    int lastBatteryLevel = -1;

    uint32_t boxLastColor = 0;
    uint8_t boxShowLoopCount = 0;
};

extern TitleBar titleBar;

#endif /* INTERFACE_CORE_TITLEBAR_H_ */
