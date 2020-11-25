#include "eeprom_io.h"

#include <EEPROM.h>

//#define DO_DEBUG
#include "debug.h"

namespace eeprom_io {
namespace {

// Based on https://www.arduino.cc/en/Tutorial/EEPROMCrc:
static const uint32_t kCrcTable[16] = {
  0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
  0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
  0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
  0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

// uint32_t crcEEPROMRange(int start, int length) {
//   CRC crc;
//   for (int offset = 0; offset < length; ++offset) {
//     crc.appendByte(EEPROM[start + offset]);
//   }
//   return crc.value();
// }

// uint32_t crcBytes(const uint8_t* src, size_t numBytes) {
//   CRC crc;
//   for (int offset = 0; offset < numBytes; ++offset) {
//     crc.appendByte(*src++);
//   }
//   return crc.value();
// }

// int putName(const char* name) {
//   int numBytes = 0;
//   while (*name != 0) {
//     EEPROM.put(numBytes++, *name++);
//   }
//   return numBytes;
// }

}  // namespace

Crc32::Crc32() : value_(~0L) {}

void Crc32::appendByte(uint8_t v) {
  DBG("Crc32::appendByte(");
  DBG2(v, DEC);
  DBG(") old value=0x");
  DBG2(value_, HEX);
  value_ = kCrcTable[(value_ ^ v) & 0x0f] ^ (value_ >> 4);
  value_ = kCrcTable[(value_ ^ (v >> 4)) & 0x0f] ^ (value_ >> 4);
  value_ = ~value_;
  DBG(", new value=0x");
  DBGLN2(value_, HEX);
}

// Store the value at the specified address.
int Crc32::put(int crcAddress) const {
  static_assert(4 == sizeof value_, "sizeof value_ is not 4");

  DBG("Crc32::put(");
  DBG(crcAddress);
  DBG(") value=0x");
  DBGLN2(value_, HEX);

  EEPROM.put(crcAddress, value_);

  ASSERT(verify(crcAddress));

  return crcAddress + static_cast<int>(sizeof value_);
}

// Validate that the computed value (value_) matches the value stored
// at the specified address.
bool Crc32::verify(int crcAddress) const {
  DBG("Crc32::verify(");
  DBG(crcAddress);
  DBG(") computed value=0x");
  DBG2(value_, HEX);

  uint32_t stored=0;
  EEPROM.get(crcAddress, stored);
  DBG(" stored value=0x");
  DBGLN2(stored, HEX);

  return value_ == stored;
}

int saveName(int toAddress, const char* name) {
  while (*name != 0) {
    EEPROM.put(toAddress++, *name++);
  }
  return toAddress;
}

bool verifyName(int atAddress, const char* name, int* afterAddress) {
  // Confirm the name matches.
  while (*name != 0) {
    char c;
    EEPROM.get(atAddress++, c);
    if (c != *name++) {
      // Names don't match.
      return false;
    }
  }
  *afterAddress = atAddress;
  return true;
}

void putBytes(int address, const uint8_t* src, size_t numBytes, Crc32* crc) {
  while (numBytes-- > 0) {
    uint8_t b = *src++;
    if (crc) {
      crc->appendByte(b);
    }
    EEPROM.update(address++, b);
  }
}

void getBytes(int address, size_t numBytes, uint8_t* dest, Crc32* crc) {
  while (numBytes-- > 0) {
    uint8_t b = EEPROM.read(address++);
    if (crc) {
      crc->appendByte(b);
    }
    *dest++ = b;
  }
}

// void putCrc(int address, const Crc32& crc) {
//   uint32_t value = crc.value();
//   putBytes(address, reinterpret_cast<const uint8_t*>(&value), 4, nullptr);
// }

// bool validateCrc(int crcAddress, const Crc32& crc) {
//   const uint32_t expectedValue = crc.value();
//   uint32_t actualValue;
//   getBytes(crcAddress, 4, reinterpret_cast<uint8_t*>(&actualValue), nullptr);
//   return expectedValue == actualValue;
// }



}  // namespace eeprom_io


// void saveBytesToEEPROM(const char* name, const uint8_t* src, size_t numBytes) {
//   int nameLen = putName(name);
//   unsigned long crc = crcBytes(src, numBytes);
//   EEPROM.put(nameLen, crc);
//   putBytes(nameLen + kSizeOfCrcValue, src, numBytes);
// }

// bool readBytesFromEEPROM(const char* name, size_t numBytes, uint8_t* dest) {
//   // Confirm the name matches.
//   int nameLen = 0;
//   while (*name != 0) {
//     char c;
//     EEPROM.get(nameLen++, c);
//     if (c != *name++) {
//       // Names don't match.
//       return false;
//     }
//   }
//   // Yup, the name matches. Read the CRC value.
//   unsigned long stored_crc;
//   EEPROM.get(nameLen, stored_crc);
//   // Confirm that the stored data has the same CRC.
//   int dataStart = nameLen + kSizeOfCrcValue;
//   unsigned long computed_crc = crcEEPROMRange(dataStart, numBytes);
//   if (stored_crc != computed_crc) {
//     // CRC values don't match. Might mean that the struct has changed size,
//     // i.e. there might be missing values.
//     return false;
//   }
//   // Looks good, copy numBytes from the EEPROM into dest.
//   getBytes(dataStart, numBytes, dest);
//   return true;
// }

// void PersistableWrapper ::ReadFrom(EEPROMCursor* cursor) {}
// void PersistableWrapper ::SaveTo(EEPROMCursor* cursor) const override {}



// void EEPROMCursor::readBytes(size_t numBytes, uint8_t* dest) {
//   while (numBytes-- > 0) {
//     *dest++ = EEPROM.read(address_++);
//   }
// }

// bool EEPROMCursor::readNamedPersistable(const char* name, Persistable* dest) {
//   unsigned long stored_crc;
//   readPart(&stored_crc);
//   int nameStartAddr = address_;
//   if (!verifyName(name)) {
//     return false;
//   }
  
//     dest->ReadFrom(this);
//   }


// }

// bool EEPROMCursor::verifyName(const char* name) {
//   // Confirm the name matches.
//   int nameLen = 0;
//   while (*name != 0) {
//     char c;
//     EEPROM.get(nameLen++, c);
//     if (c != *name++) {
//       // Names don't match.
//       return false;
//     }
//   }
//   return true;
// }
