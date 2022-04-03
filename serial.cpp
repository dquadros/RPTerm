/*
 * RPTERM - Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication via UART
 * Daniel Quadros, https://dqsoft.blogspot.com
 * 
 * Based on work by
 * - Shiela Dixon     (picoterm) https://peacockmedia.software  
 * - Miroslav Nemecek (picovga)  http://www.breatharian.eu/hw/picovga/index_en.html
 *
 * This file handles the UART
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

// Rx buffer (i.e., data from the RC2014)
#define RX_BUFFER_SIZE 1000
static uint8_t buffer_rx[RX_BUFFER_SIZE];
static int buf_rx_in, buf_rx_out;

// Tx buffer (i.e., data to the RC2014)
#define TX_BUFFER_SIZE 100
static uint8_t buffer_tx[TX_BUFFER_SIZE];
static int buf_tx_in, buf_tx_out;

// UART parameters
#define BAUD_RATE       115200
#define DATA_BITS       8
#define STOP_BITS       1
#define PARITY          UART_PARITY_NONE

static void on_uart_rx();

//--------------------------------------------------------------------+
// RX buffer routines
//--------------------------------------------------------------------+

// Put received char in the buffer
inline void put_rx(uint8_t ch) {
    buffer_rx[buf_rx_in] = ch;
    int aux = buf_rx_in+1;
    if (aux >= RX_BUFFER_SIZE) {
        aux = 0;
    }
    if (aux != buf_rx_out) {
        // buffer not full
        buf_rx_in = aux;
    }
}

// Test if buffer not empty
bool has_rx() {
    return buf_rx_in != buf_rx_out;
}

// Get next char from the buffer
uint8_t get_rx() {
    if (has_rx()) {
        uint8_t ch = buffer_rx[buf_rx_out];
        int aux = buf_rx_out+1;
        if (aux >= RX_BUFFER_SIZE) {
            aux = 0;
        }
        buf_rx_out = aux;
        return ch;
    } else {
        return 0;   // buffer empty
    }
}

//--------------------------------------------------------------------+
// TX buffer routines
//--------------------------------------------------------------------+

// Put char to transmit in the buffer
void put_tx(uint8_t ch) {
    buffer_tx[buf_tx_in] = ch;
    int aux = buf_tx_in+1;
    if (aux >= TX_BUFFER_SIZE) {
        aux = 0;
    }
    if (aux != buf_tx_out) {
        // buffer not full
        buf_tx_in = aux;
    }
}

// Test if buffer not empty
static bool has_tx() {
    return buf_tx_in != buf_tx_out;
}

// Get next char from the buffer
static uint8_t get_tx() {
    if (has_tx()) {
        uint8_t ch = buffer_tx[buf_tx_out];
        int aux = buf_tx_out+1;
        if (aux >= TX_BUFFER_SIZE) {
            aux = 0;
        }
        buf_tx_out = aux;
        return ch;
    } else {
        return 0;   // buffer empty
    }
}

//--------------------------------------------------------------------+
// UART routines
//--------------------------------------------------------------------+
void serial_init() {

    buf_rx_in = buf_rx_out = 0;
    buf_tx_in = buf_tx_out = 0;

    uart_init(UART_ID, BAUD_RATE);
    uart_set_hw_flow(UART_ID,false,false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);


    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);


    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);

}

// UART Tx task
void serial_tx_task() {
    if (has_tx() && uart_is_writable(UART_ID)) {
        uart_putc (UART_ID, get_tx());
    }
}

// A character was received
static void on_uart_rx() {
    while (uart_is_readable(UART_ID)) {
        put_rx(uart_getc(UART_ID));
    }
}
