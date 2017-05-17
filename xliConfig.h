//  xliConfig.h - Xlight SmartController configuration header

#ifndef xliConfig_h
#define xliConfig_h

/*** USER DEFINES:  ***/
#define FAILURE_HANDLING
//#define SYS_SERIAL_DEBUG
//#define SERIAL_DEBUG
//#define MAINLOOP_TIMER

/**********************/

#define PRIPSTR "%s"
#define pgm_read_word(p) (*(p))
#ifdef SERIAL_DEBUG
  #define IF_SERIAL_DEBUG(x) ({x;})
#else
  #define IF_SERIAL_DEBUG(x)
#endif

#ifdef MAINLOOP_TIMER
  #define IF_MAINLOOP_TIMER(x, name) ({unsigned long ulStart = millis(); x; SERIAL_LN("%s spent %lu ms", (name), millis() - ulStart);})
#else
  #define IF_MAINLOOP_TIMER(x, name) ({x;})
#endif

// Change it only if Config_t structure is updated
#define VERSION_CONFIG_DATA       11

// Xlight Application Identification
#define XLA_ORGANIZATION          "xlight.ca"               // Default value. Read from EEPROM
#define XLA_PRODUCT_NAME          "XController"             // Default value. Read from EEPROM
#define XLA_AUTHORIZATION         "use-token-auth"
#define XLA_TOKEN                 "your-access-token"       // Can update online

//------------------------------------------------------------------
// System level working constants
//------------------------------------------------------------------
// Running Time Environment Parameters
#define RTE_DELAY_PUBLISH         60          // Maximum publish data refresh time in seconds
#define RTE_DELAY_SYSTIMER        10          // System Timer interval, can be very fast, e.g. 50 means 25ms
#define RTE_DELAY_SELFCHECK       100         // Self-check interval
#define RTE_CLOUD_CONN_TIMEOUT    3500        // Timeout for connecting to the Cloud
#define RTE_WIFI_CONN_TIMEOUT     20000       // Timeout for attempting to connect WIFI
#define RTE_WATCHDOG_TIMEOUT      30000       // Maxium WD feed duration

// Number of ticks on System Timer
#define RTE_TICK_FASTPROCESS			1						// Pace of execution of FastProcess

// Keep alive message timeout
#define RTE_TM_KEEP_ALIVE         15

// Panel Operarion Timers
#define RTE_TM_MAX_CCT_IDLE       6           // Maximum idle time (seconds) in CCT control mode
#define RTE_TM_HELD_TO_DFU        30          // Held duration threshold for DFU
#define RTE_TM_HELD_TO_WIFI       15          // Held duration threshold to enter Wi-Fi setup
#define RTE_TM_HELD_TO_RESET      10          // Held duration threshold to reset
#define RTE_TM_HELD_TO_BASENW     5           // Held duration threshold to enable Base Network

// Maximum number of rows for any working memory table implimented using ChainClass
#define MAX_TABLE_SIZE              8

// Maximum number of device associated to one controller
#define MAX_DEVICE_PER_CONTROLLER   16

// Maximum number of nodes under one controller
#define MAX_NODE_PER_CONTROLLER     64

// Maximum conditions within a rule
#define MAX_CONDITION_PER_RULE      2

// Default value for maxBaseNetworkDuration (in seconds)
#define MAX_BASE_NETWORK_DUR    180

// Maximum JSON data length
#define COMMAND_JSON_SIZE				64
#define SENSORDATA_JSON_SIZE		196

// Maximum RF messages buffered
#define MQ_MAX_RF_RCVMSG        5
#define MQ_MAX_RF_SNDMSG        8

// Maximum Cloud Command messages buffered
#define MQ_MAX_CLOUD_MSG        6

// NodeID Convention
#define NODEID_GATEWAY          0
#define NODEID_MAINDEVICE       1
#define NODEID_MIN_DEVCIE       8
#define NODEID_MAX_DEVCIE       63
#define NODEID_MIN_REMOTE       64
#define NODEID_MAX_REMOTE       127
#define NODEID_PROJECTOR        128
#define NODEID_SMARTPHONE       139
#define NODEID_MIN_GROUP        192
#define NODEID_MAX_GROUP        223
#define NODEID_DUMMY            255

#define BR_MIN_VALUE            1
#define CT_MIN_VALUE            2700
#define CT_MAX_VALUE            6500
#define CT_SCOPE                38
#define CT_STEP                 ((CT_MAX_VALUE-CT_MIN_VALUE)/10)

#endif /* xliConfig_h */
