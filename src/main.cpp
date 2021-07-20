#include <Arduino.h>
#include <Array.h>

#include "config.h"

#include "Humidistat.h"
#include "SerialLogger.h"
#include "input/ButtonReader.h"
#include "sensor/ThermistorReader.h"

// Humidity sensor
#ifdef HUMIDISTAT_DHT
#include "sensor/DHTHumiditySensor.h"
DHT dht(PIN_DHT, DHT22);
DHTHumiditySensor hs(&dht);
//                        NTC pins
ThermistorReader trs[] = {1, 2, 3, 4};
Array<ThermistorReader*, 4> trsp{{&trs[0], &trs[1], &trs[2], &trs[3]}};
#endif
#ifdef HUMIDISTAT_SHT
#include "sensor/SHTHumiditySensor.h"
SHTSensor sht;
SHTHumiditySensor hs(&sht);
Array<ThermistorReader *, 4> trsp{{}};
#endif

// Input
#ifdef HUMIDISTAT_INPUT_KS0256
#include "input/Ks0256VoltLadder.h"
Ks0256VoltLadder voltLadder;
#endif
#ifdef HUMIDISTAT_INPUT_KS0466
#include "input/Ks0466VoltLadder.h"
Ks0466VoltLadder voltLadder;
#endif

ButtonReader buttonReader(PIN_BTN, &voltLadder);
Humidistat humidistat(&hs, lowValue, dt, Kp, Ki, Kd);

// UI
#ifdef HUMIDISTAT_UI_CHAR
#include <LiquidCrystal.h>
#include "ui/CharDisplayUI.h"
LiquidCrystal liquidCrystal(PIN_LCD_RS, PIN_LCD_ENABLE, PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3);
CharDisplayUI ui(&liquidCrystal, &buttonReader, &humidistat, trsp);
#endif
#ifdef HUMIDISTAT_UI_GRAPH
#include <U8g2lib.h>
#include "ui/GraphicalDisplayUI.h"
U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R0, PIN_LCD_CS);
GraphicalDisplayUI ui(&u8g2, &buttonReader, &humidistat, trsp);
#endif

SerialLogger serialLogger(&humidistat, trsp, dt);

void setup() {
#ifdef ARDUINO_AVR_UNO
	// Set PWM frequency on D3 and D11 to 245.10 Hz
	TCCR2B = TCCR2B & B11111000 | B00000101;
#endif

	hs.begin();
	serialLogger.begin();
	ui.begin();
}

void loop() {
	ui.update();
	humidistat.update(PIN_S1, PIN_S2);
	serialLogger.update();
}
