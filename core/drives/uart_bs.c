#include "uart_bs.h"
#include "app_uart.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include <stdint.h>
#include <string.h>

uint8_t m_rx_buf[UART_RX_BUF_SIZE];


bool UART_Write_ATCommand(const uint8_t *strAtCommand)
{
    ret_code_t err_code;

    uint8_t dummy;
    while (app_uart_get(&dummy) == NRF_SUCCESS);

    uint8_t length = strlen(strAtCommand);

    //NRF_LOG_INFO("%s",strAtCommand);

    for (uint16_t i = 0; i <length+2; i++)
    {
        do
        {
            if (length == i) err_code = app_uart_put('\r');
            else if (length+1 == i) err_code = app_uart_put('\n');
            else err_code = app_uart_put(strAtCommand[i]);

            if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
            {
                NRF_LOG_ERROR("Failed send_at_command. Error 0x%x. ", err_code);
                APP_ERROR_CHECK(err_code);
                return false;
            }
        } while (err_code == NRF_ERROR_BUSY);
    }
}

bool UART_Write_data(const uint8_t *strDat)
{
    ret_code_t err_code;

    uint8_t dummy;
    while (app_uart_get(&dummy) == NRF_SUCCESS);

    uint16_t length = strlen(strDat);

    //NRF_LOG_INFO("%s",strDat);

    for (uint16_t i = 0; i <length; i++)
    {
        do
        {
            err_code = app_uart_put(strDat[i]);

            if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
            {
                NRF_LOG_ERROR("Failed send_at_command. Error 0x%x. ", err_code);
                APP_ERROR_CHECK(err_code);
                return false;
            }
        } while (err_code == NRF_ERROR_BUSY);

        if ( (i%64) == 0 ) nrf_delay_ms(100);
    }
}

bool UART_get(uint8_t *rx_data, int32_t *timeout_us)
{
    while (app_uart_get(rx_data) != NRF_SUCCESS)
    {
        (*timeout_us) -= 10;
        if (*timeout_us <= 0)
        {
            return false;
        }
        nrf_delay_us(10);
    }
    return true;
}

uint8_t UART_Read_Response(uint8_t *rx_data, uint32_t timeout_ms, uint16_t len, const uint8_t *end_text, const uint8_t *end_text2)
{
    uint32_t timeout_us = timeout_ms * 1000;
    for (int i = 0; i < sizeof(rx_data); i++)
        rx_data[i] = '\0';
    len = len != 0 ? len : UART_RX_BUF_SIZE - 1;
    uint8_t end_text_len = end_text ? strlen(end_text) : 0;
    uint8_t end_text2_len = end_text2 ? strlen(end_text2) : 0;
    for (uint16_t x = 0; x < len; )
    {

        if (rx_data == m_rx_buf && x + 1 == len && len == UART_RX_BUF_SIZE - 1)
        {
            x = x / 2;
            for (uint16_t xx = 0; xx + x < len; xx++)
            {
                rx_data[xx] = rx_data[xx + x];
            }
            rx_data[x] = '\0';
        }

        rx_data[x + 1] = '\0';
        if (!UART_get(&(rx_data[x]), &timeout_us))
        {
            NRF_LOG_INFO("----RES TIMOUT NG [%s] %d", rx_data, x);
            return 0;
        }
        if( rx_data[x] == '\n'&&rx_data[x-1] == '\r' ) {
          if (end_text != NULL && end_text_len - 1 <= x && memcmp(rx_data , end_text , end_text_len ) == 0 ) // x - 2 - end_text_len + 1
          {
              //NRF_LOG_INFO("----RES OK [%s]", rx_data);
              return 1;
          }
          if (end_text2 != NULL && end_text2_len - 1 <= x && memcmp(rx_data , end_text2 , end_text2_len ) == 0) //x - 2 - end_text2_len + 1
          {
              //NRF_LOG_INFO("----RES NG [%s]", rx_data);
              return 2;
          }   
          if( end_text == NULL && end_text2 == NULL )
          {
              return 3;
          }
          //printf("rx_data [%s]",rx_data);
          x = 0;       
        } else {
          x++;
        }
    }
    return 10;
}

int8_t UART_Write_ATCommand_And_ReadResponse(const uint8_t *atCommand, int32_t timeout_ms, const uint8_t *expected_answer1, const uint8_t *expected_answer2)
{
    uint8_t dummy;
    while (app_uart_get(&dummy) == NRF_SUCCESS);

    UART_Write_ATCommand(atCommand);
    return UART_Read_Response(m_rx_buf, timeout_ms, 0, expected_answer1, expected_answer2);
}



void uart_test()
{
  uint32_t ret_code;
  ret_code = UART_Write_ATCommand_And_ReadResponse("AT+CPIN?",6000,"OK",NULL);
  NRF_LOG_INFO("UART TEST %d",ret_code);
}