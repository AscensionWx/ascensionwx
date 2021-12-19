#include <stdint.h>

class LoraEncoder {
    public:
        LoraEncoder(uint8_t *buffer);
        void writeUnixtime(uint32_t unixtime);
        void writeLatLng(double latitude, double longitude);
        void writeUint16(uint16_t i);
        void writeTemperature(float temperature);
        void writeUint8(uint8_t i);
        void writeHumidity(float humidity);
        void writeBitmap(bool a, bool b, bool c, bool d, bool e, bool f, bool g, bool h);
        void writeRawFloat(float value);

    private:
        uint8_t* _buffer;
        void _intToBytes(uint8_t *buf, int32_t i, uint8_t byteSize);
};
