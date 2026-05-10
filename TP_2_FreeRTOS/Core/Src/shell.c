#include "shell.h"

#include <stdio.h>
#include <string.h>

#include "usart.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <stdlib.h>

extern uint32_t led_period;
extern QueueHandle_t shellQueue;

extern char spam_msg[64];
extern uint32_t spam_count;


typedef struct {
    char c;
    int (* func)(int argc, char ** argv);
    char * description;
} shell_func_t;

static int shell_func_list_size = 0;
static shell_func_t shell_func_list[SHELL_FUNC_LIST_MAX_SIZE];

static char print_buffer[BUFFER_SIZE];

extern TaskHandle_t ledTaskHandle;

int sh_stop(int argc, char **argv)
{
    vTaskSuspend(ledTaskHandle);
    printf("LED task suspended\r\n");
    return 0;
}

int sh_start(int argc, char **argv)
{
    vTaskResume(ledTaskHandle);
    printf("LED task resumed\r\n");
    return 0;
}

static int uart_write(char * s, uint16_t size) {
    HAL_UART_Transmit(&UART_DEVICE, (uint8_t*)s, size, HAL_MAX_DELAY);
    return size;
}

static char uart_read() {
    char c;
    xQueueReceive(shellQueue, &c, portMAX_DELAY);
    return c;
}

static int sh_help(int argc, char ** argv) {
    for(int i = 0 ; i < shell_func_list_size ; i++) {
        int size = snprintf(print_buffer, BUFFER_SIZE,
                            "%c: %s\r\n",
                            shell_func_list[i].c,
                            shell_func_list[i].description);
        uart_write(print_buffer, size);
    }
    return 0;
}

int sh_led(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: l <periode_ms>\r\n");
        return -1;
    }

    led_period = atoi(argv[1]);
    printf("LED period set to %lu ms\r\n", led_period);
    return 0;
}

void shell_rx_callback(uint8_t c)
{
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(shellQueue, &c, &woken);
    portYIELD_FROM_ISR(woken);
}

int sh_spam(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage: s <message> <count>\r\n");
        return -1;
    }

    strncpy(spam_msg, argv[1], sizeof(spam_msg));
    spam_msg[sizeof(spam_msg)-1] = '\0';

    spam_count = atoi(argv[2]);

    printf("Spamming '%s' %lu times\r\n", spam_msg, spam_count);
    return 0;
}

int shell_add(char c, int (* pfunc)(int argc, char ** argv), char * description) {
    if (shell_func_list_size < SHELL_FUNC_LIST_MAX_SIZE) {
        shell_func_list[shell_func_list_size].c = c;
        shell_func_list[shell_func_list_size].func = pfunc;
        shell_func_list[shell_func_list_size].description = description;
        shell_func_list_size++;
        return 0;
    }
    return -1;
}

void shell_init() {
    int size = snprintf(print_buffer, BUFFER_SIZE,
                        "\r\n===== Monsieur Shell v0.2 =====\r\n");
    uart_write(print_buffer, size);

    shell_add('h', sh_help, "Help");

    shell_add('l', sh_led, "LED blink period");

    shell_add('s', sh_spam, "Spam message");

    shell_add('p', sh_stop, "Pause LED task");

    shell_add('r', sh_start, "Resume LED task");

}


static int shell_exec(char * buf) {
    char c = buf[0];
    int argc = 1;
    char * argv[ARGC_MAX];
    argv[0] = buf;

    for(char *p = buf ; *p != '\0' && argc < ARGC_MAX ; p++){
        if(*p == ' ') {
            *p = '\0';
            argv[argc++] = p+1;
        }
    }

    for(int i = 0 ; i < shell_func_list_size ; i++) {
        if (shell_func_list[i].c == c) {
            return shell_func_list[i].func(argc, argv);
        }
    }

    int size = snprintf(print_buffer, BUFFER_SIZE,
                        "%c: no such command\r\n", c);
    uart_write(print_buffer, size);
    return -1;
}

void shell_task(void *arg)
{
    char cmd_buffer[BUFFER_SIZE];
    int pos = 0;

    uart_write("> ", 2);

    for (;;)
    {
        char c = uart_read();

        switch (c)
        {
        case '\r':
        case '\n':
            uart_write("\r\n", 2);
            cmd_buffer[pos] = '\0';
            shell_exec(cmd_buffer);
            pos = 0;
            uart_write("> ", 2);
            break;

        case '\b':
            if (pos > 0) {
                pos--;
                uart_write("\b \b", 3);
            }
            break;

        default:
            if (pos < BUFFER_SIZE - 1) {
                cmd_buffer[pos++] = c;
                uart_write(&c, 1);
            }
            break;
        }
    }
}
