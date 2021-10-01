#include <stdint.h>

#include "ConfigPar.h"
#include "asprintf.h"
#include "imath.h"

void ConfigPar::adjust(int16_t delta) const {
	switch (var.type) {
		case ConfigParType::ui8:
			*var.ui8 += delta;
			return;
		case ConfigParType::ui16:
			*var.ui16 += delta;
			return;
		case ConfigParType::d:
			*var.d += static_cast<double>(delta) / 1000;
			return;
	}
}

char *ConfigPar::asprint() const {
	switch (var.type) {
		case ConfigParType::ui8:
			return asprintf("%-8s % 5u", label, *var.ui8);
		case ConfigParType::ui16:
			return asprintf("%-8s % 5u", label, *var.ui16);
		case ConfigParType::d:
			return asprintf("%-8s % 6.3f", label, *var.d);
	}
}

uint8_t ConfigPar::magnitude() const {
	auto mag = [](auto n) {return floor(ilog10(floor(abs(n))));};

	switch (var.type) {
		case ConfigParType::ui8:
			return mag(*var.ui8);
		case ConfigParType::ui16:
			return mag(*var.ui16);
		case ConfigParType::d:
			return mag(*var.d);
	}
}
