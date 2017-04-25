//  xlxConfig.h - Xlight Configuration Reader & Writer

#ifndef xlxConfig_h
#define xlxConfig_h

#include "xliCommon.h"
#include "xliMemoryMap.h"
#include "TimeAlarms.h"
#include "OrderedList.h"
#include "flashee-eeprom.h"

/*Note: if any of these structures are modified, the following print functions may need updating:
 - ConfigClass::print_config()
 - SmartControllerClass::print_devStatus_table()
 - SmartControllerClass::print_schedule_table()
 - SmartControllerClass::print_scenario_table()
 - SmartControllerClass::print_rule_table()
*/

#define PACK //MSVS intellisense doesn't work when structs are packed

#define CURRENT_DEVICE              (theConfig.GetMainDeviceID())
#define IS_CURRENT_DEVICE(nid)      ((nid) == CURRENT_DEVICE || CURRENT_DEVICE == NODEID_DUMMY)

// Maximum items in AST scenario table
#define MAX_ASR_SNT_ITEMS           16

//------------------------------------------------------------------
// Xlight Configuration Data Structures
//------------------------------------------------------------------
typedef struct
{
  US id;                                    // timezone id
  SHORT offset;                             // offser in minutes
  UC dst                      :1;           // daylight saving time flag
} Timezone_t;

typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
  UC State                    :1;           // Component state
  UC BR                       :7;           // Brightness of white [0..100]
  US CCT                      :16;          // CCT (warm or cold) [2700..6500]
  UC R                        :8;           // Brightness of red
  UC G                        :8;           // Brightness of green
  UC B                        :8;           // Brightness of blue
  UC L1                       :8;           // Length of thread 1
  UC L2                       :8;           // Length of thread 2
  UC L3                       :8;           // Length of thread 3
} Hue_t;

typedef struct
{
  UC version                  :8;           // Data version, other than 0xFF
  US sensorBitmap             :16;          // Sensor enable bitmap
  UC indBrightness            :8;           // Indicator of brightness
  UC typeMainDevice           :8;           // Type of the main lamp
  UC numDevices               :8;           // Number of devices
  Timezone_t timeZone;                      // Time zone
  char Organization[24];                    // Organization name
  char ProductName[24];                     // Product name
  char Token[64];                           // Token
  BOOL enableCloudSerialCmd   :1;           // Whether enable cloud serial command
  BOOL enableDailyTimeSync    :1;           // Whether enable daily time synchronization
  BOOL enableSpeaker          :1;           // Whether enable speaker
  BOOL fixedNID               :1;           // Whether fixed Node ID
  BOOL Reserved_bool          :4;           // Reserved for boolean flags
  UC numNodes;                              // Number of Nodes (include device, remote control, etc.)
  UC rfPowerLevel             :2;           // RF Power Level 0..3
  BOOL stWiFi                 :1;           // Wi-Fi status: On / Off
  UC Reserved1                :5;           // Reserved bits
  US maxBaseNetworkDuration;
  UC useCloud;                              // How to depend on the Cloud
  UC mainDevID;                             // NodeID for main device
  char bleName[24];
  char blePin[6];
  char pptAccessCode[8];
  UC asrSNT[MAX_ASR_SNT_ITEMS];
} Config_t;

//------------------------------------------------------------------
// Xlight Device Status Table Structures
//------------------------------------------------------------------
typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
  OP_FLAG op_flag         : 2;
  FLASH_FLAG flash_flag   : 1;
  RUN_FLAG run_flag       : 1;
  UC uid;						   // required
  UC node_id;          // RF nodeID
  UC present              :1;  // 0 - not present; 1 - present
  UC reserved             :7;
  UC type;                         // Type of lamp
  US token;
  Hue_t ring[MAX_RING_NUM];
} DevStatusRow_t;

#define DST_ROW_SIZE sizeof(DevStatusRow_t)

typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
  UC node_id;                       // RF nodeID
  UC present              :1;       // 0 - not present; 1 - present
  UC reserved             :7;
  UC type;                         // Type of remote
  US token;
} RemoteStatus_t;

//------------------------------------------------------------------
// Xlight Schedule Table Structures
//------------------------------------------------------------------
typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
  OP_FLAG op_flag			    : 2;
  FLASH_FLAG flash_flag		: 1;
  RUN_FLAG run_flag			  : 1;
	UC uid			            : 8;
	UC weekdays			        : 7;	  //values: 0-7
	BOOL isRepeat		        : 1;	  //values: 0-1
	UC hour				          : 5;    //values: 0-23
	UC minute				        : 6;    //values: 0-59
	AlarmId alarm_id	      : 8;
} ScheduleRow_t;

#define SCT_ROW_SIZE	sizeof(ScheduleRow_t)
#define MAX_SCT_ROWS	(int)(MEM_SCHEDULE_LEN / SCT_ROW_SIZE)

//------------------------------------------------------------------
// Xlight NodeID List
//------------------------------------------------------------------
#define LEN_NODE_IDENTITY     6
typedef struct    // Exact 12 bytes
	__attribute__((packed))
{
	UC nid;
	UC device;       // Associated device (node id)
  UC identity[LEN_NODE_IDENTITY];
  UL recentActive;
} NodeIdRow_t;

inline BOOL isIdentityEmpty(UC *pId)
{ return(!(pId[0] | pId[1] | pId[2] | pId[3] | pId[4] | pId[5])); };

inline void copyIdentity(UC *pId, uint64_t *pData)
{ memcpy(pId, pData, LEN_NODE_IDENTITY); };

inline void resetIdentity(UC *pId)
{ memset(pId, 0x00, LEN_NODE_IDENTITY); };

inline BOOL isIdentityEqual(UC *pId1, UC *pId2)
{
  for( int i = 0; i < LEN_NODE_IDENTITY; i++ ) { if(pId1[i] != pId2[i]) return false; }
  return true;
};

inline BOOL isIdentityEqual(UC *pId1, uint64_t *pData)
{
  UC pId2[LEN_NODE_IDENTITY]; copyIdentity(pId2, pData);
  return isIdentityEqual(pId1, pId2);
};

//------------------------------------------------------------------
// Xlight Rule Table Structures
//------------------------------------------------------------------
typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
	UC enabled               : 1;    // Whether the condition is enabled
  UC sr_scope              : 3;    // Sensor scope
  UC symbol                : 4;    // Sensor logic symbols
  UC connector             : 2;    // Condition logic symbols
	UC sr_id                 : 4;    // Sensor ID
  US sr_value1;
  US sr_value2;
} Condition_t;

typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
	OP_FLAG op_flag			     : 2;
	FLASH_FLAG flash_flag	   : 1;
	RUN_FLAG run_flag		     : 1;
	UC uid                   : 8;
	UC node_id				       : 8;
	UC SCT_uid               : 8;
	UC SNT_uid               : 8;
	UC notif_uid             : 8;
  // Once rule triggered, whether repeatly check
  UC tmr_int               : 1; // Whether enable timer
  UC tmr_started           : 1; // Whether timer started
  US tmr_span;             // Timer span in minutes
  UL tmr_tic_start;        // Timer started tick
  // Other trigger conditions, e.g. sensor data
  Condition_t actCond[MAX_CONDITION_PER_RULE];
} RuleRow_t;

#define RT_ROW_SIZE 	sizeof(RuleRow_t)
#define MAX_RT_ROWS		64

//------------------------------------------------------------------
// Xlight Scenerio Table Structures
//------------------------------------------------------------------

typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
	OP_FLAG op_flag				  : 2;
	FLASH_FLAG flash_flag		: 1;
	RUN_FLAG run_flag			  : 1;
	UC uid			            : 8;
  UC sw                   : 2; // Main Switch: Switch value for set power command
	Hue_t ring[MAX_RING_NUM];
	UC filter		            : 8;
} ScenarioRow_t;

#define SNT_ROW_SIZE	sizeof(ScenarioRow_t)
#define MAX_SNT_ROWS	64

// Node List Class
class NodeListClass : public OrderdList<NodeIdRow_t>
{
public:
  bool m_isChanged;

  NodeListClass(uint8_t maxl = 64, bool desc = false, uint8_t initlen = 8) : OrderdList(maxl, desc, initlen) {
    m_isChanged = false; };
  virtual int compare(NodeIdRow_t _first, NodeIdRow_t _second) {
    if( _first.nid > _second.nid ) {
      return 1;
    } else if( _first.nid < _second.nid ) {
      return -1;
    } else {
      return 0;
    }
  };
  int getMemSize();
  int getFlashSize();
  bool loadList();
  bool saveList();
  void showList(BOOL toCloud = false, UC nid = 0);
  void publishNode(NodeIdRow_t _node);
  UC requestNodeID(UC preferID, char type, uint64_t identity);
  BOOL clearNodeId(UC nodeID);

protected:
  UC getAvailableNodeId(UC preferID, UC defaultID, UC minID, UC maxID, uint64_t identity);
};

//------------------------------------------------------------------
// Xlight Configuration Class
//------------------------------------------------------------------
class ConfigClass
{
private:
  BOOL m_isLoaded;
  BOOL m_isChanged;         // Config Change Flag
  BOOL m_isDSTChanged;      // Device Status Table Change Flag
  BOOL m_isSCTChanged;      // Schedule Table Change Flag
  BOOL m_isRTChanged;		    // Rules Table Change Flag
  BOOL m_isSNTChanged;	 	  // Scenerio Table Change Flag
  UL m_lastTimeSync;

  Config_t m_config;
  Flashee::FlashDevice* P1Flash;

  void UpdateTimeZone();
  void DoTimeSync();

public:
  ConfigClass();
  void InitConfig();
  BOOL InitDevStatus(UC nodeID);

  // write to P1 using spark-flashee-eeprom
  BOOL MemWriteScenarioRow(ScenarioRow_t row, uint32_t address);
  BOOL MemReadScenarioRow(ScenarioRow_t &row, uint32_t address);

  BOOL LoadConfig();
  BOOL SaveConfig();
  BOOL IsConfigLoaded();

  BOOL LoadDeviceStatus();
  BOOL SaveDeviceStatus();

  BOOL SaveScheduleTable();
  BOOL SaveScenarioTable();

  BOOL LoadRuleTable();
  BOOL SaveRuleTable();

  BOOL LoadNodeIDList();
  BOOL SaveNodeIDList();

  BOOL IsConfigChanged();
  void SetConfigChanged(BOOL flag);

  BOOL IsDSTChanged();
  void SetDSTChanged(BOOL flag);

  BOOL IsSCTChanged();
  void SetSCTChanged(BOOL flag);

  BOOL IsRTChanged();
  void SetRTChanged(BOOL flag);

  BOOL IsSNTChanged();
  void SetSNTChanged(BOOL flag);

  BOOL IsNIDChanged();
  void SetNIDChanged(BOOL flag);

  UC GetVersion();
  BOOL SetVersion(UC ver);

  US GetTimeZoneID();
  BOOL SetTimeZoneID(US tz);

  UC GetDaylightSaving();
  BOOL SetDaylightSaving(UC flag);

  SHORT GetTimeZoneOffset();
  SHORT GetTimeZoneDSTOffset();
  BOOL SetTimeZoneOffset(SHORT offset);
  String GetTimeZoneJSON();

  String GetOrganization();
  void SetOrganization(const char *strName);

  String GetProductName();
  void SetProductName(const char *strName);

  String GetToken();
  void SetToken(const char *strName);

  String GetBLEName();
  void SetBLEName(const char *strName);

  String GetBLEPin();
  void SetBLEPin(const char *strPin);

  String GetPPTAccessCode();
  void SetPPTAccessCode(const char *strPin);
  BOOL CheckPPTAccessCode(const char *strPin);

  BOOL IsCloudSerialEnabled();
  void SetCloudSerialEnabled(BOOL sw = true);

  BOOL IsSpeakerEnabled();
  void SetSpeakerEnabled(BOOL sw = true);

  BOOL IsFixedNID();
  void SetFixedNID(BOOL sw = true);

  BOOL IsDailyTimeSyncEnabled();
  void SetDailyTimeSyncEnabled(BOOL sw = true);
  BOOL CloudTimeSync(BOOL _force = true);

  US GetSensorBitmap();
  void SetSensorBitmap(US bits);
  BOOL IsSensorEnabled(sensors_t sr);
  void SetSensorEnabled(sensors_t sr, BOOL sw = true);

  UC GetBrightIndicator();
  BOOL SetBrightIndicator(UC level);

  UC GetMainDeviceID();
  BOOL SetMainDeviceID(UC devID);

  UC GetMainDeviceType();
  BOOL SetMainDeviceType(UC type);

  UC GetRemoteNodeDevice(UC remoteID);
  BOOL SetRemoteNodeDevice(UC remoteID, US devID);

  UC GetNumDevices();
  BOOL SetNumDevices(UC num);

  UC GetNumNodes();
  BOOL SetNumNodes(UC num);

  US GetMaxBaseNetworkDur();
  BOOL SetMaxBaseNetworkDur(US dur);

  UC GetUseCloud();
  BOOL SetUseCloud(UC opt);

  BOOL GetWiFiStatus();
  BOOL SetWiFiStatus(BOOL _st);

  UC GetRFPowerLevel();
  BOOL SetRFPowerLevel(UC level);

  UC GetASR_SNT(const UC _code);
  BOOL SetASR_SNT(const UC _code, const UC _snt = 0);
  void showASRSNT();

  NodeListClass lstNodes;
  RemoteStatus_t m_stMainRemote;
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern ConfigClass theConfig;

#endif /* xlxConfig_h */
