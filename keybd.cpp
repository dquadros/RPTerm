/*
 * RPTERM - Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication via UART
 * Daniel Quadros, https://dqsoft.blogspot.com
 *
 * Based on work by
 * - Shiela Dixon     (picoterm) https://peacockmedia.software
 * - Miroslav Nemecek (picovga)  http://www.breatharian.eu/hw/picovga/index_en.html
 *
 * This file handles the USB keyboard
 * Based in the SDK example for tinyusb v0.13.0
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "include.h"

// Special keys
#define KEY_UP 0x80
#define KEY_DWN 0x81
#define KEY_LFT 0x82
#define KEY_RGT 0x83
#define KEY_HOME 0x84
#define KEY_END 0x85
#define KEY_F1 0x86
#define KEY_F2 0x87
#define KEY_F3 0x88
#define KEY_F4 0x89
#define KEY_F5 0x8A
#define KEY_F6 0x8B
#define KEY_F7 0x8C
#define KEY_F8 0x8D
#define KEY_F9 0x8E
#define KEY_F10 0x8F

#include "keycode_to_ascii.h"

// Keyboard address and instance (assumes there is only one)
static uint8_t keybd_dev_addr = 0xFF;
static uint8_t keybd_instance;

// Keyboard LED control
static uint8_t leds = 0;
static uint8_t prev_leds = 0xFF;

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

#define MAX_REPORT 4

#ifdef LOCALISE_DE
static uint8_t const keycode2ascii[128][2] = {HID_KEYCODE_TO_ASCII_DE};
#elif LOCALISE_UK
static uint8_t const keycode2ascii[128][2] = {HID_KEYCODE_TO_ASCII_UK};
#elif LOCALISE_US
static uint8_t const keycode2ascii[128][2] = {HID_KEYCODE_TO_ASCII_US};
#elif LOCALISE_BR
static uint8_t const keycode2ascii[160][2] = {HID_KEYCODE_TO_ASCII_BR};
#else
static uint8_t const keycode2ascii[128][2] = {HID_KEYCODE_TO_ASCII};
#endif

#define NKEYS (sizeof(keycode2ascii) / sizeof(keycode2ascii[0]))

// Special keys sequences
static char const *keysequence[] = {
    "\x1B[A",  // UP
    "\x1B[B",  // DOWN
    "\x1B[D",  // LEFT
    "\x1B[C",  // RIGHT
    "\x1B[H",  // HOME
    "\x1B[K",  // END
    "\x1B[OP", // F1
    "\x1B[OQ", // F2
    "\x1B[OR", // F3
    "\x1B[OS", // F4
    "\x1B[OT", // F5
    "\x1B[OU", // F6
    "\x1B[OV", // F7
    "\x1B[OW", // F8
    "\x1B[OX", // F9
    "\x1B[OY"  // F10
};

// Each HID instance has multiple reports
static uint8_t _report_count[CFG_TUH_HID];
static tuh_hid_report_info_t _report_info_arr[CFG_TUH_HID][MAX_REPORT];

static void process_kbd_report(hid_keyboard_report_t const *report);
static void process_mouse_report(hid_mouse_report_t const *report);

//--------------------------------------------------------------------+
// This will be called by the main loop
//--------------------------------------------------------------------+
void hid_app_task(void)
{
  // update keyboard leds
  if (keybd_dev_addr != 0xFF)
  { // only if keyboard attached
    if (leds != prev_leds)
    {
      tuh_hid_set_report(keybd_dev_addr, keybd_instance, 0, HID_REPORT_TYPE_OUTPUT, &leds, sizeof(leds));
      prev_leds = leds;
    }
  }
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len)
{
  // Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
  // can be used to parse common/simple enough descriptor.
  _report_count[instance] = tuh_hid_parse_report_descriptor(_report_info_arr[instance], MAX_REPORT, desc_report, desc_len);

  // request to receive report
  tuh_hid_receive_report(dev_addr, instance);
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  keybd_dev_addr = 0xFF; // keyboard not available
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
{
  uint8_t const rpt_count = _report_count[instance];
  tuh_hid_report_info_t *rpt_info_arr = _report_info_arr[instance];
  tuh_hid_report_info_t *rpt_info = NULL;

  if ((rpt_count == 1) && (rpt_info_arr[0].report_id == 0))
  {
    // Simple report without report ID as 1st byte
    rpt_info = &rpt_info_arr[0];
  }
  else
  {
    // Composite report, 1st byte is report ID, data starts from 2nd byte
    uint8_t const rpt_id = report[0];

    // Find report id in the arrray
    for (uint8_t i = 0; i < rpt_count; i++)
    {
      if (rpt_id == rpt_info_arr[i].report_id)
      {
        rpt_info = &rpt_info_arr[i];
        break;
      }
    }

    report++;
    len--;
  }

  if (rpt_info && (rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP))
  {
    switch (rpt_info->usage)
    {
      case HID_USAGE_DESKTOP_KEYBOARD:
        // Assume keyboard follow boot report layout
        keybd_dev_addr = dev_addr;
        keybd_instance = instance;
        process_kbd_report((hid_keyboard_report_t const *)report);
        break;

      case HID_USAGE_DESKTOP_MOUSE:
        // Assume mouse follow boot report layout
        process_mouse_report((hid_mouse_report_t const *)report);
        break;

      default:
        break;
    }
  }

  // continue to request to receive report
  tuh_hid_receive_report(dev_addr, instance);
}

//--------------------------------------------------------------------+
// Keyboard
//--------------------------------------------------------------------+

// look up key in a report
static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode)
{
  for (uint8_t i = 0; i < 6; i++)
  {
    if (report->keycode[i] == keycode)
    {
      return true;
    }
  }

  return false;
}

// Caps lock control
static bool capslock_key_down_in_last_report = false;
static bool capslock_key_down_in_this_report = false;
static bool capslock_on = false;

// process keyboard report
static void process_kbd_report(hid_keyboard_report_t const *report)
{
  static hid_keyboard_report_t prev_report = {0, 0, {0}}; // previous report to check key released

  // Check caps lock
  capslock_key_down_in_this_report = find_key_in_report(report, HID_KEY_CAPS_LOCK);
  if (capslock_key_down_in_this_report && !capslock_key_down_in_last_report)
  {
    // CAPS LOCK was pressed
    capslock_on = !capslock_on;
    if (capslock_on)
    {
      leds |= KEYBOARD_LED_CAPSLOCK;
    }
    else
    {
      leds &= ~KEYBOARD_LED_CAPSLOCK;
    }
  }

  // check other pressed keys
  for (uint8_t i = 0; i < 6; i++)
  {
    if (report->keycode[i]) // ignore non-ascci keys
    {
      if (find_key_in_report(&prev_report, report->keycode[i]))
      {
        // exist in previous report means the current key is holding
      }
      else
      {
        // not existed in previous report means the current key is pressed
        uint8_t key = report->keycode[i];
        uint8_t ch = (key < NKEYS) ? keycode2ascii[key][0] : 0; // unshifted key code, to test for letters
        bool const is_ctrl = report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL);
        bool is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
        if (capslock_on && (ch >= 'a') && (ch <= 'z'))
        {
          // capslock affects only letters
          is_shift = !is_shift;
        }
        ch = (key < NKEYS) ? keycode2ascii[key][is_shift ? 1 : 0] : 0;

        if (key != HID_KEY_CAPS_LOCK)
        {
          if (ch > 0x7F)
          {
            // special key
            char const *seq = keysequence[ch - 0x80];
            while (*seq)
            {
              put_tx(*seq);
              seq++;
            }
          }
          else if (is_ctrl)
          {
            // control code
            if ((ch >= 0x60) && (ch <= 0x7F))
            {
              put_tx(ch - 0x60);
            }
            else if ((ch >= 0x40) && (ch <= 0x5F))
            {
              put_tx(ch - 0x40);
            }
          }
          else if (ch != 0)
          {
            // normal key
            put_tx(ch);
          }
        }
      }
    }
  }

  // save current report
  prev_report = *report;
  capslock_key_down_in_last_report = capslock_key_down_in_this_report;
}

//--------------------------------------------------------------------+
// Mouse
//--------------------------------------------------------------------+

void cursor_movement(int8_t x, int8_t y, int8_t wheel)
{
}

static void process_mouse_report(hid_mouse_report_t const *report)
{
}
