//  xliMemoryMap.h - Xlight include file: MCU Memory Map

#ifndef xliMemoryMap_h
#define xliMemoryMap_h

// Open it if use spark-flashee-eeprom to access the emulated EEPROM,
/// instead of Photon EEPROM class (high level API)
//#define XL_FLASHEE_EEPROM

//------------------------------------------------------------------
// Emulated EEPROM: 2048bytes
//------------------------------------------------------------------
#define MEM_BANK_1_BASE           0x800C000
#define MEM_BANK_2_BASE           0x8010000

// Device Status (16*48bytes)
#ifdef XL_FLASHEE_EEPROM
#define MEM_DEVICE_STATUS_OFFSET  MEM_BANK_1_BASE
#else
#define MEM_DEVICE_STATUS_OFFSET  0x0000
#endif
#define MEM_DEVICE_STATUS_LEN     0x0300

// Parameters (256bytes)
#define MEM_CONFIG_OFFSET         (MEM_DEVICE_STATUS_OFFSET + MEM_DEVICE_STATUS_LEN)
#define MEM_CONFIG_LEN            0x0100

// Schedule (256bytes)
#define MEM_SCHEDULE_OFFSET       (MEM_CONFIG_OFFSET + MEM_CONFIG_LEN)
#define MEM_SCHEDULE_LEN          0x0100

// Node ID List (48*16bytes)
#define MEM_NODELIST_OFFSET       (MEM_SCHEDULE_OFFSET + MEM_SCHEDULE_LEN)
#define MEM_NODELIST_LEN          0x0300

//------------------------------------------------------------------
// P1 external Flash: 1MB, access via SPI flash library
//------------------------------------------------------------------
#define MEM_EXT_FLASH_BASE        0x000000

// Rules (65536 bytes)
#define MEM_RULES_OFFSET          MEM_EXT_FLASH_BASE
#define MEM_RULES_LEN             0x010000

// Scenarios (65536 bytes)
#define MEM_SCENARIOS_OFFSET      (MEM_RULES_OFFSET + MEM_RULES_LEN)
#define MEM_SCENARIOS_LEN         0x010000

// MAC List (65536 bytes)
#define MEM_MAC_LIST_OFFSET       (MEM_SCENARIOS_OFFSET + MEM_SCENARIOS_LEN)
#define MEM_MAC_LIST_LEN          0x010000

// Offline Data Cache
#define MEM_OFFLINE_DATA_OFFSET   (MEM_MAC_LIST_OFFSET + MEM_MAC_LIST_LEN)
#define MEM_OFFLINE_DATA_LEN      0x020000

// Statistics
#define MEM_REPORT_OFFSET         (MEM_OFFLINE_DATA_OFFSET + MEM_OFFLINE_DATA_LEN)
#define MEM_REPORT_LEN            0x010000

//-------------------------------
// Miscellaneous
#define MEM_MISC_OFFSET           (MEM_REPORT_OFFSET + MEM_REPORT_LEN)
#define MEM_MISC_LEN              0x080000

// Node config data (16384 bytes = 64 * 256 bytes)
/// 1 - 12: standard config template
/// 13 - 64: user defined config
#define MEM_NODECONFIG_OFFSET     MEM_MISC_OFFSET
#define MEM_NODECONFIG_LEN        0x004000

#define MEM_CONFIG_BACKUP_OFFSET  (MEM_NODECONFIG_OFFSET + MEM_NODECONFIG_LEN)
#define MEM_CONFIG_BACKUP_LEN     0x0100

#define MEM_NODELIST_BACKUP_OFFSET  (MEM_CONFIG_BACKUP_OFFSET + MEM_CONFIG_BACKUP_LEN)
#define MEM_NODELIST_BACKUP_LEN     0x0300

//-------------------------------

#endif /* xliMemoryMap_h */
