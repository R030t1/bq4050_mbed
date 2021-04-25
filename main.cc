#include "mbed.h"
#include <string.h>

#include <chrono>
using namespace std::chrono_literals;

enum bq4050_register_type {
	none, u1, u2, u4, i1, i2, i4, h1, h2, h4, f, s
};

struct bq4050_register {
	bq4050_register_type type;
	uint8_t addr;
	size_t sz;
} register_file[] = {
	{ .type = u2, .addr = 0x01, }, // r/w	RemainingCapacityAlarm
	{ .type = u2, .addr = 0x02, }, // r/w	RemainingTimeAlarm
	{ .type = h2, .addr = 0x03, }, // r/w	BatteryMode
	{ .type = i2, .addr = 0x04, }, // r/w	AtRate
	{ .type = u2, .addr = 0x05, }, // r/w	AtRateTimeToFull
	{ .type = u2, .addr = 0x06, }, // r		AtRateTimeToEmpty
	{ .type = u2, .addr = 0x07, }, // r		AtRateOK
	{ .type = u2, .addr = 0x08, }, // r		Temperature
	{ .type = u2, .addr = 0x09, }, // r		Voltage
	{ .type = i2, .addr = 0x0a, }, // r		Current
	{ .type = i2, .addr = 0x0b, }, // r		AverageCurrent
	{ .type = u1, .addr = 0x0c, }, // r		MaxError
	{ .type = u1, .addr = 0x0d, }, // r		RelativeStateOfCharge
	{ .type = u1, .addr = 0x0e, }, // r		AbsoluteStateOfCharge
	{ .type = u2, .addr = 0x0f, }, // r		RemainingCapacity				*
	{ .type = u2, .addr = 0x10, }, // r		FullChargeCapacity				*
	{ .type = u2, .addr = 0x11, }, // r 	RunTimeToEmpty					*
	{ .type = u2, .addr = 0x12, }, // r		AverageTimeToEmpty
	{ .type = u2, .addr = 0x13, }, // r		AverageTimeToFull
	{ .type = u2, .addr = 0x14, }, // r		ChargingCurrent					*
	{ .type = u2, .addr = 0x15, }, // r		ChargingVoltage					*
	{ .type = h2, .addr = 0x16, }, // r		BatteryStatus					*
	{ .type = u2, .addr = 0x17, }, // r		CycleCount
	{ .type = u2, .addr = 0x18, }, // r		DesignCapacity
	{ .type = u2, .addr = 0x19, }, // r		DesignVoltage
	{ .type = h2, .addr = 0x1a, }, // r		SpecificationInfo
	{ .type = u2, .addr = 0x1b, }, // r		ManufactureDate
	{ .type = h2, .addr = 0x1c, }, // r		SerialNumber
	{ .type =  s, .addr = 0x20, }, // r		ManufacturerName
	{ .type =  s, .addr = 0x21, }, // r		DeviceName
	{ .type =  s, .addr = 0x22, }, // r		DeviceChemistry
	{ .type =  s, .addr = 0x23, }, // r		ManufacturerData
	{ .type = u2, .addr = 0x3c, }, // r		CellVoltage3
	{ .type = u2, .addr = 0x3d, }, // r		CellVoltage2
	{ .type = u2, .addr = 0x3e, }, // r		CellVoltage1
	{ .type = u2, .addr = 0x3f, }, // r		CellVoltage0
	{ .type = i2, .addr = 0x4a, }, // r/w	BTPDischargeSet
	{ .type = i2, .addr = 0x4b, }, // r/w	BTPCharageSet
	{ .type = u2, .addr = 0x4f, }, // r		StateOfHealth					*

	// These h4 types are listed as "block" protocol, check to see if they
	// report length as first element.
	{ .type = h4, .addr = 0x50, }, // r		SafetyAlert
	{ .type = h4, .addr = 0x51, }, // r		SafetyStatus
	{ .type = h4, .addr = 0x52, }, // r		PFAlert
	{ .type = h4, .addr = 0x53, }, // r		PFStatus
	{ .type = h4, .addr = 0x54, }, // r		OperationStatus
	{ .type = h4, .addr = 0x55, }, // r		ChargingStatus
	{ .type = h4, .addr = 0x56, }, // r		GaugingStatus
	{ .type = h4, .addr = 0x57, }, // r		ManufacturingStatus
	{ .type =  s, .addr = 0x58, }, // r		AFE Register

	{ .type =  s, .addr = 0x60, }, // r		Lifetime Data Block 1
	{ .type =  s, .addr = 0x61, }, // r		Lifetime Data Block 2
	{ .type =  s, .addr = 0x62, }, // r		Lifetime Data Block 3
	{ .type =  s, .addr = 0x63, }, // r		Lifetime Data Block 4
	{ .type =  s, .addr = 0x64, }, // r		Lifetime Data Block 5

	{ .type =  s, .addr = 0x70, }, // r		ManufacturerInfo
	{ .type =  s, .addr = 0x71, }, // r		DAStatus1 (combined stats)
	{ .type =  s, .addr = 0x72, }, // r		DAStatus2 (combined stats)
};

int main() {
	int addr = 0x16;
	I2C i2c(I2C_SDA, I2C_SCL);
	i2c.frequency(100000);

	char i = 0, data[33];
	while (true) {
		memset(data, 0, sizeof(data));
		thread_sleep_for(2000);

		data[0] = 0x71;
		i2c.write(addr, data, 1, true);

		data[0] = 0x00;
		i2c.read( addr, data, 33);

		printf("%02x ", i);
		for (int j = 0; j < 33; j++)
			printf("%02x ", data[j]);
		printf("\r\n");

		
		data[0] = 0x72;
		i2c.write(addr, data, 1, true);

		data[0] = 0x00;
		i2c.read( addr, data, 15);

		printf("-- ");
		for (int j = 0; j < 15; j++)
			printf("%02x ", data[j]);
		printf("\r\n");
		i++;
	}
}
