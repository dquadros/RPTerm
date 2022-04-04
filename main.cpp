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

int main()
{
	// initialize video
	VideoInit();

	// Init terminal emulation
	terminal_init();

	// initialize tinyusb stack
	tusb_init();

	// init UART
  	serial_init();
	
	// main loop
	while (true)
	{
		// handle rx chars
		if (has_rx()) {
			terminal_handle_rx (get_rx());
		}

		// handle usb
	    tuh_task();
		hid_app_task();

		// trnasmit pending chars
		serial_tx_task();
	}
}
