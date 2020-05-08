#ifndef CONFIG_H_
#define CONFIG_H_


// <q> NB_GPS_MODULE_ENABLED  - NB-GPS Module
#ifndef NB_GPS_MODULE_ENABLED
#define NB_GPS_MODULE_ENABLED 1
#endif
// <q> NB_GPS_CONFIG_LOG_ENABLED -NB-GPS log enabled
#ifndef NB_GPS_CONFIG_LOG_ENABLED
#define NB_GPS_CONFIG_LOG_ENABLED  1
#endif 

// <o> NB_GPS_CONFIG_LOG_LEVEL  - Default Severity level
 
// <0=> Off 
// <1=> Error 
// <2=> Warning 
// <3=> Info 
// <4=> Debug 
#ifndef NB_GPS_CONFIG_LOG_LEVEL
#define NB_GPS_CONFIG_LOG_LEVEL 4
#endif

#ifndef NB_GPS_MQTT_ENABLED 
#define NB_GPS_MQTT_ENABLED   0
#endif 

#ifndef NB_GPS_HTTP_ENABLED 
#define NB_GPS_HTTP_ENABLED   1
#endif 

#endif //CONFIG_H_