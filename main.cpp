/*
 * RPTERM - Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication via UART
 * Daniel Quadros, https://dqsoft.blogspot.com
 * 
 * Based on work by
 * - Shiela Dixon     (picoterm) https://peacockmedia.software  
 * - Miroslav Nemecek (picovga)  http://www.breatharian.eu/hw/picovga/index_en.html
 *
 * Main code
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

// text screen (character code + backgound coler + foreground color, format GF_ATEXT)
u8 TextBuf[TEXTSIZE] __attribute__ ((aligned(4)));

// copy of font
static u8 Font_Copy[sizeof(FONT)] __attribute__ ((aligned(4)));

// color pallet
u8 rpterm_pallet[NCOLOR_PAL] =
{
  COL_BLACK, COL_GRAY2, COL_GRAY3,
  COL_DARKBLUE, COL_SEMIBLUE, COL_BLUE,
  COL_DARKGREEN, COL_SEMIGREEN, COL_GREEN,
  COL_DARKCYAN, COL_SEMICYAN, COL_CYAN,
  COL_DARKRED, COL_SEMIRED, COL_RED,
  COL_DARKMAGENTA, COL_SEMIMAGENTA, COL_MAGENTA,
  COL_DARKYELLOW, COL_SEMIYELLOW, COL_YELLOW,
  COL_GRAY5, COL_GRAY6, COL_WHITE
};

// Terminal mode of operation
TERM_MODE term_mode = ONLINE;

// Beep control
static int nBeep = 0;
typedef enum {
	NO_BEEP, BEEPING, WAIT_BEEP
} BEEP_STATE;
static BEEP_STATE beep_state = NO_BEEP;
static uint32_t beep_end;
static const uint32_t beep_time = 100;
static const uint32_t beep_space = 200;

// LED control
static uint32_t last_rx = 0;
static bool led_on = false;
static uint32_t led_end;
static const uint32_t led_fast = 200;
static const uint32_t led_slow = 500;

// initialize video
static void VideoInit()
{
	// copy font to RAM buffer
	memcpy(Font_Copy, FONT, sizeof(FONT));

	// run VGA core
	multicore_launch_core1(VgaCore);

	// setup videomode
	VgaCfgDef(&Cfg); // get default configuration
	Cfg.video = &VideoVGA; // video timings
	Cfg.width = WIDTH; // screen width
	Cfg.height = HEIGHT; // screen height
	VgaCfg(&Cfg, &Vmode); // calculate videomode setup

	// initialize base layer 0
	ScreenClear(pScreen);
	sStrip* t = ScreenAddStrip(pScreen, HEIGHT);
	sSegm* g = ScreenAddSegm(t, WIDTH);
	ScreenSegmCText(g, TextBuf, Font_Copy, FONTH, TEXTWB);
	
	// initialize system clock
	set_sys_clock_pll(Vmode.vco*1000, Vmode.pd1, Vmode.pd2);

	// initialize videomode
	VgaInitReq(&Vmode);
}

// Init beep pin
static void beep_init() {
	#ifdef BUZZER_PIN
	gpio_init (BUZZER_PIN);
	gpio_set_dir (BUZZER_PIN, GPIO_OUT);
	gpio_put (BUZZER_PIN, 0);
	#endif
}

// Sound a beep
void beep () {
	#ifdef BUZZER_PIN
	if (beep_state == NO_BEEP) {
		// No beep active
		beep_end = board_millis() + beep_time;
		gpio_put (BUZZER_PIN, 1);
		beep_state = BEEPING;
	}
	nBeep++;
	#endif
}

// Treats beeping
static void beep_task() {
	if ((beep_state != NO_BEEP) && (board_millis() >= beep_end)) {
		if (beep_state == BEEPING) {
			// Finish beeping, wait before next beep
			gpio_put (BUZZER_PIN, 0);
			beep_end = board_millis() + beep_space;
			beep_state = WAIT_BEEP;
		} else if (beep_state == WAIT_BEEP) {
			// End of the wait
			if (--nBeep) {
				// There are more beeps
				beep_end = board_millis() + beep_time;
				gpio_put (BUZZER_PIN, 1);
				beep_state = BEEPING;
			} else {
				beep_state = NO_BEEP;
			}

		}
	}
}

// Init LED pin
static void led_init() {
	#ifdef STATUS_LED
	gpio_init (STATUS_LED);
	gpio_set_dir (STATUS_LED, GPIO_OUT);
	gpio_put (STATUS_LED, 0);
	#endif
}

// Flash status LED
static void led_task() {
	#ifdef STATUS_LED
	uint32_t now = board_millis();
	if (now >= led_end) {
		led_on = !led_on;
		gpio_put (STATUS_LED, led_on? 1 : 0);
		uint32_t led_speed = led_fast;
		if ((now - last_rx) > 1000) {
			led_speed = led_slow;
		}
		led_end = now + led_speed;
	}
	#endif
}

// Handle keyboard input
static void kbd_task() {
	uint8_t key = get_kbd();
	switch (term_mode) {
		case ONLINE:
			switch (key) {
				case KEY_ALT_C:
					config_enter();
					term_mode = CONFIG;
					update_sl_mode();
					break;
				case KEY_ALT_L:
					term_mode = LOCAL;
					update_sl_mode();
					break;
				case KEY_ALT_R:
					// TODO
					break;
				case KEY_ALT_T:
					// TODO
					break;
				default:
					send_key(key);
					break;
			}
			break;
		case LOCAL:
			switch (key) {
				case KEY_ALT_C:
					config_enter();
					term_mode = CONFIG;
					update_sl_mode();
					break;
				case KEY_ALT_L:
					term_mode = ONLINE;
					update_sl_mode();
					break;
				case KEY_ALT_R:
					// TODO
					break;
				case KEY_ALT_T:
					// TODO
					break;
				default:
					receive_key(key);
					break;
			}
			break;
		case CONFIG:
			if (key == ESC) {
				term_mode = ONLINE;
				config_leave();
			} else {
				config_key (key);
			}
			break;
	}
}


// Main routine
int main()
{
	// initialize video
	VideoInit();

	// initialize keyboard
	keyb_init();

	// Init terminal emulation
	terminal_init();

	// initialize tinyusb stack
	tusb_init();

	// init UART
  	serial_init();

	// init status eld
	led_init();

	// init beep
	beep_init();
	beep();
	
	// main loop
	while (true)
	{
		// handle rx chars
		if (has_rx()) {
			last_rx = board_millis();	// for status led
			terminal_handle_rx (get_rx());
		}

		// handle usb
	    tuh_task();
		hid_app_task();

		// handle keys
		if (has_kbd()) {
			kbd_task();
		}

		// trnasmit pending chars
		serial_tx_task();

		// flash led
		led_task();

		// take care of beep
		beep_task();
	}
}

