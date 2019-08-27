

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include "Parola_Fonts_data.h"
#include <Font_Data.h>

// TODO: remove this
// typedef int uint32_t;

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN 14
#define DATA_PIN 23
#define CS_PIN 15

#define BOOT_TEXT "Warming up"
#define TIMEOUT_PERIOD 9000

enum di_display_states
{
    BOOTING,
    CONNECTING,
    COUNTER,
    MESSAGE,
    ERROR
};
enum di_display_type
{
    DOTMATRIX
};
enum di_display_mode
{
    STANDARD_MODE,
    VINTAGE_MODE,
    SCROLL_MODE,
    BETA_MODE
};
MD_Parola ParolaDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

class DigitalIconDisplay
{
public:
    uint32_t counter_value;
    char display_text[100];
    enum di_display_states display_state;
    enum di_display_type display_type;
    enum di_display_mode display_mode;

private:
    uint32_t current_counter_value;
    uint32_t target_counter_value;
    enum di_display_states last_frame_display_state;
    unsigned long timeout_start_millis;
    bool text_refresh = false;

public:
    DigitalIconDisplay();
    int setupIcon();
    int updateCounterValue(uint32_t new_counter_value);
    int updateCounterValue(String new_counter_value);
    int showCustomMessage(char *custom_text);
    int updateDisplayState(di_display_states updated_state);
    int updateDisplayMode(di_display_mode updated_mode);
    void loop();

private:
    int refreshScreenWithText();
    int refreshScreenWithCounter();
};

DigitalIconDisplay::DigitalIconDisplay() 
{
    counter_value = 0;
    display_state = BOOTING;
    last_frame_display_state = BOOTING;
    display_type = DOTMATRIX;
    display_mode = STANDARD_MODE;
}

int DigitalIconDisplay::setupIcon()
{
    ParolaDisplay.begin();
    ParolaDisplay.setInvert(false);
    ParolaDisplay.displayText(BOOT_TEXT, PA_CENTER, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);
    ParolaDisplay.displayAnimate();
    return 1;
}

int DigitalIconDisplay::updateDisplayState(di_display_states updated_state)
{
    last_frame_display_state = display_state;

    display_state = updated_state;
    switch (updated_state)
    {
    case BOOTING:
        /* code */
        break;

    case CONNECTING:
        /* code */
        break;

    case COUNTER:
        /* code */
        break;

    case MESSAGE:
        timeout_start_millis = millis();
        text_refresh = true;
        /* code */
        break;

    case ERROR:
        timeout_start_millis = millis();
        /* code */
        break;

    default:
        break;
    }
    return 1;
}

int DigitalIconDisplay::updateDisplayMode(di_display_mode updated_mode)
{
    switch (updated_mode)
    {
    case STANDARD_MODE:
        /* code */
        break;
    case VINTAGE_MODE:
        /* code */
        break;
    case SCROLL_MODE:
        /* code */
        break;
    case BETA_MODE:
        /* code */
        break;
    default:
        break;
    }
    return 1;
}

int DigitalIconDisplay::showCustomMessage(char *custom_text)
{
    strcpy(display_text, custom_text);
    updateDisplayState(MESSAGE);
    return 1;
}

int DigitalIconDisplay::updateCounterValue(uint32_t new_counter_value)
{
    target_counter_value = new_counter_value;
    return 1;
}

void DigitalIconDisplay::loop()
{
    switch (display_state)
    {
    case BOOTING:
        /* code */
        break;

    case CONNECTING:
        /* code */
        break;

    case COUNTER:
        {
        /* code */
        uint32_t toShow = current_counter_value;

        if (target_counter_value > current_counter_value)
        {
            toShow += (target_counter_value - current_counter_value) / 2 + 1;
            current_counter_value = toShow;
        }
        else
        {
            toShow = target_counter_value;
        }

        counter_value = toShow;
        refreshScreenWithCounter();
        }
        break;

    case MESSAGE:
    {
        unsigned long current_millis = millis();
        if (current_millis - timeout_start_millis > TIMEOUT_PERIOD)
        {
            updateDisplayState(COUNTER);
        }
        if (text_refresh)
        {
            text_refresh = false;
            refreshScreenWithText();
        }

    }
        break;

    case ERROR:
    {
        unsigned long current_millis = millis();
        if (current_millis - timeout_start_millis > TIMEOUT_PERIOD)
        {
            updateDisplayState(COUNTER);
        }

    }
        break;

    default:
        break;
    }
    last_frame_display_state = display_state;
}

int DigitalIconDisplay::refreshScreenWithText()
{
    ParolaDisplay.setFont(ExtASCII);
    delay(100);
    ParolaDisplay.displayText(display_text, PA_CENTER, 70, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    ParolaDisplay.displayAnimate();
    return 1;
}
int DigitalIconDisplay::refreshScreenWithCounter()
{
    ParolaDisplay.setFont(numeric7Seg);
    // P.displayText(msg.c_str(), PA_CENTER, 70, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    ParolaDisplay.print(String(counter_value).c_str());
    return 1;
}