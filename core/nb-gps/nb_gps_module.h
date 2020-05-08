#ifndef NB_GPS_MODULE_H_
#define NB_GPS_MODULE_H_
#include <stdint.h>
#include <string.h>
#include "stdbool.h"
#include "config.h"
typedef enum {
  GPS_STATUS_ON           = 0,
  GPS_STATUS_OFF          = 1,
}gps_status_t;

#define NB_IMEI_LEN 15
#define NB_SN_LEN 10

#define NB_OPERATOR_MAX_LEN 20
//Mqtt
#define NB_MQTT_SERVER_IP_MAX_LEN 16
#define NB_MQTT_TOPIC_MAX_LEN 64
//Http
#define NB_HTTP_URL_MAX_LEN 64
typedef struct{
  uint8_t Imei[NB_IMEI_LEN];
  uint8_t sn[NB_SN_LEN];
  uint8_t Operator[ NB_OPERATOR_MAX_LEN ];
#if NB_GPS_MQTT_ENABLED
  uint8_t MqttServerIp[ NB_MQTT_SERVER_IP_MAX_LEN ];
  uint8_t MqttServerIp_len;
  uint8_t MqttTopic[ NB_MQTT_TOPIC_MAX_LEN ];
  uint8_t MqttTopic_len;
#endif
#if NB_GPS_HTTP_ENABLED
  uint8_t HttpUrl[NB_HTTP_URL_MAX_LEN];
  uint8_t HttpUrl_len;
#endif 
  uint8_t longitude[32];
  uint8_t latitude[32];
}nb_param_t;


typedef struct {
float longitude;
float latitude;
}gps_value_t;

extern gps_value_t gps_value;

bool nb_gps_init( void );
bool nb_gps_PowerOn( void );
bool nb_gps_check_rssi( uint32_t timeout , uint8_t repeat );
bool nb_gps_check_SIM( uint32_t timeout , uint8_t repeat );
bool nb_gps_QueryMooduleFunc( uint32_t timeout , uint8_t repeat );
bool nb_gps_check_Network_reg( uint32_t timeout , uint8_t repeat );
bool nb_gps_QueryOperator( uint32_t timeout , uint8_t repeat ) ;
bool nb_gps_QuerySN( uint32_t timeout , uint8_t repeat ) ;
bool nb_gps_QueryIMEI( uint32_t timeout , uint8_t repeat ) ;

bool nb_Mqtt_Create_aWirelessLick( uint32_t timeout , uint8_t repeat ) ;
bool nb_Mqtt_Establish_Connect( uint32_t timeout , uint8_t repeat ) ;
bool nb_APN_setting( uint32_t timeout , uint8_t repeat ) ;
bool nb_mqttInit( void );
void nb_mqtt_pub_test( void );

bool nb_HttpInit( void );
bool exec_nb_Http_port(void);

bool gps_init( void );
void exec_gps_Start( void );
bool gps_start( uint32_t timeout , uint8_t repeat ) ;
bool gps_stop( uint32_t timeout , uint8_t repeat ) ;
bool gps_Query_longitude_latitude( uint32_t timeout );
void nb_CreateJson_Test(void);

bool device_post_JoinNetwork(void);
void exec_gps_Query_longitude_latitude( void );
#endif //NB_GPS_MODULE_H_