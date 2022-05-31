#pragma once

#include "Arduino.h"
#include "Wire.h"

/** Default timeout value for read operations.
 * Set this to 0 to disable timeout detection.
 */
#define DEFAULT_READ_TIMEOUT 1000

#ifndef BUFFER_LENGTH
  // band-aid fix for platforms without Wire-defined BUFFER_LENGTH (removed from some official implementations)
  #define BUFFER_LENGTH 32
#endif

static int8_t readBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t* data, uint16_t timeout = DEFAULT_READ_TIMEOUT);
static int8_t readBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t* data, uint16_t timeout = DEFAULT_READ_TIMEOUT);
static int8_t readBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t* data, uint16_t timeout = DEFAULT_READ_TIMEOUT);
static int8_t readBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t* data, uint16_t timeout = DEFAULT_READ_TIMEOUT);
static int8_t readByte(uint8_t devAddr, uint8_t regAddr, uint8_t* data, uint16_t timeout = DEFAULT_READ_TIMEOUT);
static int8_t readWord(uint8_t devAddr, uint8_t regAddr, uint16_t* data, uint16_t timeout = DEFAULT_READ_TIMEOUT);
static int8_t readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t* data, uint16_t timeout = DEFAULT_READ_TIMEOUT);
static int8_t readWords(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint16_t* data, uint16_t timeout = DEFAULT_READ_TIMEOUT);

static bool writeBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t data);
static bool writeBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t data);
static bool writeBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data);
static bool writeBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t data);
static bool writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data);
static bool writeWord(uint8_t devAddr, uint8_t regAddr, uint16_t data);
static bool writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t* data);
static bool writeWords(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint16_t* data);

/** Read single byte from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param data Container for byte value read from device
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in readTimeout)
 * @return Status of read operation (true = success)
 */
int8_t readByte(uint8_t devAddr, uint8_t regAddr, uint8_t* data, uint16_t timeout) {
  return readBytes(devAddr, regAddr, 1, data, timeout);
}

/** Read multiple bytes from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register regAddr to read from
 * @param length Number of bytes to read
 * @param data Buffer to store read data in
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in readTimeout)
 * @return Number of bytes read (-1 indicates failure)
 */
int8_t readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t* data, uint16_t timeout) {
  int8_t count = 0;
  uint32_t t1 = millis();

  // Arduino v1.0.1+, Wire library
  // Adds official support for repeated start condition, yay!

  // I2C/TWI subsystem uses internal buffer that breaks with large data requests
  // so if user requests more than BUFFER_LENGTH bytes, we have to do it in
  // smaller chunks instead of all at once
  for (uint8_t k = 0; k < length; k += min((int)length, BUFFER_LENGTH)) {
    Wire.beginTransmission(devAddr);
    Wire.write(regAddr);
    Wire.endTransmission();
    Wire.requestFrom(devAddr, (uint8_t)min(length - k, BUFFER_LENGTH));

    for (; Wire.available() && (timeout == 0 || millis() - t1 < timeout); count++) {
      data[count] = Wire.read();
      #ifdef I2CDEV_SERIAL_DEBUG
      Serial.print(data[count], HEX);
      if (count + 1 < length) {
        Serial.print(" ");
      }
      #endif
    }
  }
  // check for timeout
  if (timeout > 0 && millis() - t1 >= timeout && count < length) {
    count = -1;  // timeout
  }
  return count;
}

/** Read multiple words from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register regAddr to read from
 * @param length Number of words to read
 * @param data Buffer to store read data in
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in readTimeout)
 * @return Number of words read (-1 indicates failure)
 */
int8_t readWords(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint16_t* data, uint16_t timeout) {
  int8_t count = 0;
  uint32_t t1 = millis();

  // Arduino v1.0.1+, Wire library
  // Adds official support for repeated start condition, yay!

  // I2C/TWI subsystem uses internal buffer that breaks with large data requests
  // so if user requests more than BUFFER_LENGTH bytes, we have to do it in
  // smaller chunks instead of all at once
  for (uint8_t k = 0; k < length * 2; k += min(length * 2, BUFFER_LENGTH)) {
    Wire.beginTransmission(devAddr);
    Wire.write(regAddr);
    Wire.endTransmission();

    Wire.requestFrom(devAddr, (uint8_t)(length * 2)); // length=words, this wants bytes

    bool msb = true; // starts with MSB, then LSB
    for (; Wire.available() && count < length && (timeout == 0 || millis() - t1 < timeout);) {
      if (msb) {
        // first byte is bits 15-8 (MSb=15)
        data[count] = Wire.read() << 8;
      } else {
        // second byte is bits 7-0 (LSb=0)
        data[count] |= Wire.read();
        #ifdef I2CDEV_SERIAL_DEBUG
        Serial.print(data[count], HEX);
        if (count + 1 < length) {
          Serial.print(" ");
        }
        #endif
        count++;
      }
      msb = !msb;
    }

    Wire.endTransmission();
  }

  if (timeout > 0 && millis() - t1 >= timeout && count < length) {
    count = -1;  // timeout
  }
  return count;
}

/** Write multiple bits in a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitStart First bit position to write (0-15)
 * @param length Number of bits to write (not more than 16)
 * @param data Right-aligned value to write
 * @return Status of operation (true = success)
 */
bool writeBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t data) {
  //              010 value to write
  // fedcba9876543210 bit numbers
  //    xxx           args: bitStart=12, length=3
  // 0001110000000000 mask word
  // 1010111110010110 original value (sample)
  // 1010001110010110 original & ~mask
  // 1010101110010110 masked | value
  uint16_t w;
  if (readWord(devAddr, regAddr, &w) != 0) {
    uint16_t mask = ((1 << length) - 1) << (bitStart - length + 1);
    data <<= (bitStart - length + 1); // shift data into correct position
    data &= mask; // zero all non-important bits in data
    w &= ~(mask); // zero all important bits in existing word
    w |= data; // combine data with existing word
    return writeWord(devAddr, regAddr, w);
  } else {
    return false;
  }
}

/** write a single bit in an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitNum Bit position to write (0-7)
 * @param value New bit value to write
 * @return Status of operation (true = success)
 */
bool writeBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t data) {
  uint8_t b;
  readByte(devAddr, regAddr, &b);
  b = (data != 0) ? (b | (1 << bitNum)) : (b & ~(1 << bitNum));
  return writeByte(devAddr, regAddr, b);
}

/** write a single bit in a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitNum Bit position to write (0-15)
 * @param value New bit value to write
 * @return Status of operation (true = success)
 */
bool writeBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t data) {
  uint16_t w;
  readWord(devAddr, regAddr, &w);
  w = (data != 0) ? (w | (1 << bitNum)) : (w & ~(1 << bitNum));
  return writeWord(devAddr, regAddr, w);
}

/** Write single byte to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register address to write to
 * @param data New byte value to write
 * @return Status of operation (true = success)
 */
bool writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data) {
  return writeBytes(devAddr, regAddr, 1, &data);
}

/** Write single word to a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register address to write to
 * @param data New word value to write
 * @return Status of operation (true = success)
 */
bool writeWord(uint8_t devAddr, uint8_t regAddr, uint16_t data) {
  return writeWords(devAddr, regAddr, 1, &data);
}

/** Write multiple bits in an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitStart First bit position to write (0-7)
 * @param length Number of bits to write (not more than 8)
 * @param data Right-aligned value to write
 * @return Status of operation (true = success)
 */
bool writeBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data) {
  //      010 value to write
  // 76543210 bit numbers
  //    xxx   args: bitStart=4, length=3
  // 00011100 mask byte
  // 10101111 original value (sample)
  // 10100011 original & ~mask
  // 10101011 masked | value
  uint8_t b;
  if (readByte(devAddr, regAddr, &b) != 0) {
    uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
    data <<= (bitStart - length + 1); // shift data into correct position
    data &= mask; // zero all non-important bits in data
    b &= ~(mask); // zero all important bits in existing byte
    b |= data; // combine data with existing byte
    return writeByte(devAddr, regAddr, b);
  } else {
    return false;
  }
}

/** Write multiple bytes to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register address to write to
 * @param length Number of bytes to write
 * @param data Buffer to copy new data from
 * @return Status of operation (true = success)
 */
bool writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t* data) {
  uint8_t status = 0;
  Wire.beginTransmission(devAddr);
  Wire.write((uint8_t) regAddr); // send address
  for (uint8_t i = 0; i < length; i++) {
    Wire.write((uint8_t) data[i]);
  }

  status = Wire.endTransmission();
  return status == 0;
}

/** Write multiple words to a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register address to write to
 * @param length Number of words to write
 * @param data Buffer to copy new data from
 * @return Status of operation (true = success)
 */
bool writeWords(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint16_t* data) {
  uint8_t status = 0;
  Wire.beginTransmission(devAddr);
  Wire.write(regAddr); // send address

  for (uint8_t i = 0; i < length; i++) {
    Wire.write((uint8_t)(data[i] >> 8));    // send MSB
    Wire.write((uint8_t)data[i]);         // send LSB
  }

  status = Wire.endTransmission();
  return status == 0;
}

/** Read a single bit from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitNum Bit position to read (0-7)
 * @param data Container for single bit value
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in readTimeout)
 * @return Status of read operation (true = success)
 */
int8_t readBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t* data, uint16_t timeout) {
  uint8_t b;
  uint8_t count = readByte(devAddr, regAddr, &b, timeout);
  *data = b & (1 << bitNum);
  return count;
}

/** Read a single bit from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitNum Bit position to read (0-15)
 * @param data Container for single bit value
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in readTimeout)
 * @return Status of read operation (true = success)
 */
int8_t readBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t* data, uint16_t timeout) {
  uint16_t b;
  uint8_t count = readWord(devAddr, regAddr, &b, timeout);
  *data = b & (1 << bitNum);
  return count;
}

/** Read multiple bits from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitStart First bit position to read (0-7)
 * @param length Number of bits to read (not more than 8)
 * @param data Container for right-aligned value (i.e. '101' read from any bitStart position will equal 0x05)
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in readTimeout)
 * @return Status of read operation (true = success)
 */
int8_t readBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t* data, uint16_t timeout) {
  // 01101001 read byte
  // 76543210 bit numbers
  //    xxx   args: bitStart=4, length=3
  //    010   masked
  //   -> 010 shifted
  uint8_t count, b;
  if ((count = readByte(devAddr, regAddr, &b, timeout)) != 0) {
    uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
    b &= mask;
    b >>= (bitStart - length + 1);
    *data = b;
  }
  return count;
}

/** Read multiple bits from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitStart First bit position to read (0-15)
 * @param length Number of bits to read (not more than 16)
 * @param data Container for right-aligned value (i.e. '101' read from any bitStart position will equal 0x05)
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in readTimeout)
 * @return Status of read operation (1 = success, 0 = failure, -1 = timeout)
 */
int8_t readBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t* data, uint16_t timeout) {
  // 1101011001101001 read byte
  // fedcba9876543210 bit numbers
  //    xxx           args: bitStart=12, length=3
  //    010           masked
  //           -> 010 shifted
  uint8_t count;
  uint16_t w;
  if ((count = readWord(devAddr, regAddr, &w, timeout)) != 0) {
    uint16_t mask = ((1 << length) - 1) << (bitStart - length + 1);
    w &= mask;
    w >>= (bitStart - length + 1);
    *data = w;
  }
  return count;
}

/** Read single word from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param data Container for word value read from device
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in readTimeout)
 * @return Status of read operation (true = success)
 */
int8_t readWord(uint8_t devAddr, uint8_t regAddr, uint16_t* data, uint16_t timeout) {
  return readWords(devAddr, regAddr, 1, data, timeout);
}





