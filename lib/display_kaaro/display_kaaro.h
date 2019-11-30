#ifndef _DigitalIconDisplay_H_
#define _DigitalIconDisplay_H_

#include <MD_Parola.h>
#include <MD_MAX72xx.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

#define MAX_DEVICES 4
#define CLK_PIN 14
#define DATA_PIN 23
#define CS_PIN 15

#define BOOT_TEXT " Cowork.Network.Grow "
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
    void stripe();
    void spiral();
    void bounce();
    void scrollText(char *p);
    int updateCounterValue(uint32_t new_counter_value);
    int updateCounterValue(String new_counter_value, bool isString);
    int showCustomMessage(char *custom_text);
    int showCustomMessage(String custom_text);
    int updateDisplayState(di_display_states updated_state);
    int updateDisplayMode(di_display_mode updated_mode);
    void loop();

private:
    int refreshScreenWithText();
    int refreshScreenWithCounter();
};
#endif
