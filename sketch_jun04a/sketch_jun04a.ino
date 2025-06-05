/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*  Open ELC Firmware for Alienware PC's.
 *  Philippe Michaud-Boudreault <philmb3487@proton.me>
 *
 * This project consists of open lights controller firmware. It has been tested
 * on an Alienware m18 R2 (14700HX, RTX 4070 model).
 *
 * TODO: HSE should be external clock oscillator 12 MHz
 *       There's a ROM attached to the I2C bus, what to do with it?
 */
#include <Arduino.h>
#include <Wire.h>
#include <stm32l412xx.h>

class Pin {
public:
    Pin(auto port, unsigned pin, bool invert = false, int pull = GPIO_PULLUP)
        : m_pin(pin)
        , m_port(port)
        , m_invert(invert)
        , m_pull(pull)
    {}

    void setup()
    {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = (1 << m_pin);
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = m_pull;
        HAL_GPIO_Init(m_port, &GPIO_InitStruct);
    }

    operator unsigned int() const {
        auto ps = HAL_GPIO_ReadPin(m_port, (1 << m_pin));
        return (m_invert? !ps : ps);
    }

protected:
    unsigned m_pin;
    GPIO_TypeDef *m_port;
    bool m_invert;
    int m_pull;
};

namespace i2cparams {
    constexpr uint8_t TCL_ADDR     = 0x62;
    constexpr uint8_t TCL_RST_ADDR = 0x6B;
}

////////////// PIN ////////////////
// AC:         PB0     (active low)
// Charging:   PB1
// Lid:        PB12
///////////////////////////////////

Pin ac(GPIOB, 0, true);
Pin chg(GPIOB, 1);
Pin lid(GPIOB, 12);

// I2C device found at address 0x62
// I2C device found at address 0x68
// I2C device found at address 0x6B
void scanBus() {
    delay(1000);
    byte address, error;
    unsigned nDevices;
    for (address = 0; address < 127; address++) {
        // The i2c scanner uses the return value of
        // the Write.endTransmission to see if
        // a device did acknowledge the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
            nDevices++;
        } else if (error == 4) {
            Serial.print("Unknown error at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    }
    Serial.flush();
}

namespace TLC59116 {
    namespace regs {
        constexpr uint8_t MODE1 = 0x00;
        constexpr uint8_t MODE2 = 0x01;
        constexpr uint8_t PWM0 = 0x02;
        constexpr uint8_t LEDOUT0 = 0x14;
        constexpr uint8_t LEDOUT1 = 0x15;
        constexpr uint8_t LEDOUT2 = 0x16;
        constexpr uint8_t LEDOUT3 = 0x17;
    }
    using namespace regs;

    /* Write to a register */
    void w8(uint8_t reg, uint8_t val)
    {
        Wire.beginTransmission(i2cparams::TCL_ADDR);
        Wire.write(reg);
        Wire.write(val);
        Wire.endTransmission();
    }

    // Arduino-side of things, initialization code
    void init()
    {
        Wire.setSDA(PB7);
        Wire.setSCL(PB6);
        Wire.begin();
    }

    // Reset the chip and put output channels in PWM mode
    void reset()
    {
        Wire.beginTransmission(i2cparams::TCL_RST_ADDR);
        Wire.write(0xA5);
        Wire.write(0x5A);
        Wire.endTransmission();
        delay(10);

        w8(MODE1, 0x01);  // default values, with OSC normal mode.
        delay(1);         // delay of 500us required after changing OSC
        w8(MODE2, 0x00);  // default values
        /* set channels to PWM mode */
        w8(LEDOUT0, 0xAA);
        w8(LEDOUT1, 0xAA);
        w8(LEDOUT2, 0xAA);
        w8(LEDOUT3, 0x2A); // out15 unused
    }

    // Change the brightness for a particular channel (output LED)
    void set(uint8_t i, uint8_t val)
    {
        w8(PWM0 + (i & 0x0f), val);
    }
}

// 0  :  ring top red
// 1  :  ring top green
// 2  :  ring top blue
// 3  :  ring bottom red
// 4  :  ring bottom green
// 5  :  ring bottom blue
// 6  :  lid head red
// 7  :  lid head green
// 8  :  lid head blue
// 9  :  x
// 10 :  x
// 11 :  x
// 12 :  power head red
// 13 :  power head green
// 14 :  power head blue
// 15 :  x

void setRing(unsigned r, unsigned g, unsigned b)
{
    TLC59116::set(0, r);
    TLC59116::set(3, r);
    TLC59116::set(1, g);
    TLC59116::set(4, g);
    TLC59116::set(2, b);
    TLC59116::set(5, b);
}

void setPowerHead(unsigned r, unsigned g, unsigned b)
{
    TLC59116::set(12, r);
    TLC59116::set(13, g);
    TLC59116::set(14, b);
}

void setLidHead(unsigned r, unsigned g, unsigned b)
{
    TLC59116::set(6, r);
    TLC59116::set(7, g);
    TLC59116::set(8, b);
}

void setup() {
    TLC59116::init();
    TLC59116::reset();
    ac.setup();
    chg.setup();
    lid.setup();
}

void loop() {

    if (ac) {
        setRing( 0, 240, 240 );
        setLidHead( 0, 240, 240 );
        setPowerHead(  chg? 240: 0, chg? 165: 240, chg? 0: 240);
    } else {
        setRing( 0, 120, 120 );
        setLidHead( 0, 120, 120 );
        setPowerHead( 0, 120, 120 );
    }
    delay(10);
}
