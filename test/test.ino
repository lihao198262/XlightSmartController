#ifdef UNIT_TEST_ENABLE

#include "application.h"
#include "unitTest.h"

#include "SparkIntervalTimer.h"

#include "xlSmartController.h"
#include "xliCommon.h"
#include "xliMemoryMap.h"
#include "xliPinMap.h"
#include "xliConfig.h"

#include "xlxCloudObj.h"
#include "xlxConfig.h"
#include "xlxLogger.h"
#include "xlxSerialConsole.h"

//><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
// Intergration Tests
//><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
test(serialconsole)
{
  //String in ="";
  /// Format_1: Single row
  /// Change csc to 1
  theSys.CldJSONConfig("{'op':2, 'fl':0, 'run':0, 'uid':'c1','csc':1}");

  theSys.CldJSONConfig("{'op':1, 'fl':0, 'run':0, 'uid':'s1','ring1':[1,8,8,8,8,8], 'ring2':[1,8,8,8,8,8], 'ring3':[1,8,8,8,8,8], 'filter':0}");
  theSys.CldJSONCommand("{'cmd':0, 'data':'?'}");
  theSys.CldJSONCommand("{'cmd':0, 'data':'? show'}");
  theSys.CldJSONCommand("{'cmd':0, 'data':'check rf'}");
  theSys.CldJSONCommand("{'cmd':0, 'data':'check wifi'}");
  theSys.CldJSONCommand("{'cmd':0, 'data':'show debug'}");
  theSys.CldJSONCommand("{'cmd':0, 'data':'show net'}");
  theSys.CldJSONCommand("{'cmd':0, 'data':'ping'}");
  theSys.CldJSONCommand("{'cmd':0, 'data':'sys reset'}");
  theSys.CldJSONCommand("{'cmd':0, 'data':'show time'}");}
  theSys.CldJSONCommand("{'cmd':6, 'node_id':1}");}

test(cloudinput)
{
  /// Notes: if run in Cloud functions window, symbol \ should be removed
  /// Format_2: Multiple rows
  //theSys.CldJSONConfig("{'rows':2, 'data': [{'op':1, 'fl':0, 'run':0, 'uid':'s1', 'hue':0x0101080808080800}, {'op':1, 'fl':0, 'run':0, 'uid':'s1', 'hue':0x0201080808080800}, {'op':1, 'fl':0, 'run':0, 'uid':'s1', 'hue':0x0301080808080800}]");

  /// Format_3: Concatenate strings
  //// First string
  theSys.CldJSONConfig("{'x0': '{\"op\":1,\"fl\":0,\"run\":0,\"uid\":\"s4\",\"ring1\": '}");
  //// String in middle
  theSys.CldJSONConfig("{'x1': '[1,0,0,0,255,0],\"ring2\":[1,0,0,0,255,0], '}");
  //// Last string: same as Format_1 or Format_2
  theSys.CldJSONConfig("\"ring3\":[1,0,0,0,255,0],\"brightness\":48,\"filter\":0}");
  // {'x0': '{"op":1,"fl":0,"run":0,"uid\":"s4","ring1": '}
  // {'x1': '[1,0,0,0,255,0],"ring2":[1,0,0,0,255,0], '}
  // "ring3":[1,0,0,0,255,0],"brightness":48,"filter":0}

  //// Format_3 test case
  theSys.CldJSONCommand("{'x0': '{"cmd\":\"serial\", '}");
  theSys.CldJSONCommand("{'x1': '\"data\":\"check '}");
  theSys.CldJSONCommand("wifi\"}");
  // {'x0': '{"cmd":"serial", '}
  // {'x1': '"data":"check '}
  // wifi"}

  //// Format_3 test case
  theSys.CldJSONCommand("{'x0': ' '}");
  theSys.CldJSONCommand("{'cmd':0, 'data':'show net'}");

  // SetTimeZone & SetCurrentTime
  theSys.CldSetTimeZone("{'id':90, 'offset':-300, 'dst':1}");
  theSys.CldSetTimeZone("{'id':88, 'offset':-240, 'dst':0}");
  theSys.CldSetCurrentTime("2016-08-30 14:30:30");
  theSys.CldSetCurrentTime("2016-08-31");
  theSys.CldSetCurrentTime("15:31:31");
  theSys.CldSetCurrentTime("sync");

  //---------------------------------------------------
  // Demo scenerio
  // Example 1: Turn on the lights
  theSys.CldJSONConfig("{'op':1, 'fl':0, 'run':0, 'uid':'s1','sw':1}");
  // Execute scenerio to make sure it is saved immediatly
  /// Option 1: could command
  theSys.CldJSONCommand("{'cmd':4, 'node_id':1, 'SNT_id':1}");
  /// Option 2: serial command
  // send 1:15:1

  // Example 2: Turn on the lights, 80% brightness and CCT=3500
  theSys.CldJSONConfig("{'op':1, 'fl':0, 'run':0, 'uid':'s3','ring0':[1,80,3500,0,0,0]}");
  // Execute scenerio to make sure it is saved immediatly
  /// Option 1: could command
  theSys.CldJSONCommand("{'cmd':4, 'node_id':1, 'SNT_id':1}");
  /// Option 2: serial command
  // send 1:15:1

  // Example 3: Turn off the lights
  theSys.CldJSONConfig("{'op':1, 'fl':0, 'run':0, 'uid':'s2','sw':0}");
  // Execute scenerio to make sure it is saved immediatly
  /// Option 1: could command
  theSys.CldJSONCommand("{'cmd':4, 'node_id':1, 'SNT_id':2}");
  /// Option 2: serial command
  // send 1:15:2

  // Demo schedule
  // Example 1: 8:30am daily
  theSys.CldJSONConfig("{'x0': '{\"op\":1, \"fl\":0, \"run\":0, \"uid\": \"a1\", \"isRepeat\":1, '}");
  theSys.CldJSONConfig("\"weekdays\":0, \"hour\":8, \"minute\":30}");
  //{'x0': '{"op":1, "fl":0, "run":0, "uid": "a1", "isRepeat":1, '}
  // "weekdays":0, "hour":8, "minute":30}

  // Example 2: 22:30pm daily
  theSys.CldJSONConfig("{'x0': '{\"op\":1, \"fl\":0, \"run\":0, \"uid\": \"a2\", \"isRepeat\":1, '}");
  theSys.CldJSONConfig("\"weekdays\":0, \"hour\":22, \"minute\":30}");

  // Demo rules
  // Condition rules
  /// Example 1: if brightness (ALS) < 50, turn on the lights
  theSys.CldJSONConfig("{'x0': '{\"op\":1,\"fl\":0,\"run\":0,\"uid\":\"r1\",\"node_uid\": '}");
  theSys.CldJSONConfig("{'x1': '1,\"SNT_uid\":1,\"tmr_int\":1, '}");
  theSys.CldJSONConfig("\"cond0\":[1,1,4,2,1,50,0]}");
  // condition0: [1(enable),1(SR_SCOPE_NODE),4(SR_SYM_LT),2(COND_SYM_OR),1(sensorALS),50(value1),0(value2)]

  /// Example 2: if brightness (ALS) >= 80, turn off the lights
  theSys.CldJSONConfig("{'x0': '{\"op\":1,\"fl\":0,\"run\":0,\"uid\":\"r2\",\"node_uid\": '}");
  theSys.CldJSONConfig("{'x1': '1,\"SNT_uid\":2,\"tmr_int\":1, '}");
  theSys.CldJSONConfig("\"cond0\":[1,1,3,2,1,80,0]}");
  // condition0: [1(enable),1(SR_SCOPE_NODE),3(SR_SYM_GE),2(COND_SYM_OR),1(sensorALS),80(value1),0(value2)]

  /// Example 3: if detect motion (PIR == 1), turn on the lights
  theSys.CldJSONConfig("{'x0': '{\"op\":1,\"fl\":0,\"run\":0,\"uid\":\"r3\",\"node_uid\": '}");
  theSys.CldJSONConfig("{'x1': '1,\"SNT_uid\":1,\"tmr_int\":1, '}");
  theSys.CldJSONConfig("\"cond0\":[1,1,0,2,4,1,0]}");
  // condition0: [1(enable),1(SR_SCOPE_NODE),0(SR_SYM_EQ),2(COND_SYM_OR),4(sensorPIR),1(value1),0(value2)]
  //{'x0': '{"op":1,"fl":0,"run":0,"uid":"r3","node_uid": '}
  // {'x1': '1,"SNT_uid":1,"tmr_int":1, '}
  // "cond0":[1,1,0,2,4,1,0]}

  /// Example 4: if no motion (PIR == 0), turn off the lights
  theSys.CldJSONConfig("{'x0': '{\"op\":1,\"fl\":0,\"run\":0,\"uid\":\"r4\",\"node_uid\": '}");
  theSys.CldJSONConfig("{'x1': '1,\"SNT_uid\":2,\"tmr_int\":1, '}");
  theSys.CldJSONConfig("\"cond0\":[1,1,0,2,4,0,0]}");
  // condition0: [1(enable),1(SR_SCOPE_NODE),0(SR_SYM_EQ),2(COND_SYM_OR),4(sensorPIR),0(value1),0(value2)]
  //{'x0': '{"op":1,"fl":0,"run":0,"uid":"r4","node_uid": '}
  // {'x1': '1,"SNT_uid":2,"tmr_int":1, '}
  // "cond0":[1,1,0,2,4,0,0]}

  /// Example 5: if brightness (ALS) < 70 AND motion (PIR == 1), turn on the lights
  theSys.CldJSONConfig("{'x0': '{\"op\":1,\"fl\":0,\"run\":0,\"uid\":\"r1\",\"node_uid\": '}");
  theSys.CldJSONConfig("{'x1': '1,\"SNT_uid\":1,\"tmr_int\":1, '}");
  theSys.CldJSONConfig("{'x1': '\"cond0\":[1,1,4,1,1,70,0], '}");
  theSys.CldJSONConfig("\"cond1\":[1,1,0,1,4,1,0]}");
  // condition0: [1(enable),1(SR_SCOPE_NODE),4(SR_SYM_LT),1(COND_SYM_AND),1(sensorALS),70(value1),0(value2)]
  // condition1: [1(enable),1(SR_SCOPE_NODE),0(SR_SYM_EQ),1(COND_SYM_AND),4(sensorPIR),1(value1),0(value2)]

  /// Example 6: if brightness (ALS) >= 65 AND no motion (PIR == 0), turn off the lights
  theSys.CldJSONConfig("{'x0': '{\"op\":1,\"fl\":0,\"run\":0,\"uid\":\"r2\",\"node_uid\": '}");
  theSys.CldJSONConfig("{'x1': '1,\"SNT_uid\":2,\"tmr_int\":1, '}");
  theSys.CldJSONConfig("{'x1': '\"cond0\":[1,1,3,1,1,65,0], '}");
  theSys.CldJSONConfig("\"cond1\":[1,1,0,1,4,0,0]}");
  // condition0: [1(enable),1(SR_SCOPE_NODE),3(SR_SYM_GE),1(COND_SYM_AND),1(sensorALS),65(value1),0(value2)]
  // condition1: [1(enable),1(SR_SCOPE_NODE),0(SR_SYM_EQ),1(COND_SYM_AND),4(sensorPIR),0(value1),0(value2)]

  // Schedule rules
  /// Example 1: turn on the lights at 8:30am daily
  theSys.CldJSONConfig("{'x0': '{\"op\":1,\"fl\":0,\"run\":0,\"uid\":\"r5\",\"node_uid\": '}");
  theSys.CldJSONConfig("1,'\"SCT_uid\":1,\"SNT_uid\":1}");

  /// Example 2: turn off the lights at 22:30pm daily
  theSys.CldJSONConfig("{'x0': '{\"op\":1,\"fl\":0,\"run\":0,\"uid\":\"r5\",\"node_uid\": '}");
  theSys.CldJSONConfig("1,'\"SCT_uid\":2,\"SNT_uid\":2}");

  // Schedule & Condition rules
  // with timer, ToDo:
  //---------------------------------------------------
}

test(alarm_all_red)
{
  // This sends a scenerio of all red rings, a schedule row to set a
  // daily repeating alarm, and a rules row to execute it

  //scenerio row uid = 0
  theSys.CldJSONConfig("{'x0': '{\"op\":1,\"fl\":0,\"run\":0,\"uid\":\"s0\",\"ring1\": '}");
  theSys.CldJSONConfig("{'x1': '[1,0,0,255,0,0],\"ring2\":[1,0,0,255,0,0], '}");
  theSys.CldJSONConfig("\"ring3\":[1,0,0,255,0,0],\"brightness\":99}");

  //schedule row uid = 1
  theSys.CldJSONConfig("{'x0': '{\"op\":1, \"fl\":0, \"run\":0, \"uid\": \"a1\", \"isRepeat\":0, '}");
  theSys.CldJSONConfig("\"weekdays\":2, \"hour\":20, \"minute\":18, \"alarm_id\":255}");

  //rules row uid = 0
  theSys.CldJSONConfig("{'x0': '{\"op\":1, \"fl\":0, \"run\":0, \"uid\": \"r0\", '}");
  theSys.CldJSONConfig("\"node_id\":1, \"SCT_uid\":1, \"SNT_uid\":0, \"notif_uid\":0}");
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Call Start Func to Init Tests
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  bool flag;

  void testOrderedList(bool desc) {
    SERIAL_LN("testOrderedList()");
    NodeListClass lstTest(32, desc);
    NodeIdRow_t lv_Node;
    lv_Node.nid = 1;
    lv_Node.recentActive = Time.now();
    SERIAL_LN("Add node:%d at %d", lv_Node.nid, lstTest.add(&lv_Node));
    lv_Node.nid = 2;
    lv_Node.recentActive = Time.now() + lv_Node.nid;
    SERIAL_LN("Add node:%d at %d", lv_Node.nid, lstTest.add(&lv_Node));
    lv_Node.nid = 6;
    lv_Node.recentActive = Time.now() + lv_Node.nid;
    SERIAL_LN("Add node:%d at %d", lv_Node.nid, lstTest.add(&lv_Node));
    lv_Node.nid = 4;
    lv_Node.recentActive = Time.now() + lv_Node.nid;
    SERIAL_LN("Add node:%d at %d", lv_Node.nid, lstTest.add(&lv_Node));
    lv_Node.nid = 3;
    lv_Node.recentActive = Time.now() + lv_Node.nid;
    SERIAL_LN("Add node:%d at %d", lv_Node.nid, lstTest.add(&lv_Node));

    SERIAL_LN("Node List - count:%d, size:%d", lstTest.count(), lstTest.size());
    lv_Node.nid = 2;
    lv_Node.recentActive = 123456;
    lstTest.update(&lv_Node);
    lv_Node.recentActive = 0;
    if( lstTest.get(&lv_Node) >= 0 ) {
      SERIAL_LN("Node:%d, value:%d", lv_Node.nid, lv_Node.recentActive);
    }

    for(int i=0; i < lstTest.count(); i++) {
      SERIAL_LN("Index: %d - node:%d", i, lstTest._pItems[i].nid);
    }

    lv_Node.nid = 3;
    if( lstTest.remove(&lv_Node) ) {
      SERIAL_LN("One node:%d removed, Node List - count:%d, size:%d", lv_Node.nid, lstTest.count(), lstTest.size());
    }
  }

  int start(String input)
  {
    flag = true;
    return 1;
  }

  void setup()
  {
    Particle.function("RunUnitTests", start);
    flag = false;
    TheSerial.begin(SERIALPORT_SPEED_DEFAULT);

  	//Test output location
  	Test::out = &Serial;

	//Test Selection (use ::exclude(char *pattern) and ::include(char *pattern))
	Test::exclude("*");
	//Test::include("serialconsole");
  //Test::include("cloudinput");
  Test::include("alarm_all_red");

	   //Additional Setup
    for(int i = 10; i > 0; i--)
    {
      TheSerial.println(i);
      delay(500);
    }
    TheSerial.println ("starting setup functions");
  	//IntervalTimer sysTimer;
  	theSys.Init();
  	theConfig.LoadConfig();
  	theSys.InitPins();
  	theSys.InitRadio();
  	theSys.InitNetwork();
  	theSys.InitCloudObj();
  	theSys.InitSensors();
    theConsole.Init();
  	theSys.Start();
  	while (Time.now() < 2000) {
  		Particle.process();
  	}

    //testOrderedList(false);

  	//Test output location
  	Test::out = &Serial;

  	//Test Selection (use ::exclude(char *pattern) and ::include(char *pattern))
  	Test::exclude("*");
  	//Test::include("serialconsole");
    //Test::include("cloudinput");
    Test::include("cloudinthree");

  	//Additional Setup
    for(int i = 10; i > 0; i--)
    {
      TheSerial.println(i);
      delay(500);
    }
    Serial.println ("starting setup functions");
  	//IntervalTimer sysTimer;
  	theSys.Init();
  	theConfig.LoadConfig();
  	theSys.InitPins();
  	theSys.InitRadio();
  	theSys.InitNetwork();
  	theSys.InitCloudObj();
  	theSys.InitSensors();
    theConsole.Init();
  	theSys.Start();
  	while (Time.now() < 2000) {
  		Particle.process();
  	}
  }

  void loop()
  {
    //TheSerial.print (".");
    static UC tick = 0;

    IF_MAINLOOP_TIMER( theSys.ProcessCommands(), "ProcessCommands" );

    IF_MAINLOOP_TIMER( theSys.CollectData(tick++), "CollectData" );

  	IF_MAINLOOP_TIMER( theSys.ReadNewRules(), "ReadNewRules" );

    IF_MAINLOOP_TIMER( theSys.SelfCheck(RTE_DELAY_SELFCHECK), "SelfCheck" );

    if (flag)
      Test::run();
    }

#endif
