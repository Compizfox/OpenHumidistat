#ifndef HUMIDISTAT_GRAPHICALDISPLAYUI_H
#define HUMIDISTAT_GRAPHICALDISPLAYUI_H

#include <U8g2lib.h>
#include <SPI.h>

#include CONFIG_HEADER
#include "ConfigPar.h"
#include "advanceEnum.h"
#include "ControllerUI.h"
#include "EEPROMConfig.h"
#include "../control/SingleHumidistat.h"
#include "../control/CascadeHumidistat.h"

/// TUI for 128*64 px graphical display using U8g2.
/// Holds references to a U8g2lib instance for writing to display, an EEPROMConfig instance to edit the config, and
/// to a Humidistat instance to show/edit its state.
/// \tparam Humidistat_t Either SingleHumidistat or CascadeHumidistat
template<class Humidistat_t>
class GraphicalDisplayUI : public ControllerUI {
private:
	/// Tab definitions
	enum class Tab {
		main,
		config,
		_last = config,
	};

	/// Config tab selection definitions
	enum class Selection {
		par,
		number,
		actions,
	};

	/// Config tab action definitions
	enum class Action {
		save,
		reset,
		_last = reset
	};

	U8G2 &u8g2;
	EEPROMConfig &eepromConfig;
	Humidistat_t &humidistat;

	// States
	Tab currentTab = Tab::main;     //!< Currently active tab
	uint8_t currentPar = 0;         //!< Currently active config parameter
	Selection currentSelection = Selection::par;
	Action currentAction = Action::save;
	uint8_t currentDigit = 3;

	uint8_t frame = 0;              //!< Frame counter (overflows, but that's OK)
	uint8_t configSaveTimer = 0;    //!< Timer containing the current value of the cooldown on saving config to EEPROM

	const uint16_t longPressDuration = config::longPressDuration;
	const uint8_t configSaveCooldown = config::configSaveCooldown;

	const uint8_t nConfigPars;     //!< Total number of config parameters
	const ConfigPar configPars[12]; //!< Array of config parameters

	/// Draw the Main tab
	// (declaration, implementation specialised)
	void drawMain();

	/// Draw the Config tab
	void drawConfig() {
		u8g2.setFont(u8g2_font_6x12_tr);

		// Print config parameters in scrolling menu
		for (uint8_t i = 0; i < 3; i++) {
			// Index of parameter to draw
			int8_t nPar = currentPar + i - 1;

			// Handling for first and last parameter
			if (currentPar == 0) nPar++;
			if (currentPar == nConfigPars - 1) nPar--;

			uint8_t row = 22 + i * 10;

			char *buf = configPars[nPar].asprint();
			u8g2.drawStr(0, row, buf);
			delete buf;

			if (currentSelection != Selection::actions) {
				uint8_t x, w;
				// Draw cursor on active parameter/digit
				if (currentSelection == Selection::par) {
					x = 0;
					w = 40;
				} else if (currentSelection == Selection::number) {
					x = 60 + currentDigit * 6;
					// Skip the decimal separator
					if (configPars[currentPar].var.type == ConfigPar::ConfigParType::d &&
					    currentDigit > configPars[currentPar].magnitude())
						x += 6;
					w = 6;
				}

				if (nPar == currentPar) {
					u8g2.setDrawColor(2);
					u8g2.drawBox(x, row - 8, w, 10);
					u8g2.setDrawColor(1);
				}
			}
		}

		// Actions
		u8g2.drawStr(100, 32, "Save");
		u8g2.drawStr(100, 42, "Reset");
		if (currentSelection == Selection::actions) {
			uint8_t y;
			if (currentAction == Action::save) {
				y = 32 - 8;
			}
			if (currentAction == Action::reset) {
				y = 42 - 8;
			}

			u8g2.setDrawColor(2);
			u8g2.drawBox(100, y, 40, 10);
			u8g2.setDrawColor(1);
		}

		// Mode
		if (eepromConfig.configStore.loadedFromEEPROM)
			u8g2.drawStr(80, 10, "EEPROM");

		if (configSaveTimer != 0) {
			u8g2.drawStr(85, 22, "saved");
			u8g2.setCursor(115, 22);
			u8g2.print(configSaveTimer * refreshInterval / 1000);
		}


		// Bottom bar
		u8g2.setFont(u8g2_font_unifont_t_symbols);
		u8g2.drawGlyph(0, 66, 9664);
		u8g2.drawGlyph(30, 66, 9650);
		u8g2.drawGlyph(36, 66, 9660);
		u8g2.drawGlyph(65, 66, 9654);
		u8g2.drawGlyph(95, 66, 9679);
		u8g2.setFont(u8g2_font_6x12_tr);
		if (currentSelection == Selection::par) {
			u8g2.drawStr(10, 62, "tab");
			u8g2.drawStr(45, 62, "par");
			u8g2.drawStr(75, 62, "menu");
			u8g2.drawStr(105, 62, "edit");
		}
		if (currentSelection == Selection::number) {
			u8g2.drawStr(45, 62, "adj");
			u8g2.drawStr(105, 62, "OK");
		}
		if (currentSelection == Selection::actions) {
			u8g2.drawStr(10, 62, "back");
			u8g2.drawStr(45, 62, "");
			u8g2.drawStr(75, 62, "back");
			u8g2.drawStr(105, 62, "OK");
		}
	}

	/// Draw common elements in Main tab
	void DrawMainCommon() {
		u8g2.setFont(u8g2_font_6x12_tr);

		// Humidity box
		u8g2.drawFrame(-1, 13, 52, 33);
		u8g2.drawVLine(13, 27, 19);
		u8g2.drawStr(0, 23, "Humidity");
		u8g2.drawHLine(0, 26, 51);

		u8g2.drawStr(0, 35, "PV");

		if (humidistat.active) {
			u8g2.drawBox(0, 36, 13, 9);
			u8g2.setDrawColor(0);
		}
		u8g2.drawStr(0, 44, "SP");
		u8g2.setDrawColor(1);

		printf(14, 35, "%5.1f%%", humidistat.getHumidity());
		printf(14, 44, "%5.1f%%", humidistat.sp);

		// CV
		if (!humidistat.active) {
			u8g2.drawBox(0, 46, 15, 9);
			u8g2.setDrawColor(0);
		}
		u8g2.drawStr(0, 54, "CV: ");
		u8g2.setDrawColor(1);
		printf(20, 54, "%3.0f%%", humidistat.cv * 100);

		// Mode
		if (humidistat.active)
			u8g2.drawStr(80, 10, "auto");
		else
			u8g2.drawStr(80, 10, "manual");

		// Bottom bar
		u8g2.drawHLine(0, 54, 128);
		u8g2.setFont(u8g2_font_unifont_t_symbols);
		u8g2.drawGlyph(0, 66, 9664);
		u8g2.drawGlyph(40, 66, 9650);
		u8g2.drawGlyph(50, 66, 9660);
		u8g2.drawGlyph(90, 66, 9679);
		u8g2.setFont(u8g2_font_6x12_tr);
		u8g2.drawStr(10, 62, "tab");
		u8g2.drawStr(60, 62, "adj");
		u8g2.drawStr(100, 62, "mode");
	}

	/// Draw the tab bar
	void drawTabBar() {
		u8g2.setFont(u8g2_font_6x12_tr);
		u8g2.drawFrame(0, 0, 128, 14);

		if (currentTab == Tab::main) {
			u8g2.drawBox(1, 1, 32, 12);
			u8g2.setDrawColor(0);
		}
		u8g2.drawStr(5, 10, "Main");
		u8g2.setDrawColor(1);

		if (currentTab == Tab::config) {
			u8g2.drawBox(32, 1, 46, 12);
			u8g2.setDrawColor(0);
		}

		u8g2.drawStr(38, 10, "Config");
		u8g2.setDrawColor(1);
		u8g2.drawVLine(78, 1, 12);

		// Spinning indicator
		u8g2.setFont(u8g2_font_unifont_t_symbols);
		uint8_t i = (frame / 2) % 4;
		u8g2.drawGlyph(118, 10, 0x25f3 - i);
	}

	bool handleInput(Buttons state, uint16_t pressedFor) override {
		// First handle common input actions between tabs
		if (state == Buttons::NONE) {
			return false;
		}

		// Tab-specific handling
		switch (currentTab) {
			case Tab::main:
				return handleInputMain(state, pressedFor);
			case Tab::config:
				return handleInputConfig(state, pressedFor);
		}
	}

	/// Handle input on the Main tab
	bool handleInputMain(Buttons state, uint16_t pressedFor) {
		int8_t delta = 0;
		if (state == Buttons::LEFT) {
			advanceEnum(currentTab);
			return true;
		} else if (state == Buttons::UP) {
			delta = 1;
		} else if (state == Buttons::DOWN) {
			delta = -1;
		} else if (state == Buttons::SELECT) {
			// Toggle active state
			humidistat.active = !humidistat.active;
			return true;
		}

		// Long press coarse adjustment
		if (pressedFor > longPressDuration)
			delta *= adjustStep;
		if (pressedFor > longPressDuration * 10)
			delta *= 10;

		if (humidistat.active) {
			adjustValue(delta, humidistat.sp, 0, 100);
		} else {
			adjustValue(delta, humidistat.cv, humidistat.getCvMin(), humidistat.getCvMax());
		}
		return true;
	}

	/// Handle input on the Config tab
	bool handleInputConfig(Buttons state, uint8_t pressedFor) {
		// Ugly state machine below, maybe refactor?
		if (currentSelection == Selection::par) {
			if (state == Buttons::SELECT) {
				currentSelection = Selection::number;
				return true;
			}
			if (state == Buttons::LEFT) {
				advanceEnum(currentTab);
				return true;
			}
			if (state == Buttons::RIGHT) {
				currentSelection = Selection::actions;
				return true;
			}
			if (state == Buttons::UP) {
				// Go up in parameter list
				currentPar = currentPar - 1 % nConfigPars;
				// Handle wrap-around
				if (currentPar == 255) currentPar = nConfigPars - 1;
				return true;
			}
			if (state == Buttons::DOWN) {
				// Go down in parameter list
				currentPar = (currentPar + 1) % nConfigPars;
				return true;
			}
		} else if (currentSelection == Selection::number) {
			// Move selected digit left/right
			if (state == Buttons::LEFT) {
				currentDigit--;
				if (currentDigit == 255) currentDigit = 3;
				return true;
			}
			if (state == Buttons::RIGHT) {
				currentDigit = (currentDigit + 1) % 4;
				return true;
			}
			// Go back to parameter selection
			if (state == Buttons::SELECT) {
				currentSelection = Selection::par;
				return true;
			}
			// Adjust digit up/down
			if (state == Buttons::UP) {
				configPars[currentPar].adjust(ipow(10, 3 - currentDigit));
				return true;
			}
			if (state == Buttons::DOWN) {
				configPars[currentPar].adjust(-ipow(10, 3 - currentDigit));
				return true;
			}
		} else if (currentSelection == Selection::actions) {
			if (state == Buttons::LEFT || state == Buttons::RIGHT) {
				currentSelection = Selection::par;
				return true;
			}
			if (state == Buttons::UP || state == Buttons::DOWN) {
				advanceEnum(currentAction);
				return true;
			}
			if (state == Buttons::SELECT) {
				if (currentAction == Action::save) {
					if (configSaveTimer == 0) {
						eepromConfig.save();
						humidistat.updatePIDParameters();
						configSaveTimer = configSaveCooldown;
					}
					return true;
				}
				if (currentAction == Action::reset) {
					eepromConfig.reset();
					return true;
				}
			}
		}
	}

	void draw() override {
		lastRefreshed = millis();

		u8g2.clearBuffer();
		drawTabBar();
		switch (currentTab) {
			case Tab::main:
				drawMain();
				break;
			case Tab::config:
				drawConfig();
				break;
		}
		u8g2.sendBuffer();

		// Keep track of frames
		frame++;

		// Decrement cooldown timer
		if (configSaveTimer != 0)
			configSaveTimer--;
	}

	void drawSplash() override {
		u8g2.setCursor(0, 24);
		u8g2.setFont(u8g2_font_helvR14_tr);
		u8g2.print("Humidistat");
		u8g2.sendBuffer();
	}

	void drawInfo() override {}

	void clear() override {
		u8g2.clear();
	}

	void setCursor(uint8_t col, uint8_t row) override {
		u8g2.setCursor(col, row);
	}

public:
	///@{
	/// Constructor.
	/// \param u8g2         Pointer to a U8G2 instance
	/// \param buttonReader Pointer to a ButtonReader instance
	/// \param humidistat   Pointer to a Humidistat instance
	/// \param trs          Array of 4 pointers to ThermistorReader instances
	/// \param eepromConfig Pointer to a EEPROMConfig instance
	explicit GraphicalDisplayUI(U8G2 *u8g2, const ButtonReader *buttonReader, SingleHumidistat *humidistat,
	                            Array<const ThermistorReader *, 4> trs, EEPROMConfig *eepromConfig)
			: ControllerUI(u8g2, buttonReader, trs), u8g2(*u8g2), eepromConfig(*eepromConfig),
			  humidistat(*humidistat), nConfigPars(5), configPars{
					{&eepromConfig->configStore.HC_Kp,      "Kp"},
					{&eepromConfig->configStore.HC_Ki,      "Ki"},
					{&eepromConfig->configStore.HC_Kd,      "Kd"},
					{&eepromConfig->configStore.dt,         "dt"},
					{&eepromConfig->configStore.S_lowValue, "LV"},
			} {}

	explicit GraphicalDisplayUI(U8G2 *u8g2, const ButtonReader *buttonReader, CascadeHumidistat *humidistat,
	                            Array<const ThermistorReader *, 4> trs, EEPROMConfig *eepromConfig)
			: ControllerUI(u8g2, buttonReader, trs), u8g2(*u8g2), eepromConfig(*eepromConfig),
			  humidistat(*humidistat), nConfigPars(12), configPars{
					{&eepromConfig->configStore.HC_Kp, "HC Kp"},
					{&eepromConfig->configStore.HC_Ki, "HC Ki"},
					{&eepromConfig->configStore.HC_Kd, "HC Kd"},
					{&eepromConfig->configStore.HC_Kf, "HC Kf"},
					{&eepromConfig->configStore.FC_Kp, "FC Kp"},
					{&eepromConfig->configStore.FC_Ki, "FC Ki"},
					{&eepromConfig->configStore.FC_Kd, "FC Kd"},
					{&eepromConfig->configStore.FC_Kf, "FC Kf"},
					{&eepromConfig->configStore.FC_dt, "FC dt"},
					{&eepromConfig->configStore.HC_totalFlowrate, "Total FR"},
					{&eepromConfig->configStore.dt, "dt"},
					{&eepromConfig->configStore.S_lowValue, "LV"},
			} {}
	///@}

	void begin() override {
		SPI.begin();
		u8g2.begin();
	}
};

template<>
void GraphicalDisplayUI<SingleHumidistat>::drawMain() {
	DrawMainCommon();

	// PID box
	double pTerm, iTerm, dTerm;
	humidistat.getTerms(pTerm, iTerm, dTerm);

	u8g2.drawFrame(52, 13, 47, 33);
	u8g2.drawStr(54, 23, "P");
	u8g2.drawStr(54, 32, "I");
	u8g2.drawStr(54, 41, "D");
	u8g2.drawVLine(60, 13, 33);

	printf(62, 23, "%6.2f", pTerm);
	printf(62, 32, "%6.2f", iTerm);
	printf(62, 41, "%6.2f", dTerm);

	// Temperature box
	u8g2.setCursor(105, 23);
	u8g2.print(humidistat.getTemperature(), 1);

	// Thermistors
	for (size_t i = 0; i < trs.size(); ++i) {
		if (trs[i]) {
			printNTC(105, 23 - 9 * i, i);
		}
	}
}

template<>
void GraphicalDisplayUI<CascadeHumidistat>::drawMain() {
	DrawMainCommon();

	// Flow box
	u8g2.drawFrame(50, 13, 65, 33);
	u8g2.drawStr(55, 23, "F");
	u8g2.drawStr(66, 23, "wet");
	u8g2.drawStr(91, 23, "dry");
	u8g2.drawHLine(50, 26, 64);
	u8g2.drawVLine(64, 13, 33);
	u8g2.drawVLine(89, 13, 33);

	u8g2.drawStr(52, 35, "PV");
	u8g2.drawStr(52, 44, "CV");

	printf(65, 35, "%3.2f", humidistat.getInner(0)->pv);
	printf(65, 44, "%3.0f%%", humidistat.getInner(0)->cv * 100);
	printf(90, 35, "%3.2f", humidistat.getInner(1)->pv);
	printf(90, 44, "%3.0f%%", humidistat.getInner(1)->cv * 100);
}

#endif //HUMIDISTAT_GRAPHICALDISPLAYUI_H
