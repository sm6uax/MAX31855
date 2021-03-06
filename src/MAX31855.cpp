/*! @file MAX31855.cpp
 @section MAX38155cpp_intro_section Description

Arduino Library for Microchip SRAM access\n\n
See main library header file for details
*/
#include "MAX31855.h" // Include the header definition
//MAX31855_Class::MAX31855_Class()  {}  ///< Empty & unused class constructor
MAX31855_Class::~MAX31855_Class() {}  ///< Empty & unused class destructor

/***************************************************************************************************************//*!
* @brief   Initialize library with Hardware SPI (overloaded function
* @details When called with one parameter which represents the chip-select pin then hardware SPI is used, otherwise 
*          when called with 3 parameters they define the software SPI pins SPI to be used. Since the MAX31855 is a 
*          1-way device, there is no practical way to check for a device, so a dummy read is done to see if the 
*          device is responding and that defines whether the function returns a true or false
*          success
* @param[in] chipSelect Chip-Select pin number
* @param[in] reverse Option boolean switch to indicate that the wires are (intentionally) reversed
* @return    true if the device could be read, otherwise false
******************************************************************************************************************/
bool MAX31855_Class::begin(const uint8_t chipSelect,uint8_t *errcode , const bool reverse) 
{
  _reversed = reverse;     // Set to true if contacts reversed
  _cs       = chipSelect;  // Copy value for later use
  pinMode(_cs, OUTPUT);    // Make the chip select pin output
  digitalWrite(_cs, HIGH); // High means ignore master

  _spi.begin();   // Initialize SPI communication       
  readRaw();               // Try to read the raw data
  *errcode =_errorCode;
  if (_errorCode)
  {
    return false;
  }
  else
  {
    return true;
  }
} // of method begin()

/***************************************************************************************************************//*!
* @brief   Initialize library with Softwaree SPI (overloaded function
* @details When called with one parameter which represents the chip-select pin then hardware SPI is used, otherwise
*          when called with 3 parameters they define the software SPI pins SPI to be used. Since the MAX31855 is a
*          1-way device, there is no practical way to check for a device, so a dummy read is done to see if the
*          device is responding and that defines whether the function returns a true or false
*          success
* @param[in] chipSelect Chip-Select pin number
* @param[in] miso       Master-In-Slave-Out pin number
* @param[in] sck        System clock pin number
* @param[in] reverse Option boolean switch to indicate that the wires are (intentionally) reversed
* @return    true if the device could be read, otherwise false
*******************************************************************************************************************/
bool MAX31855_Class::begin(const uint8_t chipSelect,const uint8_t miso,const uint8_t sck,const bool reverse)
{
  _reversed = reverse;     // Set to true if contacts reversed
  _cs       = chipSelect;  // Store SPI Chip-Select pin
  _miso     = 14;//miso;        // Store SPI Master-in Slave-Out pin
  _sck      = 16;//sck;         // Store SPI System clock pin
  pinMode(_cs, OUTPUT);    // Make the chip select pin output
  digitalWrite(_cs, HIGH); // High means ignore master
  pinMode(_sck, OUTPUT);   // Make system clock pin output
  pinMode(_miso, INPUT);   // Make master-in slave-out input
  readRaw();               // Read the raw data
  if (_errorCode)
  {
    return false;
  }
  else
  {
    return true;
  }
} // of method begin()

/***************************************************************************************************************//*!
* @brief   Return the device fault state
* @details The fault state of the device from the last read attempt is stored in a private variable, this is
*          returned by this call
* @return  MAX31855 fault state from the last read
*******************************************************************************************************************/
uint8_t MAX31855_Class::fault()
{
  return _errorCode;
} // of method fault()

/***************************************************************************************************************//*!
* @brief   returns the 32 bits of raw data from the MAX31855 device
* @details Sometimes invalid readings are returned (0x7FFFFFFF) so this routine will loop several times until a 
*          valid reading is returned, with a maximum of READING_RETRIES attempts.
* @return  Raw temperature reading
*******************************************************************************************************************/
int32_t MAX31855_Class::readRaw() 
{
  int32_t dataBuffer = 0;
  for(uint8_t retryCounter=0;retryCounter<READING_RETRIES;retryCounter++) // Loop until good reading or overflow
  {
    //digitalWrite(_cs,LOW);                     // Tell MAX31855 that it is active
    //delayMicroseconds(SPI_DELAY_MICROSECONDS); // Give device time to respond
    if(_sck==0) // Hardware SPI
    {
      _spi.beginTransaction(SPISettings(4000000,MSBFIRST,SPI_MODE0)); // Start transaction at 4MHz MSB
      digitalWrite(_cs,LOW);                     // Tell MAX31855 that it is active
      delayMicroseconds(SPI_DELAY_MICROSECONDS);
      dataBuffer   = _spi.transfer(0);                                 // Read a byte
      dataBuffer <<= 8;                                               // Shift over left 8 bits
      dataBuffer  |= _spi.transfer(0);                                 // Read a byte
      dataBuffer <<= 8;                                               // Shift over left 8 bits
      dataBuffer  |= _spi.transfer(0);                                 // Read a byte
      dataBuffer <<= 8;                                               // Shift over left 8 bits
      dataBuffer  |= _spi.transfer(0);                                 // Read a byte
      _spi.endTransaction();                                           // Terminate SPI transaction
    }
    else  // Software SPI
    {
      digitalWrite(_sck, LOW);                     // Toggle the system clock low
      delayMicroseconds(SPI_DELAY_MICROSECONDS);   // Give device time to respond
      for(uint8_t i=0;i<32;i++) {                  // Loop for each bit to be read
        digitalWrite(_sck, LOW);                   // Toggle the system clock low
        delayMicroseconds(SPI_DELAY_MICROSECONDS); // Give device time to respond
        dataBuffer <<= 1;                          // Shift over 1 bit
        if (digitalRead(_miso)) dataBuffer |= 1;   // set rightmost bit if true
        digitalWrite(_sck, HIGH);                  // Toggle the system clock high
        delayMicroseconds(SPI_DELAY_MICROSECONDS); // Give device time to respond
      } // of read each bit from software SPI bus
    } // of if-then-else we are using HW SPI
    digitalWrite(_cs,HIGH);         // MAX31855 no longer active
    _errorCode = dataBuffer & B111; // Mask fault code bits
    if (!_errorCode)
    {
      break; // Leave loop as soon as we get a good reading
    } // if-then no error found
    delay(25); // Wait a bit before retrying
  } // of for-next number of retries
  return dataBuffer;
} // of method readRaw()

/***************************************************************************************************************//*!
* @brief   returns the probe temperature
* @details The temperature is returned in milli-degrees Celsius so that no floating point needs to be used and no 
}          precision is lost
* @return  Probe Temperature in milli/degrees
*******************************************************************************************************************/
int32_t MAX31855_Class::readProbe(uint8_t *errcode)
{
  
  int32_t rawBuffer  = readRaw();    
  *errcode = _errorCode;
  // Read the raw data into variable
  int32_t dataBuffer = rawBuffer;                    // Copy to working variable
  if (dataBuffer & B111) dataBuffer = INT32_MAX;     // if error bits set then return error
  else
  {
    dataBuffer = dataBuffer >> 18;                   // remove unused ambient values
    if(dataBuffer & 0x2000) {
      dataBuffer |= 0xFFFE000; // 2s complement bits if negative
    }
    dataBuffer *= (int32_t)250;                      // Sensitivity is 0.25�C
  } // of if we have an error
  if (_reversed) // If the thermocouple pins are reverse we have to switch readings around
  {
    int32_t ambientBuffer = (rawBuffer&0xFFFF)>>4;          // remove probe & fault values
    if(ambientBuffer & 0x2000) ambientBuffer |= 0xFFFF000;  // 2s complement bits if negative
    ambientBuffer = ambientBuffer*(int32_t)625/(int32_t)10; // Sensitivity is 0.0625�C
    dataBuffer = (ambientBuffer-dataBuffer)+ambientBuffer;  // Invert the delta temperature
  } // of if-then the thermocouple pins reversed
  return dataBuffer;
} // of method readProbe()

/***************************************************************************************************************//*!
* @brief   returns the ambient temperature
* @details The temperature is returned in milli-degrees Celsius so that no floating point needs to be used and no
}          precision is lost
* @return  Ambient Temperature in milli/degrees
*******************************************************************************************************************/
int32_t MAX31855_Class::readAmbient()
{
  int32_t dataBuffer = readRaw();                     // Read the raw data into variable
  if (dataBuffer & B111) dataBuffer = INT32_MAX;      // if error bits set then return error
  else
  {
    dataBuffer = (dataBuffer&0xFFFF)>>4;              // remove probe & fault values
    if(dataBuffer & 0x2000) dataBuffer |= 0xFFFF000;  // 2s complement bits if negative
    dataBuffer = dataBuffer*(int32_t)625/(int32_t)10; // Sensitivity is 0.0625�C
  } // of if we have an error
  return dataBuffer;
} // of method readAmbient()
