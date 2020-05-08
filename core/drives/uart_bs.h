#ifndef  UART_BS_H_
#define  UART_BS_H_
#include <stdint.h>
#include <string.h>
#include "stdbool.h"

#define UART_RX_BUF_SIZE     256
extern uint8_t m_rx_buf[UART_RX_BUF_SIZE];


bool UART_Write_data(const uint8_t *strDat);
int8_t UART_Write_ATCommand_And_ReadResponse(const uint8_t *atCommand, int32_t timeout_ms, const uint8_t *expected_answer1, const uint8_t *expected_answer2);
uint8_t UART_Read_Response(uint8_t *rx_data, uint32_t timeout_ms, uint16_t len, const uint8_t *end_text, const uint8_t *end_text2);
bool UART_Write_ATCommand(const uint8_t *strAtCommand);

void uart_test();
#endif //UART_BS_H_