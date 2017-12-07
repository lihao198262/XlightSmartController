//  xlCommon.h - Xlight common definitions header

#ifndef xliCommon_h
#define xliCommon_h

#include "application.h"
#include "xliConfig.h"

#define BITTEST(var,pos)          (((var)>>(pos)) & 0x0001)
#define BITMASK(pos)              (0x0001 << (pos))
#define BITSET(var,pos)           ((var) | BITMASK(pos))
#define BITUNSET(var,pos)         ((var) & (~BITMASK(pos)))
#define _BV(x)                    (1<<(x))

//Here, common data types have been given alternative names through #define statements

// Common Data Type
#define BOOL                      boolean
#define UC                        uint8_t
#define US                        uint16_t
#define UL                        uint32_t
#define S_BYTE                    int8_t
#define SHORT                     int16_t
#define LONG                      int32_t

// Device Status
#define STATUS_OFF                0x00        // Power Off
#define STATUS_INIT               0x01        // Initializing
#define STATUS_BMW                0x02        // Basic Mode of Working
#define STATUS_DIS                0x03        // WAN Disconnected
#define STATUS_NWS                0x04        // Normal Working Status
#define STATUS_SLP                0x05        // Sleep
#define STATUS_RST                0x06        // Reseting
#define STATUS_ERR                0x07        // Fatal Error

// Cloud Dependency
#define CLOUD_DISABLE             0x00        // Disable Cloud
#define CLOUD_ENABLE              0x01        // Default, Enable Cloud, use it if available
#define CLOUD_MUST_CONNECT        0x02        // Enable Cloud and get connected anyhow

// Serial Port Speed
#define SERIALPORT_SPEED_LOW      9600
#define SERIALPORT_SPEED_14400    14400
#define SERIALPORT_SPEED_MID      19200
#define SERIALPORT_SPEED_57600    57600
#define SERIALPORT_SPEED_HIGH     115200
#define SERIALPORT_SPEED_DEFAULT  SERIALPORT_SPEED_HIGH

// Sensor Read Speed (in seconds)
#define SEN_DHT_SPEED_LOW         30
#define SEN_DHT_SPEED_NORMAL      5
#define SEN_DHT_SPEED_HIGH        2

#define SEN_ALS_SPEED_LOW         10
#define SEN_ALS_SPEED_NORMAL      5
#define SEN_ALS_SPEED_HIGH        2

#define SEN_PIR_SPEED_LOW         4
#define SEN_PIR_SPEED_NORMAL      2
#define SEN_PIR_SPEED_HIGH        1

// Row State Flags for Sync between Cloud, Flash, and Working Memory
enum OP_FLAG {GET, POST, PUT, DELETE};
enum FLASH_FLAG {UNSAVED, SAVED};
enum RUN_FLAG {UNEXECUTED, EXECUTED};

//enum values for CldJSONCommand()
enum COMMAND {CMD_SERIAL, CMD_POWER, CMD_COLOR, CMD_BRIGHTNESS, CMD_SCENARIO, CMD_CCT, CMD_QUERY, CMD_EFFECT, CMD_EXT};

// Switch value for set power command
#define DEVICE_SW_OFF               0       // Turn Off
#define DEVICE_SW_ON                1       // Turn On
#define DEVICE_SW_TOGGLE            2       // Toggle
#define DEVICE_SW_DUMMY             3       // Detail followed

// Update operator for set brightness & CCT command
#define OPERATOR_SET                0
#define OPERATOR_ADD                1
#define OPERATOR_SUB                2
#define OPERATOR_MUL                3
#define OPERATOR_DIV                4

// Filter (special effect)
#define FILTER_SP_EF_NONE           0
#define FILTER_SP_EF_BREATH         1       // Normal breathing light
#define FILTER_SP_EF_FAST_BREATH    2       // Fast breathing light
#define FILTER_SP_EF_FLORID         3       // Randomly altering color
#define FILTER_SP_EF_FAST_FLORID    4       // Fast randomly altering color

// Macros for UID identifiers
#define CLS_RULE                  'r'
#define CLS_SCHEDULE              'a'
#define CLS_SCENARIO              's'
#define CLS_NOTIFICATION          'n'
#define CLS_SENSOR                'e'
#define CLS_LIGHT_STATUS          'h'
#define CLS_TOPOLOGY              't'
#define CLS_CONFIGURATION         'c'

// Node type
#define NODE_TYP_GW               'g'
#define NODE_TYP_LAMP             'l'
#define NODE_TYP_REMOTE           'r'
#define NODE_TYP_SYSTEM           's'
#define NODE_TYP_THIRDPARTY       't'

// BLE Configuration Settings
#define XLIGHT_BLE_SSID           "Xlight"        // BT module name
#define XLIGHT_BLE_CLASS          0x9A050C        // Class of Device (CoD)
#define XLIGHT_BLE_PIN            "1234"          // Pairing password for the BT module

// Sensor list: maximun 16 sensors
typedef enum
{
  sensorDHT = 0,
  sensorALS,
  sensorMIC,
  sensorVIBRATION,
  sensorPIR,
  sensorSMOKE,
  sensorGAS,
  sensorDUST,
  sensorLEAK,
  sensorBEAT,
  sensorIRKey = 15,
  sensorDHT_h = 16, // won't occupy bit position
  sensorMIC_b,
  sensorPM25,
  sensorPM10,
  sensorTVOC,
  sensorCH2O,
  sensorCO2
} sensors_t;

// Sensor scope
#define SR_SCOPE_CONTROLLER         0     // Sensor on controller
#define SR_SCOPE_NODE               1     // Sensor on specific node
#define SR_SCOPE_ANY                2     // Sensor on any node under the controller
#define SR_SCOPE_GROUP              3     // Sensor on within the same node group

// Sensor logic symbols
#define SR_SYM_EQ                   0     // Equals
#define SR_SYM_NE                   1     // Not Equal
#define SR_SYM_GT                   2     // Greater Than
#define SR_SYM_GE                   3     // Greater than or Equal to
#define SR_SYM_LT                   4     // Less Than
#define SR_SYM_LE                   5     // Less than or Equal to
#define SR_SYM_BW                   6     // Between
#define SR_SYM_NB                   7     // Not Between

// Condition logic symbols
#define COND_SYM_NOT                0     // NOT
#define COND_SYM_AND                1     // AND
#define COND_SYM_OR                 2     // OR

// Device (lamp) type
#define MAX_RING_NUM                3
typedef enum
{
  devtypUnknown = 0,
  // Color ring - Rainbow
  devtypCRing3,
  devtypCRing2,
  devtypCBar,
  devtypCFrame,
  devtypCWave,
  devtypCRing1 = 31,

  // White ring - Sunny
  devtypWRing3 = 32,
  devtypWRing2,
  devtypWBar,
  devtypWFrame,
  devtypWWave,
  devtypWSquare60,      // 60 * 60
  devtypWPanel120_30,   // 120 * 30
  devtypWBlackboard,    // Blackboard lamp
  devtypWRing1 = 95,

  // Color & Motion ring - Mirage
  devtypMRing3 = 96,
  devtypMRing2,
  devtypMBar,
  devtypMFrame,
  devtypMWave,
  devtypMRing1 = 127,

  devtypDummy = 255
} devicetype_t;

#define RING_ID_ALL                 0
#define RING_ID_1                   1
#define RING_ID_2                   2
#define RING_ID_3                   3

#define IS_SUNNY(DevType)           ((DevType) >= devtypWRing3 && (DevType) <= devtypWRing1)
#define IS_RAINBOW(DevType)         ((DevType) >= devtypCRing3 && (DevType) <= devtypCRing1)
#define IS_MIRAGE(DevType)          ((DevType) >= devtypMRing3 && (DevType) <= devtypMRing1)
#define IS_VALID_LAMP(DevType)      ((DevType) >= devtypMRing3 && (DevType) <= devtypMRing1)
#define IS_VALID_REMOTE(DevType)    ((DevType) >= remotetypRFSimply && (DevType) <= remotetypRFEnhanced)

#define IS_GROUP_NODEID(nID)        (nID >= NODEID_MIN_GROUP && nID <= NODEID_MAX_GROUP)
#define IS_SPECIAL_NODEID(nID)      (nID >= NODEID_PROJECTOR && nID <= NODEID_SMARTPHONE)
#define IS_NOT_DEVICE_NODEID(nID)   ((nID < NODEID_MIN_DEVCIE || nID > NODEID_MAX_DEVCIE) && nID != NODEID_MAINDEVICE)
#define IS_NOT_REMOTE_NODEID(nID)   (nID < NODEID_MIN_REMOTE || nID > NODEID_MAX_REMOTE)

// Remote type
typedef enum
{
  remotetypUnknown = 224,
  remotetypRFSimply,
  remotetypRFStandard,
  remotetypRFEnhanced,
  remotetypDummy
} remotetype_t;

// Specify system serial port, could be Serial, USBSerial1, Serial1 or Seria2
#ifndef TheSerial
#define TheSerial       Serial
#endif

#define SERIAL          TheSerial.printf
#define SERIAL_LN       TheSerial.printlnf

#ifndef BLEPort
#define BLEPort         Serial1             // Could be serial1 or serial2
#endif

//--------------------------------------------------
// Tools & Helpers
//--------------------------------------------------
int wal_stricmp(const char *a, const char *b);
int wal_strnicmp(const char *a, const char *b, uint8_t len);
uint8_t h2i(const char c);
char* PrintUint64(char *buf, uint64_t value, bool bHex = true);
char* PrintMacAddress(char *buf, const uint8_t *mac, char delim = ':', bool bShort = true);
uint64_t StringToUInt64(const char *strData);
inline time_t tmConvert_t(US YYYY, UC MM, UC DD, UC hh, UC mm, UC ss)  // inlined for speed
{
  struct tm t;
  t.tm_year = YYYY-1900;
  t.tm_mon = MM - 1;
  t.tm_mday = DD;
  t.tm_hour = hh;
  t.tm_min = mm;
  t.tm_sec = ss;
  t.tm_isdst = 0;  // not used
  time_t t_of_day = mktime(&t);
  return t_of_day;
};
//////////////////rfscanner///////////////////////
// I_GET_NONCE sub-type
enum {
    SCANNER_PROBE = 0,
    SCANNER_SETUP_RF,           // by NodeID & SubID
    SCANNER_SETUPDEV_RF,        // by UniqueID

    SCANNER_GETCONFIG = 8,      // by NodeID & SubID
    SCANNER_SETCONFIG,
    SCANNER_GETDEV_CONFIG,      // by UniqueID
    SCANNER_SETDEV_CONFIG,

    SCANNER_TEST_NODE = 16,     // by NodeID & SubID
    SCANNER_TEST_DEVICE,        // by UniqueID
};

#endif /* xliCommon_h */
