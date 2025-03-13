/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include <string.h>
#include "hardware/timer.h"


const int ECHO_PIN = 15;
const int TRIG_PIN = 16;

volatile int time_init = 0;
volatile int time_end = 0;
volatile bool timeout_fired = false;
volatile bool sec_fired = false;
volatile bool measuring = false;

const int TIMEOUT_US = 30000;

int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    timeout_fired = true;
    return 0;
}

void gpio_callback(uint gpio, uint32_t events)
{
    if (events == GPIO_IRQ_EDGE_RISE) 
    {
        time_init = to_us_since_boot(get_absolute_time());  // Começa a contar tempo
        timeout_fired = false;
        add_alarm_in_us(TIMEOUT_US, alarm_callback, NULL, false);
    }
    else if (events == GPIO_IRQ_EDGE_FALL)
    {
        time_end = to_us_since_boot(get_absolute_time());  // Finaliza contagem
        measuring = false;
    }
}

static void rtc_callback(void)
{
    sec_fired = true;
    //printf("fired\n");
}

// Função para medir distância
void measure_distance()
{
    gpio_put(TRIG_PIN, 1);
    sleep_us(10);
    gpio_put(TRIG_PIN, 0);
    measuring = true;
    sleep_ms(30);
}

// Função para obter tempo atual
void print_timestamp()
{
    datetime_t t;
    rtc_get_datetime(&t);
    printf("%02d:%02d:%02d - ", t.hour, t.min, t.sec);
}

// Função principal de leitura
void read_sensor(bool is_running)
{
    if (!is_running)
        return;
    measure_distance();

    if (timeout_fired)
    {
        print_timestamp();
        printf("Falha\n");
        return;
    }

    // Caso em que funciona
    if (time_end > time_init)
    {
        int64_t duration = time_end - time_init;
        float distance = (duration * 0.0343) / 2;
        print_timestamp();
        printf("%.2f cm\n", distance);
    }
}

int main()
{
    stdio_init_all();

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);

    gpio_set_irq_enabled_with_callback(
        ECHO_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);

    rtc_init();

    printf("Digite 'Start' para iniciar e 'Stop' para parar.\n");

    bool running = false;
    while (true)
{
    char command[10] = {0};  // Buffer para armazenar o comando
    int index = 0;

    while (index < 9) 
    {   
        int caracter = getchar_timeout_us(100000); // Espera até 100ms 

        if (caracter == PICO_ERROR_TIMEOUT)
        {
            break; 
        }
        else if (caracter == '\n' || caracter == '\r')
        {
            command[index] = '\0'; 
            break;
        }
        else
        {
            command[index++] = (char)caracter;
        }
    }

    if (strcmp(command, "Start") == 0)
    {
        running = true;
        printf("Leitura iniciada.\n");
    }
    else if (strcmp(command, "Stop") == 0)
    {
        running = false;
        printf("Leitura parada.\n");
    }

    read_sensor(running);
    sleep_ms(500);
}
}
