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
 * The The HID host code in the tinyUSB stack will select the boot protocol and 
 * a zero idle rate (device only send reports if there is a change) when a HID 
 * device is mounted.
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

#include "keycode_to_ascii.h"

#define MAX_KEY 6   // Maximun number of pressed key in the boot layout report

// Keyboard buffer
#define KBD_BUFFER_SIZE 100
static uint8_t buffer_kbd[KBD_BUFFER_SIZE];
static int buf_kbd_in, buf_kbd_out;


// Keyboard address and instance (assumes there is only one)
static uint8_t keybd_dev_addr = 0xFF;
static uint8_t keybd_instance;

// Auto repeat control
#define REPEAT_START    1000
#define REPEAT_INTERVAL 100

static uint8_t  repeat_keycode [MAX_KEY];
static uint8_t  repeat_char [MAX_KEY];
static uint32_t repeat_time [MAX_KEY];

// Caps lock control
static bool capslock_key_down_in_last_report = false;
static bool capslock_key_down_in_this_report = false;
static bool capslock_on = false;

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

// Each HID instance has multiple reports
static uint8_t _report_count[CFG_TUH_HID];
static tuh_hid_report_info_t _report_info_arr[CFG_TUH_HID][MAX_REPORT];

static void process_kbd_report(hid_keyboard_report_t const *report);
static void process_mouse_report(hid_mouse_report_t const *report);


// Module init
void keyb_init(void)
{
    buf_kbd_in = buf_kbd_out = 0;
}

//--------------------------------------------------------------------+
// Keyboard buffer routines
//--------------------------------------------------------------------+

// Put key in the buffer
static inline void put_kbd(uint8_t key) {
    buffer_kbd[buf_kbd_in] = key;
    int aux = buf_kbd_in+1;
    if (aux >= KBD_BUFFER_SIZE) {
        aux = 0;
    }
    if (aux != buf_kbd_out) {
        // buffer not full
        buf_kbd_in = aux;
    }
}

// Test if buffer not empty
bool has_kbd() {
    return buf_kbd_in != buf_kbd_out;
}

// Get next key from the buffer
uint8_t get_kbd() {
    if (has_kbd()) {
        uint8_t key = buffer_kbd[buf_kbd_out];
        int aux = buf_kbd_out+1;
        if (aux >= KBD_BUFFER_SIZE) {
            aux = 0;
        }
        buf_kbd_out = aux;
        return key;
    } else {
        return 0;   // buffer empty
    }
}

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

    // auto-repeat keys
    for (int i = 0; i < MAX_KEY; i++)
    {
      if (repeat_char[i] && (board_millis() > repeat_time[i]))
      {
        put_kbd(repeat_char[i]);
        repeat_time[i] += REPEAT_INTERVAL;
      }
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
  // Check if at least one of the reports is for a keyboard
  for (int i = 0; i < _report_count[instance]; i++) 
  {
    if ((_report_info_arr[instance][i].usage_page == HID_USAGE_PAGE_DESKTOP) && 
        (_report_info_arr[instance][i].usage == HID_USAGE_DESKTOP_KEYBOARD)) 
    {
        keybd_dev_addr = dev_addr;
        keybd_instance = instance;
    }
  }

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
  for (uint8_t i = 0; i < MAX_KEY; i++)
  {
    if (report->keycode[i] == keycode)
    {
      return true;
    }
  }

  return false;
}

// process keyboard report
static void process_kbd_report(hid_keyboard_report_t const *report)
{
  static hid_keyboard_report_t prev_report = {0, 0, {0}}; // previous report to check key released

  // clear released keys in the auto repeat control
  for (int i = 0; i < MAX_KEY; i++)
  {
    if (repeat_keycode[i] && !find_key_in_report(report, repeat_keycode[i])) {
      repeat_keycode[i] = 0;
      repeat_char[i] = 0;
    }
  }

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
  for (uint8_t i = 0; i < MAX_KEY; i++)
  {
    uint8_t key = report->keycode[i];
    if ((key != 0) && (key != HID_KEY_CAPS_LOCK) && !find_key_in_report(&prev_report, key)) // ignore fillers, Caps lock and keys already pressed
    {
      // Find corresponding ASCII code
      uint8_t ch = (key < NKEYS) ? keycode2ascii[key][0] : 0; // unshifted key code, to test for letters
      bool const is_ctrl = report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL);
      bool is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
      bool is_alt = report->modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT);
      if (capslock_on && (ch >= 'a') && (ch <= 'z'))
      {
        // capslock affects only letters
        is_shift = !is_shift;
      }
      ch = (key < NKEYS) ? keycode2ascii[key][is_shift ? 1 : 0] : 0;
      if (is_ctrl)
      {
        // control char
        if ((ch >= 0x60) && (ch <= 0x7F))
        {
          ch = ch - 0x60;
        }
        else if ((ch >= 0x40) && (ch <= 0x5F))
        {
          ch = ch - 0x40;
        }
      }
      if (is_alt)
      {
        switch (ch) 
        {
          case 'c': case 'C':
            ch = KEY_ALT_C;
            break;
          case 'l': case 'L':
            ch = KEY_ALT_L;
            break;
          case 'r': case 'R':
            ch = KEY_ALT_R;
            break;
          case 't': case 'T':
            ch = KEY_ALT_T;
            break;
        }
      }

      // record key for auto repeat
      for (int j = 0; j < MAX_KEY; j++)
      {
        if (repeat_keycode[j] == 0)
        {
          repeat_keycode[j] = key;
          repeat_char[j] = ch;
          repeat_time[j] = board_millis() + REPEAT_START;
          break;
        }
      }

      // store the key
      put_kbd (ch);
    }
  }

  // save current status
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
