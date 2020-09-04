//#include <stdbool.h>
// #include <stdio.h>
//#include <stdint.h>
//#include <string.h>

#include "riscv_soc_utils.h"
#include "gpio.h"
#include "event.h"
#include "uart.h"
#include "encoding.h"
#include "timer.h"


#include "printf.h"

#define SW_0    1 << 28
#define SW_1    1 << 29
#define SW_2    1 << 30
#define SW_3    1 << 31

int run_leds = 0;

typedef int size_t;
#define NULL ((void*) 0)

void bmark_standard_loop_standard_load_pcnt();
void bmark_standard_loop_postinc_load_pcnt();
void bmark_hw_loop_postinc_load_pcnt();

extern uint32_t  _start;
volatile uint32_t* const printf_buffer = (uint32_t*) PRINTF_VERILATOR;

size_t _write(int fildes, const void *buf, size_t nbyte) {
    const uint8_t* cbuf = (const uint8_t*) buf;
    for (size_t i = 0; i < nbyte; ++i) {
    #if VERILATOR == 1
        #warning "[PLEASE READ] PRINTF output will be redirected to verilator dump peripheral"
        *printf_buffer = cbuf[i];
    #else
        #warning "[PLEASE READ] PRINTF output will be redirected to UART peripheral"
        uart_sendchar((char)cbuf[i]);
    #endif
    }
    return nbyte;
}

void loop_leds(){
    run_leds = false;
    // greens = 1,4,7,10
    // blues = 2,5,8,11
    // red = 0,3,6,9
    for (int j = 0; j<3;j++){
        for (int i = j;i<12;i+=3){
            set_gpio_pin_value(i,true);
            for (int t=0;t<100000;t++);
        }
        for (int i = j;i<12;i+=3){
            set_gpio_pin_value(i,false);
            for (int t=0;t<100000;t++);
        }
    }
}

void isr_gpio(void) {
    int_periph_clear(GPIO_EVENT);
    printf("\n\rGPIO ISR!");
    run_leds = true;
}

void isr_uart(void) {
    uint8_t uart_rx = *(volatile int*) UART_REG_RBR;

    int_periph_clear(UART_EVENT);
    printf("\n\rUART ISR received = %c",uart_rx);
    if (uart_rx == 'a')
        run_leds = true;
}

void isr_m_timer(void) {
    int_periph_clear(TIMER_A_OUTPUT_CMP);
    printf("\n\rTimer ISR!");
}

void setup_irqs(){
    int_periph_clear(UART_EVENT);
    int_periph_clear(TIMER_A_OUTPUT_CMP);
    int_periph_clear(GPIO_EVENT);

    for (int i = 28;i<32;i++)
        set_gpio_pin_irq_en(i, 1);

    for (int i = 28;i<32;i++)
        set_gpio_pin_irq_type(i, GPIO_IRQ_LEV1);

    #if VERILATOR == 0
    set_cmp(20000000);
    #else
    set_cmp(10000);
    #endif

    int_periph_enable(GPIO_EVENT);
    int_periph_enable(TIMER_A_OUTPUT_CMP);
    int_periph_enable(UART_EVENT);
    #if VERILATOR == 0
        int_periph_enable(UART_EVENT);
    #endif
    cfg_int(true);
}

void setup_gpios(){
    for (int i = 0;i<12;i++)
        set_gpio_pin_direction(i,DIR_OUT);
    for (int i = 28;i<32;i++)
        set_gpio_pin_direction(i,DIR_IN);
    for (int i = 0;i<12;i+=1)
        set_gpio_pin_value(i,false);

    enable_gpio_pins_read(SW_0|SW_1|SW_2|SW_3);
}

void dummy_putc(void* ptr, char c) {
	uart_sendchar(c);
}

unsigned long read_cycles(void)
{
    unsigned long cycles;
    asm volatile ("rdcycle %0" : "=r" (cycles));
    return cycles;
}

void do_loop() {
    while(1){
        if (run_leds)
            loop_leds();
    }
}

void test_cycles_instrs(const char* descr, void (*f) (void)) {
    volatile int c0, c1, i0, i1;

    #define READ_INSTR 0x781
    #define READ_CYC 0x780

    tfp_printf("==== Benchmark '%s' ====\r\n", descr);  
    c0 = read_csr(0x780);
    i0 = read_csr(0x781);
    f();
    c1 = read_csr(0x780);
    i1 = read_csr(0x781);
    tfp_printf("CYCLES: %d, INSTRS:%d\r\n", c1 - c0, i1 - i0);
    tfp_printf("==== Benchmark end ====\r\n");
}


int main(void) {
    // Set the reset address to the entry point
    volatile uint32_t *address_rst = (uint32_t *)RST_CTRL_BASE_ADDR;

    *(address_rst) = (uint32_t )&_start;

    // To calculate uart speed,
    // consider the following:
    // baud_rate = periph_clk / 2^(parameter)
    // uart_set_cfg(0, 7);
    // 7 => 117187 ~> 115200
    uart_set_cfg(0, 7);


	init_printf(NULL, dummy_putc);

    setup_gpios();
//    setup_irqs();
//   start_timer();

    tfp_printf("####### hello from RISCY\r\n");



    volatile int priv = read_csr(0xCC1);

    tfp_printf("SAVED CC1: %d\r\n", priv);

    int val = 1 + (2) + (1 << 5);
    write_csr(0xCC0, val);

    // TODO if it returns all zeros please uncomment that
    // TODO why??
    asm volatile (
		"csrw 0x785,x0\n" 
    );

    tfp_printf("========= Benchmarking xor+popcount on two 32 vectors of 4byte elements =========\r\n");

    test_cycles_instrs("xor+popcount: standard loop/standard load", bmark_standard_loop_standard_load_pcnt);
    test_cycles_instrs("xor+popcount: standard loop/post-increment load", bmark_standard_loop_postinc_load_pcnt);
    test_cycles_instrs("xor+popcount: hw loop/post-increment load", bmark_hw_loop_postinc_load_pcnt);



    do_loop();
    
}


void bmark_standard_loop_standard_load_pcnt() {
    volatile unsigned int a[32];
    volatile unsigned int b[32];

    asm volatile(
    "li x5, 0\n"
    "li x4, 32\n"
    "Lstart1:\n"
    "lw t3, 0(x10)\n"
    "lw t4, 0(x11)\n"
    "xor t5,t3,t4\n"
    "p.cnt t5,t5\n"
    "addi x10,x10,4\n"
    "addi x11,x11,4\n"
    "addi x5,x5,1\n"
    "bne x4,x5,Lstart1"
    );
}


void bmark_standard_loop_postinc_load_pcnt() {
    volatile unsigned int a[32];
    volatile unsigned int b[32];

    asm volatile(
    "li x5, 0\n"
    "li x4, 32\n"
    "Lstart2:\n"
    "p.lw t3,4(x10!)\n"
    "p.lw t4,4(x11!)\n"
    "xor t5,t3,t4\n"
    "p.cnt t5,t5\n"
    "addi x5,x5,1\n"
    "bne x4,x5,Lstart2"
    );
}


void bmark_hw_loop_postinc_load_pcnt() {
    volatile unsigned int a[32];
    volatile unsigned int b[32];

    asm volatile(
    "li x5, 0\n"
    "li x4, 32\n"
    "lp.setupi x1,32,stop_loop\n"
    "p.lw t3,4(x10!)\n"
    "p.lw t4,4(x11!)\n"
    "xor t5,t3,t4\n"
    "stop_loop: p.cnt t5,t5\n"
    );
}
