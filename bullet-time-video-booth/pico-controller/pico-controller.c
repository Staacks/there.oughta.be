#include <stdio.h>
#include "pico/stdlib.h"

#define FOCUS_PIN 0
#define TRIGGER_PIN 1

void setupGPIO() {
    //Might not really bew necessary, but since the cameras have their individual power supplies and pull the focus and trigger pin up to "their" 3.3V, I would avoid actively driving up these pins to the Pico's 3.3V. Instead we use a common ground, keep the pins passive and only drive them to ground in order to trigger the cameras
    gpio_init(FOCUS_PIN);
    gpio_put(FOCUS_PIN, 0);
    gpio_set_dir(FOCUS_PIN, GPIO_IN);
    gpio_pull_up(FOCUS_PIN);
    gpio_init(TRIGGER_PIN);
    gpio_put(TRIGGER_PIN, 0);
    gpio_set_dir(TRIGGER_PIN, GPIO_IN);
    gpio_pull_up(TRIGGER_PIN);
}

void focusStart() {
    gpio_set_dir(FOCUS_PIN, GPIO_OUT);
    printf("Focus on\n");
}

void focusEnd() {
    gpio_set_dir(FOCUS_PIN, GPIO_IN);
    printf("Focus off\n");
}

void trigger() {
    gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
    sleep_ms(100);
    gpio_set_dir(TRIGGER_PIN, GPIO_IN);
    printf("Triggered\n");
}

int main() {
    stdio_init_all();
    setupGPIO();
    while (true) {
        switch (getchar_timeout_us(0)) {
            case 'f':
                focusStart();
                break;
            case 't':
                trigger();
                focusEnd();
                break;
        }
        sleep_ms(1);
    }
    return 0;
}