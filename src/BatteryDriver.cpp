/*

This file is part of STEMI hexapod library.

Copyright (C) 2017 STEMI d.o.o.
Authors: Luka Fucek, et al.
Contact: info@stemi.education
Web: https://www.stemi.education/

STEMI hexapod library is free software exclusively
for non-commercial activities.

In case of developing any comercial activities involving STEMI
hexapod library please contact stemi.education for providing custom
license at: info@stemi.education.

For non-commercial activities you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

STEMI hexapod library is
distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with STEMI hexapod library.
If not, see http://www.gnu.org/licenses/.

For additional information please check http://www.stemi.education.

*/


#include "BatteryDriver.h"

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_7;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;


BatteryDriver::BatteryDriver()
{

	timeStarted = millis();
	preferences.begin("my-app", false);
	batteryPinCalibrationValue = preferences.getFloat("batCalibVal",0);
	Serial.println("Battery sense calibration data loaded: ");
	Serial.println(batteryPinCalibrationValue);

	robot.battery.voltage = readBatteryVoltage();


}

void BatteryDriver::checkState()
{
	robot.battery.voltage = LPFvoltage(readBatteryVoltage());

	switch (robot.battery.state)
	{
	case BATTERY_LOW_STATE:
		//from low to mid
		if (robot.battery.voltage > BATTERY_MID1_V + BATTERY_HYSTERESIS)
		{
			robot.battery.state = BATTERY_MID_STATE;
			robot.battery.percentage = BATTERY_MID_P;
		}
		//from mid to low
		if (robot.battery.voltage < BATTERY_LOW_V - BATTERY_HYSTERESIS/2)
		{
			robot.battery.state = BATTERY_EMPTY_STATE;
			robot.battery.percentage = BATTERY_EMPTY_P;
			//shut down servos and put to battery empty mode
			//robot._setServoPower(0);
			robot.setMode(ROBOT_BATTERY_EMPTY_MODE);
		}
		break;
	case BATTERY_MID_STATE:
		//from mid to high
		if (robot.battery.voltage > BATTERY_MID2_V + BATTERY_HYSTERESIS)
		{
			robot.battery.state = BATTERY_HIGH_STATE;
			robot.battery.percentage = BATTERY_HIGH_P;
		}
		//from mid to low
		if (robot.battery.voltage < BATTERY_MID1_V - BATTERY_HYSTERESIS)
		{
			robot.battery.state = BATTERY_LOW_STATE;
			robot.battery.percentage = BATTERY_LOW_P;
		}
		break;
	case BATTERY_HIGH_STATE:
		//from high to mid
		if (robot.battery.voltage < BATTERY_MID2_V - BATTERY_HYSTERESIS)
		{
			robot.battery.state = BATTERY_MID_STATE;
			robot.battery.percentage = BATTERY_MID_P;
		}
		break;
	}

#ifdef DEBUG_VOLTAGES
	Serial.print(batteryPinCalibrationValue);
	Serial.print(" ");
	Serial.print(readBatteryVoltage());
	Serial.print(" ");
	Serial.print(robot.battery.voltage);
	Serial.print(" ");
	Serial.print(robot.battery.state);
	Serial.print(" ");
	Serial.println(robot.battery.percentage);
#endif // DEBUG_VOLTAGES


}

float BatteryDriver::readBatteryVoltage()
{
	float senVal = (float)(analogRead(BATTERY_STATUS_PIN));
	return  (-0.000000000023926 * pow(senVal, 3) + 0.000000094746 * pow(senVal, 2) + 0.00074539 * senVal + 0.14925) * 2.0 + batteryPinCalibrationValue;
}

float BatteryDriver::LPFvoltage(float valueNew)
{
	float alpha = 0.5;
	return alpha * robot.battery.voltage + (1 - alpha)*(valueNew);
}

void BatteryDriver::calibrateBatteryPin()
{
	if ((millis() - timeStarted) < 8000)
	{
		batteryPinCalibrationValue = 0;
		float batteryPinCalibrationValueSum = 0;
		for (int i = 0; i < 10; i++)
		{
			batteryPinCalibrationValueSum += BATTERY_PIN_CALIBRATION_REF_V - readBatteryVoltage();
			delay(2);
		}
		batteryPinCalibrationValue += batteryPinCalibrationValueSum / 10;
		preferences.putFloat("batCalibVal", batteryPinCalibrationValue);
		Serial.print("Stored battery calibration value: ");
		Serial.println(batteryPinCalibrationValue);
		Serial.print(" measured: ");
		Serial.println(readBatteryVoltage());
	}
	else
	{
		Serial.println("Battery Caliration Unavailable");
	}
}
