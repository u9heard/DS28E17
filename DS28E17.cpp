#include <Arduino.h>
#include <OneWire.h>

#define ONEWIRE_TIMEOUT 50
#define DS28E17_WRITE 0x4B
#define DS28E17_READ 0x87
#define DS28E17_MEMMORY_READ 0x2D
#define RW_DELAY 50

#define SHT_ADDRESS 0x44
#define SHT_READ_LOW 0x2416
#define SHT_READ_MEDIUM 0x240B
#define SHT_READ_HIGH 0x2400
#define SHT_HEAT_ON 0x306D
#define SHT_HEAT_OFF 0x3066
#define SHT_READ_STATUS 0xF32D
#define SHT_CLR_STATUS 0x3041

typedef uint8_t DS28E17Addr[8];

class DS28E17{
    public:
        DS28E17(uint16_t timeout = 50){
            _timeout = timeout;
        }

        DS28E17(OneWire *oneWirew, uint16_t timeout = 50){
            oneWire = oneWirew;
            _timeout = timeout;
        }

        void setOneWire(OneWire *oneWirew){
            oneWire = oneWirew;
        }

        void setAddress(uint8_t *addr){
            address = addr;
        }

        bool onHeater(){
            uint8_t data[2];
            
            data[0] = ((SHT_HEAT_ON >> 8) & 0xFF);
            data[1] = (SHT_HEAT_ON & 0xFF);

            return _write(SHT_ADDRESS, data, 2);
        }

        bool offHeater(){
            uint8_t data[2];
            
            data[0] = ((SHT_HEAT_OFF >> 8) & 0xFF);
            data[1] = (SHT_HEAT_OFF & 0xFF);

            return _write(SHT_ADDRESS, data, 2);
        }

        bool clearStatus(){
            uint8_t data[2];

            data[0] = ((SHT_CLR_STATUS >> 8) & 0xFF);
            data[1] = (SHT_CLR_STATUS & 0xFF);

            return _write(SHT_ADDRESS, data, 2);
        }

        bool readStatus(){
            uint8_t data[2];
            uint8_t buffer[3];
            
            data[0] = ((SHT_READ_STATUS >> 8) & 0xFF);
            data[1] = (SHT_READ_STATUS & 0xFF);

            if(_write(SHT_ADDRESS, data, 2) == false){
                return false;
            }

            delay(50);

            if(_read(SHT_ADDRESS, buffer, 3) == false){
                return false;
            }

            _shtStatus[0] = buffer[0];
            _shtStatus[1] = buffer[1];
            _shtStatus[2] = buffer[2];

            return true;
        }

        uint8_t* getStatus(){
            return _shtStatus;
        }

        bool UpdateData(uint16_t command = SHT_READ_LOW){
            uint8_t data[2];
            uint8_t buffer[6];

            data[0] = ((command >> 8) & 0xFF);
            data[1] = (command & 0xFF);

            if (_write(SHT_ADDRESS, data, 2) == false){
                return false;
            }
            delay(RW_DELAY); // This is temporary 
            if(_read(SHT_ADDRESS, buffer, 6) == false){
                return false;
            }

            _rawTemperature = (buffer[0] << 8) + buffer[1];
            _rawHumidity = (buffer[3] << 8) + buffer[4];

            return true;
        }                                       

        float getTemperature(){
            return _rawTemperature * (175.0 / 65535) - 45;
        }

        float getHumidity(){
            return _rawHumidity * (100.0 / 65535);
        }

        void setTimeout(uint16_t tOut){
            _timeout = tOut;
        }

    private:
        OneWire *oneWire;
        uint8_t *address;
        uint16_t _rawTemperature;
        uint16_t _rawHumidity;
        uint16_t _timeout;
        uint8_t _shtStatus[3];

        bool _read(uint8_t i2cAddr, uint8_t *buffer, uint8_t bufferLength){
            uint8_t header[3];
            uint8_t headerLength = 3;

            header[0] = DS28E17_READ;
            header[1] = i2cAddr << 1 | 0x01;
            header[2] = bufferLength;


            uint8_t crc[2];
            uint16_t crc16 = oneWire->crc16(header, headerLength);
            crc16 = ~crc16;
            crc[1] = crc16 >> 8;
            crc[0] = crc16 & 0xFF;
            
            oneWire->reset();
            oneWire->select(address);
            oneWire->write_bytes(header, headerLength, 0);
            oneWire->write_bytes(crc, sizeof(crc), 0);

            uint8_t timeout = 0;
            while (oneWire->read_bit() == true){
                delay(1);
                timeout++;
                if (timeout > _timeout){
                    oneWire->depower();
                    return false;
                }
            }
            
            uint8_t stat = oneWire->read();
            uint8_t writeStat = header[0] == DS28E17_READ ? 0 : oneWire->read();
            
            if ((stat != 0x00) || (writeStat != 0x00)) {
                oneWire->depower();
                return false;
            }

            for (int i=0; i<bufferLength; i++){
                buffer[i] = oneWire->read();
            }

            oneWire->depower(); 
            
            return true; 
        }

        bool _write(uint8_t i2cAddr, uint8_t *data, uint8_t dataLength){
            uint8_t header[3];
            uint8_t headerLength = 3;

            header[0] = DS28E17_WRITE;
            header[1] = i2cAddr << 1;
            header[2] = dataLength;

            uint8_t crc[2];
            uint16_t crc16 = oneWire->crc16(header, headerLength);
            crc16 = oneWire->crc16(data, dataLength, crc16);
            crc16 = ~crc16;
            crc[1] = crc16 >> 8;                 
            crc[0] = crc16 & 0xFF;               
            
            oneWire->reset();
            oneWire->select(address);
            oneWire->write_bytes(header, headerLength, 0);
            oneWire->write_bytes(data, dataLength, 0);
            oneWire->write_bytes(crc, sizeof(crc), 0);

            uint8_t timeout = 0;
            while (oneWire->read_bit() == true){
                    delay(1);
                    timeout++;
                    if (timeout > _timeout){
                        oneWire->depower();
                        return false;
                }
            }

            uint8_t stat = oneWire->read();
            uint8_t writeStat = oneWire->read();
            
            if ((stat != 0x00) || (writeStat != 0x00)) {
                oneWire->depower();
                return false;
            }
            
            oneWire->depower(); 

            return true;    
        }

        
        uint8_t crc8(const uint8_t *data, uint8_t len) {
            const uint8_t POLY(0x31);
            uint8_t crc(0xFF);

            for (uint8_t j = len; j; --j) 
            {
                crc ^= *data++;

                for (uint8_t i = 8; i; --i) 
                {
                    crc = (crc & 0x80) ? (crc << 1) ^ POLY : (crc << 1);
                }
            }
            return crc;
    }
    
};