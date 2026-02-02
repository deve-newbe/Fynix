/**
 *  \file       FileBin_DWARF.h
 *  \brief      DWARF binary format parser
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

#ifndef FILEBIN_DWARF_H
#define FILEBIN_DWARF_H

#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>

typedef enum
{
    FILEBIN_DWARF_ELEMENT__UNKNOWN,
    FILEBIN_DWARF_ELEMENT_COMPILE_UNIT,
    FILEBIN_DWARF_ELEMENT_BASE_TYPE,
    FILEBIN_DWARF_ELEMENT_TYPEDEF,
    FILEBIN_DWARF_ELEMENT_VOLATILE,
    FILEBIN_DWARF_ELEMENT_ENUMERATION,
    FILEBIN_DWARF_ELEMENT_ARRAY,
    FILEBIN_DWARF_ELEMENT_ARRAY_DIM,
    FILEBIN_DWARF_ELEMENT_STRUCTURE,
    FILEBIN_DWARF_ELEMENT_MEMBER,
    FILEBIN_DWARF_ELEMENT_VARIABLE,
    FILEBIN_DWARF_ELEMENT_CONSTANT,
} FileBin_DWARF_ElementType;

typedef enum
{
    FileBin_VARINFO_TYPE__UNKNOWN = 0,
    FileBin_VARINFO_TYPE_BOOLEAN,
    FileBin_VARINFO_TYPE_UINT8,
    FileBin_VARINFO_TYPE_SINT8,
    FileBin_VARINFO_TYPE_UINT16,
    FileBin_VARINFO_TYPE_SINT16,
    FileBin_VARINFO_TYPE_UINT32,
    FileBin_VARINFO_TYPE_SINT32,
    FileBin_VARINFO_TYPE_UINT64,
    FileBin_VARINFO_TYPE_SINT64,
    FileBin_VARINFO_TYPE_FLOAT32,
    FileBin_VARINFO_TYPE_FLOAT64,
    FileBin_VARINFO_TYPE_ENUM,
    FileBin_VARINFO_TYPE_STRUCT,
    FileBin_VARINFO_TYPE__LENGTH
} FileBin_DWARF_VarInfoLenType;

struct FileBin_DWARF_AbbrevAttr
{
    uint64_t attribute;
    uint64_t form;
};

struct FileBin_DWARF_Abbrev
{
    uint32_t code;          // abbrev code (ULEB128)
    uint32_t tag;           // DW_TAG_*
    bool hasChildren;       // DW_CHILDREN_yes/no
    std::vector<FileBin_DWARF_AbbrevAttr> attributes;
};

/* This will store the set of Dwarf abbrev contained in a specific abbrev offset */
typedef struct
{
    uint32_t abbrevOffset;  // from .debug_info CU header
    std::unordered_map<uint32_t, FileBin_DWARF_Abbrev> abbrevTable;
} FileBin_DWARF_CompileUnitDataType;

struct TreeElementType;
struct TypeDefType;
struct FileBin_VarInfoType;

typedef struct FileBin_VarInfoType
{
    uint32_t Addr;
    uint32_t TypeOffset; // to lookup hash
    FileBin_DWARF_VarInfoLenType DataType;
    struct FileBin_VarInfoType *next = nullptr;
    struct FileBin_VarInfoType *child = nullptr;
    std::vector<uint8_t> data;
    FileBin_DWARF_ElementType elementType;
    bool isQualifier;
    std::vector<uint32_t> Size;
} FileBin_DWARF_VarInfoType;

typedef struct
{
    uint32_t Length_Bytes;
    uint8_t Version;
    uint32_t AbrevOffset;
    uint32_t Offset;
    uint32_t AddrSize;
    FileBin_DWARF_CompileUnitDataType* AbbrevInfo;
    std::unordered_map<uint32_t, TreeElementType*> typeList; // <-- now TypeDefType is known
    std::unordered_map<uint32_t, TreeElementType*> varDeclaration; // <-- now TypeDefType is known
    uint8_t UnitType; //DWARF5
} FileBin_DWARF_CompileUnitType;

typedef struct TreeElementType
{
    struct TreeElementType *next = nullptr;
    struct TreeElementType *child = nullptr;
    std::vector<uint8_t> data;
    FileBin_DWARF_ElementType elementType;
    uint32_t typeOffset;
    FileBin_DWARF_CompileUnitType *cu = nullptr;
    uint32_t Addr;
    std::vector<uint32_t> Size; /* Size of the element in memory (in bytes) */
    uint32_t Location;
    bool isDeclaration; //is just a forward declaration, not the actual definition
    //struct TreeElementType* specification = nullptr; // points to declaration if this is a definition
} TreeElementType;


class FileBin_DWARF
{
    private:

        std::unordered_map<uint32_t, FileBin_DWARF_CompileUnitDataType> AbbrevOffsetCache;
        std::vector<FileBin_DWARF_CompileUnitType*> CompilationUnit; /* Array of Compilation Units (DWARF top-most level DIE) extracted from abbrev */
        const uint8_t* fileBase;

        inline uint64_t FileBin_DWARF_ReadULEB128(const uint8_t*& ptr);
        inline int64_t FileBin_DWARF_ReadSLEB128(const uint8_t*& ptr);


        void Reset();
        void DeleteTree(TreeElementType* node);
        void FreeTree(TreeElementType* node);
        void FreeSymTree(FileBin_DWARF_VarInfoType* node);

    public:

        FileBin_DWARF_VarInfoType* SymbolRoot;
        uint32_t InfoOffset;
        uint32_t StrOffset;
        TreeElementType *DataRoot; /* data extracred from .debug_info thanks to CompilationUnit */
        uint8_t HeaderSize_Byte;

        FileBin_DWARF(void);

        uint8_t Parse(std::string file_name, uint32_t Offset, uint32_t Len, uint32_t InfoOffset, uint32_t InfoLen, uint32_t StrOffset);

        void ParseAllAbbrvSectionHeader(const uint8_t* fileData, uint32_t AbbrevOffset, uint32_t InfoOffset, uint32_t InfoLen);
        FileBin_DWARF_CompileUnitDataType* ParseAbbrevOffset(const uint8_t* abbrevPtr);
        TreeElementType* ParseDIE(const uint8_t*& ptr, const uint8_t* fileBase, uint32_t cuOffset, uint32_t infoLen, FileBin_DWARF_CompileUnitType* cu,TreeElementType* parent);
        std::vector<uint8_t> ReadAttributeValue(const uint8_t*& ptr, uint32_t form, uint8_t addrSize, const uint8_t* fileBase);

        uint8_t SymbolResolveType(TreeElementType* node, FileBin_DWARF_VarInfoType* parent);
        void SymbolTraverse(TreeElementType* node, FileBin_DWARF_VarInfoType *parent);

        void PrintAllAbbrevInfo() const;
        static std::string FileBin_DWARF_DW_AT_ToString(uint16_t StrCode);
        static std::string FileBin_DWARF_DW_FORM_ToString(uint16_t StrCode);
        static std::string FileBin_DWARF_DW_TAG_ToString(uint16_t StrCode);
};

#endif // FILEBIN_DWARF_H
