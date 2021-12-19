
#include <encoders/LoraEncoder.h>
/*
    This library is used to re-encode data sent by the Kanda micro-controllers. Note that
    it is very IMPORTANT this code be maintained to match the Arduino library 
    used on the Kanda micro-controllers for LoRaWAN.

    As of December 8, 2021 the original source code meant for Arduino can be found here under
        an MIT license. However, check with Kanda to make sure their exact library matches.
     https://github.com/thesolarnomad/lora-serialization/blob/master/src/LoraEncoder.cpp

    Since eosio contracts are also written in C++, porting encoder from the Arduino to
        eosio blockchain contract is trivial task. Only adjustments needed was to remove
        ARDUINO namespace/headers and change all "byte" instances to uint8_t .
*/

LoraEncoder::LoraEncoder(uint8_t *buffer) {
  _buffer = buffer;
}

void LoraEncoder::_intToBytes(uint8_t *buf, int32_t i, uint8_t byteSize) {
    for(uint8_t x = 0; x < byteSize; x++) {
        buf[x] = (uint8_t) (i >> (x*8));
    }
}

void LoraEncoder::writeUnixtime(uint32_t unixtime) {
    _intToBytes(_buffer, unixtime, 4);
    _buffer += 4;
}

void LoraEncoder::writeLatLng(double latitude, double longitude) {
    int32_t lat = latitude * 1e6;
    int32_t lng = longitude * 1e6;

    _intToBytes(_buffer, lat, 4);
    _intToBytes(_buffer + 4, lng, 4);
    _buffer += 8;
}

void LoraEncoder::writeUint16(uint16_t i) {
    _intToBytes(_buffer, i, 2);
    _buffer += 2;
}

void LoraEncoder::writeUint8(uint8_t i) {
    _intToBytes(_buffer, i, 1);
    _buffer += 1;
}

void LoraEncoder::writeHumidity(float humidity) {
    int16_t h = (int16_t) (humidity * 100);
    _intToBytes(_buffer, h, 2);
    _buffer += 2;
}

void LoraEncoder::writeRawFloat(float value) {
  uint32_t asbytes=*(reinterpret_cast<uint32_t*>(&value));
  _intToBytes(_buffer, asbytes, 4);
  _buffer += 4;
}

/**
* Uses a 16bit two's complement with two decimals, so the range is
* -327.68 to +327.67 degrees
*/
void LoraEncoder::writeTemperature(float temperature) {
    int16_t t = (int16_t) (temperature * 100);
    if (temperature < 0) {
        t = ~-t;
        t = t + 1;
    }
    _buffer[0] = (uint8_t) ((t >> 8) & 0xFF);
    _buffer[1] = (uint8_t) t & 0xFF;
    _buffer += 2;
}

void LoraEncoder::writeBitmap(bool a, bool b, bool c, bool d, bool e, bool f, bool g, bool h) {
    uint8_t bitmap = 0;
    // LSB first
    bitmap |= (a & 1) << 7;
    bitmap |= (b & 1) << 6;
    bitmap |= (c & 1) << 5;
    bitmap |= (d & 1) << 4;
    bitmap |= (e & 1) << 3;
    bitmap |= (f & 1) << 2;
    bitmap |= (g & 1) << 1;
    bitmap |= (h & 1) << 0;
    writeUint8(bitmap);
}
