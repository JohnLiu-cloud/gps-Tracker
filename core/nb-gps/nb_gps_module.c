#include "nb_gps_module.h"
#include "nb_gps_cmd.h"
#include "uart_bs.h"
#include "app_timer.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_delay.h"
#include "nrf_log_ctrl.h"
#include "nrf_gpio.h"
#include "boards.h"
#include"cJSON.h"

#define STR_NB_GPS_POWERON "AT command ready"
#define RESPONSE_OK "OK"

#define DEFAULT_MQTT_SERVER_IP "113.92.156.11"
#define DEFAULT_MQTT_TOPIC "tp1/test"

#define DEFAULT_HTTP_URL "http://18.223.112.101/api/up_gps_log"

static uint32_t timer_count = 0;
//static uint32_t gps_start_count = 0;
static uint32_t gps_stop_count = 0;
nb_param_t nb_param;
gps_status_t gps_status;
static bool m_exec_enabled;
static bool m_measu_finish;

gps_value_t gps_value;

#if NRF_MODULE_ENABLED(NB_GPS_MODULE)

#define NRF_LOG_MODULE_NAME nb_gps

#if NB_GPS_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL       NB_GPS_CONFIG_LOG_LEVEL
#else // NB_GPS_CONFIG_LOG_LEVEL
#define NRF_LOG_LEVEL       0
#endif // NB_GPS_CONFIG_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define TIMER_INTERVAL  APP_TIMER_TICKS(1000)
APP_TIMER_DEF(m_nb_gps_timer_id);

static void nb_gps_timer(void * p_context)
{
    timer_count++;
    if( timer_count%10 == 0 ) { //10 sec
      if( gps_status == GPS_STATUS_ON ) {
        m_exec_enabled = true;      
      }
    }
    if( timer_count%60 == 0 ) { //1 min
      if( gps_status == GPS_STATUS_OFF ) {
        gps_stop_count++;
      }
    }
}
void exec_gps_Query_longitude_latitude( void )
{
  if( m_exec_enabled ) {
    m_exec_enabled = false;
    gps_Query_longitude_latitude( 1000 );
  }
}

void exec_gps_Start( void )
{
  bool ret_code;
  if( gps_stop_count >= 5 ) { //5 min
    gps_stop_count = 0;
    NRF_LOG_INFO("exec_gps_Start");
    ret_code = gps_start( 3000 , 3 );
  }
}
static void nb_gps_timers_start(void)
{
   ret_code_t err_code;
    err_code = app_timer_start(m_nb_gps_timer_id, TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the timer module.
 */
static void timers_init(void)
{
    uint32_t err_code;
    err_code = app_timer_create(&m_nb_gps_timer_id, APP_TIMER_MODE_REPEATED, nb_gps_timer);
    APP_ERROR_CHECK(err_code);
}

static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

bool nb_gps_init( void )
{
  bool ret_code;
  timers_init();
  
  nrf_gpio_cfg_output(MODULE_POWER_ON);
  nrf_gpio_pin_set(MODULE_POWER_ON);
  nrf_delay_ms(1000);
  nrf_gpio_pin_clear(MODULE_POWER_ON);

  if( nb_gps_PowerOn() == false ) {
      return false;
  }
  nrf_delay_ms(5000);
  nb_gps_QueryIMEI(3000,2);
  nb_gps_QuerySN(3000,2);
  ret_code = nb_gps_check_SIM( 3000 , 5 );
  ret_code = nb_gps_QueryMooduleFunc( 3000 , 30 );
  ret_code = nb_gps_check_Network_reg( 3000 , 30 );
  nb_gps_QueryOperator( 3000 , 2 );
  ret_code = nb_gps_check_rssi( 3000 , 30 );
  return ret_code;

}

static uint8_t string_to_hex( uint8_t *p_str , uint8_t num , uint8_t str_sepa )
{
  uint8_t ret;
  uint8_t count;
  uint8_t tag;
  uint8_t len = strlen(p_str);
  count = 0;
  tag = 0;
  for( uint8_t i = 0 ; i < len ; i ++ ) {
      if( p_str[ i ] == str_sepa || p_str[ i ] == '\r' ) {
          if( count == num ) {
              //NRF_LOG_INFO("toHex:%d",i);
              if( ( i - tag ) == 1 ) {

                 ret = p_str[ i - 1 ] - 0x30;

              } else if ( ( i - tag ) == 2 ) {

                 ret = ( ( p_str[ i - 2 ] - 0x30 ) * 10 );
                 ret += ( p_str[ i - 1 ] - 0x30 );

              } else if ( ( i - tag ) == 3 ) {
                 ret = ( ( p_str[ i - 3] - 0x30 ) * 100 );
                 ret += ( ( p_str[ i - 2 ] - 0x30 ) * 10 );
                 ret += ( p_str[ i - 1 ] - 0x30 );

              }

              //NRF_LOG_INFO("toHex:%d",ret);
              return ret;          
          }
          count++;
          tag = i + 1;
      }
  }
  return 0;
}

static bool str_to_str( uint8_t *p_str , uint8_t *p_str1 )
{
  uint8_t n;
  uint8_t len = strlen(p_str);
  bool str_start;
  str_start = false;
  n = 0;
  for( uint8_t i = 0 ; i < len ; i ++ ) {
    if( p_str[ i ] == '"' ) {
      if( str_start ) {
        str_start = false;
        return true;
      } else {
        str_start = true;
        continue;
      }
    } 
    if( str_start ) {
      *( p_str1 + n ) = p_str[ i ];
      *( p_str1 + n + 1 ) = 0;
      n++;
    }
  }
  return false;
}
/*
static bool hex_to_string(uint16_t hex , uint8_t *p_str )
{
  if( hex >= 10000 ) {
    return false;
  } else if( hex >= 1000 ) {
    *(p_str+0) = 0x30 + (hex/1000);  
    *(p_str+1) = 0x30 + ((hex/100)%10); 
    *(p_str+2) = 0x30 + ((hex/10)%10); 
    *(p_str+3) = 0x30 + (hex%10);
    *(p_str+4) = 0;
  } else if( hex >= 100 ) {
    *(p_str+0) = 0x30 + (hex/100);  
    *(p_str+1) = 0x30 + ((hex/10)%10); 
    *(p_str+2) = 0x30 + (hex%10);
    *(p_str+3) = 0;   
  } else if( hex >= 10 ) {
    *(p_str+0) = 0x30 + (hex/10);  
    *(p_str+1) = 0x30 + (hex%10);
    *(p_str+2) = 0;     
  } else {
    *(p_str+0) = 0x30 + hex;  
    *(p_str+1) = 0;     
  }
}
*/
bool nb_gps_PowerOn( void )
{
  if( UART_Read_Response( m_rx_buf , 5000 , 0 , STR_NB_GPS_POWERON , NULL) == 1 ) {
      nrf_delay_ms(300);
      if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT", 5000, RESPONSE_OK , NULL) ) {
          return true;
      }
      return false;
  }
  return false;
}

bool nb_gps_check_SIM( uint32_t timeout , uint8_t repeat ) 
{
  uint8_t i;
  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+CPIN?", timeout, "+CPIN: READY" , NULL) ) {
         //NRF_LOG_INFO("check_SIM True");
         return true;
    }
    nrf_delay_ms(2000);
  }
  return false;
}

bool nb_gps_QueryMooduleFunc( uint32_t timeout , uint8_t repeat ) 
{
  uint8_t i;
  uint8_t network;
  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+CFUN?", timeout, "+CFUN: " , NULL) ) {
      network = m_rx_buf[7];
      if( network == '1' ) {
        return true;
      }
    }
    nrf_delay_ms(2000);
  }
  return false;
}

bool nb_gps_check_rssi( uint32_t timeout , uint8_t repeat )  //After 9s rssi:0~31
{
  uint8_t rssi;
  uint8_t i;
  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+CSQ?", timeout, "+CSQ: " , NULL) )
    {
        rssi = string_to_hex( &m_rx_buf[6] , 0 , ',' );
        if( rssi > 1 && rssi <= 31 ) {
          return true;
        }
    }
    nrf_delay_ms(2000);
  }
  return false;
}
bool nb_gps_check_Network_reg( uint32_t timeout , uint8_t repeat )//After 15s
{
    uint32_t ret_code;
    uint8_t network_reg;
    uint8_t i;
    for( i = 0 ; i < repeat ; i++ ) {
        if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+CEREG?", timeout, "+CEREG: " , NULL) )
        {
            //NRF_LOG_HEXDUMP_INFO(m_rx_buf,20);
            network_reg = string_to_hex( &m_rx_buf[8] , 1 , ',' );
            //NRF_LOG_INFO("Network Reg:%d",network_reg);
            if( network_reg == 1 ) {
                return true;
            }  
        }
        nrf_delay_ms(2000);
    }
    return false;
}

bool nb_gps_QueryIMEI( uint32_t timeout , uint8_t repeat ) 
{
  uint8_t rssi;
  uint8_t i;
  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+CGSN?", timeout, "+CGSN: " , NULL) ) {
        str_to_str( &m_rx_buf[7] , nb_param.Imei );
        NRF_LOG_INFO("IMEI:");
        NRF_LOG_HEXDUMP_INFO(nb_param.Imei,strlen(nb_param.Imei));
        return true;
    }
    nrf_delay_ms(2000);
  }
  return false;
}

bool nb_gps_QuerySN( uint32_t timeout , uint8_t repeat ) 
{
  uint8_t rssi;
  uint8_t i;
  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+CFSN?", timeout, "+CFSN: " , NULL) ) {
        str_to_str( &m_rx_buf[7] , nb_param.sn );
        NRF_LOG_INFO("SN:");
        NRF_LOG_HEXDUMP_INFO(nb_param.sn,strlen(nb_param.sn));
        return true;
    }
    nrf_delay_ms(2000);
  }
  return false;
}


bool nb_gps_QueryOperator( uint32_t timeout , uint8_t repeat ) 
{
  uint8_t i;
  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+COPS?", timeout, "+COPS: " , NULL) ) {
        str_to_str( &m_rx_buf[7] , nb_param.Operator );
        NRF_LOG_INFO("Oper:");
        NRF_LOG_HEXDUMP_INFO(nb_param.Operator,strlen(nb_param.Operator));
        return true;
    }
    nrf_delay_ms(2000);
  }
  return false;
}


#if NB_GPS_MQTT_ENABLED
//AT+CGDCONT=1,"IP","CTLTE"
bool nb_APN_setting( uint32_t timeout , uint8_t repeat ) 
{
  uint8_t i;
  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+CGDCONT=1,\"IP\",\"CTLTE\"", timeout, RESPONSE_OK , NULL) ) {
        NRF_LOG_INFO("APN OK.");
        return true;
    }
    nrf_delay_ms(2000);
  }
  return false;
}
//AT+MIPCALL=1
bool nb_Mqtt_Create_aWirelessLick( uint32_t timeout , uint8_t repeat ) 
{
  uint8_t i;
  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+MIPCALL=1", timeout, "+MIPCALL: " , NULL) ) {
        if( m_rx_buf[10] != '0' ) {
          return true;
        }
/*
        for( uint8_t n=0 ; n<NB_IP_LEN ; n++ ) {
          nb_param.nb_Ip[n] = string_to_hex( &m_rx_buf[10] , n , '.' );
        }
        NRF_LOG_INFO("IP:");
        NRF_LOG_HEXDUMP_INFO(nb_param.nb_Ip,NB_IP_LEN);
        return true;
*/
    }
    nrf_delay_ms(2000);
  }
  return false;
}

//AT+MQTTOPEN=1,"113.116.128.8",1883,0,10
bool nb_Mqtt_Establish_Connect( uint32_t timeout , uint8_t repeat ) 
{
  uint8_t i;
  uint8_t Establish_Connect;
  uint8_t atCmd[128];
  memset( atCmd , 0 , sizeof(atCmd) );
  sprintf( atCmd , "AT+MQTTOPEN=1,\"%s\",1883,0,10" , nb_param.MqttServerIp );

  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse(atCmd, timeout, "+MQTTOPEN: " , NULL) ) {
      Establish_Connect = string_to_hex( &m_rx_buf[11] , 1 , ',' );
      //NRF_LOG_INFO("nb_gps_mqtt_Establish_Connect %d",Establish_Connect);
      if( Establish_Connect == 1 ) {
        return true;
      }        
    }
    nrf_delay_ms(2000);
  }
  return false;
}


//AT+MQTTPUB=1,"tp1/test",2,1,"Hello" 
bool nb_MqttPublish_dat( uint8_t *p_str , uint32_t timeout , uint8_t repeat ) 
{
  uint8_t i;
  uint8_t pub_success;
  uint8_t atCmd[128];
  memset( atCmd , 0 , sizeof(atCmd) );
  sprintf( atCmd , "AT+MQTTPUB=1,\"%s\",2,1,\"%s\"" , nb_param.MqttTopic , p_str );

  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse( atCmd , timeout, "+MQTTPUB: " , NULL) ) {
      pub_success = string_to_hex( &m_rx_buf[10] , 1 , ',' );
      //NRF_LOG_INFO("nb_gps_publish_dat %d",pub_success);
      if( pub_success == 1 ) {
        return true;
      }        
    }
    nrf_delay_ms(2000);
  }
  return false;
}

bool nb_mqttInit( void )
{
  bool ret_code;
  memset( nb_param.MqttServerIp , 0 ,sizeof(nb_param.MqttServerIp) );
  nb_param.MqttServerIp_len = strlen(DEFAULT_MQTT_SERVER_IP);
  memcpy( nb_param.MqttServerIp , DEFAULT_MQTT_SERVER_IP , nb_param.MqttServerIp_len );

  memset( nb_param.MqttTopic , 0 ,sizeof(nb_param.MqttTopic) );
  nb_param.MqttTopic_len = strlen(DEFAULT_MQTT_TOPIC);
  memcpy( nb_param.MqttTopic , DEFAULT_MQTT_TOPIC , nb_param.MqttTopic_len );

  ret_code = nb_APN_setting( 3000 , 3 );
  ret_code = nb_Mqtt_Create_aWirelessLick( 5000 , 3 );
  ret_code = nb_Mqtt_Establish_Connect( 5000 , 3 );
  return ret_code;
}

void nb_mqtt_pub_test( void )
{
  bool ret_code;
  ret_code = nb_MqttPublish_dat( "Hello" , 5000 , 3 );
  nrf_delay_ms(500);
  ret_code = nb_MqttPublish_dat( "John" , 5000 , 3 );
}
#endif // NB_GPS_MQTT_ENABLED

#if NB_GPS_HTTP_ENABLED

bool nb_Http_Create_aWirelessLick( uint32_t timeout , uint8_t repeat ) 
{
  uint8_t i;
  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+MIPCALL=1,\"CTLTE\"", timeout, "+MIPCALL: " , NULL) ) {
        if( m_rx_buf[10] != '0' ) {
          return true;
        }
    }
    nrf_delay_ms(2000);
  }
  return false;
}

bool nb_Http_set_url( uint32_t timeout , uint8_t repeat ) 
{
  uint8_t i;
  uint8_t Establish_Connect;
  uint8_t atCmd[128];
  memset( atCmd , 0 , sizeof(atCmd) );
  sprintf( atCmd , "AT+HTTPSET=\"URL\",\"%s\"" , nb_param.HttpUrl );


  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse(atCmd, timeout, RESPONSE_OK , NULL) ) {
      return true;       
    }
    nrf_delay_ms(2000);
  } 
  return false;
}

bool nb_Http_set_Type( uint32_t timeout , uint8_t repeat ) 
{
  uint8_t i;
  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+HTTPSET=\"CONTYPE\",\"application/json\"", timeout, RESPONSE_OK , NULL) ) {
        if( m_rx_buf[10] != '0' ) {
          return true;
        }
    }
    nrf_delay_ms(2000);
  }
  return false;
}



bool nb_Http_Post_PublishDat( uint8_t *p_str , uint32_t timeout , uint8_t repeat ) 
{
  uint8_t i;
  uint8_t atCmd[128];
  memset( atCmd , 0 , sizeof(atCmd) );
  sprintf( atCmd , "AT+HTTPDATA=%d" , strlen(p_str) );

  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse(atCmd, timeout, ">" , NULL) ) {
       UART_Write_data(p_str);
       if( 1== UART_Read_Response(m_rx_buf, timeout, 0, RESPONSE_OK, NULL) ) {
          return true;
       }
    }
    nrf_delay_ms(2000);
  }
  return false;
}


bool nb_Http_Post_Start( uint32_t timeout , uint8_t repeat ) 
{
  uint8_t i;
  uint16_t status;
  for( i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+HTTPACT=1", timeout, "+HTTPRES: " , NULL) ) {
        status = string_to_hex( &m_rx_buf[10] , 1 , ',' );
        if( status == 200 ) {
          NRF_LOG_INFO("+HTTPRES: 200");
          return true;
        }
    }
    nrf_delay_ms(500);
  }
  return false;
}


/*
bool device_post_JoinNetwork(void)
{

  cJSON *pstFeat = NULL;
  cJSON *json = cJSON_CreateObject();
  cJSON_AddItemToObject(json,"device",cJSON_CreateString("John"));

  cJSON_AddNumberToObject(json,"longitude",(12.123));
  cJSON_AddNumberToObject(json,"latitude",(345.1234));

  uint8_t* json_string = cJSON_PrintUnformatted(json);//cJSON_Print(json);

  NRF_LOG_HEXDUMP_INFO( json_string , strlen(json_string) );
  nb_Http_Post_PublishDat( json_string , 5000 , 1 );
  cJSON_Delete(json);
  return false;
}
*/
static bool nb_gps_post_Dat( void )
{
  uint8_t json[128];
  sprintf( json , "{\"device\":\"John\",\"longitude\":%s,\"latitude\":%s}" , nb_param.longitude , nb_param.latitude   );
  
  NRF_LOG_HEXDUMP_INFO( json , strlen(json) );
  nb_Http_Post_PublishDat( json , 5000 , 1 );

  return false;
}

bool nb_HttpInit( void )
{
  bool ret_code;
  memset( nb_param.HttpUrl , 0 ,sizeof(nb_param.HttpUrl) );
  nb_param.HttpUrl_len = strlen(DEFAULT_HTTP_URL);
  memcpy( nb_param.HttpUrl , DEFAULT_HTTP_URL , nb_param.HttpUrl_len );

  ret_code = nb_Http_Create_aWirelessLick( 3000 , 3 );  
  ret_code = nb_Http_set_url( 3000 , 3 );  
  ret_code = nb_Http_set_Type( 3000 , 3 );
  //memcpy( nb_param.latitude , "12.234" , 6);
  //memcpy( nb_param.longitude , "123.789" , 7);
  //m_measu_finish = true;

  //exec_nb_Http_port();
  nrf_delay_ms(1000);
  return ret_code;
}

bool exec_nb_Http_port(void)
{
  bool ret_code;
  //ret_code = nb_Http_Post_PublishDat( HTTP_TEST, 3000 , 3 );
  if( m_measu_finish ) {
    //device_post_JoinNetwork();
    m_measu_finish = false;

    nb_gps_post_Dat();
    nrf_delay_ms(200);
    ret_code = nb_Http_Post_Start( 15000 , 1 );
  }
  return ret_code;
}
#endif // NB_GPS_HTTP_ENABLED

bool gps_start( uint32_t timeout , uint8_t repeat ) 
{
  for( uint8_t i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+GTGPSPOWER=1", timeout, RESPONSE_OK , NULL) ) {
      gps_status = GPS_STATUS_ON;
      return true;       
    }
    nrf_delay_ms(2000);
  } 
  return false;
}
bool gps_stop( uint32_t timeout , uint8_t repeat ) 
{
  for( uint8_t i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+GTGPSPOWER=0", timeout, RESPONSE_OK , NULL) ) {
      gps_status = GPS_STATUS_OFF;
      nrf_delay_ms(1000);
      return true;       
    }
    nrf_delay_ms(2000);
  } 
  return false;
}

bool gps_QueryMode( uint32_t timeout , uint8_t repeat ) 
{
  for( uint8_t i = 0 ; i < repeat ; i++ ) {
    if ( 1 == UART_Write_ATCommand_And_ReadResponse("AT+GTGPSCFG?", timeout, RESPONSE_OK , NULL) ) {
      return true;       
    }
    nrf_delay_ms(2000);
  } 
  return false;
}
bool gps_Query_longitude_latitude( uint32_t timeout ) 
{
  int8_t ret_cpde;
  uint16_t Current;
  uint16_t Prev;
  uint16_t len;
  bool active;
  active = false;
  Current = 0;
  Prev = 0;
  ret_cpde = UART_Write_ATCommand_And_ReadResponse("AT+GTGPS?", timeout, "$GNRMC" , RESPONSE_OK ) ;//GNRMC GPRMC
  if ( 1 == ret_cpde ) {
        for( uint16_t i = 0 ; i < strlen(m_rx_buf) ; i++ ) {
          if( m_rx_buf[i] == ',' ) {
            Prev = Current;
            Current = i;
          }
          if( m_rx_buf[i] == 'A' ) {
            active = true;
          } 
          if( m_rx_buf[i] == 'V' ) {
            active = false;
          } 
          if( m_rx_buf[i] == 'N' && active ) {
            len = Current-Prev-1;
            memcpy( nb_param.latitude , &m_rx_buf[Prev+1] , len );
            nb_param.latitude[len] = 0;
          } else if( m_rx_buf[i] == 'S' && active ) {
            len = Current-Prev-1;
            memcpy( nb_param.latitude , &m_rx_buf[Prev+1] , len );  
            nb_param.latitude[len] = 0;
          }
          if( m_rx_buf[i] == 'E' && active ) {
            len = Current-Prev-1;
            memcpy( nb_param.longitude , &m_rx_buf[Prev+1] , len );
            nb_param.longitude[len] = 0;
            m_measu_finish = true;
            NRF_LOG_INFO("GPS:%s %s",nb_param.latitude , nb_param.longitude);
            gps_stop( 3000 , 1 );
            return true;
          }
        }
    return false;       
  } 
  return false;
}

bool gps_init( void )
{
  bool ret_code;
  nrf_delay_ms(2000);
  ret_code = gps_start( 3000 , 3 );
  gps_QueryMode( 3000 , 3 );
  nb_gps_timers_start();
  return ret_code;
}
#endif //#if NRF_MODULE_ENABLED(NB_GPS_MODULE)