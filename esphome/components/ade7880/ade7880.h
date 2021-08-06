#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"

/** 
 * ADE Defines
 * =========== *
 */
#include "ade7880_registers.h"

/** GPIO Defines --> TO CHECK!!! */ 
#define ADE_PM0_GPIO        25
#define ADE_PM1_GPIO        26
#define ADE_RESET_GPIO      27
#define ADE_GPIO_PIN_SEL    ((1ULL<<ADE_PM0_GPIO) | (1ULL<<ADE_PM1_GPIO) | (1ULL<<ADE_RESET_GPIO))

/** I²C Defines --> TO CHECK!!! */
#define SLAVE_7B_ADDRESS    0b0111000 /**< ADE7880 7-bit address, as described on the datasheet */
#define I2C_PORT            I2C_NUM_0
#define I2C_SDA_PIN         GPIO_NUM_21
#define I2C_SCL_PIN         GPIO_NUM_22
#define WRITE_BIT           I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT            I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN        0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS       0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL             0x0         /*!< I2C ack value */
#define NACK_VAL            0x1         /*!< I2C nack value */
#define DEBUG               0

/** PGA Gains, must be 0, 1, 2, 4, 8 or 16 */ 
/** PGA = Programmable Gain Amplifier      */
#define ADE_PGA_IGAIN       0   /**< PGA1 */
#define ADE_PGA_NGAIN       0   /**< PGA2 */
#define ADE_PGA_VGAIN       0   /**< PGA3 */

#define ADE_USE_ROGOWSKI    0   /* Defaults to 0: No rogowski coils used */
#define ADE_USE_60HZ        0   /** 50Hz in Europe, therefore 0 */

/** Configuration register values --> TO CHECK!!! */
#define ADE_PMAX_VAL        0x19CE5DE /**< From datasheet: instataneous power when inputs are at full scale */
#define ADE_FS_VAL          1024000 /** Frequency at which the energy is accumulated */
#define ADE_WTHR_VAL        0x03 /**< Default value. For this one and the following, see app note AN-1171, page 5 (Rev. A). */
#define ADE_VARTHR_VAL      0x03 /**< Default value */
#define ADE_VATHR_VAL       0x03 /**< Default value */    
#define ADE_VLEVEL_VAL      0x38000 /**< Default value */
#define ADE_VNOM_VAL        0x23C354  /** From eq. 42, VNOM = V/Vfs * 3766572 = 220*sqrt(2)/500 * 3766572 */
#define ADE_CFXDEN_VAL      0x0DB3 /**< Based on AN-1171. Calibrate later. */

/** ADE Readings --> TO CHECK!!!  */
#define ADE_FULLSCALE_REG   5326737 /**< Max numeric value (positive/negative) corresponding to a max voltage at input */
#define ADE_FULLSCALE_VAL   0.5f /**< Max voltage at the ADC input */
#define ADE_VOLTAGE_ATT     (1.0f / 1001.0f) /**< Voltage attenuation */
#define ADE_CURRENT_FULL    49.4975f /**< Peak full scale current. Irms = 35, Ip = 35 * sqrt(2) */
#define ADE_PMAX            27059678 /**< Instantaneous power when inputs at full scale and in phase */


namespace esphome {
namespace ade7880 {

class ADE7880 : public i2c::I2CDevice, public PollingComponent {
 public:
  void set_irq_pin(uint8_t irq_pin) {
    has_irq_ = true;
    irq_pin_number_ = irq_pin;
  }
  void set_voltage_a_sensor(sensor::Sensor *voltage_a_sensor) { voltage_a_sensor_ = voltage_a_sensor; }
  void set_voltage_b_sensor(sensor::Sensor *voltage_b_sensor) { voltage_b_sensor_ = voltage_b_sensor; }
  void set_voltage_c_sensor(sensor::Sensor *voltage_c_sensor) { voltage_c_sensor_ = voltage_c_sensor; }
  void set_current_a_sensor(sensor::Sensor *current_a_sensor) { current_a_sensor_ = current_a_sensor; }
  void set_current_b_sensor(sensor::Sensor *current_b_sensor) { current_b_sensor_ = current_b_sensor; }
  void set_current_c_sensor(sensor::Sensor *current_c_sensor) { current_c_sensor_ = current_c_sensor; }
  void set_active_power_a_sensor(sensor::Sensor *active_power_a_sensor) {
    active_power_a_sensor_ = active_power_a_sensor;
  }
  void set_active_power_b_sensor(sensor::Sensor *active_power_b_sensor) {
    active_power_b_sensor_ = active_power_b_sensor;
  }
  void set_active_power_c_sensor(sensor::Sensor *active_power_c_sensor) {
    active_power_c_sensor_ = active_power_c_sensor;
  }
  
  /**
  * @brief   Quickly calculate the log base 2 of a number for the PGA gains
  * */
  static uint32_t quick_log2(uint32_t num)
  {
    uint32_t log = 0;
    while (num >>= 1)
      ++log;
    return log;
  }
  
  
  void setup() override {
    if (this->has_irq_) {
      auto pin = GPIOPin(this->irq_pin_number_, INPUT);
      this->irq_pin_ = &pin;
      this->irq_pin_->setup();
    }
    
    /* Setup procedure ADE7880 according to specification ( https://www.analog.com/media/en/technical-documentation/data-sheets/ADE7880.pdf ) */
    this->set_timeout(100, [this]() {
      // Configure ADE GPIOs
      
      // Reset ADE
      
      // Set Power Mode
      /* We assume that the ADE chip powers up in PSM0 (normal) mode. This assumes that PM1 is grounded (externally) and PM0 is held high (open). */
      
      // Choose I2C as main interface and lock
      /* When the ADE7880 enters PSM0 (normal) power mode, the I²C port is the active serial port. Therefore it is sufficient to lock the interface. */
      this->ade_write_<uint8_t>(CONFIG2, 0x02);
      
      // Set gains (PGA = Programmable Gain Amplifier)
      uint16_t gain;
      
      gain =  (quick_log2(ADE_PGA_VGAIN) << 6) | 
              (quick_log2(ADE_PGA_NGAIN) << 3) | 
              (quick_log2(ADE_PGA_IGAIN) << 0);
      
      ade_write_<uint16_t>(Gain, gain);
      
      // Set CONFIG register
#if ADE_USE_ROGOWSKI
      this->ade_write_<uint32_t>(DICOEFF, 0xFF8000);
      this->ade_write_<uint16_t>(CONFIG, 0x0001); /* Not sure about number of bits */
#endif
      // Set COMPMODE register
#if ADE_USE_60HZ
      /* Bit 14 of COMPMODE register set to 1, all other bits set at default value */
      this->ade_write_<uint16_t>(COMPMODE, 0x41FF);
#endif
      
      // Initialize all the other data memory RAM registers
      this->ade_write_<uint32_t>(AIGAIN, 0x000000);
      this->ade_write_<uint32_t>(AVGAIN, 0x000000);
      this->ade_write_<uint32_t>(BIGAIN, 0x000000);
      this->ade_write_<uint32_t>(BVGAIN, 0x000000);
      this->ade_write_<uint32_t>(CIGAIN, 0x000000);
      this->ade_write_<uint32_t>(CVGAIN, 0x000000);
      this->ade_write_<uint32_t>(NIGAIN, 0x000000);

      this->ade_write_<uint32_t>(APGAIN, 0x000000);
      this->ade_write_<uint32_t>(AWATTOS, 0x000000);
      this->ade_write_<uint32_t>(BPGAIN, 0x000000);
      this->ade_write_<uint32_t>(BWATTOS, 0x000000);
      this->ade_write_<uint32_t>(CPGAIN, 0x000000);
      this->ade_write_<uint32_t>(CWATTOS, 0x000000);
      this->ade_write_<uint32_t>(AIRMSOS, 0x000000);
      this->ade_write_<uint32_t>(AVRMSOS, 0x000000);
      this->ade_write_<uint32_t>(BIRMSOS, 0x000000);
      this->ade_write_<uint32_t>(BVRMSOS, 0x000000);
      this->ade_write_<uint32_t>(CIRMSOS, 0x000000);
      this->ade_write_<uint32_t>(CVRMSOS, 0x000000);
      this->ade_write_<uint32_t>(NIRMSOS, 0x000000);
      this->ade_write_<uint32_t>(HPGAIN, 0x000000);
      this->ade_write_<uint32_t>(ISUMLVL, 0x000000);

      this->ade_write_<uint32_t>(AFWATTOS, 0x000000);
      this->ade_write_<uint32_t>(BFWATTOS, 0x000000);
      this->ade_write_<uint32_t>(CFWATTOS, 0x000000);
      this->ade_write_<uint32_t>(AFVAROS, 0x000000);
      this->ade_write_<uint32_t>(BFVAROS, 0x000000);
      this->ade_write_<uint32_t>(CFVAROS, 0x000000);
      this->ade_write_<uint32_t>(AFIRMSOS, 0x000000);
      this->ade_write_<uint32_t>(BFIRMSOS, 0x000000);
      this->ade_write_<uint32_t>(CFIRMSOS, 0x000000);
      this->ade_write_<uint32_t>(HXWATTOS, 0x000000);
      this->ade_write_<uint32_t>(HYWATTOS, 0x000000);
      this->ade_write_<uint32_t>(HZWATTOS, 0x000000);
      this->ade_write_<uint32_t>(HXVAROS, 0x000000);
      this->ade_write_<uint32_t>(HYVAROS, 0x000000);
      this->ade_write_<uint32_t>(HZVAROS, 0x000000);
      this->ade_write_<uint32_t>(HXIRMSOS, 0x000000);
      this->ade_write_<uint32_t>(HYIRMSOS, 0x000000);
      this->ade_write_<uint32_t>(HZIRMSOS, 0x000000);
      this->ade_write_<uint32_t>(HXVRMSOS, 0x000000);
      this->ade_write_<uint32_t>(HYVRMSOS, 0x000000);
      this->ade_write_<uint32_t>(HZVRMSOS, 0x000000);
      this->ade_write_<uint32_t>(AIRMS, 0x000000);
      this->ade_write_<uint32_t>(AVRMS, 0x000000);
      this->ade_write_<uint32_t>(BIRMS, 0x000000);
      this->ade_write_<uint32_t>(BVRMS, 0x000000);
      this->ade_write_<uint32_t>(CIRMS, 0x000000);
      this->ade_write_<uint32_t>(CVRMS, 0x000000);
      this->ade_write_<uint32_t>(NIRMS, 0x000000);
      // Write the last register in the queue three times to ensure that its value is written into the RAM.
      this->ade_write_<uint32_t>(ISUM, 0x000000);
      this->ade_write_<uint32_t>(ISUM, 0x000000);
      this->ade_write_<uint32_t>(ISUM, 0x000000);

      // Initialize the WTHR, VARTHR, VATHR, VLEVEL and VNOM registers
      this->ade_write_<uint8_t>(WTHR, ADE_WTHR_VAL);
      this->ade_write_<uint8_t>(VARTHR, ADE_VARTHR_VAL);
      this->ade_write_<uint8_t>(VATHR, ADE_VATHR_VAL);
      this->ade_write_<uint32_t>(VLEVEL, ADE_VLEVEL_VAL);
      this->ade_write_<uint32_t>(VNOM, ADE_VNOM_VAL);
      
      // Initialize CF1DEN, CF2DEN, and CF3DEN
      this->ade_write_<uint16_t>(CF1DEN, ADE_CFXDEN_VAL);
      this->ade_write_<uint16_t>(CF2DEN, ADE_CFXDEN_VAL);
      this->ade_write_<uint16_t>(CF3DEN, ADE_CFXDEN_VAL);
      
      // Enable RAM protection
      this->ade_write_<uint8_t>(0xE7FE, 0xAD);
      this->ade_write_<uint8_t>(0xE7E3, 0x80);
      
      // Read back all data memory RAM registers to ensure that they initialized with the desired values.
      this->ade_read_<uint32_t>(AIGAIN);
      this->ade_read_<uint32_t>(AVGAIN);
      this->ade_read_<uint32_t>(BIGAIN);
      this->ade_read_<uint32_t>(BVGAIN);
      this->ade_read_<uint32_t>(CIGAIN);
      this->ade_read_<uint32_t>(CVGAIN);
      this->ade_read_<uint32_t>(NIGAIN);
      this->ade_read_<uint32_t>(DICOEFF);
      this->ade_read_<uint32_t>(APGAIN);
      this->ade_read_<uint32_t>(AWATTOS);
      this->ade_read_<uint32_t>(BPGAIN);
      this->ade_read_<uint32_t>(BWATTOS);
      this->ade_read_<uint32_t>(CPGAIN);
      this->ade_read_<uint32_t>(CWATTOS);
      this->ade_read_<uint32_t>(AIRMSOS);
      this->ade_read_<uint32_t>(AVRMSOS);
      this->ade_read_<uint32_t>(BIRMSOS);
      this->ade_read_<uint32_t>(BVRMSOS);
      this->ade_read_<uint32_t>(CIRMSOS);
      this->ade_read_<uint32_t>(CVRMSOS);
      this->ade_read_<uint32_t>(NIRMSOS);
      this->ade_read_<uint32_t>(HPGAIN);
      this->ade_read_<uint32_t>(ISUMLVL);
      this->ade_read_<uint32_t>(VLEVEL);
      this->ade_read_<uint32_t>(AFWATTOS);
      this->ade_read_<uint32_t>(BFWATTOS);
      this->ade_read_<uint32_t>(CFWATTOS);
      this->ade_read_<uint32_t>(AFVAROS);
      this->ade_read_<uint32_t>(BFVAROS);
      this->ade_read_<uint32_t>(CFVAROS);
      this->ade_read_<uint32_t>(AFIRMSOS);
      this->ade_read_<uint32_t>(BFIRMSOS);
      this->ade_read_<uint32_t>(CFIRMSOS);
      this->ade_read_<uint32_t>(HXWATTOS);
      this->ade_read_<uint32_t>(HYWATTOS);
      this->ade_read_<uint32_t>(HZWATTOS);
      this->ade_read_<uint32_t>(HXVAROS);
      this->ade_read_<uint32_t>(HYVAROS);
      this->ade_read_<uint32_t>(HZVAROS);
      this->ade_read_<uint32_t>(HXIRMSOS);
      this->ade_read_<uint32_t>(HYIRMSOS);
      this->ade_read_<uint32_t>(HZIRMSOS);
      this->ade_read_<uint32_t>(HXVRMSOS);
      this->ade_read_<uint32_t>(HYVRMSOS);
      this->ade_read_<uint32_t>(HZVRMSOS);
      this->ade_read_<uint32_t>(AIRMS);
      this->ade_read_<uint32_t>(AVRMS);
      this->ade_read_<uint32_t>(BIRMS);
      this->ade_read_<uint32_t>(BVRMS);
      this->ade_read_<uint32_t>(CIRMS);
      this->ade_read_<uint32_t>(CVRMS);
      this->ade_read_<uint32_t>(NIRMS);
      this->ade_read_<uint32_t>(ISUM);

      // Start the DSP
      this->ade_write_<uint16_t>(RUN, 0x0001);
      
      // Read the energy registers xWATTHR, xVAHR, xFWATTHR, and xFVARHR to erase their content and start energy accumulation from a known state.
      this->ade_read_<uint32_t>(AWATTHR);
      this->ade_read_<uint32_t>(BWATTHR);
      this->ade_read_<uint32_t>(CWATTHR);
      this->ade_read_<uint32_t>(AVAHR);
      this->ade_read_<uint32_t>(BVAHR);
      this->ade_read_<uint32_t>(CVAHR);
      this->ade_read_<uint32_t>(AFWATTHR);
      this->ade_read_<uint32_t>(BFWATTHR);
      this->ade_read_<uint32_t>(CFWATTHR);
      this->ade_read_<uint32_t>(AFVARHR);
      this->ade_read_<uint32_t>(BFVARHR);
      this->ade_read_<uint32_t>(CFVARHR);
      
      // Enable the CF1, CF2, CF3 frequency convertor outputs
      this->ade_write_<uint16_t>(CFMODE, 0x08A0); /* To double check */
      
      // Setup done
      this->is_setup_ = true;
    });
  }

  void dump_config() override;

  void update() override;

 protected:
  template<typename T> bool ade_write_(uint16_t reg, T value) {
    std::vector<uint8_t> data;
    data.push_back(reg >> 8);
    data.push_back(reg >> 0);
    for (int i = sizeof(T) - 1; i >= 0; i--)
      data.push_back(value >> (i * 8));
    return this->write_bytes_raw(data);
  }
  template<typename T> optional<T> ade_read_(uint16_t reg) {
    uint8_t hi = reg >> 8;
    uint8_t lo = reg >> 0;
    if (!this->write_bytes_raw({hi, lo}))
      return {};
    auto ret = this->read_bytes_raw<sizeof(T)>();
    if (!ret.has_value())
      return {};
    T result = 0;
    for (int i = 0, j = sizeof(T) - 1; i < sizeof(T); i++, j--)
      result |= T((*ret)[i]) << (j * 8);
    return result;
  }

  bool has_irq_ = false;
  uint8_t irq_pin_number_;
  GPIOPin *irq_pin_{nullptr};
  bool is_setup_{false};
  sensor::Sensor *voltage_a_sensor_{nullptr};
  sensor::Sensor *voltage_b_sensor_{nullptr};
  sensor::Sensor *voltage_c_sensor_{nullptr};
  sensor::Sensor *current_a_sensor_{nullptr};
  sensor::Sensor *current_b_sensor_{nullptr};
  sensor::Sensor *current_c_sensor_{nullptr};
  sensor::Sensor *active_power_a_sensor_{nullptr};
  sensor::Sensor *active_power_b_sensor_{nullptr};
  sensor::Sensor *active_power_c_sensor_{nullptr};
};

}  // namespace ade7880
}  // namespace esphome
