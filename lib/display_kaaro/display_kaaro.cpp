#include <display_kaaro.h>
#include "Parola_Fonts_data.h"
#include <Font_Data.h>
uint32_t stoi(String payload, int len);

uint8_t scrollSpeed = 25;
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 2000;


MD_Parola ParolaDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

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
    ParolaDisplay.displayText(BOOT_TEXT, PA_CENTER, 0, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
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
int DigitalIconDisplay::showCustomMessage(String custom_text) 
{
    char buff[100];
    custom_text.toCharArray(buff, 100);
    return showCustomMessage(buff);
}

int DigitalIconDisplay::updateCounterValue(uint32_t new_counter_value)
{
    target_counter_value = new_counter_value;
    return 1;
}

int DigitalIconDisplay::updateCounterValue(String new_counter_value, bool isString) {
    return updateCounterValue(
        stoi(new_counter_value, new_counter_value.length())
    ); 
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

uint32_t stoi(String payload, int len)
{
  uint32_t i = 0;
  uint32_t result = 0;
  for (i = 0; i < len; i++)
  {
    result *= 10;
    result += (char)payload[i] - '0';
  }

  return result;
}
