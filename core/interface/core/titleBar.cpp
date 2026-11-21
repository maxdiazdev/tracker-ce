#include "titleBar.h"
#include "core/interfaceDefs.h"
#include "display.h"

#include <cstring>

#if GUI_HANDLE_SYNTH
#include "Profiler.h"
#include "Synth.h"
#endif

TitleBar titleBar;

static elapsedMillis refreshTimer;

#ifdef TRACKER_MINI
#define BATTERY_ENABLED 1
#endif

#ifdef BATTERY_ENABLED
static constexpr int16_t kRightLabelX = 790 - 28; // right edge, left of the battery
#else
static constexpr int16_t kRightLabelX = 790; // right edge
#endif
static constexpr uint32_t kRightLabelStyle =
  controlStyleRightX | controlStyleCenterY | controlStyleFont4;
// the right label is cut with "..." where it would reach the title area (plus a 10 px gap)
static constexpr uint16_t kRightLabelWidth = kRightLabelX - (9 + TitleBar::kTitleWidth) - 10;

void TitleBar::init(Synth* synth, [[maybe_unused]] Battery* battery)
{
    this->synth = synth;
#ifdef BATTERY_ENABLED
    batteryDriver = battery;
    lastBatteryLevel = batteryDriver->getState() ? batteryDriver->getLevel() : kBatteryNotPresent;
#else
    batteryDriver = nullptr;
#endif
}

void TitleBar::initControls()
{
    strControlProperties prop;
    prop.x = 9;
    prop.y = 13;
    prop.w = kTitleWidth;
    prop.colors = display.getTheme().topBarLabelColors;
    prop.style = kTitleStyle;
    titleLabel = display.createControl<cLabel>(&prop);

    prop.y = 13;
    prop.x = kRightLabelX;
    prop.colors = display.getTheme().topBarLabelColors;
    prop.style = kRightLabelStyle;
    rightLabel = display.createControl<cLabel>(&prop);

#ifdef BATTERY_ENABLED
    prop.style = controlStyleShowBitmap | controlStyleCenterY;
    prop.colors = display.getTheme().batteryIconColors;
    prop.x = 795 - 28;
    prop.y = 13;
    prop.w = 28;
    prop.h = 13;
    prop.value = lastBatteryLevel;
    battery = display.createControl<cBattery>(&prop);
#endif

    prop.style = 0;
    prop.colors = &display.getTheme().red;
    prop.x = 297;
    prop.y = 0;
    prop.w = 170;
    prop.h = 1;
    prop.value = 0;
    performBox = display.createControl<SimpleBox>(&prop);

    barLabelData.line.color = display.getTheme().topBarLabelColors[2];
    barLabelData.line.x1 = 0;
    barLabelData.line.x2 = 800;
    barLabelData.line.y1 = 28;
    barLabelData.line.y2 = 28;

    prop.style = (controlStyleBackground | controlStyleSpecialFx);
    prop.colors = display.getTheme().topBarLabelColors;
    prop.x = 0;
    prop.y = 0;
    prop.w = 800;
    prop.h = 28;
    prop.value = 0;
    prop.data = &barLabelData;
    bar = display.createControl<cLabel>(&prop);
}

void TitleBar::refreshColors()
{
    display.setControlColors(titleLabel, display.getTheme().topBarLabelColors);
    display.setControlColors(rightLabel, display.getTheme().topBarLabelColors);
#ifdef BATTERY_ENABLED
    display.setControlColors(battery, display.getTheme().batteryIconColors);
#endif
    display.setControlColors(performBox, &display.getTheme().red);
    barLabelData.line.color = display.getTheme().topBarLabelColors[2];
    display.setControlColors(bar, display.getTheme().topBarLabelColors);
}

void TitleBar::show(bool showRightLabel)
{
    display.setControlShow(titleLabel);
    display.refreshControl(titleLabel);

    if (showRightLabel)
    {
        display.setControlShow(rightLabel);
        display.refreshControl(rightLabel);
    }
    else
    {
        display.setControlHide(rightLabel);
    }

#ifdef BATTERY_ENABLED
    display.setControlShow(battery);
    display.refreshControl(battery);
#endif

    display.setControlShow(bar);
    display.refreshControl(bar);

    visible = true;
}

const strFont* TitleBar::getTitleFont() const
{
    const uint32_t style = kTitleStyle; // read by FONT_INDEX_FROM_STYLE
    return &fonts[FONT_INDEX_FROM_STYLE];
}

void TitleBar::setTitle(const char* text)
{
    display.setControlText(titleLabel, text);
    display.refreshControl(titleLabel);
}

void TitleBar::setRightLabel(const char* text)
{
    display.setControlText(rightLabel, text);
    display.refreshControl(rightLabel);
}

uint16_t TitleBar::lengthBeforeCut(const strFont* font, const char* text, uint16_t widthLimit)
{
    const uint16_t length = strlen(text);
    if (getTextWidth(font, text, length) <= widthLimit)
    {
        return length;
    }

    int keep = getMaxTextLengthThatFitSpecificWidth(
      font, text, widthLimit - getTextWidth(font, kCutMarker, strlen(kCutMarker)));
    while (keep > 0 && text[keep - 1] == ' ')
    {
        keep--; // no space before "..."
    }
    return keep;
}

void TitleBar::showCurrentInstrumentOnRightLabel(bool picked)
{
    // "-> " marks the picked instrument; a name too wide for the label is cut and marked "..."
    static constexpr const char* kPickedMarker = "-> ";
    // "-> " + "48. " + a full sample name, with room for "..."
    static char shownName[SAMPLE_NAME_SIZE + 16];
    shownName[0] = 0;
    const char* prefix = picked ? kPickedMarker : "";

    uint8_t idx = mtProject.values.lastUsedInstrument;

    if (idx >= SYNTH_INSTRUMENTS_START_IDX)
    {
#if GUI_HANDLE_SYNTH
        assert(synth);
        assert((idx - SYNTH_INSTRUMENTS_START_IDX) < kSynthInstrumentsCount);
        snprintf(shownName,
                 sizeof(shownName),
                 "%s%s",
                 prefix,
                 synth->getSynthName((idx - SYNTH_INSTRUMENTS_START_IDX)).c_str());
#endif
    }
    else if (idx >= MIDI_INSTRUMENTS_START_IDX)
    {
        snprintf(shownName,
                 sizeof(shownName),
                 "%s%d. MIDI Channel %d",
                 prefix,
                 idx + 3,
                 (idx - MIDI_INSTRUMENTS_START_IDX) + 1);
    }
    else
    {
        snprintf(shownName,
                 sizeof(shownName),
                 "%s%d. %s",
                 prefix,
                 idx + 1,
                 mtProject.instrument[idx].isActive ? mtProject.instrument[idx].sample.file_name
                                                    : "Empty");
    }

    const uint32_t style = kRightLabelStyle; // read by FONT_INDEX_FROM_STYLE
    const strFont* font = &fonts[FONT_INDEX_FROM_STYLE];
    const uint16_t keep = lengthBeforeCut(font, shownName, kRightLabelWidth);
    if (shownName[keep] != 0)
    {
        strcpy(&shownName[keep], kCutMarker); // keep < strlen, so "..." fits the buffer
    }

    display.setControlText(rightLabel, shownName);
    display.setControlShow(rightLabel);
    display.refreshControl(rightLabel);
}

void TitleBar::setBatteryValue([[maybe_unused]] int valuePercent, [[maybe_unused]] bool charging)
{
#ifdef BATTERY_ENABLED
    if (charging)
    {
        display.setAddControlStyle(battery, controlStyleTransformImg);
    }
    else
    {
        display.setRemoveControlStyle(battery, controlStyleTransformImg);
    }

    display.setControlValue(battery, valuePercent);
    display.refreshControl(battery);
#endif
}

void TitleBar::showRightLabel(bool show)
{
    if (show)
    {
        display.setControlShow(rightLabel);
    }
    else
    {
        display.setControlHide(rightLabel);
    }
    display.refreshControl(rightLabel);
}

void TitleBar::showBattery(bool show)
{
    if (show)
    {
        display.setControlShow(battery);
    }
    else
    {
        display.setControlHide(battery);
    }
    display.refreshControl(battery);
}

void TitleBar::hide()
{
    display.setControlHide(titleLabel);
    // display.refreshControl(titleLabel);

    display.setControlHide(rightLabel);
    // display.refreshControl(rightLabel);

#ifdef BATTERY_ENABLED
    display.setControlHide(battery);
    // display.refreshControl(battery);
#endif

    display.setControlHide(bar);
    display.refreshControl(bar);

    visible = false;
}

void TitleBar::refresh()
{
    if (refreshTimer < 300)
    {
        return;
    }

    refreshTimer = 0;

#if GUI_HANDLE_SYNTH
    Profiler::AudioProcessorUsage usage;
    Profiler::getInstance()->getAudioProcessingUsagePercent(usage, true);

    bool showBox = false;
    uint32_t boxColor = 0;

    if (usage.tempMaximum >= kPerformance1Treshold)
    {
        showBox = true;
        boxShowLoopCount = 0;

        if (usage.tempMaximum >= kPerformance3Treshold)
        {
            boxColor = display.getTheme().red;
        }
        else if (usage.tempMaximum >= kPerformance2Treshold)
        {
            boxColor = display.getTheme().orange;
        }
        else if (usage.tempMaximum >= kPerformance1Treshold)
        {
            boxColor = display.getTheme().yellow;
        }
    }

    if (showBox && boxLastColor != boxColor)
    {
        boxLastColor = boxColor;
        display.setControlColors(performBox, &boxLastColor);
        display.setControlShow(performBox);
        display.refreshControl(performBox);
    }
    else if (!showBox && boxLastColor)
    {
        if (boxShowLoopCount >= kBoxHideDelayLoopCount)
        {
            display.setControlHide(performBox);
            display.refreshControl(performBox);
            boxLastColor = 0;
        }
        else
        {
            boxShowLoopCount++;
        }
    }
#endif

#ifdef BATTERY_ENABLED
    if (!visible)
    {
        return;
    }

    bool state = batteryDriver->getState();
    int level = state ? batteryDriver->getLevel() : kBatteryNotPresent;
    bool charging = state ? batteryDriver->getCharging() : false;

    if (lastBatteryLevel != level || charging != lastBatteryCharging)
    {
        lastBatteryCharging = charging;
        lastBatteryLevel = level;
        setBatteryValue(lastBatteryLevel, lastBatteryCharging);
    }
#endif
}
