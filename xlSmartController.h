//  xlSmartController.h - Xlight SmartController project scale definitions header

#ifndef xlSmartController_h
#define xlSmartController_h

#include "xliCommon.h"
#include "xlxCloudObj.h"
#include "xlxConfig.h"
#include "xlxChain.h"
#include "MyMessage.h"

//------------------------------------------------------------------
// Xlight Command Queue Structures
//------------------------------------------------------------------

//ToDo: Create command queue


//------------------------------------------------------------------
// Smart Controller Class
//------------------------------------------------------------------
class SmartControllerClass : public CloudObjClass
{
private:
  BOOL m_isRF;
  BOOL m_isLAN;
  BOOL m_isWAN;

  String hue_to_string(Hue_t hue);
  bool updateDevStatusRow(MyMessage msg);

public:
  SmartControllerClass();
  void Init();
  void InitRadio();
  void InitNetwork();
  void InitPins();
  void InitSensors();
  void InitCloudObj();

  BOOL Start();
  UC GetStatus();
  BOOL SetStatus(UC st);
  void ResetSerialPort();
  void Restart();

  BOOL CheckRF();
  BOOL CheckWiFi();
  BOOL CheckNetwork();
  BOOL SelfCheck(US ms);
  BOOL CheckRFBaseNetEnableDur();
  BOOL IsRFGood();
  BOOL IsBLEGood();
  BOOL IsLANGood();
  BOOL IsWANGood();

  BOOL connectWiFi();
  BOOL connectCloud();

  // Process all kinds of commands
  void ProcessLocalCommands();
  void ProcessCommands();
  void ProcessCloudCommands();
  void CollectData(UC tick);
  //bool ExecuteLightCommand(String mySerialStr);
  bool ProcessPanel();

  // Device Control Functions
  int DevSoftSwitch(UC sw, UC dev = 0, const UC subID = 0);

  // High speed system timer process
  void FastProcess();

  // Cloud interface implementation
  int CldSetTimeZone(String tzStr);
  int CldPowerSwitch(String swStr);
  int CldSetCurrentTime(String tmStr = "");
  int ExeJSONCommand(String jsonCmd);
  int ExeJSONConfig(String jsonData);

  // Parsing Functions
  bool ParseCmdRow(JsonObject& data);
  UC CreateColorPayload(UC *payl, uint8_t ring, uint8_t State, uint8_t BR, uint8_t W, uint8_t R, uint8_t G, uint8_t B);

  // Cloud Interface Action Types
  bool Change_Rule(RuleRow_t row);
  bool Change_Schedule(ScheduleRow_t row);
  bool Change_Scenario(ScenarioRow_t row);
  bool Action_Rule(ListNode<RuleRow_t> *rulePtr);
  bool Action_Schedule(OP_FLAG parentFlag, UC uid, UC rule_uid);

  bool Check_SensorData(UC _thisNd, UC _scope, UC _sr, UC _nd, UC _symbol, US _val1, US _val2);
  bool Execute_Rule(ListNode<RuleRow_t> *rulePtr, bool _init = false, const UC _sr = 255, const UC _nd = 0);

  //LinkedLists (Working memory tables)
  ChainClass<DevStatusRow_t> DevStatus_table = ChainClass<DevStatusRow_t>(MAX_DEVICE_PER_CONTROLLER);
  ChainClass<ScheduleRow_t> Schedule_table = ChainClass<ScheduleRow_t>(MAX_TABLE_SIZE);
  ChainClass<ScenarioRow_t> Scenario_table = ChainClass<ScenarioRow_t>(MAX_TABLE_SIZE);
  ChainClass<RuleRow_t> Rule_table = ChainClass<RuleRow_t>(256); // 65536/24 is too big = (int)(MEM_RULES_LEN / sizeof(RuleRow_t))

  //Print LinkedLists (Working memory tables)
  String print_devStatus_table(int row);
  void print_schedule_table(int row);
  void print_scenario_table(int row);
  void print_rule_table(int row);

  // Action Loop & Helper Methods
  void ReadNewRules(bool force = false);
  bool CreateAlarm(ListNode<ScheduleRow_t>* scheduleRow, uint32_t tag = 0);
  bool DestoryAlarm(AlarmId alarmID, UC SCT_uid);
  void OnSensorDataChanged(const UC _sr, const UC _nd);

  // UID search functions
  ListNode<ScheduleRow_t> *SearchSchedule(UC uid);
  ListNode<ScenarioRow_t> *SearchScenario(UC uid);
  ListNode<DevStatusRow_t> *SearchDevStatus(UC dest_id); //destination node
  ListNode<DevStatusRow_t> *m_pMainDev;

  // Device Operations, will be moved to dedicate class later
  BOOL FindCurrentDevice();
  ListNode<DevStatusRow_t> *FindDevice(UC _nodeID);
  void CheckDevTimeout();

  UC GetDevOnOff(UC _nodeID);
  UC GetDevBrightness(UC _nodeID);
  US GetDevCCT(UC _nodeID);

  US VerifyDevicePresence(UC *_assoDev, UC _nodeID, UC _devType, uint64_t _identity);
  BOOL ToggleLampOnOff(UC _nodeID = NODEID_MAINDEVICE, const UC subID = 0);
  BOOL ChangeLampBrightness(UC _nodeID = NODEID_MAINDEVICE, UC _percentage = 50, const UC subID = 0);
  BOOL ChangeLampCCT(UC _nodeID = NODEID_MAINDEVICE, US _cct = 3000, const UC subID = 0);
  BOOL ChangeBR_CCT(UC _nodeID, UC _br, US _cct, const UC subID = 0);
  BOOL ChangeLampScenario(UC _nodeID, UC _scenarioID, UC _replyTo = 0, const UC _sensor = 0);
  BOOL RequestDeviceStatus(UC _nodeID, const UC subID = 0);
  BOOL ConfirmLampOnOff(UC _nodeID, UC _st);
  BOOL ConfirmLampBrightness(UC _nodeID, UC _st, UC _percentage, UC _ringID = RING_ID_ALL);
  BOOL ConfirmLampCCT(UC _nodeID, US _cct, UC _ringID = RING_ID_ALL);
  BOOL ConfirmLampHue(UC _nodeID, UC _white, UC _red, UC _green, UC _blue, UC _ringID = RING_ID_ALL);
  BOOL ConfirmLampTop(UC _nodeID, UC *_payl, UC _len);
  BOOL ConfirmLampFilter(UC _nodeID, UC _filter);
  BOOL ConfirmLampPresent(ListNode<DevStatusRow_t> *pDev, bool _up);
  BOOL QueryDeviceStatus(UC _nodeID, UC _ringID = RING_ID_ALL);
  BOOL RebootNode(UC _nodeID, const UC subID = 0);
  BOOL IsAllRingHueSame(ListNode<DevStatusRow_t> *pDev);

  // Utils
  void Array2Hue(JsonArray& data, Hue_t& hue);     // Copy JSON array to Hue structure
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern SmartControllerClass theSys;

#endif /* xlSmartController_h */
