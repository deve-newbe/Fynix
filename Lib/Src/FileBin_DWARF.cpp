/**
 *  \file       FileBin_DWARF.cpp
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

#include "FileBin_DWARF.h"
#include "FileBin_DWARF_Def.h"
#include <cassert>
#include <queue>
#include <string>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

class MappedFile
{
    public:
        const uint8_t* data = nullptr;
        size_t size = 0;

    #if defined(_WIN32) || defined(_WIN64)
    private:
        HANDLE hFile = INVALID_HANDLE_VALUE;
        HANDLE hMap = NULL;
    #else
    private:
        int fd = -1;
    #endif

    public:
        bool open(const std::string& fileName)
        {
    #if defined(_WIN32) || defined(_WIN64)
            hFile = CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE) return false;

            LARGE_INTEGER fsize;
            if (!GetFileSizeEx(hFile, &fsize)) return false;
            size = static_cast<size_t>(fsize.QuadPart);

            hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
            if (!hMap) return false;

            data = static_cast<const uint8_t*>(MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0));
            if (!data) return false;

    #else
            fd = ::open(fileName.c_str(), O_RDONLY);
            if (fd < 0) return false;

            struct stat st;
            if (fstat(fd, &st) < 0) return false;
            size = st.st_size;

            data = static_cast<const uint8_t*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
            if (data == MAP_FAILED) return false;
    #endif
            return true;
        }

        void close()
        {
    #if defined(_WIN32) || defined(_WIN64)
            if (data) UnmapViewOfFile(data);
            if (hMap) CloseHandle(hMap);
            if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    #else
            if (data) munmap((void*)data, size);
            if (fd >= 0) ::close(fd);
    #endif
            data = nullptr;
            size = 0;
        }

        ~MappedFile()
        {
            close();
        }
};

#define LIBPARSER_DWARF_BYTESLEBMAX 24
#define LIBPARSER_DWARF_BITSPERBYTE 8
#define LIBPARSER_DWARF_DEBUG_ABBRV (0)
#define LIBPARSER_DWARF_DEBUG (0)

uint8_t HeaderSize_Byte;

FileBin_DWARF::FileBin_DWARF()
{
    this->DataRoot = nullptr;
    this->SymbolRoot = nullptr;
}

std::string FileBin_DWARF::FileBin_DWARF_DW_TAG_ToString(uint16_t StrCode)
{
    std::string strVal;

    switch (StrCode)
    {
    case DW_TAG_array_type:                 strVal = "DW_TAG_array_type"; break;
    case DW_TAG_class_type:                 strVal = "DW_TAG_class_type"; break;
    case DW_TAG_entry_point:                strVal = "DW_TAG_entry_point"; break;
    case DW_TAG_enumeration_type:           strVal = "DW_TAG_enumeration_type"; break;
    case DW_TAG_formal_parameter:           strVal = "DW_TAG_formal_parameter"; break;
    case DW_TAG_imported_declaration:       strVal = "DW_TAG_imported_declaration"; break;
    case DW_TAG_label:                      strVal = "DW_TAG_label"; break;
    case DW_TAG_lexical_block:              strVal = "DW_TAG_lexical_block"; break;
    case DW_TAG_member:                     strVal = "DW_TAG_member"; break;
    case DW_TAG_pointer_type:               strVal = "DW_TAG_pointer_type"; break;
    case DW_TAG_reference_type:             strVal = "DW_TAG_reference_type"; break;
    case DW_TAG_compile_unit:               strVal = "DW_TAG_compile_unit"; break;
    case DW_TAG_string_type:                strVal = "DW_TAG_string_type"; break;
    case DW_TAG_structure_type:             strVal = "DW_TAG_structure_type"; break;
    case DW_TAG_subroutine_type:            strVal = "DW_TAG_subroutine_type"; break;
    case DW_TAG_typedef:                    strVal = "DW_TAG_typedef"; break;
    case DW_TAG_union_type:                 strVal = "DW_TAG_union_type"; break;
    case DW_TAG_unspecified_parameters:     strVal = "DW_TAG_unspecified_parameters"; break;
    case DW_TAG_variant:                    strVal = "DW_TAG_variant"; break;
    case DW_TAG_common_block:               strVal = "DW_TAG_common_block"; break;
    case DW_TAG_common_inclusion:           strVal = "DW_TAG_common_inclusion"; break;
    case DW_TAG_inheritance:                strVal = "DW_TAG_inheritance"; break;
    case DW_TAG_inlined_subroutine:         strVal = "DW_TAG_inlined_subroutine"; break;
    case DW_TAG_module:                     strVal = "DW_TAG_module"; break;
    case DW_TAG_ptr_to_member_type:         strVal = "DW_TAG_ptr_to_member_type"; break;
    case DW_TAG_set_type:                   strVal = "DW_TAG_set_type"; break;
    case DW_TAG_subrange_type:              strVal = "DW_TAG_subrange_type"; break;
    case DW_TAG_with_stmt:                  strVal = "DW_TAG_with_stmt"; break;
    case DW_TAG_access_declaration:         strVal = "DW_TAG_access_declaration"; break;
    case DW_TAG_base_type:                  strVal = "DW_TAG_base_type"; break;
    case DW_TAG_catch_block:                strVal = "DW_TAG_catch_block"; break;
    case DW_TAG_const_type:                 strVal = "DW_TAG_const_type"; break;
    case DW_TAG_constant:                   strVal = "DW_TAG_constant"; break;
    case DW_TAG_enumerator:                 strVal = "DW_TAG_enumerator"; break;
    case DW_TAG_file_type:                  strVal = "DW_TAG_file_type"; break;
    case DW_TAG_friend:                     strVal = "DW_TAG_friend"; break;
    case DW_TAG_namelist:                   strVal = "DW_TAG_namelist"; break;
    case DW_TAG_namelist_item:              strVal = "DW_TAG_namelist_item"; break;
    case DW_TAG_packed_type:                strVal = "DW_TAG_packed_type"; break;
    case DW_TAG_subprogram:                 strVal = "DW_TAG_subprogram"; break;
    case DW_TAG_template_type_parameter:    strVal = "DW_TAG_template_type_parameter"; break;
    case DW_TAG_template_value_parameter:   strVal = "DW_TAG_template_value_parameter"; break;
    case DW_TAG_thrown_type:                strVal = "DW_TAG_thrown_type"; break;
    case DW_TAG_try_block:                  strVal = "DW_TAG_try_block"; break;
    case DW_TAG_variant_part:               strVal = "DW_TAG_variant_part"; break;
    case DW_TAG_variable:                   strVal = "DW_TAG_variable"; break;
    case DW_TAG_volatile_type:              strVal = "DW_TAG_volatile_type"; break;
    case DW_TAG_dwarf_procedure:            strVal = "DW_TAG_dwarf_procedure"; break;
    case DW_TAG_restrict_type:              strVal = "DW_TAG_restrict_type"; break;
    case DW_TAG_interface_type:             strVal = "DW_TAG_interface_type"; break;
    case DW_TAG_namespace:                  strVal = "DW_TAG_namespace"; break;
    case DW_TAG_imported_module:            strVal = "DW_TAG_imported_module"; break;
    case DW_TAG_unspecified_type:           strVal = "DW_TAG_unspecified_type"; break;
    case DW_TAG_partial_unit:               strVal = "DW_TAG_partial_unit"; break;
    case DW_TAG_imported_unit:              strVal = "DW_TAG_imported_unit"; break;
    case DW_TAG_mutable_type:               strVal = "DW_TAG_mutable_type"; break;
    case DW_TAG_condition:                  strVal = "DW_TAG_condition"; break;
    case DW_TAG_shared_type:                strVal = "DW_TAG_shared_type"; break;
    case DW_TAG_type_unit:                  strVal = "DW_TAG_type_unit"; break;
    case DW_TAG_rvalue_reference_type:      strVal = "DW_TAG_rvalue_reference_type"; break;
    case DW_TAG_template_alias:             strVal = "DW_TAG_template_alias"; break;
    case DW_TAG_coarray_type:               strVal = "DW_TAG_coarray_type"; break;
    case DW_TAG_generic_subrange:           strVal = "DW_TAG_generic_subrange"; break;
    case DW_TAG_dynamic_type:               strVal = "DW_TAG_dynamic_type"; break;
    case DW_TAG_atomic_type:                strVal = "DW_TAG_atomic_type"; break;
    case DW_TAG_call_site:                  strVal = "DW_TAG_call_site"; break;
    case DW_TAG_call_site_parameter:        strVal = "DW_TAG_call_site_parameter"; break;
    case DW_TAG_skeleton_unit:              strVal = "DW_TAG_skeleton_unit"; break;
    case DW_TAG_immutable_type:             strVal = "DW_TAG_immutable_type"; break;
    default:                                strVal = "Invalid DW_TAG"; break;
    }

    return strVal;
}

std::string FileBin_DWARF::FileBin_DWARF_DW_AT_ToString(uint16_t StrCode)
{
    std::string section_header_type_str ;
    switch (StrCode)
    {
    case DW_AT_sibling:              section_header_type_str = "DW_AT_sibling"; break;
    case DW_AT_location:              section_header_type_str = "DW_AT_location"; break;
    case DW_AT_name:              section_header_type_str = "DW_AT_name"; break;
    case DW_AT_ordering:              section_header_type_str = "DW_AT_ordering"; break;
    case DW_AT_subscr_data:              section_header_type_str = "DW_AT_subscr_data"; break;
    case DW_AT_byte_size:              section_header_type_str = "DW_AT_byte_size"; break;
    case DW_AT_bit_offset:              section_header_type_str = "DW_AT_bit_offset"; break;
    case DW_AT_bit_size:              section_header_type_str = "DW_AT_bit_size"; break;
    case DW_AT_element_list:              section_header_type_str = "DW_AT_element_list"; break;
    case DW_AT_stmt_list:              section_header_type_str = "DW_AT_stmt_list"; break;
    case DW_AT_low_pc:              section_header_type_str = "DW_AT_low_pc"; break;
    case DW_AT_high_pc:              section_header_type_str = "DW_AT_high_pc"; break;
    case DW_AT_language:              section_header_type_str = "DW_AT_language"; break;
    case DW_AT_member:              section_header_type_str = "DW_AT_member"; break;
    case DW_AT_discr:              section_header_type_str = "DW_AT_discr"; break;
    case DW_AT_discr_value:              section_header_type_str = "DW_AT_discr_value"; break;
    case DW_AT_visibility:              section_header_type_str = "DW_AT_visibility"; break;
    case DW_AT_import:              section_header_type_str = "DW_AT_import"; break;
    case DW_AT_string_length:              section_header_type_str = "DW_AT_string_length"; break;
    case DW_AT_common_reference:              section_header_type_str = "DW_AT_common_reference"; break;
    case DW_AT_comp_dir:              section_header_type_str = "DW_AT_comp_dir"; break;
    case DW_AT_const_value:              section_header_type_str = "DW_AT_const_value"; break;
    case DW_AT_containing_type:              section_header_type_str = "DW_AT_containing_type"; break;
    case DW_AT_default_value:              section_header_type_str = "DW_AT_default_value"; break;
    case DW_AT_inline:              section_header_type_str = "DW_AT_inline"; break;
    case DW_AT_is_optional:              section_header_type_str = "DW_AT_is_optional"; break;
    case DW_AT_lower_bound:              section_header_type_str = "DW_AT_lower_bound"; break;
    case DW_AT_producer:              section_header_type_str = "DW_AT_producer"; break;
    case DW_AT_prototyped:              section_header_type_str = "DW_AT_prototyped"; break;
    case DW_AT_return_addr:              section_header_type_str = "DW_AT_return_addr"; break;
    case DW_AT_start_scope:              section_header_type_str = "DW_AT_start_scope"; break;
    case DW_AT_bit_stride:              section_header_type_str = "DW_AT_bit_stride"; break;
    case DW_AT_upper_bound:              section_header_type_str = "DW_AT_upper_bound"; break;
    case DW_AT_abstract_origin:              section_header_type_str = "DW_AT_abstract_origin"; break;
    case DW_AT_accessibility:              section_header_type_str = "DW_AT_accessibility"; break;
    case DW_AT_address_class:              section_header_type_str = "DW_AT_address_class"; break;
    case DW_AT_artificial:              section_header_type_str = "DW_AT_artificial"; break;
    case DW_AT_base_types:              section_header_type_str = "DW_AT_base_types"; break;
    case DW_AT_calling_convention:              section_header_type_str = "DW_AT_calling_convention"; break;
    case DW_AT_count:              section_header_type_str = "DW_AT_count"; break;
    case DW_AT_data_member_location:              section_header_type_str = "DW_AT_data_member_location"; break;
    case DW_AT_decl_column:              section_header_type_str = "DW_AT_decl_column"; break;
    case DW_AT_decl_file:              section_header_type_str = "DW_AT_decl_file"; break;
    case DW_AT_decl_line:              section_header_type_str = "DW_AT_decl_line"; break;
    case DW_AT_declaration:              section_header_type_str = "DW_AT_declaration"; break;
    case DW_AT_discr_list:              section_header_type_str = "DW_AT_discr_list"; break;
    case DW_AT_encoding:              section_header_type_str = "DW_AT_encoding"; break;
    case DW_AT_external:              section_header_type_str = "DW_AT_external"; break;
    case DW_AT_frame_base:              section_header_type_str = "DW_AT_frame_base"; break;
    case DW_AT_friend:              section_header_type_str = "DW_AT_friend"; break;
    case DW_AT_identifier_case:              section_header_type_str = "DW_AT_identifier_case"; break;
    case DW_AT_macro_info:              section_header_type_str = "DW_AT_macro_info"; break;
    case DW_AT_namelist_item:              section_header_type_str = "DW_AT_namelist_item"; break;
    case DW_AT_priority:              section_header_type_str = "DW_AT_priority"; break;
    case  DW_AT_segment:              section_header_type_str = "DW_AT_segment"; break;
    case  DW_AT_specification:              section_header_type_str = "DW_AT_specification"; break;
    case  DW_AT_static_link:              section_header_type_str = "DW_AT_static_link"; break;
    case  DW_AT_type:              section_header_type_str = "DW_AT_type"; break;
    case  DW_AT_use_location:              section_header_type_str = "DW_AT_use_location          "; break;
    case  DW_AT_variable_parameter:              section_header_type_str = "DW_AT_variable_parameter    "; break;
    case  DW_AT_virtuality:              section_header_type_str = "DW_AT_virtuality            "; break;
    case  DW_AT_vtable_elem_location:              section_header_type_str = "DW_AT_vtable_elem_location  "; break;
    case  DW_AT_allocated:              section_header_type_str = "DW_AT_allocated             "; break;
    case  DW_AT_associated:              section_header_type_str = "DW_AT_associated            "; break;
    case  DW_AT_data_location:              section_header_type_str = "DW_AT_data_location         "; break;
    case  DW_AT_byte_stride:              section_header_type_str = "DW_AT_byte_stride           "; break;
    case  DW_AT_entry_pc:              section_header_type_str = "DW_AT_entry_pc              "; break;
    case  DW_AT_use_UTF8:              section_header_type_str = "DW_AT_use_UTF8              "; break;
    case  DW_AT_extension:              section_header_type_str = "DW_AT_extension             "; break;
    case  DW_AT_ranges:              section_header_type_str = "DW_AT_ranges                "; break;
    case  DW_AT_trampoline:              section_header_type_str = "DW_AT_trampoline            "; break;
    case  DW_AT_call_column:              section_header_type_str = "DW_AT_call_column           "; break;
    case  DW_AT_call_file:              section_header_type_str = "DW_AT_call_file             "; break;
    case  DW_AT_call_line:              section_header_type_str = "DW_AT_call_line             "; break;
    case  DW_AT_description:              section_header_type_str = "DW_AT_description           "; break;
    case  DW_AT_binary_scale:              section_header_type_str = "DW_AT_binary_scale          "; break;
    case  DW_AT_decimal_scale:              section_header_type_str = "DW_AT_decimal_scale         "; break;
    case  DW_AT_small:              section_header_type_str = "DW_AT_small                 "; break;
    case  DW_AT_decimal_sign:              section_header_type_str = "DW_AT_decimal_sign          "; break;
    case  DW_AT_digit_count:              section_header_type_str = "DW_AT_digit_count           "; break;
    case  DW_AT_picture_string:              section_header_type_str = "DW_AT_picture_string        "; break;
    case  DW_AT_mutable:              section_header_type_str = "DW_AT_mutable               "; break;
    case  DW_AT_threads_scaled:              section_header_type_str = "DW_AT_threads_scaled        "; break;
    case  DW_AT_explicit:              section_header_type_str = "DW_AT_explicit"; break;
    case  DW_AT_object_pointer:              section_header_type_str = "DW_AT_object_pointer        "; break;
    case  DW_AT_endianity:              section_header_type_str = "DW_AT_endianity             "; break;
    case  DW_AT_elemental:              section_header_type_str = "DW_AT_elemental             "; break;
    case  DW_AT_pure:              section_header_type_str = "DW_AT_pure                  "; break;
    case  DW_AT_recursive:              section_header_type_str = "DW_AT_recursive             "; break;
    case  DW_AT_signature:              section_header_type_str = "DW_AT_signature             "; break;
    case  DW_AT_main_subprogram:              section_header_type_str = "DW_AT_main_subprogram       "; break;
    case  DW_AT_data_bit_offset:              section_header_type_str = "DW_AT_data_bit_offset       "; break;
    case  DW_AT_const_expr:              section_header_type_str = "DW_AT_const_expr            "; break;
    case  DW_AT_enum_class:              section_header_type_str = "DW_AT_enum_class            "; break;
    case  DW_AT_linkage_name:              section_header_type_str = "DW_AT_linkage_name          "; break;
    case  DW_AT_string_length_bit_size:              section_header_type_str = "DW_AT_string_length_bit_size"; break;
    case  DW_AT_string_length_byte_size:              section_header_type_str = "DW_AT_string_length_byte_siz"; break;
    case  DW_AT_rank:              section_header_type_str = "DW_AT_rank                  "; break;
    case  DW_AT_str_offsets_base:              section_header_type_str = "DW_AT_str_offsets_base      "; break;
    case  DW_AT_addr_base:              section_header_type_str = "DW_AT_addr_base             "; break;
    case  DW_AT_dwo_id:              section_header_type_str = "DW_AT_dwo_id                "; break;
    case  DW_AT_dwo_name:              section_header_type_str = "DW_AT_dwo_name              "; break;
    case  DW_AT_reference:              section_header_type_str = "DW_AT_reference             "; break;
    case  DW_AT_rvalue_reference:              section_header_type_str = "DW_AT_rvalue_reference      "; break;
    case  DW_AT_macros:              section_header_type_str = "DW_AT_macros                "; break;
    case  DW_AT_call_all_calls:              section_header_type_str = "DW_AT_call_all_calls        "; break;
    case  DW_AT_call_all_source_calls:              section_header_type_str = "DW_AT_call_all_source_calls "; break;
    case  DW_AT_call_all_tail_calls:              section_header_type_str = "DW_AT_call_all_tail_calls   "; break;
    case  DW_AT_call_return_pc:              section_header_type_str = "DW_AT_call_return_pc        "; break;
    case  DW_AT_call_value:              section_header_type_str = "DW_AT_call_value            "; break;
    case  DW_AT_call_origin:              section_header_type_str = "DW_AT_call_origin           "; break;
    case  DW_AT_call_parameter:              section_header_type_str = "DW_AT_call_parameter        "; break;
    case  DW_AT_call_pc:              section_header_type_str = "DW_AT_call_pc               "; break;
    case  DW_AT_call_tail_call:              section_header_type_str = "DW_AT_call_tail_call        "; break;
    case  DW_AT_call_target:              section_header_type_str = "DW_AT_call_target           "; break;
    case  DW_AT_call_target_clobbered:              section_header_type_str = "DW_AT_call_target_clobbered "; break;
    case  DW_AT_call_data_location:              section_header_type_str = "DW_AT_call_data_location    "; break;
    case  DW_AT_call_data_value:              section_header_type_str = "DW_AT_call_data_value       "; break;
    case  DW_AT_noreturn:              section_header_type_str = "DW_AT_noreturn              "; break;
    case  DW_AT_alignment:              section_header_type_str = "DW_AT_alignment             "; break;
    case  DW_AT_export_symbols:              section_header_type_str = "DW_AT_export_symbols        "; break;
    case  DW_AT_deleted:              section_header_type_str = "DW_AT_deleted               "; break;
    case  DW_AT_defaulted:              section_header_type_str = "DW_AT_defaulted             "; break;
    case  DW_AT_loclists_base:              section_header_type_str = "DW_AT_loclists_base         "; break;
    default:        section_header_type_str = "Invalid DW_AT: " + std::to_string((int)StrCode); break;
    }

    return section_header_type_str;
}

std::string FileBin_DWARF::FileBin_DWARF_DW_FORM_ToString(uint16_t StrCode)
{
    std::string section_header_type_str;

    switch (StrCode)
    {
    case DW_FORM_addr           : section_header_type_str = "DW_FORM_addr";             break;
    case DW_FORM_block2         : section_header_type_str = "DW_FORM_block2";           break;
    case DW_FORM_block4         : section_header_type_str = "DW_FORM_block4";           break;
    case DW_FORM_data2          : section_header_type_str = "DW_FORM_data2";            break;
    case DW_FORM_data4          : section_header_type_str = "DW_FORM_data4";            break;
    case DW_FORM_data8          : section_header_type_str = "DW_FORM_data8";            break;
    case DW_FORM_string         : section_header_type_str = "DW_FORM_string";           break;
    case DW_FORM_block          : section_header_type_str = "DW_FORM_block";            break;
    case DW_FORM_block1         : section_header_type_str = "DW_FORM_block1";           break;
    case DW_FORM_data1          : section_header_type_str = "DW_FORM_data1";            break;
    case DW_FORM_flag           : section_header_type_str = "DW_FORM_flag";             break;
    case DW_FORM_sdata          : section_header_type_str = "DW_FORM_sdata";            break;
    case DW_FORM_strp           : section_header_type_str = "DW_FORM_strp";             break;
    case DW_FORM_udata          : section_header_type_str = "DW_FORM_udata";            break;
    case DW_FORM_ref_addr       : section_header_type_str = "DW_FORM_ref_addr";         break;
    case DW_FORM_ref1           : section_header_type_str = "DW_FORM_ref1";             break;
    case DW_FORM_ref2           : section_header_type_str = "DW_FORM_ref2";             break;
    case DW_FORM_ref4           : section_header_type_str = "DW_FORM_ref4";             break;
    case DW_FORM_ref8           : section_header_type_str = "DW_FORM_ref8";             break;
    case DW_FORM_ref_udata      : section_header_type_str = "DW_FORM_ref_udata";        break;
    case DW_FORM_indirect       : section_header_type_str = "DW_FORM_indirect";         break;
    case DW_FORM_sec_offset     : section_header_type_str = "DW_FORM_sec_offset";       break;
    case DW_FORM_exprloc        : section_header_type_str = "DW_FORM_exprloc";          break;
    case DW_FORM_flag_present   : section_header_type_str = "DW_FORM_flag_present";     break;
    case DW_FORM_strx           : section_header_type_str = "DW_FORM_strx";             break;
    case DW_FORM_addrx          : section_header_type_str = "DW_FORM_addrx";            break;
    case DW_FORM_ref_sup4       : section_header_type_str = "DW_FORM_ref_sup4";         break;
    case DW_FORM_strp_sup       : section_header_type_str = "DW_FORM_strp_sup";         break;
    case DW_FORM_data16         : section_header_type_str = "DW_FORM_data16";           break;
    case DW_FORM_line_strp      : section_header_type_str = "DW_FORM_line_strp";        break;
    case DW_FORM_ref_sig8       : section_header_type_str = "DW_FORM_ref_sig8";         break;
    case DW_FORM_implicit_const : section_header_type_str = "DW_FORM_implicit_const";   break;
    case DW_FORM_loclistx       : section_header_type_str = "DW_FORM_loclistx";         break;
    case DW_FORM_rnglistx       : section_header_type_str = "DW_FORM_rnglistx";         break;
    case DW_FORM_ref_sup8       : section_header_type_str = "DW_FORM_ref_sup8";         break;
    case DW_FORM_strx1          : section_header_type_str = "DW_FORM_strx1";            break;
    case DW_FORM_strx2          : section_header_type_str = "DW_FORM_strx2";            break;
    case DW_FORM_strx3          : section_header_type_str = "DW_FORM_strx3";            break;
    case DW_FORM_strx4          : section_header_type_str = "DW_FORM_strx4";            break;
    case DW_FORM_addrx1         : section_header_type_str = "DW_FORM_addrx1";           break;
    case DW_FORM_addrx2         : section_header_type_str = "DW_FORM_addrx2";           break;
    case DW_FORM_addrx3         : section_header_type_str = "DW_FORM_addrx3";           break;
    case DW_FORM_addrx4         : section_header_type_str = "DW_FORM_addrx4";           break;
    default                     : section_header_type_str = "Invalid DW_FORM";          break;
    }

    return section_header_type_str;
}

inline uint64_t FileBin_DWARF::FileBin_DWARF_ReadULEB128(const uint8_t*& ptr)
{
    uint64_t result = 0;
    unsigned shift = 0;

    while (true)
    {
        uint8_t byte = *ptr++;
        result |= uint64_t(byte & 0x7F) << shift;

        if ((byte & 0x80) == 0)
            return result;

        shift += 7;
        if (shift >= 64)
            throw std::runtime_error("ULEB128 too large");
    }
}

inline int64_t FileBin_DWARF::FileBin_DWARF_ReadSLEB128(
    const uint8_t*& ptr)
{
    int64_t result = 0;
    int shift = 0;

    while (true)
    {
        uint8_t byte = *ptr++;
        result |= int64_t(byte & 0x7F) << shift;
        shift += 7;

        if (!(byte & 0x80))
        {
            if (shift < 64 && (byte & 0x40))
                result |= -((int64_t)1 << shift);
            return result;
        }
    }
}

FileBin_DWARF_CompileUnitDataType* FileBin_DWARF::ParseAbbrevOffset(const uint8_t* abbrevPtr)
{
    uint32_t abbrevOffset = static_cast<uint32_t>(abbrevPtr - this->fileBase); // fileBase = mmap base

    /* The Abbrev are stored using their offset in the file as unique identifier (will be user later for .debug_info structure
     * info retrieval). Each abbrev will store an array of DW_TAG number that will include a set of DW_AT + DW_FORM */
    std::unordered_map<uint32_t, FileBin_DWARF_CompileUnitDataType>::iterator it = AbbrevOffsetCache.find(abbrevOffset);

    /* If abbrev offset already parsed, skip it */
    if (it != AbbrevOffsetCache.end())
    {
        std::cout << "already parsed" << std::endl;
        return &it->second;
    }

    FileBin_DWARF_CompileUnitDataType cu;
    cu.abbrevOffset = abbrevOffset;

    const uint8_t* ptr = abbrevPtr;

    while (true)
    {
        uint64_t code = FileBin_DWARF_ReadULEB128(ptr);
        if (code == 0)
        {
            break; // end of abbrev table
        }

        /* Array of DW_TAG */
        FileBin_DWARF_Abbrev abbrev;
        abbrev.code = static_cast<uint32_t>(code);
        abbrev.tag = static_cast<uint32_t>(FileBin_DWARF_ReadULEB128(ptr));
        abbrev.hasChildren = (*ptr++ != 0);

#if (1 == LIBPARSER_DWARF_DEBUG)
        std::cout << "Abbrev " << abbrev.code << "\n";
#endif

        while (true)
        {
            uint64_t attr = FileBin_DWARF_ReadULEB128(ptr);
            uint64_t form = FileBin_DWARF_ReadULEB128(ptr);

            if (attr == 0 && form == 0)
            {
                break;
            }      
#if (1 == LIBPARSER_DWARF_DEBUG)
            std::cout << "  Attr " << FileBin_DWARF_DW_AT_ToString(attr) << "\n";
#endif
            abbrev.attributes.push_back({static_cast<uint32_t>(attr), static_cast<uint32_t>(form)});
        }

        cu.abbrevTable.emplace(abbrev.code, std::move(abbrev));
    }

    // Cache and return
    auto [insIt, _] = AbbrevOffsetCache.emplace(abbrevOffset, std::move(cu));

    return &insIt->second;
}

void FileBin_DWARF::PrintAllAbbrevInfo() const
{
    for (const auto& cachePair : AbbrevOffsetCache)
    {
        uint32_t abbrevOffset = cachePair.first;
        const FileBin_DWARF_CompileUnitDataType& cu = cachePair.second;

        std::cout << "Abbrev Table at Offset: 0x" << std::hex << abbrevOffset << std::dec << "\n";

        for (const auto& pair : cu.abbrevTable)
        {
            const FileBin_DWARF_Abbrev& abbrev = pair.second;
            std::cout << "  Abbrev Code: " << abbrev.code
                      << " Tag: " << FileBin_DWARF_DW_TAG_ToString(abbrev.tag)
                      << " HasChildren: " << abbrev.hasChildren << "\n";

            for (const FileBin_DWARF_AbbrevAttr& attrForm : abbrev.attributes)
            {
                std::cout << "    Attribute: " << FileBin_DWARF_DW_AT_ToString(attrForm.attribute)
                << " Form: " << FileBin_DWARF_DW_FORM_ToString(attrForm.form) << "\n";
            }
        }
    }
}

static uint16_t readU16(const uint8_t*& p)
{
    uint16_t v = p[0] | (p[1] << 8);
    p += 2;
    return v;
}

static uint32_t readU32(const uint8_t*& p)
{
    uint32_t v =
        (uint32_t)p[0] |
        ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) |
        ((uint32_t)p[3] << 24);
    p += 4;
    return v;
}

inline uint64_t readU64(const uint8_t*& ptr)
{
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i)
    {
        val |= static_cast<uint64_t>(*ptr++) << (i * 8);
    }
    return val;
}

std::vector<uint8_t> FileBin_DWARF::ReadAttributeValue(const uint8_t*& ptr, uint32_t form, uint8_t addrSize, const uint8_t* fileBase) // base of file (for .strp strings)
{
    std::vector<uint8_t> data;

    switch (form)
    {
        case DW_FORM_string:
        {
            // null-terminated string in place
            const uint8_t* start = ptr;
            while (*ptr) ++ptr;
            data.insert(data.end(), start, ptr);
            ++ptr; // skip null terminator
            break;
        }

        case DW_FORM_block1:
        {
            uint8_t blockLen = *ptr++;
            data.insert(data.end(), ptr, ptr + blockLen);
            ptr += blockLen;
            break;
        }

        case DW_FORM_block2:
        {
            uint16_t blockLen = readU16(ptr);
            data.insert(data.end(), ptr, ptr + blockLen);
            ptr += blockLen;
            break;
        }

        case DW_FORM_strp:
        {
            uint32_t strOffset = readU32(ptr);
            const uint8_t* strStart = fileBase + StrOffset + strOffset;
            const uint8_t* strEnd = strStart;
            while (*strEnd) ++strEnd;
            data.insert(data.end(), strStart, strEnd);
            break;
        }

        case DW_FORM_data1:
        {
            data.push_back(*ptr++);
            break;
        }

        case DW_FORM_data2:
        {
            uint16_t v = readU16(ptr);
            data.resize(2);
            std::memcpy(data.data(), &v, 2);
            break;
        }

        case DW_FORM_data4:
        case DW_FORM_ref4:
        {
            uint32_t v = readU32(ptr);
            data.resize(4);
            std::memcpy(data.data(), &v, 4);
            break;
        }

        case DW_FORM_data8:
        case DW_FORM_ref8:
        {
            uint64_t v = readU64(ptr);
            data.resize(8);
            std::memcpy(data.data(), &v, 8);
            break;
        }

        case DW_FORM_addr:
        {
            if (addrSize == 8)
            {
                uint64_t v = readU64(ptr);
                data.resize(8);
                std::memcpy(data.data(), &v, 8);
            }
            else
            {
                uint32_t v = readU32(ptr);
                data.resize(4);
                std::memcpy(data.data(), &v, 4);
            }
            break;
        }

        case DW_FORM_udata:
        {
            uint64_t v = FileBin_DWARF_ReadULEB128(ptr);
            do {
                data.push_back(static_cast<uint8_t>(v & 0xFF));
                v >>= 8;
            } while (v);
            break;
        }

        case DW_FORM_sdata:
        {
            int64_t v = FileBin_DWARF_ReadSLEB128(ptr);
            data.resize(sizeof(v));
            std::memcpy(data.data(), &v, sizeof(v));
            break;
        }

        case DW_FORM_sec_offset:
        {
            uint32_t v = readU32(ptr);
            data = { static_cast<uint8_t>(v & 0xFF),
                    static_cast<uint8_t>((v >> 8) & 0xFF),
                    static_cast<uint8_t>((v >> 16) & 0xFF),
                    static_cast<uint8_t>((v >> 24) & 0xFF) };
            break;
        }

        case DW_FORM_exprloc:
        {
            uint64_t len = FileBin_DWARF_ReadULEB128(ptr);
            data.insert(data.end(), ptr, ptr + len);
            ptr += len;
            break;
        }

        case DW_FORM_ref_udata:
        {
            uint64_t len = FileBin_DWARF_ReadULEB128(ptr);
            //data.insert(data.end(), ptr, ptr + len);
            //ptr += len;
        }

        case DW_FORM_flag:
        {
            data.push_back(*ptr++);
            break;
        }

        case DW_FORM_flag_present:
        {
            data.push_back(1);
            break;
        }

        default:
        {
            std::cerr << "[ERROR] Unsupported DW_FORM: " << form << "\n";
            break;
        }
    }

    return data;
}

TreeElementType* FileBin_DWARF::ParseDIE(const uint8_t*& ptr, const uint8_t* fileBase, uint32_t cuOffset, uint32_t infoLen, FileBin_DWARF_CompileUnitType* cu, TreeElementType* parent)
{
    TreeElementType* prev = nullptr;
    const uint8_t* sectionEnd = fileBase + cuOffset + infoLen;
    uint32_t CurrentAbbrevOffset2;

    while (ptr < sectionEnd)
    {
        const uint8_t* dieStart = ptr;
        // Set CurrentAbbrevOffset to the offset of this DIE relative to the CU start
        CurrentAbbrevOffset2 = static_cast<uint32_t>(dieStart - fileBase);

        uint64_t abbrevCode = FileBin_DWARF_ReadULEB128(ptr);
        if (abbrevCode == 0)
            break; // end of siblings

        auto it = cu->AbbrevInfo->abbrevTable.find(static_cast<uint32_t>(abbrevCode));
        if (it == cu->AbbrevInfo->abbrevTable.end())
        {
            std::cerr << "[WARNING] Abbrev code " << abbrevCode << " not found\n";
            break;
        }

        FileBin_DWARF_Abbrev& abbrev = it->second;

        TreeElementType* node = new TreeElementType();
        node->cu = cu;

        // Parse attributes
        for (const FileBin_DWARF_AbbrevAttr& attrForm : abbrev.attributes)
        {
            std::vector<uint8_t> data;
            //int16_t attrSize = FileBin_DWARF::DW_FORM_GetLength(attrForm.form);
#if (1 == LIBPARSER_DWARF_DEBUG)
            std::cout << "  Attr: " << FileBin_DWARF_DW_AT_ToString(attrForm.attribute)
                      << " Form: " << FileBin_DWARF_DW_FORM_ToString(attrForm.form)
                      << std::endl;
#endif
            data = ReadAttributeValue(ptr, attrForm.form, cu->AddrSize, fileBase);

            switch (abbrev.tag)
            {

            /* Compile Unit information stores at a different class than the rest of attributes */
            case DW_TAG_compile_unit:
            {
                node->elementType = FILEBIN_DWARF_ELEMENT_COMPILE_UNIT;

                switch (attrForm.attribute)
                {
                    case DW_AT_name:
                    {
                        node->data = data;
                        break;
                    }
                }

                break;

            }

            case DW_TAG_enumeration_type:
            {
                node->elementType = FILEBIN_DWARF_ELEMENT_ENUMERATION;

                uint32_t typeOffset = CurrentAbbrevOffset2 - cu->Offset - InfoOffset;

                auto it = cu->typeList.find(typeOffset);
                if (it == cu->typeList.end())
                {
                    cu->typeList.emplace(typeOffset, node);
                }

                switch (attrForm.attribute)
                {
                    case DW_AT_name:
                    {
                        node->data = data;
                        break;
                    }

                    case DW_AT_byte_size:
                    {
                        node->Size.push_back(data[0]);
                        break;
                    }
                }
                break;
            }

            case DW_TAG_enumerator:
            {
                node->elementType = FILEBIN_DWARF_ELEMENT_ENUMERATION;

                switch (attrForm.attribute)
                {
                    case DW_AT_name:
                    {
                        node->data = data;
                        break;
                    }
                }
                break;
            }

            case DW_TAG_volatile_type:
            {
                node->elementType = FILEBIN_DWARF_ELEMENT_VOLATILE;

                uint32_t typeOffset = CurrentAbbrevOffset2 - cu->Offset - InfoOffset;

                auto it = cu->typeList.find(typeOffset);
                if (it == cu->typeList.end())
                {
                    cu->typeList.emplace(typeOffset, node);
                }

                switch (attrForm.attribute)
                {
                    case DW_AT_type:
                    {
                        node->typeOffset = 0;
                        for (size_t i = 0; i < data.size(); ++i)
                            node->typeOffset += static_cast<uint32_t>(data[i]) << (i * 8);
                        break;
                    }
                }
                break;
            }

            case DW_TAG_array_type:
            {
                node->elementType = FILEBIN_DWARF_ELEMENT_ARRAY;
                uint32_t typeOffset = CurrentAbbrevOffset2 - cu->Offset - InfoOffset;// - CU_HEADER_SIZE;

                // std::cout << "ARRAY OFF " << typeOffset << std::endl;

                auto it = cu->typeList.find(typeOffset);
                if (it != cu->typeList.end())
                {
                    //typeNode = it->second;  // reuse already parsed type
                } else {
                    cu->typeList.emplace(typeOffset, node); // reserve slot
                }

                switch (attrForm.attribute)
                {
                    case DW_AT_type:
                    {
                        node->typeOffset = 0;
                        for (size_t i = 0; i < data.size(); ++i)
                            node->typeOffset += static_cast<uint32_t>(data[i]) << (i * 8);
                        break;
                    }
                }
                break;
            }

            case DW_TAG_subrange_type:
            {
                node->elementType = FILEBIN_DWARF_ELEMENT_ARRAY_DIM;

                switch (attrForm.attribute)
                {
                    case DW_AT_type:
                    {
                        node->typeOffset = 0;
                        for (size_t i = 0; i < data.size(); ++i)
                            node->typeOffset += static_cast<uint32_t>(data[i]) << (i * 8);
                        break;
                    }
                    case DW_AT_count:
                    {
                        uint32_t Size = 0;

                        for (size_t i = 0; i < data.size(); ++i)
                            Size += static_cast<uint32_t>(data[i]) << (i * 8);

                        // std::cout << "dim coun t "<< (int)Size << std::endl;

                        node->Size.push_back(Size);
                        break;
                    }
                    case DW_AT_upper_bound:
                    {
                        uint32_t val = 0;
                        for (size_t i = 0; i < data.size(); ++i)
                            val += static_cast<uint32_t>(data[i]) << (i * 8);

                        // Convert Upper Bound to Count
                        node->Size.push_back(val + 1);
                        break;
                    }
                }
                break;
            }

            case DW_TAG_typedef:
            {
                node->elementType = FILEBIN_DWARF_ELEMENT_TYPEDEF;

                uint32_t typeOffset = CurrentAbbrevOffset2 - cu->Offset - InfoOffset;

                auto it = cu->typeList.find(typeOffset);
                if (it == cu->typeList.end())
                {
                    cu->typeList.emplace(typeOffset, node);
                }

                switch (attrForm.attribute)
                {
                    case DW_AT_type:
                    {
                        node->typeOffset = 0;
                        for (size_t i = 0; i < data.size(); ++i)
                            node->typeOffset += static_cast<uint32_t>(data[i]) << (i * 8);
                        break;
                    }
                }
                break;
            }

            case DW_TAG_base_type:
            {
                node->elementType = FILEBIN_DWARF_ELEMENT_BASE_TYPE;

                uint32_t typeOffset = CurrentAbbrevOffset2 - cu->Offset - InfoOffset;

                auto it = cu->typeList.find(typeOffset);
                if (it == cu->typeList.end())
                {
                    cu->typeList.emplace(typeOffset, node);
                }

                switch (attrForm.attribute)
                {
                    case DW_AT_name:
                    {
                        node->data = data;
                        break;
                    }

                    case DW_AT_byte_size:
                    {
                        node->Size.push_back(data[0]);
                        break;
                    }
                }
                break;
            }

            case DW_TAG_structure_type:
            {
                node->elementType = FILEBIN_DWARF_ELEMENT_STRUCTURE;

                uint32_t typeOffset = CurrentAbbrevOffset2 - cu->Offset - InfoOffset;

                auto it = cu->typeList.find(typeOffset);
                if (it == cu->typeList.end())
                {
                    cu->typeList.emplace(typeOffset, node);
                }

                switch (attrForm.attribute)
                {
                    case DW_AT_type:
                    {
                        node->typeOffset = 0;
                        for (size_t i = 0; i < data.size(); ++i)
                            node->typeOffset += static_cast<uint32_t>(data[i]) << (i * 8);
                        break;
                    }
                    case DW_AT_byte_size:
                    {
                        uint32_t Size = 0;

                        for (size_t i = 0; i < data.size(); ++i)
                            Size += static_cast<uint32_t>(data[i]) << (i * 8);

                        // std::cout << "dim coun t "<< (int)Size << std::endl;

                        node->Size.push_back(Size);
                    }
                }
                break;
            }

            case DW_TAG_member:
            {
                node->elementType = FILEBIN_DWARF_ELEMENT_MEMBER;

                uint32_t typeOffset = CurrentAbbrevOffset2 - cu->Offset - InfoOffset;

                auto it = cu->typeList.find(typeOffset);
                if (it == cu->typeList.end())
                {
                    cu->typeList.emplace(typeOffset, node);
                }

                switch (attrForm.attribute)
                {
                    case DW_AT_name:
                    {
                        node->data = data;
                        break;
                    }
                    case DW_AT_type:
                    {
                        node->typeOffset = 0;
                        for (size_t i = 0; i < data.size(); ++i)
                            node->typeOffset += static_cast<uint32_t>(data[i]) << (i * 8);
                        break;
                    }
                    case DW_AT_data_member_location:
                    {
                        node->Location = 0;

                        for (size_t i = 0; i < data.size(); ++i)
                            node->Location += static_cast<uint32_t>(data[i]) << (i * 8);
                        break;
                    }
                }
                break;
            }

            case DW_TAG_variable:
            {
                node->elementType = FILEBIN_DWARF_ELEMENT_VARIABLE;

                switch (attrForm.attribute)
                {
                    case DW_AT_name:
                    {
                        node->data = data;

                        break;
                    }
                    case DW_AT_type:
                    {
                        node->typeOffset = 0;
                        for (size_t i = 0; i < data.size(); ++i)
                            node->typeOffset += static_cast<uint32_t>(data[i]) << (i * 8);
                        break;
                    }
                    case DW_AT_location:
                    {
                        node->Addr = 0;
                        for (size_t i = 1; i < data.size(); ++i)
                        {
                            node->Addr += static_cast<uint32_t>(data[i]) << ((i - 1) * 8);
                        }
                        break;
                    }
                    case DW_AT_declaration:
                    {
                        node->isDeclaration = true;
                        uint32_t dieOffset = CurrentAbbrevOffset2 - cu->Offset - InfoOffset;
                        cu->varDeclaration.emplace(dieOffset, node);
                        break;
                    }

                    case DW_AT_specification:
                    {
                        // If this is a definition, DW_AT_specification points to the declaration
                        uint32_t specOffset = 0;
                        for (size_t i = 0; i < data.size(); ++i)
                        {
                            specOffset |= static_cast<uint32_t>(data[i]) << (8 * i);
                        }

                        auto it = cu->varDeclaration.find(specOffset);

                        if (it != cu->varDeclaration.end())
                        {
                            TreeElementType* declNode = it->second;

                            // Copy relevant info from declaration
                            if (node->data.empty())
                                node->data = declNode->data;

                            if (node->typeOffset == 0)
                                node->typeOffset = declNode->typeOffset;

                            node->Addr = declNode->Addr;
                        }
                        break;
                    }
                }
                break;
            }

            case DW_TAG_const_type:
            {
                node->elementType = FILEBIN_DWARF_ELEMENT_CONSTANT;
                uint32_t typeOffset = CurrentAbbrevOffset2 - cu->Offset - InfoOffset;

                auto it = cu->typeList.find(typeOffset);
                if (it == cu->typeList.end())
                {
                    cu->typeList.emplace(typeOffset, node);
                }

                switch (attrForm.attribute)
                {
                    case DW_AT_type:
                    {
                        node->typeOffset = 0;
                        for (size_t i = 0; i < data.size(); ++i)
                            node->typeOffset += static_cast<uint32_t>(data[i]) << (i * 8);
                        break;
                    }
                }
                break;
            }
            default:
            {
                //std::cerr << "Unknown DW_TAG " << (int)abbrev.tag << std::endl;
            }
            }
        }

        // Recurse into children
        if (abbrev.hasChildren)
            node->child = ParseDIE(ptr, fileBase, cuOffset, infoLen, cu, node);

        // Sibling linkage
        if (prev)
        {
            prev->next = node;
        }
        else if (parent)
        {
            parent->child = node;
        }

        prev = node;
    }

    return parent ? parent->child : prev;
}

uint8_t FileBin_DWARF::SymbolResolveType(TreeElementType* node, FileBin_DWARF_VarInfoType* parent)
{
    FileBin_DWARF_VarInfoType* newVar = nullptr;
    uint8_t symbolSize = 0;
    uint32_t elementSize = 0;

    if (node->elementType == FILEBIN_DWARF_ELEMENT_BASE_TYPE)
    {
        newVar = new FileBin_DWARF_VarInfoType();
        newVar->data = node->data;
        newVar->isQualifier = true;
        newVar->Addr = parent->Addr;

        std::string str(newVar->data.begin(), newVar->data.end());

        if (str == "unsigned char" || str == "char")
        {
            newVar->DataType = FileBin_VARINFO_TYPE_UINT8;
        }
        else if (str == "signed char")
        {
            newVar->DataType = FileBin_VARINFO_TYPE_SINT8;
        }
        else if (str == "unsigned short" || str == "short unsigned int")
        {
            newVar->DataType = FileBin_VARINFO_TYPE_UINT16;
        }
        else if (str == "short" || str == "short int")
        {
            newVar->DataType = FileBin_VARINFO_TYPE_SINT16;
        }
        else if (str == "unsigned long" || str == "long unsigned int" || str == "unsigned int")
        {
            newVar->DataType = FileBin_VARINFO_TYPE_UINT32;
        }
        else if (str == "long" || str == "long int" || str == "int")
        {
            newVar->DataType = FileBin_VARINFO_TYPE_SINT32;
        }
        else if (str == "unsigned long long" || str == "long long unsigned int")
        {
            newVar->DataType = FileBin_VARINFO_TYPE_UINT64;
        }
        else if (str == "long long" || str == "long long int")
        {
            newVar->DataType = FileBin_VARINFO_TYPE_SINT64;
        }
        else if (str == "float")
        {
            newVar->DataType = FileBin_VARINFO_TYPE_FLOAT32;
        }
        else if (str == "double")
        {
            newVar->DataType = FileBin_VARINFO_TYPE_FLOAT64;
        }
        else
        {
            std::cerr << "Unknown type " + str + "\n";
            newVar->DataType = FileBin_VARINFO_TYPE__UNKNOWN;
        }

        /* Propagate type */
        parent->DataType = newVar->DataType;

        if (node->Size.size() > 0)
        {
            symbolSize = node->Size.at(0);
            newVar->Size.push_back(symbolSize);
            parent->Size = newVar->Size;
        }
        else
        {
            std::cerr << "Type with no size\n";
        }

        if (!parent->child)
        {
            parent->child = newVar;
        }
        else
        {
            FileBin_DWARF_VarInfoType* last = parent->child;
            while (last->next) last = last->next;
            last->next = newVar;
        }
    }
    else if (node->elementType == FILEBIN_DWARF_ELEMENT_CONSTANT)
    {
        newVar = new FileBin_DWARF_VarInfoType();
        newVar->data = {'C', 'O', 'N'};
        newVar->TypeOffset = node->typeOffset;
        newVar->isQualifier = true;
        newVar->Addr = parent->Addr;

        // Append to parent's child list
        if (!parent->child)
        {
            parent->child = newVar;
        }
        else
        {
            FileBin_DWARF_VarInfoType* last = parent->child;
            while (last->next) last = last->next;
            last->next = newVar;
        }

        /* Propagate type */
        //parent->DataType = newVar->DataType;

        // Lookup type in CU typeList
        if ((node->cu)&&(newVar->TypeOffset != 0))
        {
            auto it = node->cu->typeList.find(node->typeOffset);
            if (it != node->cu->typeList.end())
            {
                TreeElementType* typeNode = it->second;
                // Recursively traverse the type tree and attach as child
                symbolSize += SymbolResolveType(typeNode, newVar);
                // Propagate dimension definition
                /* Propagate type */
                parent->DataType = newVar->DataType;
                parent->Size = newVar->Size;
                //newVar->Size.push_back(symbolSize);
            }
        }
    }
    else if (node->elementType == FILEBIN_DWARF_ELEMENT_VOLATILE)
    {
        newVar = new FileBin_DWARF_VarInfoType();
        newVar->data = {'V', 'O', 'L', 'A', 'T', 'I', 'L', 'E'};
        newVar->TypeOffset = node->typeOffset;
        newVar->isQualifier = true;
        newVar->Addr = parent->Addr;
        // Append to parent's child list
        if (!parent->child)
        {
            parent->child = newVar;
        }
        else
        {
            FileBin_DWARF_VarInfoType* last = parent->child;
            while (last->next) last = last->next;
            last->next = newVar;
        }

        // Lookup type in CU typeList
        if (node->cu && (newVar->TypeOffset != 0))
        {
            auto it = node->cu->typeList.find(node->typeOffset);
            if (it != node->cu->typeList.end())
            {
                // Recursively traverse the type tree and attach as child
                symbolSize += SymbolResolveType(it->second, newVar);

                /* Propagate type */
                parent->DataType = newVar->DataType;
                parent->Size = newVar->Size;
            }
        }
    }
    else if (node->elementType == FILEBIN_DWARF_ELEMENT_TYPEDEF)
    {
        newVar = new FileBin_DWARF_VarInfoType();
        newVar->data = {'T', 'I', 'P'};
        newVar->TypeOffset = node->typeOffset;
        newVar->isQualifier = true;
        newVar->Addr = parent->Addr;

        // Append to parent's child list
        if (!parent->child)
        {
            parent->child = newVar;
        }
        else
        {
            FileBin_DWARF_VarInfoType* last = parent->child;
            while (last->next) last = last->next;
            last->next = newVar;
        }

        // Lookup type in CU typeList
        if ((node->cu)&&(newVar->TypeOffset != 0))
        {
            auto it = node->cu->typeList.find(node->typeOffset);
            if (it != node->cu->typeList.end())
            {
                TreeElementType* typeNode = it->second;
                symbolSize += SymbolResolveType(typeNode, newVar);
                /* Propagate type */
                parent->DataType = newVar->DataType;
                parent->Size = newVar->Size;
            }
        }
    }
    else if (node->elementType == FILEBIN_DWARF_ELEMENT_STRUCTURE)
    {
        uint8_t structTotalSize = 0;
        newVar = new FileBin_DWARF_VarInfoType();
        newVar->data = {'S', 'T', 'R'};
        newVar->TypeOffset = node->typeOffset;
        newVar->isQualifier = true;
        newVar->Addr = parent->Addr;

        // Append to parent's child list
        if (!parent->child)
        {
            parent->child = newVar;
        }

        TreeElementType* nodeMember = node->child;
        FileBin_DWARF_VarInfoType* newNodeMember = parent->child;

        // Only create child if nodeMember exists
        if (nodeMember != nullptr)
        {
            newNodeMember->child = new FileBin_DWARF_VarInfoType();
            newNodeMember = newNodeMember->child;
        }

        while (nodeMember != nullptr)
        {
            newNodeMember->data = nodeMember->data;

            if ((nodeMember->cu) && (nodeMember->typeOffset != 0))
            {
                auto it = nodeMember->cu->typeList.find(nodeMember->typeOffset);
                if (it != nodeMember->cu->typeList.end())
                {
                    TreeElementType* typeNode = it->second;
                    newNodeMember->Addr = parent->Addr + nodeMember->Location;
                    elementSize = SymbolResolveType(typeNode, newNodeMember);
                    symbolSize += elementSize;
                }
            }


            // Only create next node if there is actually a next node
            if (nodeMember->next != nullptr)
            {
                newNodeMember->next = new FileBin_DWARF_VarInfoType();
                newNodeMember = newNodeMember->next;
            }

            nodeMember = nodeMember->next;
        }

        //newVar->Size.push_back(symbolSize);
        newVar->Size = node->Size;
        symbolSize = node->Size.at(0);
        parent->Size = node->Size;

    }
    else if (node->elementType == FILEBIN_DWARF_ELEMENT_ENUMERATION)
    {
        newVar = new FileBin_DWARF_VarInfoType();
        newVar->data = {'E', 'N', 'U'};
        newVar->TypeOffset = node->typeOffset;
        newVar->Addr = parent->Addr;
        newVar->isQualifier = true;

        parent->DataType = FileBin_VARINFO_TYPE_ENUM;

        // Append to parent's child list
        if (!parent->child)
        {
            parent->child = newVar;
        }

        TreeElementType* nodeMember = node->child;
        FileBin_DWARF_VarInfoType* newNodeMember =  parent->child;

        newNodeMember->child = new FileBin_DWARF_VarInfoType();
        newNodeMember = newNodeMember->child;

        while (nodeMember != nullptr)
        {
            newNodeMember->data = nodeMember->data;
            newNodeMember->isQualifier = true;

            if (nodeMember->next != nullptr)
            {
                newNodeMember->next = new FileBin_DWARF_VarInfoType();
                newNodeMember = newNodeMember->next;
            }

            nodeMember = nodeMember->next;
        }
        if (node->Size.size() > 0)
        {
            parent->Size.push_back(node->Size.at(0));
        }
    }
    else if (node->elementType == FILEBIN_DWARF_ELEMENT_ARRAY)
    {
        newVar = new FileBin_DWARF_VarInfoType();
        newVar->data = {'A', 'R', 'R'};
        newVar->TypeOffset = node->typeOffset;
        newVar->Addr = parent->Addr;
        newVar->isQualifier = true;

        // Append to parent's child list
        if (!parent->child)
        {
            parent->child = newVar;
        }

        TreeElementType* nodeMember = node->child;

        while (nodeMember != nullptr)
        {
            if (nodeMember->Size.size() > 0)
            {
                newVar->Size.push_back(nodeMember->Size.at(0));
                parent->Size.push_back(nodeMember->Size.at(0));
            }
            nodeMember = nodeMember->next;
        }

        // Lookup type in CU typeList
        if ((node->cu)&&(newVar->TypeOffset != 0))
        {
            auto it = node->cu->typeList.find(node->typeOffset);
            if (it != node->cu->typeList.end())
            {
                TreeElementType* typeNode = it->second;
                symbolSize += SymbolResolveType(typeNode, newVar);
                newVar->Size.push_back(symbolSize);
                parent->Size.push_back(symbolSize);

                /* Propagate type */
                parent->DataType = newVar->DataType;
            }
        }
    }
    else if (FILEBIN_DWARF_ELEMENT_ARRAY_DIM == node->elementType)
    {
        newVar = new FileBin_DWARF_VarInfoType();
        newVar->data = {'D', 'I', 'M', 'A', 'Y'};
        newVar->TypeOffset = node->typeOffset;
        newVar->isQualifier = true;
        newVar->Addr = parent->Addr;

        std::cout << "DIMARRAY" << std::endl;

        // Append to parent's child list
        if (!parent->child)
        {
            parent->child = newVar;
        }
        else
        {
            FileBin_DWARF_VarInfoType* last = parent->child;
            while (last->next) last = last->next;
            last->next = newVar;
        }


        // Lookup type in CU typeList
        if ((node->cu)&&(newVar->TypeOffset != 0))
        {
            auto it = node->cu->typeList.find(node->typeOffset);
            if (it != node->cu->typeList.end())
            {
                TreeElementType* typeNode = it->second;
                // Recursively traverse the type tree and attach as child
                symbolSize += SymbolResolveType(typeNode, newVar);
                newVar->Size.push_back(symbolSize);
            }
        }
    }
    return symbolSize;
}

void FileBin_DWARF::SymbolTraverse(TreeElementType* node, FileBin_DWARF_VarInfoType* parent)
{

    while (node != nullptr)
    {

        FileBin_DWARF_VarInfoType* newVar = nullptr;

        // Only create a FileBin_VarInfoType node for actual variables that are also not just a declaration
        if ((node->elementType == FILEBIN_DWARF_ELEMENT_VARIABLE) && (!node->isDeclaration))
        {

            //std::cout << "Address: " << std::hex << node->Addr << std::endl;

            newVar = new FileBin_DWARF_VarInfoType();
            newVar->data = node->data;
            newVar->Addr = node->Addr;
            newVar->TypeOffset = node->typeOffset;

            // Append to parent's child list
            if (!parent->child)
            {
                parent->child = newVar;
            }
            else
            {
                FileBin_DWARF_VarInfoType* last = parent->child;
                while (last->next) last = last->next;
                last->next = newVar;
            }

            // Lookup type in CU typeList
            if (node->cu)
            {
                if (newVar->TypeOffset != 0)
                {
                    auto it = node->cu->typeList.find(node->typeOffset);
                    if (it != node->cu->typeList.end())
                    {
                        TreeElementType* typeNode = it->second;
                        // Recursively traverse the type tree and attach as child
                        SymbolResolveType(typeNode, newVar);
                    }
                }
                else
                {
                    std::cout << "Unable to resolve type: " << std::hex << node->Addr << std::endl;
                }
            }
        }
        // Move to next sibling
        node = node->next;
    }
}

void FileBin_DWARF::ParseAllAbbrvSectionHeader(const uint8_t* fileData, uint32_t AbbrevOffset, uint32_t InfoOffset, uint32_t InfoLen)
{
    const uint8_t* start = fileData + InfoOffset;
    const uint8_t* end   = start + InfoLen;

    uint32_t offset = 0;

    while (offset < InfoLen)
    {
        const uint8_t* ptr = start + offset;

        if (ptr + 4 > end)
            break;

        // Allocate new CU
        FileBin_DWARF_CompileUnitType* newCU = new FileBin_DWARF_CompileUnitType();

        // unit_length (does not include itself)
        uint32_t unitLength = readU32(ptr);
        if (unitLength == 0)
        {
            delete newCU;
            break;
        }

        newCU->Length_Bytes = unitLength;
        newCU->Offset       = offset;

        if (ptr + unitLength > end)
        {
            delete newCU;
            throw std::runtime_error("Truncated CU");
        }

        // DWARF version
        newCU->Version = readU16(ptr);

        if (newCU->Version <= 4)
        {
            // DWARF 2–4
            newCU->AbrevOffset = readU32(ptr);
            newCU->AddrSize    = *ptr++;
            newCU->UnitType    = DW_UT_compile;
            HeaderSize_Byte    = 11;
        }
        else if (newCU->Version == 5)
        {
            // DWARF 5
            newCU->UnitType    = *ptr++;
            newCU->AddrSize    = *ptr++;
            newCU->AbrevOffset = readU32(ptr);
            HeaderSize_Byte    = 12;
        }
        else
        {
            delete newCU;
            throw std::runtime_error("Unsupported DWARF version");
        }

#if (1 == LIBPARSER_DWARF_DEBUG)
        std::cout << "CU [" << CompilationUnit.size() << "] "
                  << "Length: " << newCU->Length_Bytes
                  << " Offset: 0x" << std::hex << newCU->Offset
                  << " (absolute 0x" << (InfoOffset + newCU->Offset) << ") "
                  << "AbbrevOffset=0x" << newCU->AbrevOffset
                  << " (absolute 0x" << (AbbrevOffset + newCU->AbrevOffset) << ")"
                  << std::dec << std::endl;
#endif
        // Append CU
        this->CompilationUnit.push_back(newCU);

        // Advance offset (unit_length + sizeof(unit_length))
        offset += newCU->Length_Bytes + 4;
    }
}

void FileBin_DWARF::FreeTree(TreeElementType* node)
{
    if (!node) return;
    FreeTree(node->child);
    FreeTree(node->next);
    delete node;
}

void FileBin_DWARF::FreeSymTree(FileBin_DWARF_VarInfoType* node)
{
    if (!node) return;
    FreeSymTree(node->child);
    FreeSymTree(node->next);
    delete node;
}

uint8_t FileBin_DWARF::Parse(std::string file_name, uint32_t Offset, uint32_t Len, uint32_t InfoOffset, uint32_t InfoLen, uint32_t StrOffset)
{
    // Open memory-mapped file
    MappedFile file;
    if (!file.open(file_name))
    {
        std::cout << "Failed to open file: " << file_name << "\n";
        return 1;
    }

    // Store offsets
    this->StrOffset = StrOffset;
    this->InfoOffset = InfoOffset;
    this->fileBase = file.data;

    FreeTree(this->DataRoot);
    this->DataRoot = nullptr;

    for (auto* cu : this->CompilationUnit)
    {
        delete cu;
    }
    this->CompilationUnit.clear();

    this->AbbrevOffsetCache.clear();

    FreeSymTree(this->SymbolRoot);
    this->SymbolRoot = nullptr;

    /* Identify and parse all compilation units headers (does not go deeper into parsing) */
    this->ParseAllAbbrvSectionHeader(file.data, Offset, InfoOffset, InfoLen);
    uint32_t cuCnt = static_cast<uint32_t>(CompilationUnit.size());
    if (cuCnt == 0)
    {
        std::cout << "Failefile: " << file_name << "\n";
        return 0;
    }
    //this->PrintAllAbbrevInfo();

    std::vector<TreeElementType*> cuTreeNodes(cuCnt); // Direct mapping
    TreeElementType* currItem = new TreeElementType();
    this->DataRoot = currItem;

    // Build TreeElementType for each compilation unit
    for (uint32_t t = 0; t < cuCnt; t++)
    {
        cuTreeNodes[t] = currItem; // Map index 't' to the tree node

        const uint8_t* abbrevPtr = file.data + Offset + CompilationUnit[t]->AbrevOffset;
        CompilationUnit[t]->AbbrevInfo = ParseAbbrevOffset(abbrevPtr);
        const uint8_t* cuStart = file.data + InfoOffset + CompilationUnit[t]->Offset + HeaderSize_Byte;
        //const uint8_t* ptr = cuStart;
        ParseDIE(cuStart, file.data, InfoOffset + CompilationUnit[t]->Offset, CompilationUnit[t]->Length_Bytes, CompilationUnit[t], currItem);

        if (t < cuCnt - 1)
        {
            currItem->next = new TreeElementType();
            currItem = currItem->next;
        }
    }

    // -----------------------------
    // Multithreaded SymbolTraverse
    // -----------------------------
    std::mutex symbolMutex;
    FileBin_DWARF_VarInfoType* lastSymbol = nullptr;
    std::vector<FileBin_DWARF_VarInfoType*> cuSymbols(cuCnt, nullptr);
    std::vector<std::thread> threads2;

    // Determine maximum concurrent threads
    const unsigned int maxThreads = std::max(1u, std::thread::hardware_concurrency());

    std::mutex queueMutex;
    std::queue<uint32_t> tasks;
    for (uint32_t t = 0; t < cuCnt; ++t)
        tasks.push(t);

    auto worker = [&]() {
        while (true) {
            uint32_t t;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (tasks.empty()) return;
                t = tasks.front();
                tasks.pop();
            }

            // FIX: Direct O(1) access. No more for loop here.
            TreeElementType* targetNode = cuTreeNodes[t];

            FileBin_DWARF_VarInfoType* cuSymbol = new FileBin_DWARF_VarInfoType();

            // Navigation depends on how ParseDIE attaches data:
            // Usually, the first child is the DW_TAG_compile_unit
            if (targetNode->child) {
                cuSymbol->data = targetNode->child->data;
                cuSymbol->elementType = FILEBIN_DWARF_ELEMENT_COMPILE_UNIT;

                // Recurse into children of the CU (functions, types, globals)
                if (targetNode->child->child)
                {
                    SymbolTraverse(targetNode->child->child, cuSymbol);
                }
            }

            cuSymbols[t] = cuSymbol;
        }
    };

    // Launch threads (limited to maxThreads)
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < std::min(maxThreads, cuCnt); ++i)
        threads.emplace_back(worker);

    // Wait for threads to finish
    for (auto& th : threads)
        th.join();

    // Append to global Symbol list sequentially
    lastSymbol = nullptr;
    for (uint32_t t = 0; t < cuCnt; ++t) {
        if (!cuSymbols[t]) continue;

        if (!this->SymbolRoot)
            this->SymbolRoot = cuSymbols[t];
        else
            lastSymbol->next = cuSymbols[t];

        lastSymbol = cuSymbols[t];
    }

    std::cout << "[INFO] Parsed symbols from " << cuCnt
              << " compilation units using " << threads.size() << " threads\n";



    return 0;
}
