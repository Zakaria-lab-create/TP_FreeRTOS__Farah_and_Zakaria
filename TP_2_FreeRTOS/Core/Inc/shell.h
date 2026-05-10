#ifndef INC_LIB_SHELL_SHELL_H_
#define INC_LIB_SHELL_SHELL_H_

#include <stdint.h>

#define UART_DEVICE huart1

#define ARGC_MAX 8
#define BUFFER_SIZE 40
#define SHELL_FUNC_LIST_MAX_SIZE 64

void shell_init(void);
int shell_add(char c, int (* pfunc)(int argc, char ** argv), char * description);
void shell_task(void *arg);
void shell_rx_callback(uint8_t c);

int sh_stop(int argc, char **argv);

int sh_start(int argc, char **argv);

int sh_led(int argc, char **argv);



#endif
