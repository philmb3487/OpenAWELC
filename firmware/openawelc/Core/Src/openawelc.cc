/*
 * openawelc.cc
 *
 *  Created on: Jun 16, 2025
 *      Author: philmb
 */

#include "main.h"

#include <cmath>
#include <numbers>
#include <optional>
#include <tuple>

void I2C_LED_RESET()
{
	{
		uint8_t cmd[2] = { 0xA5, 0x5A };
		HAL_I2C_Master_Transmit(&hi2c1, 0x6B << 1, cmd, 2, 100);
		HAL_Delay(100);
	}
	{
		uint8_t cmd[2] = { 0x00, 0x01 };   // default values, with OSC normal mode.
		HAL_I2C_Master_Transmit(&hi2c1, 0x62 << 1, cmd, 2, 100);
		HAL_Delay(100);
	}
	{
		uint8_t cmd[2] = { 0x01, 0x00 };   // default values
		HAL_I2C_Master_Transmit(&hi2c1, 0x62 << 1, cmd, 2, 100);
		HAL_Delay(100);
	}
	/* set channels to PWM mode */
	{
		uint8_t cmd[2] = { 0x14, 0xAA };
		HAL_I2C_Master_Transmit(&hi2c1, 0x62 << 1, cmd, 2, 100);
		HAL_Delay(100);
	}
	{
		uint8_t cmd[2] = { 0x15, 0xAA };
		HAL_I2C_Master_Transmit(&hi2c1, 0x62 << 1, cmd, 2, 100);
		HAL_Delay(100);
	}
	{
		uint8_t cmd[2] = { 0x16, 0xAA };
		HAL_I2C_Master_Transmit(&hi2c1, 0x62 << 1, cmd, 2, 100);
		HAL_Delay(100);
	}
	{
		uint8_t cmd[2] = { 0x17, 0xAA };
		HAL_I2C_Master_Transmit(&hi2c1, 0x62 << 1, cmd, 2, 100);
		HAL_Delay(100);
	}
}

void I2C_LED_SET(int i, int val)
{
	uint8_t cmd[2];
	cmd[0] = 2 + (i & 0xf);
	cmd[1] = val;
	HAL_I2C_Master_Transmit(&hi2c1, 0x62 << 1, cmd, 2, 100);
}

void update_lid_head(std::tuple<uint8_t, uint8_t, uint8_t> color,
		             double alpha = 1.)
{
	const auto [red, green, blue] = color;
	I2C_LED_SET(6, red * alpha);
	I2C_LED_SET(7, green * alpha);
	I2C_LED_SET(8, blue * alpha);
}

void update_power_head(std::tuple<uint8_t, uint8_t, uint8_t> color,
				       double alpha = 1.)
{
	const auto [red, green, blue] = color;
	I2C_LED_SET(12, red * alpha);
	I2C_LED_SET(13, green * alpha);
	I2C_LED_SET(14, blue * alpha);
}

void update_ring(std::tuple<uint8_t, uint8_t, uint8_t> color,
		         double alpha = 1.)
{
	const auto [red, green, blue] = color;
	I2C_LED_SET(0, red * alpha);
	I2C_LED_SET(1, green * alpha);
	I2C_LED_SET(2, blue * alpha);
	I2C_LED_SET(3, red * alpha);
	I2C_LED_SET(4, green * alpha);
	I2C_LED_SET(5, blue * alpha);
}

extern "C"
void openawelc_setup(void)
{
	I2C_LED_RESET();
}

extern "C"
void openawelc_loop(void)
{
	/*  https://thingpulse.com/breathing-leds-cracking-the-algorithm-behind-our-breathing-pattern/  */
	uint8_t brightness;
	uint32_t millis = HAL_GetTick();
	brightness = (std::exp(std::sin(millis / 2000.0 * std::numbers::pi)) - 0.368) * 42.546;

	bool computer_sleeping = (! HAL_GPIO_ReadPin(COMPUTER_SLEEPINGn_GPIO_Port, COMPUTER_SLEEPINGn_Pin));
	bool lid = (! HAL_GPIO_ReadPin(LIDn_GPIO_Port, LIDn_Pin));
	bool ac_power = (! HAL_GPIO_ReadPin(AC_POWERn_GPIO_Port, AC_POWERn_Pin));
	bool charging = (  HAL_GPIO_ReadPin(BAT_CHGn_GPIO_Port, BAT_CHGn_Pin));
	bool bat_low = (  HAL_GPIO_ReadPin(BAT_LOW_GPIO_Port, BAT_LOW_Pin));

	/* The LEDs' quite bright at full value, so tone it down a bit */
	constexpr double sleeping_factor = 0.2;
	constexpr double running_factor = 0.66;


	/* One possible improvement: putting these color values below in a theme file */
	if (computer_sleeping) {
		update_lid_head({brightness,brightness,brightness}, sleeping_factor);

		if (lid)
			update_power_head({0,0,0});
		else
			update_power_head({brightness,brightness,brightness}, running_factor);
	} else {
		if (charging) {
			update_power_head({0xf0,0xa5,0x00}, running_factor);
		} else {
			if (ac_power)
				update_power_head({0,240,240}, running_factor);
			else if (bat_low)
				update_power_head({240,0,0}, running_factor);
			else
				update_power_head({0,240,0}, running_factor);
		}

		update_lid_head({0,240,240}, running_factor);
	}

	/* In any case, we don't want the ring lights */
	update_ring({0,0,0});

	/* Low-power savings, while still allowing ticks' wake-ups */
	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}
