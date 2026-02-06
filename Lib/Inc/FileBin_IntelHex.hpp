#ifndef LIB_INTELHEX_H
#define LIB_INTELHEX_H

#include <iostream>
#include <vector>
#include <QByteArray>

using namespace std;

typedef enum
{
    LIB_FIRMWAREBIN__UNKNOWN = 0u,
    LIB_FIRMWAREBIN_BIN,
    LIB_FIRMWAREBIN_HEX,
    LIB_FIRMWAREBIN__LENGTH,
} Lib_FirwareBinType;

class FileBin_IntelHex_Record
{
    public:
        uint8_t ByteCount;
        uint16_t Address;
        uint8_t RecordType;
        vector<uint8_t> Data;

        static int8_t Lib_IntelHex_AsciiToNum(uint8_t Number)
        {
            switch (Number) {
                    case 48: return 0;
                    case 49: return 1;
                    case 50: return 2;
                    case 51: return 3;
                    case 52: return 4;
                    case 53: return 5;
                    case 54: return 6;
                    case 55: return 7;
                    case 56: return 8;
                    case 57: return 9;
                    case 65: return 10;
                    case 66: return 11;
                    case 67: return 12;
                    case 68: return 13;
                    case 69: return 14;
                    case 70: return 15;
                }
                return -1;
        }

        static uint8_t Lib_IntelHex_NumToAscii(uint8_t Number)
        {
            switch (Number) {
                  case 0:  return 48;
                  case 1:  return 49;
                  case 2:  return 50;
                  case 3:  return 51;
                  case 4:  return 52;
                  case 5:  return 53;
                  case 6:  return 54;
                  case 7:  return 55;
                  case 8:  return 56;
                  case 9:  return 57;
                  case 10:  return 65;
                  case 11:  return 66;
                  case 12:  return 67;
                  case 13:  return 68;
                  case 14:  return 69;
                  case 15:  return 70;
                }
                return -1;
        }

        FileBin_IntelHex_Record();

        bool Parse(uint8_t *pBuffer, uint16_t Length);

};

class FileBin_IntelHex_Page
{
    public:
        FileBin_IntelHex_Page(void);

        uint32_t BaseAddress;
        uint32_t Length_Bytes;
        vector<uint8_t> Byte;
};

class FileBin_IntelHex_Memory
{
    public:
        vector<FileBin_IntelHex_Page> Page;

        FileBin_IntelHex_Memory(void);

        bool Load(const char *filePathAbs, Lib_FirwareBinType FirmwareBinType);
        bool Save(string filename);
        void Clear(void);
        bool GetMemPageOffset(uint32_t *pPage, uint32_t *pOffset, uint32_t Address);


        uint8_t ReadMem_uint8(uint32_t Address);
        int8_t ReadMem_sint8(uint32_t Address);
        uint16_t ReadMem_uint16(uint32_t Address);
        int16_t ReadMem_sint16(uint32_t Address);
        uint32_t ReadMem_uint32(uint32_t Address);
        int32_t ReadMem_sint32(uint32_t Address);
        float ReadMem_float32(uint32_t Address);

        void WriteMem_float32(uint32_t Address, float value);
        void WriteMem_uint16(uint32_t Address, uint16_t value);
        void WriteMem_sint16(uint32_t Address, int16_t value);
        void WriteMem_boolean(uint32_t Address, uint8_t value);
        void WriteMem_uint8(uint32_t Address, uint8_t value);
        void WriteMem_sint8(uint32_t Address, int8_t value);
        void WriteMem_sint32(uint32_t Address, int32_t value);
        void WriteMem_uint32(uint32_t Address, uint32_t value);
};





#endif // LIB_INTELHEX_H
