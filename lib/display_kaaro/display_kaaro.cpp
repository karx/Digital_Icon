#include <display_kaaro.h>
#include "Parola_Fonts_data.h"
#include <Font_Data.h>
uint32_t stoi(String payload, int len);

uint8_t scrollSpeed = 25;
int scrollEffectIn = PA_SCAN_HORIZ;
int scrollEffectOut = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 2000;


MD_Parola ParolaDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

DigitalIconDisplay::DigitalIconDisplay() 
{
    counter_value = 0;
    display_state = BOOTING;
    last_frame_display_state = BOOTING;
    display_type = DOTMATRIX;
    display_mode = STANDARD_MODE;
}

void DigitalIconDisplay::stripe()
// Demonstrates animation of a diagonal stripe moving across the display
// with points plotted outside the display region ignored.
{
    mx.begin();
    mx.control(MD_MAX72XX::INTENSITY, 10);
  const uint16_t maxCol = MAX_DEVICES*ROW_SIZE;
  const uint8_t	stripeWidth = 10;

  Serial.println("\nEach individually by row then col");
  mx.clear();

  for (uint16_t col=0; col<maxCol + ROW_SIZE + stripeWidth; col++)
  {
    for (uint8_t row=0; row < ROW_SIZE; row++)
    {
      mx.setPoint(row, col-row, true);
      mx.setPoint(row, col-row - stripeWidth, false);
    }
    delay(100);
  }
}

void DigitalIconDisplay::spiral()
// setPoint() used to draw a spiral across the whole display
{
  Serial.println("\nSpiral in");
  int  rmin = 0, rmax = ROW_SIZE-1;
  int  cmin = 0, cmax = (COL_SIZE*MAX_DEVICES)-1;

  mx.clear();
  while ((rmax > rmin) && (cmax > cmin))
  {
    // do row
    for (int i=cmin; i<=cmax; i++)
    {
      mx.setPoint(rmin, i, true);
      delay(100/MAX_DEVICES);
    }
    rmin++;

    // do column
    for (uint8_t i=rmin; i<=rmax; i++)
    {
      mx.setPoint(i, cmax, true);
      delay(100/MAX_DEVICES);
    }
    cmax--;

    // do row
    for (int i=cmax; i>=cmin; i--)
    {
      mx.setPoint(rmax, i, true);
      delay(100/MAX_DEVICES);
    }
    rmax--;

    // do column
    for (uint8_t i=rmax; i>=rmin; i--)
    {
      mx.setPoint(i, cmin, true);
      delay(100/MAX_DEVICES);
    }
    cmin++;
  }
}

void DigitalIconDisplay::bounce()
// Animation of a bouncing ball
{
  const int minC = 0;
  const int maxC = mx.getColumnCount()-1;
  const int minR = 0;
  const int maxR = ROW_SIZE-1;

  int  nCounter = 0;

  int  r = 0, c = 2;
  int8_t dR = 1, dC = 1;	// delta row and column

  Serial.println("\nBouncing ball");
  mx.clear();

  while (nCounter++ < 200)
  {
    mx.setPoint(r, c, false);
    r += dR;
    c += dC;
    mx.setPoint(r, c, true);
    delay(100/2);

    if ((r == minR) || (r == maxR))
      dR = -dR;
    if ((c == minC) || (c == maxC))
      dC = -dC;
  }
}

void scrollText(char *p)
{
  uint8_t charWidth;
  uint8_t cBuf[8];  // this should be ok for all built-in fonts

  Serial.println("\nScrolling text");
  mx.clear();

  while (*p != '\0')
  {
    charWidth = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);

    for (uint8_t i=0; i<=charWidth; i++)	// allow space between characters
    {
      mx.transform(MD_MAX72XX::TSL);
      if (i < charWidth)
        mx.setColumn(0, cBuf[i]);
      delay(100);
    }
  }
}




int DigitalIconDisplay::setupIcon()
{
    ParolaDisplay.begin();
    ParolaDisplay.setInvert(false);
    ParolaDisplay.setIntensity(15);
    ParolaDisplay.displayText(BOOT_TEXT, PA_CENTER, 70, 100, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    delay(1000);
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
    loop();
    return 1;
}
int DigitalIconDisplay::showCustomMessage(String custom_text) 
{
    char buff[100];
    custom_text.toCharArray(buff, 100);
    return showCustomMessage(buff);
}
int DigitalIconDisplay::showCustomMessage(String custom_text, uint8_t size) 
{
    char *buff = new char[size + 1];
    custom_text.toCharArray(buff, size);
    return showCustomMessage(buff);
}

int DigitalIconDisplay::updateCounterValue(uint32_t new_counter_value)
{
    target_counter_value = new_counter_value;
    return target_counter_value;
}

int DigitalIconDisplay::updateCounterValue(String new_counter_value, bool isString) {
    ParolaDisplay.setFont(numeric7Seg);
    ParolaDisplay.print(String(new_counter_value).c_str());
    return updateCounterValue(
        stoi(new_counter_value, new_counter_value.length())
    ); 
}
int DigitalIconDisplay::updateTextAnimationIn() {
    updateTextAnimationIn(-1);
}
int DigitalIconDisplay::updateTextAnimationIn(int mode) {
    if (mode > -1) 
        scrollEffectIn = mode;
    else
        scrollEffectIn++;
    refreshScreenWithText();    // To play a sample
    return 1;
}

int DigitalIconDisplay::updateTextAnimationOut() {
    updateTextAnimationOut(-1);
}
int DigitalIconDisplay::updateTextAnimationOut(int mode) {
    if (mode > -1)
        scrollEffectOut = mode;
    else 
        scrollEffectOut++;
    refreshScreenWithText();    //To play a sample
    return 1;
}

void DigitalIconDisplay::loop()
{
    ParolaDisplay.displayAnimate();
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
            toShow += (target_counter_value - current_counter_value) / 5 + 1;
            current_counter_value = toShow;
        }
        else if (target_counter_value < current_counter_value)
        {
            toShow -= (current_counter_value - target_counter_value) / 5 + 1;
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
    ParolaDisplay.displayText(display_text, PA_CENTER, 70, 1000, (textEffect_t)scrollEffectIn, (textEffect_t)scrollEffectOut);
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
