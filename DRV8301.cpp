#include "DRV8301.h"

// DRV8301 class constructor
// mosi  DRV8301 SPI data in pin
// miso  DRV8301 SPI data out pin
// sclk  DRV8301 SPI clock pin
// cs  DRV8301 SPI chip select pin
// en_gate   DRV8301 enable pin
// fault  DRV8301 fault pin (pull_up)
DRV8301::DRV8301(int mosi, int miso, int sclk, int cs, int en_gate, int fault)
    :drv8301_mosi_pin(mosi), drv8301_miso_pin(miso), drv8301_sclk_pin(sclk),
    drv8301_cs_pin(cs), drv8301_en_gate_pin(en_gate), drv8301_fault_pin(fault)
{

}

#pragma GCC push_options
#pragma GCC optimize("O0") //Don't use GCC optimize
/**
 * Use for SPI timing's delay function
 * It's only test on STM32F405/F407 168MHz
 */
void DRV8301::spi_delay(void)
{
    for (int i = 0; i < 22; i++)
        ;
}
#pragma GCC pop_options

// SPI transfer 16 bit value
uint16_t DRV8301::spi_transfer(uint16_t txdata)
{
    uint16_t rxdata = 0;

    for (int i = 0; i < 16; i++)
    {
        digitalWrite(drv8301_mosi_pin, bitRead(txdata, 15 - i));
        digitalWrite(drv8301_sclk_pin, HIGH);
        spi_delay();
        digitalWrite(drv8301_sclk_pin, LOW);
        bitWrite(rxdata, 15 - i, digitalRead(drv8301_miso_pin));
        spi_delay();
    }

    return rxdata;
}

// Read DRV8301's register
int DRV8301::drv8301_read_reg(uint16_t reg)
{
    uint16_t read_data;

    digitalWrite(drv8301_cs_pin, LOW);
    read_data = spi_transfer(0x8000 | ((reg & 0x000F) << 11));
    digitalWrite(drv8301_cs_pin, HIGH);

    digitalWrite(drv8301_cs_pin, LOW);
    read_data = spi_transfer(0xffff);
    digitalWrite(drv8301_cs_pin, HIGH);

    return read_data;
}

// Set DRV8301's register
void DRV8301::drv8301_write_reg(uint16_t reg, uint16_t data)
{
    digitalWrite(drv8301_cs_pin, LOW);
    spi_transfer(((reg & 0x000F) << 11) | (data & 0x07FF));
    digitalWrite(drv8301_cs_pin, HIGH);
}

// Initialize pin and initialize DRV8301
void DRV8301::begin(DRV8301_PWM_INPUT_MODE pwm_mode)
{
    /** Initialize pin */
    pinMode(drv8301_en_gate_pin, OUTPUT);
    digitalWrite(drv8301_en_gate_pin, LOW);
    pinMode(drv8301_fault_pin, INPUT_PULLUP);
    pinMode(drv8301_cs_pin, OUTPUT);
    digitalWrite(drv8301_cs_pin, HIGH);
    pinMode(drv8301_mosi_pin, OUTPUT);
    pinMode(drv8301_miso_pin, INPUT);
    pinMode(drv8301_sclk_pin, OUTPUT);
    digitalWrite(drv8301_sclk_pin, LOW);

    /** Configure register */
    drv8301_ctrl_reg1_val = 0x0000;
    drv8301_ctrl_reg1_val = OCP_MODE_DISABLE | OC_ADJ_SET(27); //Disable OC

    switch (pwm_mode)
    {
    case PWM_INPUT_MODE_3PWM:
        drv8301_ctrl_reg1_val |= PWM_MODE_3_PWM_INPUTS;
        break;

    case PWM_INPUT_MODE_6PWM:
        drv8301_ctrl_reg1_val |= PWM_MODE_6_PWM_INPUTS;
        break;
    }

    reset();
}

// Reset DRV8301
void DRV8301::reset(void)
{
    /** Reset timing */
    digitalWrite(drv8301_en_gate_pin, LOW);
    delayMicroseconds(40);
    digitalWrite(drv8301_en_gate_pin, HIGH);
    delay(20);

    /** Update register value */
    drv8301_read_reg(DRV8301_STATUS_REG1);
    drv8301_write_reg(DRV8301_CONTROL_REG1, drv8301_ctrl_reg1_val);
}

// Detect if DRV8301 has fault occurred
// retval   0:no faults 1:has faults
int DRV8301::is_fault(void)
{
    return (int)!digitalRead(drv8301_fault_pin);
}

// Read DRV8301's fault value
// retval DRV8301's fault value
int DRV8301::read_fault(void)
{
    uint16_t reg1, reg2;
    reg1 = drv8301_read_reg(DRV8301_STATUS_REG1) & 0x07FF;
    reg2 = drv8301_read_reg(DRV8301_STATUS_REG2) & 0x07FF;
    reg1 &= 0x03FF;
    reg2 &= 0x0080;
    return (int)(reg1 | reg2 << 3);
}

// Get DRV8301's chip id
// retval   chip id
int DRV8301::get_id(void)
{
    return drv8301_read_reg(DRV8301_STATUS_REG2) & 0x000F;
}
