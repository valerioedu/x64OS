#include "pic.h"
#include "../../../lib/definitions.h"
#include "../../drivers/keyboard/keyboard.h"
#include "../../threading/threading.h"

static volatile uint64_t g_timer_ticks = 0;

static volatile uint8_t g_last_scancode = 0;

void isr_timer_handler() {
    g_timer_ticks++;
    //schedule();

    pic_send_eoi(0);
}

uint64_t timer_get_ticks() {
    return g_timer_ticks;
}