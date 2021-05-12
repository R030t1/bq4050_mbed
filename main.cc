#include "mbed.h"

#include "mbedtls/sha1.h"

#include "nanopb/pb_common.h"
#include "nanopb/pb_encode.h"
#include "bq4050.pb.h"


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

// Minimum viable use:
//   1. Set DesignCapacity and DesignVoltage.
//   2. Enable lifetime monitoring.				0x44 0x0023
//
// Stretch:
//   1. Set security keys.						0x44 0x0035
//   2. Set authentication key.					0x44 0x0037
//

// TODO: Retturn size from block read instead of passing it in. It's the first
// byte. Unable to construct working algortihm out of mbed primitives; there
// is a long pause after writing the 0x44 before the read that causes device to
// abort.
void bq4050_mfr(I2C &i2c, int addr, uint16_t cmd, int n, char *data) {
	char ob[33] = { 0 };
	
	ob[0] = 0x44; ob[1] = 2;
	ob[2] = (cmd & 0x00ff) << 0; ob[3] = (cmd & 0xff00) >> 8;
	i2c.write(addr, ob, 4);

	ob[1] = ob[2] = ob[3] = 0; // ob[0] = 0x44;
	i2c.write(addr, ob, 1, true);
	i2c.read( addr, data, n);
}

int main() {
	mbedtls_sha1_context dg;
	//mbedtls_sha1_init(&dg);
	//mbedtls_sha1_starts_ret(&dg);

	DigitalOut led(D13);

	int addr = 0x16;
	I2C i2c(I2C_SDA, I2C_SCL);
	i2c.frequency(100000);

	char i = 0, data[34];

	int dfaddr = 0x4000;

	/* Reset, no idea what this really does. Just CPU?
	data[0] = 0x44; data[1] = 0x02;
	data[2] = 0x41; data[3] = 0x00;
	i2c.write(addr, data, 4);
	*/

	/* Successfully writes design capacity. Voltage is two bytes after.
	 * There are other addresses that match the design capacity but only one
	 * that matches the design voltage, around 0x4440.
	int a = 0x4440 + 13; // + 13
	data[0] = 0x44; data[1] = 0x02;
	data[2] = (a & 0x00ff) >> 0;
	data[3] = (a & 0xff00) >> 8;
	i2c.write(addr, data, 4);

	i2c.write(addr, data, 1, true);
	i2c.read( addr, data, 5);

	printf("%04x: %02x %02x\r\n", a, data[3], data[4]);

	data[0] = 0x44; data[1] = 0x04;
	data[2] = (a & 0x00ff) >> 0;
	data[3] = (a & 0xff00) >> 8;
	data[4] = 0xac; data[5] = 0x0d;
	i2c.write(addr, data, 6);
	*/

	/*
	for (int k = dfaddr; k < 0x5fff; k += 32) {
		data[0] = 0x44; data[1] = 0x02;
		data[2] = (k & 0x00ff) >> 0;
		data[3] = (k & 0xff00) >> 8;
		i2c.write(addr, data, 4);

		data[1] = data[2] = data[3] = 0; // data[0] = 0x44;
		i2c.write(addr, data, 1, true);
		i2c.read( addr, data, 34);

		printf("%04x: ", k);
		for (int j = 3; j < 34; j++)
			printf("%02x ", data[j]);
		printf("\r\n");
	}

	while (true);
	*/

	while (true) {
		led = !led;
		memset(data, 0, sizeof(data));
		thread_sleep_for(500);

		data[0] = 0x0d; data[1] = 0x00; data[2] = 0x00;
		i2c.write(addr, data, 1, true);
		i2c.read( addr, data, 1);

		printf("%02x| %d\r\n", i, data[0]);

		int opstatus;

		// OperationStatus
		data[0] = 0x54; data[1] = 0x00; data[2] = 0x00;
		i2c.write(addr, data, 1, true);
		i2c.read( addr, data, 5);

		printf("%02x| ", i);
		for (int j = 0; j < 5; j++)
			printf("%02x ", data[j]);

		opstatus = 
			(data[1] <<  0) | (data[2] <<  8) |
			(data[3] << 16) | (data[4] << 24);
		printf(" (%x %x)",
			(opstatus & (1 << 9)) >> 9,
			(opstatus & (1 << 8)) >> 8
		);
		printf("\r\n");

		int n;

		memset(data, 0, sizeof(data));
		n = 7;
		bq4050_mfr(i2c, addr, 0x0054, 7, data);
		printf("--| ");
		for (int j = 0; j < n; j++)
			printf("%02x ", data[j]);

		opstatus = 
			(data[3] <<  0) | (data[4] <<  8) |
			(data[5] << 16) | (data[6] << 24);
		printf(" (%x %x)",
			(opstatus & (1 << 9)) >> 9,
			(opstatus & (1 << 8)) >> 8
		);
		printf("\r\n");

		memset(data, 0, sizeof(data));
		n = 12;
		bq4050_mfr(i2c, addr, 0x0002, 12, data);
		printf("--| ");
		for (int j = 0; j < n; j++)
			printf("%02x ", data[j]);
		printf("\r\n");

		memset(data, 0, sizeof(data));
		data[0] = 0x18;
		i2c.write(addr, data, 1, true);
		i2c.read( addr, data, 2);
		int descap = data[1] << 8 | data[0];
		printf("--| %04x %d\r\n", descap, descap);
		
		memset(data, 0, sizeof(data));
		data[0] = 0x19;
		i2c.write(addr, data, 1, true);
		i2c.read( addr, data, 2);
		int desvol = data[1] << 8 | data[0];
		printf("--| %04x %d\r\n", desvol, desvol);

		/*

		// BatteryStatus
		data[0] = 0x16; data[1] = 0x00; data[2] = 0x00;
		i2c.write(addr, data, 1, true);
		i2c.read( addr, data, 2);

		int bq4050_errno = (data[1] << 8 | data[0]) & 0x0f;
		printf("--| %02d\r\n", bq4050_errno);

		data[0] = 0x44; data[1] = 0x71; data[2] = 0x00;
		i2c.write(addr, data, 3, true);
		i2c.read( addr, data, 33);

		printf("--| ");
		for (int j = 0; j < 33; j++)
			printf("%02x ", data[j]);
		printf("\r\n");

		data[0] = 0x71; data[1] = 0x00; data[2] = 0x00;
		i2c.write(addr, data, 1, true);
		i2c.read( addr, data, 33);
		
		printf("--| ");
		for (int j = 0; j < 33; j++)
			printf("%02x ", data[j]);
		printf("\r\n");

		*/

		/*
		 * 0x0 = OK
		 * 0x1 = Busy
		 * 0x2 = ReservedCommand
		 * 0x3 = UnsupportedCommand
		 * 0x4 = AccessDenied
		 * 0x5 = Overflow/Underflow
		 * 0x6 = BadSize
		 * 0x7 = UnknownError
		 */

		i++;
		continue;

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
