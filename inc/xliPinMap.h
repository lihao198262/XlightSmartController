//  xliPinMap.h - Xlight include file: MCU Pin Map
/// SBS updated 20150429 in accordance with EF H/W design
/// SBS updated 20161027:
///  available pins:
///    D1 (35), D5 (53), D6 (55), D7 (54), A7 (30),
///    P1S3 (40), P1S4 (47), P1S5 (48)
///  PIN_SEN_PIR & PIN_SEN_LIGHT moved to the light
///  no need MIC if use voice recognition module
///    use serial2 by moving RGB-G (32) & RGB-B (31) pin to A6 (24) & A7 (30),
///        and leave RGB-R (29) unchanged
//
#ifndef xliPinMap_h
#define xliPinMap_h

// MCU Type: Particle family, and Photon or P1
#define MCU_TYPE_Particle

#ifdef MCU_TYPE_Particle
 #define MCU_TYPE_P1
#endif

//------------------------------------------------------------------
// MCU Pin Usage
//------------------------------------------------------------------
// System Control
#ifdef MCU_TYPE_P1
/*
    #define P1S0                  40          // P1S0, 12b GPIO, may also be PWM
    #define P1S1                  41          // P1S1, 12b GPIO, may also be PWM
    #define P1S2                  42          // P1S2, 12b GPIO, may also be PWM
    #define P1S3                  44          // P1S3, 12b GPIO, may also be PWM
    #define P1S4                  47          // P1S4, 12b GPIO, may also be PWM
    #define P1S5                  48          // P1S5, 12b GPIO, may also be PWM

    #define PIN_BTN_SETUP         46          // Setup button
    #define PIN_BTN_RESET         34          // Reset button
    #define PIN_LED_RED           29          // RGBR, Status RGB LED
    #define PIN_LED_GREEN         32          // RGBG, Status RGB LED
    #define PIN_LED_BLUE          31          // RGBB, Status RGB LED
*/

    // P1 specific GPIO pins, can also be used as digital GPIOs
    #define PIN_MOSI_LED          P1S0        // Panel HC595 data line
    #define PIN_LATCH_LED         P1S1        // Panel HC595 chip enable, P1S2 for old controller at AC
    #define PIN_SCK_LED           P1S2        // Panel HC595 clock

    // HOME_EDITION
    #define PIN_BLE_EN            P1S4        // BLE Enable
    #define PIN_BLE_STATE         P1S5        // BLE State

    // BUSINESS_EDITION
    #define PIN_SOFT_KEY_4        P1S3        // Relay control key 4
    #define PIN_BTN_EXT_3         P1S4          // Externsion button, may also be PWM
    #define PIN_BTN_EXT_4         P1S5          // Externsion button, may also be PWM
#endif

// Digital only GPIO pins (D0 - D7, D0-D4 may also be PWM)
#define PIN_KNOB_B_PHASE          D2          // Knob: B_PHASE
#define PIN_KNOB_A_PHASE          D3          // Knob: A_PHASE
#define PIN_KNOB_BUTTON           D4          // Knob: button

// Analog GPIO pins (12-bit A0 - A7), can also be used as digital GPIOs
#define PIN_RF24_CE		   		      A0
#define PIN_RF24_CS		   	 	      A2
//#define PIN_SEN_MIC               A6          // Sensor: ECT MIC, DAC
//#define PIN_SEN_LIGHT             A7          // Sensor: ALS, may also be PWM

#define PIN_SEN_DHT               D0          // Sensor: temperature and humidity
//#define PIN_SEN_PIR               D7          // Sensor: infra red motion, may also be PWM

// BUSINESS_EDITION
#define PIN_SOFT_KEY_1            D1          // Relay control key 1
#define PIN_SOFT_KEY_2            A7          // Relay control key 2
#define PIN_SOFT_KEY_3            A6          // Relay control key 3

/*
  RF24L01 connector pinout:
  GND    VCC
  CE     CSN
  SCK    MOSI
  MISO   IRQ

  RF2.4 Module
  SPARK CORE    SHIELD SHIELD    NRF24L01+
  GND           GND              1 (GND)
  3V3 (3.3V)    3.3V             2 (3V3)
  A0 (CSN)      9  (D6)          3 (CE)
  A2 (SS)       10 (SS)          4 (CSN)
  A3 (SCK)      13 (SCK)         5 (SCK)
  A5 (MOSI)     11 (MOSI)        6 (MOSI)
  A4 (MISO)     12 (MISO)        7 (MISO)
  *A1(IRQ)      PC3(IRQ)          8(IRQ)

  NOTE: Also place a 10-100uF cap across the power inputs of
        the NRF24L01+.  I/O o fthe NRF24 is 5V tolerant, but
        do NOT connect more than 3.3V to pin 2(3V3)!!!
*/

// On-board Special pins
/*
#define PIN_OBD_RST               RST         // Active-low reset input
#define PIN_OBD_VBAT              VBAT        // Internal VBAT
#define PIN_OBD_3V3               3V3         // 3.3V DC
*/
// HOME_EDITION
#define PIN_BLE_RX                RX          // BLE RX, may also be PWM
#define PIN_BLE_TX                TX          // BLE TX, may also be PWM

// BUSINESS_EDITION
#define PIN_BTN_EXT_1             TX          // Externsion button, may also be PWM
#define PIN_BTN_EXT_2             RX          // Externsion button, may also be PWM

//------------------------------------------------------------------
// Sensor Features
//------------------------------------------------------------------
// Uncomment whatever type you're using!
#define SEN_TYPE_DHT              DHT11		   // DHT 11
//#define SEN_TYPE_DHT              DHT22		   // DHT 22 (AM2302)
//#define SEN_TYPE_DHT              DHT21		   // DHT 21 (AM2301)

// Light sensor output scale: y = x / 3.3 * 4096
#define SEN_LIGHT_MIN             0		       // 0 volt
#define SEN_LIGHT_MAX             3720       // 3 volt -> (3 / 3.3 * 4096)

#endif /* xliPinMap_h */
