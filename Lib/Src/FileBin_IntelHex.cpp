/**
 *  \file       FileBin_IntelHex.cpp
 *  \brief      Intel HEX binary format parser
 *
 *  \version    1.0
 *  \date       Jun 12, 2016
 *  \author     Xavier Descarrega - DEVE embedded designs <info@deve.tech>
 *
 *  \copyright  MIT License
 *
 *              Copyright (c) 2016 Xavier Descarrega
 *
 *              Permission is hereby granted, free of charge, to any person obtaining a copy
 *              of this software and associated documentation files (the "Software"), to deal
 *              in the Software without restriction, including without limitation the rights
 *              to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *              copies of the Software, and to permit persons to whom the Software is
 *              furnished to do so, subject to the following conditions:
 *
 *              The above copyright notice and this permission notice shall be included in all
 *              copies or substantial portions of the Software.
 *
 *              THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *              IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *              FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *              AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER
 *              LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *              OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *              SOFTWARE.
 *
 */

#include "FileBin_IntelHex.hpp"
#include "QFile"
#include <QDataStream>
#include "Log.h"


FileBin_IntelHex_Page::FileBin_IntelHex_Page(void)
{
    this->Length_Bytes = 0;
}

FileBin_IntelHex_Record::FileBin_IntelHex_Record()
{
    this->ByteCount = 0;
}

bool FileBin_IntelHex_Record::Parse(uint8_t *pBuffer, uint16_t Length)
{
    uint8_t crc;
    bool bOk = false;

    /* Line start character */
    if (pBuffer[0] == ':')
    {
        crc = 0;
        this->ByteCount = (Lib_IntelHex_AsciiToNum(pBuffer[1]) * 16) + Lib_IntelHex_AsciiToNum(pBuffer[2]);
        this->Address = (Lib_IntelHex_AsciiToNum(pBuffer[3]) * 4096) + (Lib_IntelHex_AsciiToNum(pBuffer[4]) * 256) + (Lib_IntelHex_AsciiToNum(pBuffer[5]) * 16) + Lib_IntelHex_AsciiToNum(pBuffer[6]);       
        this->RecordType = (Lib_IntelHex_AsciiToNum(pBuffer[7]) * 16) + Lib_IntelHex_AsciiToNum(pBuffer[8]);

        //cout << "New register, address: " << std::hex << this->Address << " length: " << std::dec << (int)this->ByteCount << " type: " << std::dec << (int)this->RecordType << endl;

        for (uint16_t i = 0u; i < this->ByteCount; i++)
        {
            Data.push_back((Lib_IntelHex_AsciiToNum(pBuffer[9+2*i]) * 16) + Lib_IntelHex_AsciiToNum(pBuffer[10+2*i]));
            //cout << " ->" << std::hex << (Lib_IntelHex_AsciiToNum(pBuffer[9+2*i]) * 16) + Lib_IntelHex_AsciiToNum(pBuffer[10+2*i]) << endl;
        }

        for (uint32_t i = 0u; i < this->ByteCount + 4u; i++)
        {
            crc -= (Lib_IntelHex_AsciiToNum(pBuffer[(i * 2u) + 1u]) * 16u) + Lib_IntelHex_AsciiToNum(pBuffer[(i * 2u) + 2u]);
        }

        //crc &= 0xFF;
        /* 2's complement */
        //crc = 0x100 - crc;
        //crc &= 0xFF;

        uint8_t crcChk = ((Lib_IntelHex_AsciiToNum(pBuffer[(this->ByteCount * 2) + 9]) * 16u) + Lib_IntelHex_AsciiToNum(pBuffer[(this->ByteCount * 2) + 10])) & 0x00FF;

        if (crc != crcChk)
        {
            cout << "Wrong CRC, should " << (int)crcChk << " is " << (int)crc << endl;
        }

        bOk = (crc == crcChk);
    }

    return bOk;
}

FileBin_IntelHex_Memory::FileBin_IntelHex_Memory()
{
}

void FileBin_IntelHex_Memory::Clear(void)
{
    for (uint32_t i = 0; i < this->Page.size(); i++)
    {
        this->Page.at(i).Byte.clear();
        this->Page.at(i).Length_Bytes = 0;
    }
    this->Page.clear();
}

bool FileBin_IntelHex_Memory::GetMemPageOffset(uint32_t *pPage, uint32_t *pOffset, uint32_t Address)
{
    /* Look for page */
    bool pageFound = false;
    uint32_t minAddress, maxAddress;
    uint32_t cnt = 0;
   // uint8_t fBuf[4];

    if (this->Page.size() == 0)
    {
        return false;
    }

    minAddress = this->Page.at(0).BaseAddress;
    maxAddress = this->Page.at(this->Page.size() - 1).BaseAddress + this->Page.at(this->Page.size() - 1).Byte.size();
    //cout << "uint16 -> Looking for address " << (uint32_t)Address << " len " << "range" << std::hex << minAddress << ":" << maxAddress << endl;//this->Page.size() << endl;

    /* Assume memory pages have monotonic increasing ranges */
    if ((Address < minAddress) || (Address > maxAddress))
    {
        cout << "Error address out of range: 0x" << std::hex << Address << endl;
        return false;
    }

    /* Move to page with right offset */
    while ((!pageFound) && ((cnt + 1) < this->Page.size()))
    {
        //cout << cnt << "page init " << this->Page.at(cnt).BaseAddress << endl;
        if (this->Page.at(cnt + 1).BaseAddress > Address)
        {
            pageFound = true;
        }
        else
        {
            cnt++;
        }

    }

    *pPage = cnt;
    *pOffset = Address - this->Page.at(cnt).BaseAddress;

    return true;

}

uint8_t FileBin_IntelHex_Memory::ReadMem_uint8(uint32_t Address)
{
    uint8_t fBuf[64];
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {
        if (this->Page.at(Page).Byte.size() < (Offset + 1))
        {
            cout << "Error reading memory" << endl;
            return 0.0f;
        }

        fBuf[0] = this->Page.at(Page).Byte.at(Offset);

        return (fBuf[0]);
    }

    return 0;
}

int8_t FileBin_IntelHex_Memory::ReadMem_sint8(uint32_t Address)
{
    uint8_t fBuf[64];
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {
        if (this->Page.at(Page).Byte.size() < (Offset + 1))
        {
            cout << "Error reading memory" << endl;
            return 0.0f;
        }

        fBuf[0] = this->Page.at(Page).Byte.at(Offset);

        return (fBuf[0]);
    }

    return 0;
}

uint16_t FileBin_IntelHex_Memory::ReadMem_uint16(uint32_t Address)
{

    uint8_t fBuf[64];
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {

        if (this->Page.at(Page).Byte.size() < (Offset + 1 + 1))
        {
            cout << "Error reading memory" << endl;
            return 0.0f;
        }

        fBuf[0] = this->Page.at(Page).Byte.at(Offset);
        fBuf[1] = this->Page.at(Page).Byte.at(Offset+1);

        return (fBuf[1] << 8u | fBuf[0]);
    }

    return 0;

}

int16_t FileBin_IntelHex_Memory::ReadMem_sint16(uint32_t Address)
{
    uint8_t fBuf[64];
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {

        if (this->Page.at(Page).Byte.size() < (Offset + 1 + 1))
        {
            cout << "Error reading memory" << endl;
            return 0.0f;
        }

        fBuf[0] = this->Page.at(Page).Byte.at(Offset);
        fBuf[1] = this->Page.at(Page).Byte.at(Offset+1);

        return (fBuf[1] << 8u | fBuf[0]);
    }

    return 0;
}

uint32_t FileBin_IntelHex_Memory::ReadMem_uint32(uint32_t Address)
{
    uint8_t fBuf[64];
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {

        if (this->Page.at(Page).Byte.size() < (Offset + 3 + 1))
        {
            cout << "Error reading memory" << endl;
            return 0.0f;
        }

        fBuf[0] = this->Page.at(Page).Byte.at(Offset);
        fBuf[1] = this->Page.at(Page).Byte.at(Offset+1);
        fBuf[2] = this->Page.at(Page).Byte.at(Offset+2);
        fBuf[3] = this->Page.at(Page).Byte.at(Offset+3);

        return (fBuf[3] << 24u) | (fBuf[2] << 16u) | (fBuf[1] << 8u | fBuf[0]);
    }

    return 0;
}

int32_t FileBin_IntelHex_Memory::ReadMem_sint32(uint32_t Address)
{
    uint8_t fBuf[64];
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {

        if (this->Page.at(Page).Byte.size() < (Offset + 3 + 1))
        {
            cout << "Error reading memory" << endl;
            return 0.0f;
        }

        fBuf[0] = this->Page.at(Page).Byte.at(Offset);
        fBuf[1] = this->Page.at(Page).Byte.at(Offset+1);
        fBuf[2] = this->Page.at(Page).Byte.at(Offset+2);
        fBuf[3] = this->Page.at(Page).Byte.at(Offset+3);

        return (fBuf[3] << 24u) | (fBuf[2] << 16u) | (fBuf[1] << 8u | fBuf[0]);
    }

    return 0;
}

float FileBin_IntelHex_Memory::ReadMem_float32(uint32_t Address)
{
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {

        if (this->Page.at(Page).Byte.size() < (Offset + 3 + 1))
        {
            cout << "Error reading memory" << endl;
            return 0.0f;
        }

        uint8_t fBuf[4];

        fBuf[0] = this->Page.at(Page).Byte.at(Offset);
        fBuf[1] = this->Page.at(Page).Byte.at(Offset+1);
        fBuf[2] = this->Page.at(Page).Byte.at(Offset+2);
        fBuf[3] = this->Page.at(Page).Byte.at(Offset+3);

        float val = *(float *)&fBuf;
        return val;
    }

    return 0.0f;
}

void FileBin_IntelHex_Memory::WriteMem_boolean(uint32_t Address, uint8_t value)
{
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {

        if (this->Page.at(Page).Byte.size() < (Offset + 1))
        {
            cout << "Error writing memory" << endl;
            return;
        }

        this->Page.at(Page).Byte.at(Offset) = value & 0xFF;
    }

}

void FileBin_IntelHex_Memory::WriteMem_uint8(uint32_t Address, uint8_t value)
{
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {

        if (this->Page.at(Page).Byte.size() < (Offset + 1))
        {
            cout << "Error writing memory" << endl;
            return;
        }

        this->Page.at(Page).Byte.at(Offset) = value & 0xFF;
    }

}

void FileBin_IntelHex_Memory::WriteMem_sint8(uint32_t Address, int8_t value)
{
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {

        if (this->Page.at(Page).Byte.size() < (Offset + 1))
        {
            cout << "Error writing memory" << endl;
            return;
        }

        this->Page.at(Page).Byte.at(Offset) = value & 0xFF;
    }

}

void FileBin_IntelHex_Memory::WriteMem_uint16(uint32_t Address, uint16_t value)
{

    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {

        if (this->Page.at(Page).Byte.size() < (Offset + 2))
        {
            cout << "Error writing memory" << endl;
            return;
        }

        this->Page.at(Page).Byte.at(Offset) = value & 0xFF;
        this->Page.at(Page).Byte.at(Offset+1) =(value >> 8u) & 0xFF;

        std::cout << "writing memory 0x" << std::hex << (int)Address << " value " << value << std::endl;
    }

}

void FileBin_IntelHex_Memory::WriteMem_sint16(uint32_t Address, int16_t value)
{
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {

        if (this->Page.at(Page).Byte.size() < (Offset + 2))
        {
            cout << "Error writing memory" << endl;
            return;
        }

        this->Page.at(Page).Byte.at(Offset) = value & 0xFF;
        this->Page.at(Page).Byte.at(Offset+1) =(value >> 8u) & 0xFF;
    }

}

void FileBin_IntelHex_Memory::WriteMem_uint32(uint32_t Address, uint32_t value)
{
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {

        if (this->Page.at(Page).Byte.size() < (Offset + 4))
        {
            cout << "Error writing memory" << endl;
            return;
        }

        this->Page.at(Page).Byte.at(Offset) = value & 0xFF;
        this->Page.at(Page).Byte.at(Offset+1) =(value >> 8u) & 0xFF;;
        this->Page.at(Page).Byte.at(Offset+2) =(value >> 16u) & 0xFF;;
        this->Page.at(Page).Byte.at(Offset+3) =(value >> 24u) & 0xFF;;
    }

}

void FileBin_IntelHex_Memory::WriteMem_sint32(uint32_t Address, int32_t value)
{
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {

        if (this->Page.at(Page).Byte.size() < (Offset + 3 + 1))
        {
            cout << "Error writing memory" << endl;
            return;
        }

        this->Page.at(Page).Byte.at(Offset) = value & 0xFF;
        this->Page.at(Page).Byte.at(Offset+1) =(value >> 8u) & 0xFF;;
        this->Page.at(Page).Byte.at(Offset+2) =(value >> 16u) & 0xFF;;
        this->Page.at(Page).Byte.at(Offset+3) =(value >> 24u) & 0xFF;;
    }

}

void FileBin_IntelHex_Memory::WriteMem_float32(uint32_t Address, float value)
{
    uint32_t Page, Offset;

    if (GetMemPageOffset(&Page, &Offset, Address))
    {

        if (this->Page.at(Page).Byte.size() < (Offset + 3 + 1))
        {
            cout << "Error writing memory" << endl;
            return;
        }

        union {
            float a;
            unsigned char bytes[4];
          } thing;

        thing.a = value;

         this->Page.at(Page).Byte.at(Offset) = thing.bytes[0];
         this->Page.at(Page).Byte.at(Offset+1) = thing.bytes[1];
         this->Page.at(Page).Byte.at(Offset+2) = thing.bytes[2];
         this->Page.at(Page).Byte.at(Offset+3) = thing.bytes[3];
    }

}

bool FileBin_IntelHex_Memory::Save(string filename)
{
    // Attempt to load file
    QFile file(filename.c_str());
    //QDataStream in(&file);

    if (!file.open(QIODevice::WriteOnly | QFile::Truncate))
    {
        //QMessageBox::information(this, tr("Unable to open file"),
        //file.errorString());
        return false;
    }

    //this->Page.size()
    for (uint32_t i = 0 ; i < this->Page.size() ; i++)
    {

        uint8_t recordArray[255];
        recordArray[0] = ':';
        recordArray[1] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii(0);
        recordArray[2] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii(2);
        recordArray[3] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii(0);
        recordArray[4] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii(0);
        recordArray[5] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii(0);
        recordArray[6] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii(0);
        recordArray[7] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii(0);
        recordArray[8] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii(4);

        uint32_t baseAddr = this->Page.at(i).BaseAddress;
        //cout << "base " << (baseAddr>>24u) << endl;
        recordArray[9] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((baseAddr >> 28u) & 0xF);
        recordArray[10] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((baseAddr >> 24u) & 0xF);
        recordArray[11] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((baseAddr >> 20u) & 0xF);
        recordArray[12] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((baseAddr >> 16u) & 0xF);


        uint8_t crc = 0;

        crc -= 0x02;
        crc -= 0x00;
        crc -= 0x04;
        crc -= baseAddr >> 24u;
        crc -= baseAddr >> 16u;


        recordArray[13] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((crc >> 4) & 0xF);
        recordArray[14] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((crc) & 0xF);
        recordArray[15] = 13;
        recordArray[16] = 0;
        file.write((const char *)recordArray);
        file.write("\n");
        for (uint32_t j = 0 ; j < this->Page.at(i).Byte.size() ; j++)
        {
            recordArray[1] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii(1);
            recordArray[2] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii(0);

            recordArray[3] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((j >> 12u) & 0xf);
            recordArray[4] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((j >> 8u) & 0xf);
            recordArray[5] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((j >> 4u) & 0xf);
            recordArray[6] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((j >> 0u) & 0xf);
            recordArray[7] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii(0);
            recordArray[8] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii(0);

            uint8_t byte;// = this->Page.at(i).Byte.at(j);
            uint8_t byteCnt = 0;
            uint8_t crc = 0;
            crc -= 0x10;
            crc -= (j >> 8u) & 0xFF;
            crc -= (j >> 0u) & 0xFF;

            //cout << "crc1 " << (int)j << endl;


            /* There is at least 1 byte to process */
            while (1)
            {
                if (j < this->Page.at(i).Byte.size())
                {
                    byte = this->Page.at(i).Byte.at(j);
                    crc -= byte;
                }
                else
                {
                    /* Padding */
                    byte = 0;
                }

                recordArray[9 + (byteCnt*2)] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((byte >> 4u) & 0xf);
                recordArray[10 + (byteCnt*2)] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((byte >> 0u) & 0xf);
                recordArray[11 + (byteCnt*2)] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((crc >> 4u) & 0xf);
                recordArray[12 + (byteCnt*2)] = FileBin_IntelHex_Record::Lib_IntelHex_NumToAscii((crc >> 0u) & 0xf);
                recordArray[13 + (byteCnt*2)] = 13;
                recordArray[14 + (byteCnt*2)] = 0;


                byteCnt++;
                if (byteCnt >= 16)
                {
                    break;
                }
                else
                {
                    j++;
                }
            }

           // cout << "crc " << (int)crc << endl;


            file.write((const char *)recordArray);
            file.write("\n");
        }
    }

    file.close();
    return true;
}

bool FileBin_IntelHex_Memory::Load(const char *filePathAbs, Lib_FirwareBinType FirmwareBinType)
{
    /* Attempt to load file */
    QFile file(filePathAbs);

    if (!file.open(QIODevice::ReadOnly))
    {
        LOG(std::string(" ERRO] Lib_IntelHex_Memory::") + __func__ + ": Error loading binary file");
        return false;
    }

    this->Clear();


    if (FirmwareBinType == LIB_FIRMWAREBIN_HEX)
    {

        while (!file.atEnd())
        {
            QByteArray line = file.readLine();
            FileBin_IntelHex_Record newRecord;

            if (!newRecord.Parse((uint8_t *)line.toStdString().c_str(), line.toStdString().length()))
            {
                LOG(std::string(" ERRO] Lib_IntelHex_Memory::") + __func__ + ": Error parsing .hex file");
                file.close();
                return false;
            }

            if (newRecord.RecordType == 0x02)
            {
                // if coming from a previous page, fill in the padding zeroes
                if ((this->Page.size() > 0) && ((this->Page.back().Byte.size() % 32) != 0))
                {
                    uint32_t lengthToPad = 32 - (this->Page.back().Byte.size() % 32);

                    for (uint32_t j = 0; j < lengthToPad ; j++)
                    {
                        this->Page.back().Byte.push_back(0);
                        this->Page.back().Length_Bytes++;
                    }
                }

                FileBin_IntelHex_Page newPage;
                newPage.BaseAddress = (((newRecord.Data.at(0) << 8u) + newRecord.Data.at(1)) << 4u);
                this->Page.push_back(newPage);
            }

            /* New segment definition record */
            else if (newRecord.RecordType == 0x04)
            {
                // if coming from a previous page, fill in the padding zeroes
                /* TODO this strongly depends on architecture, tricore requires 32bit alignment */
                if ((this->Page.size() > 0) && ((this->Page.back().Byte.size() % 32) != 0))
                {
                    uint32_t lengthToPad = 32 - (this->Page.back().Byte.size() % 32);

                    for (uint32_t j = 0; j < lengthToPad ; j++)
                    {
                        this->Page.back().Byte.push_back(0);
                        this->Page.back().Length_Bytes++;
                    }
                }

                FileBin_IntelHex_Page newPage;
                newPage.BaseAddress = (((newRecord.Data.at(0) << 8u) + newRecord.Data.at(1)) << 16u);
                this->Page.push_back(newPage);
            }

            /* New data definition record */
            else if (newRecord.RecordType == 0x00)
            {
                if (this->Page.size() > 0)
                {
                    for (uint32_t j = this->Page.back().Length_Bytes; j < newRecord.Address; j++)
                    {
                        this->Page.back().Byte.push_back(0);
                        this->Page.back().Length_Bytes++;
                    }

                    for (uint32_t j = 0; j < newRecord.ByteCount; j++)
                    {
                        this->Page.back().Byte.push_back(newRecord.Data.at(j));
                        this->Page.back().Length_Bytes++;
                    }
                }
            }
        }
    }
    else if (FirmwareBinType == LIB_FIRMWAREBIN_BIN)
    {
        uint8_t byte;
        FileBin_IntelHex_Page newPage;

        uint32_t lenFound = 0u;

        newPage.BaseAddress = lenFound;
        this->Page.push_back(newPage);

        while ((!file.atEnd()))
        {

            // return value from 'file.read' should always be sizeof(char).
            file.read((char *)&byte,sizeof(uint8_t));
            this->Page.back().Byte.push_back(byte);
            this->Page.back().Length_Bytes++;
            lenFound++;
        }
    }

    file.close();

    return true;
}
