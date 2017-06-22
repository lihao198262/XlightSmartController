#ifndef __XLI_NODECONFIG_H
#define __XLI_NODECONFIG_H

// Node config protocol
/// MySensor Cmd = C_INTERNAL
/// MySensor Type = I_CONFIG

/// Use MySensor Sensor as Node Config Field (NCF), and payload as config data
#define NCF_QUERY                       0       // Query NCF, payload length = 0 (query) or n (ack)
#define NCF_MAP_SENSOR                  1       // Sensor Bitmap, payload length = 2
#define NCF_MAP_FUNC                    2       // Function Bitmap, payload length = 2

#define NCF_DEV_ASSOCIATE               10      // Associate node to device(s), payload length = 2 to 8, a device per uint16_t
#define NCF_DEV_EN_SDTM                 11      // Simple Direct Test Mode flag, payload length = 2
#define NCF_DEV_MAX_NMRT                12      // Max. Node Message Repeat Times, payload length = 2
#define NCF_DEV_SET_SUBID               13      // Set device subid, payload length = 2
#define NCF_DEV_CONFIG_MODE             14      // Put Device into Config Mode, payload length = 2

#define NCF_DATA_ALS_RANGE              50      // Lower and upper threshholds of ALS, payload length = 2
#define NCF_DATA_TEMP_RANGE             51      // Tempreture threshholds, payload length = 2
#define NCF_DATA_HUM_RANGE              52      // Humidity threshholds, payload length = 2
#define NCF_DATA_PM25_RANGE             53      // PM2.5 threshholds, payload length = 2
#define NCF_DATA_PIR_RANGE              54      // PIR control brightness (off br, on br), payload length = 2
#define NCF_DATA_FN_SCENARIO            60      // Scenario ID for Remote Fn keys (b1=fn_id, b2=scenario_id), payload length = 2
#define NCF_DATA_FN_HUE                 61      // Hue for Remote Fn keys (b1_7-4=bmDevice, b1_3-0=fn_id, b2-11=Hue), payload length = 12

#define NCF_LEN_DATA_FN_HUE             12

#endif /* __XLI_NODECONFIG_H */
