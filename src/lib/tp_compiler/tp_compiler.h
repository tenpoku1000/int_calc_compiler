
// Copyright (C) 2018 Shin'ichi Ichikawa. Released under the MIT license.

#if ! defined(INT_CALC_COMPILER_H_)
#define INT_CALC_COMPILER_H_

#include <windows.h>
#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

// config section:

#define TP_CONFIG_OPTION_IS_OUTPUT_CURRENT_DIR 'c'
#define TP_CONFIG_OPTION_IS_OUTPUT_LOG_FILE 'l'
#define TP_CONFIG_OPTION_IS_NO_OUTPUT_MESSAGES 'm'
#define TP_CONFIG_OPTION_IS_NO_OUTPUT_FILES 'n'
#define TP_CONFIG_OPTION_IS_ORIGIN_WASM 'r'
#define TP_CONFIG_OPTION_IS_SOURCE_CMD_PARAM 's'
#define TP_CONFIG_OPTION_IS_TEST_MODE 't'
#define TP_CONFIG_OPTION_IS_OUTPUT_WASM_FILE 'w'
#define TP_CONFIG_OPTION_IS_OUTPUT_X64_FILE 'x'

#define TP_SOURCE_CODE_STRING_BUFFER_SIZE 256
#define TP_SOURCE_CODE_STRING_LENGTH_MAX (TP_SOURCE_CODE_STRING_BUFFER_SIZE - 1)

// message section:

#define TP_MESSAGE_BUFFER_SIZE 1024
#define TP_TEST_FNAME_NUM_MAX 999

typedef enum TP_LOG_TYPE_
{
    TP_LOG_TYPE_DEFAULT,
    TP_LOG_TYPE_HIDE_AFTER_DISP,
    TP_LOG_TYPE_DISP_FORCE,
    TP_LOG_TYPE_HIDE
}TP_LOG_TYPE;

typedef enum TP_LOG_PARAM_TYPE_
{
    TP_LOG_PARAM_TYPE_STRING,
    TP_LOG_PARAM_TYPE_INT32_VALUE,
    TP_LOG_PARAM_TYPE_UINT64_VALUE
}TP_LOG_PARAM_TYPE;

typedef union tp_log_param_element_union{
    uint8_t* member_string;
    int32_t member_int32_value;
    size_t member_uint64_value;
}TP_LOG_PARAM_ELEMENT_UNION;

typedef struct tp_put_log_element_{
    TP_LOG_PARAM_TYPE member_type;
    TP_LOG_PARAM_ELEMENT_UNION member_body;
}TP_LOG_PARAM_ELEMENT;

#define TP_PUT_LOG_MSG(symbol_table, log_type, format_string, ...) \
    tp_put_log_msg( \
        (symbol_table), (log_type), (format_string), __FILE__, __func__, __LINE__, \
        (TP_LOG_PARAM_ELEMENT[]){ __VA_ARGS__ }, \
        sizeof((TP_LOG_PARAM_ELEMENT[]){ __VA_ARGS__ }) / sizeof(TP_LOG_PARAM_ELEMENT) \
    )
#define TP_MSG_FMT(format_string) (format_string)
#define TP_LOG_PARAM_STRING(string) (TP_LOG_PARAM_ELEMENT){ \
    .member_type = TP_LOG_PARAM_TYPE_STRING, \
    .member_body.member_string = (string) \
}
#define TP_LOG_PARAM_INT32_VALUE(value) (TP_LOG_PARAM_ELEMENT){ \
    .member_type = TP_LOG_PARAM_TYPE_INT32_VALUE, \
    .member_body.member_int32_value = (value) \
}
#define TP_LOG_PARAM_UINT64_VALUE(value) (TP_LOG_PARAM_ELEMENT){ \
    .member_type = TP_LOG_PARAM_TYPE_UINT64_VALUE, \
    .member_body.member_uint64_value = (value) \
}
#define TP_LOG_MSG_ICE "Internal compiler error."
#define TP_PUT_LOG_MSG_ICE(symbol_table) \
    TP_PUT_LOG_MSG( \
        (symbol_table), TP_LOG_TYPE_HIDE_AFTER_DISP, \
        TP_MSG_FMT("%1"), TP_LOG_PARAM_STRING(TP_LOG_MSG_ICE) \
    );
#define TP_PUT_LOG_MSG_TRACE(symbol_table) \
    TP_PUT_LOG_MSG( \
        (symbol_table), TP_LOG_TYPE_HIDE, \
        TP_MSG_FMT("TRACE: %1 function"), TP_LOG_PARAM_STRING(__func__) \
    );
#define TP_GET_LAST_ERROR(symbol_table) tp_get_last_error((symbol_table), __FILE__, __func__, __LINE__);
#define TP_PRINT_CRT_ERROR(symbol_table) tp_print_crt_error((symbol_table), __FILE__, __func__, __LINE__);
#define TP_FREE(symbol_table, ptr, size) tp_free((symbol_table), (ptr), (size), __FILE__, __func__, __LINE__);
#define TP_FREE2(symbol_table, ptr, size) tp_free2((symbol_table), (ptr), (size), __FILE__, __func__, __LINE__);

// output file section:

#define TP_LOG_FILE_PREFIX "int_calc"

#define TP_WRITE_LOG_DEFAULT_FILE_NAME "log"
#define TP_WRITE_LOG_DEFAULT_EXT_NAME "log"

#define TP_TOKEN_DEFAULT_FILE_NAME "token"
#define TP_TOKEN_DEFAULT_EXT_NAME "log"

#define TP_PARSE_TREE_DEFAULT_FILE_NAME "parse_tree"
#define TP_PARSE_TREE_DEFAULT_EXT_NAME "log"

#define TP_OBJECT_HASH_DEFAULT_FILE_NAME "object_hash"
#define TP_OBJECT_HASH_DEFAULT_EXT_NAME "log"

#define TP_WASM_DEFAULT_FILE_NAME "int_calc"
#define TP_WASM_DEFAULT_EXT_NAME "wasm"

#define TP_X64_DEFAULT_FILE_NAME "int_calc"
#define TP_X64_DEFAULT_EXT_NAME "bin"

#define TP_INDENT_UNIT 4
#define TP_INDENT_FORMAT_BUFFER_SIZE 32
#define TP_INDENT_STRING_BUFFER_SIZE 4096

#define TP_MAKE_INDENT_STRING(indent_level) \
\
    uint8_t temp_prev_indent_string[TP_INDENT_FORMAT_BUFFER_SIZE]; \
    memset(temp_prev_indent_string, 0, sizeof(temp_prev_indent_string)); \
\
    sprintf_s( \
        temp_prev_indent_string, sizeof(temp_prev_indent_string), \
        "%% %dc", (indent_level * TP_INDENT_UNIT) - 1 \
    ); \
\
    uint8_t temp_indent_string[TP_INDENT_FORMAT_BUFFER_SIZE]; \
    memset(temp_indent_string, 0, sizeof(temp_indent_string)); \
    sprintf_s( \
        temp_indent_string, sizeof(temp_indent_string), \
        "%% %dc", (indent_level * TP_INDENT_UNIT)); \
\
    uint8_t prev_indent_string[TP_INDENT_STRING_BUFFER_SIZE]; \
    memset(prev_indent_string, 0, sizeof(prev_indent_string)); \
    sprintf_s(prev_indent_string, sizeof(prev_indent_string), temp_prev_indent_string, ' '); \
\
    uint8_t indent_string[TP_INDENT_STRING_BUFFER_SIZE]; \
    memset(indent_string, 0, sizeof(indent_string)); \
    sprintf_s(indent_string, sizeof(indent_string), temp_indent_string, ' '); \
\
    { \
        errno_t err = _set_errno(0); \
    }

// input file section:

#define TP_MAX_LINE_BYTES 4095
#define TP_BUFFER_SIZE (TP_MAX_LINE_BYTES + 1)
#define TP_MAX_FILE_BYTES (TP_MAX_LINE_BYTES * 4096)

// token section:

#define TP_TOKEN_SIZE_ALLOCATE_UNIT 256

typedef enum TP_SYMBOL_
{
    TP_SYMBOL_NULL,
    TP_SYMBOL_ID,
    TP_SYMBOL_CONST_VALUE,
    TP_SYMBOL_PLUS,
    TP_SYMBOL_MINUS,
    TP_SYMBOL_MUL,
    TP_SYMBOL_DIV,
    TP_SYMBOL_LEFT_PAREN,
    TP_SYMBOL_RIGHT_PAREN,
    TP_SYMBOL_EQUAL,
    TP_SYMBOL_SEMICOLON
}TP_SYMBOL;

typedef enum TP_SYMBOL_TYPE_
{
    TP_SYMBOL_UNSPECIFIED_TYPE,
    TP_SYMBOL_ID_INT32,
    TP_SYMBOL_TYPE_INT32,
    TP_SYMBOL_CONST_VALUE_INT32
}TP_SYMBOL_TYPE;

#define TP_MAX_ID_BYTES 63
#define TP_ID_SIZE (TP_MAX_ID_BYTES + 1)
#define TP_MAX_ID_NUM 4095

typedef struct tp_token_{
    TP_SYMBOL member_symbol;
    TP_SYMBOL_TYPE member_symbol_type;
    rsize_t member_line;
    rsize_t member_column;
    uint8_t member_string[TP_ID_SIZE];
    int32_t member_i32_value;
}TP_TOKEN;

// parse tree section:

#define IS_TOKEN_ID(token) ((token) && (TP_SYMBOL_ID == (token)->member_symbol))
#define IS_TOKEN_CONST_VALUE(token) ((token) && (TP_SYMBOL_CONST_VALUE == (token)->member_symbol))
#define IS_TOKEN_PLUS(token) ((token) && (TP_SYMBOL_PLUS == (token)->member_symbol))
#define IS_TOKEN_MINUS(token) ((token) && (TP_SYMBOL_MINUS == (token)->member_symbol))
#define IS_TOKEN_MUL(token) ((token) && (TP_SYMBOL_MUL == (token)->member_symbol))
#define IS_TOKEN_DIV(token) ((token) && (TP_SYMBOL_DIV == (token)->member_symbol))
#define IS_TOKEN_LEFT_PAREN(token) ((token) && (TP_SYMBOL_LEFT_PAREN == (token)->member_symbol))
#define IS_TOKEN_RIGHT_PAREN(token) ((token) && (TP_SYMBOL_RIGHT_PAREN == (token)->member_symbol))
#define IS_TOKEN_EQUAL(token) ((token) && (TP_SYMBOL_EQUAL == (token)->member_symbol))
#define IS_TOKEN_SEMICOLON(token) ((token) && (TP_SYMBOL_SEMICOLON == (token)->member_symbol))

#define IS_TOKEN_TYPE_UNSPECIFIED_TYPE(token) ((token) && (TP_SYMBOL_UNSPECIFIED_TYPE == (token)->member_symbol_type))
#define IS_TOKEN_TYPE_ID_INT32(token) ((token) && (TP_SYMBOL_ID_INT32 == (token)->member_symbol_type))
#define IS_TOKEN_TYPE_TYPE_INT32(token) ((token) && (TP_SYMBOL_TYPE_INT32 == (token)->member_symbol_type))
#define IS_TOKEN_TYPE_CONST_VALUE_INT32(token) ((token) && (TP_SYMBOL_CONST_VALUE_INT32 == (token)->member_symbol_type))

#define IS_TOKEN_STRING_ID_INT32(token) \
    ((token) && (TP_SYMBOL_ID == (token)->member_symbol) && (0 == strcmp("int32_t", (token)->member_string)))

typedef enum TP_PARSE_TREE_TYPE_
{
    TP_PARSE_TREE_TYPE_NULL = 0,
    TP_PARSE_TREE_TYPE_TOKEN,
    TP_PARSE_TREE_TYPE_NODE
}TP_PARSE_TREE_TYPE;

typedef union tp_parse_tree_element_union_{
    TP_TOKEN* member_tp_token;
    struct tp_parse_tree_* member_child;
}TP_PARSE_TREE_ELEMENT_UNION;

typedef struct tp_parse_tree_element_{
    TP_PARSE_TREE_TYPE member_type;
    TP_PARSE_TREE_ELEMENT_UNION member_body;
}TP_PARSE_TREE_ELEMENT;

typedef enum TP_PARSE_TREE_GRAMMER_
{
    TP_PARSE_TREE_GRAMMER_PROGRAM,
    TP_PARSE_TREE_GRAMMER_STATEMENT_1,
    TP_PARSE_TREE_GRAMMER_STATEMENT_2,
    TP_PARSE_TREE_GRAMMER_EXPRESSION_1,
    TP_PARSE_TREE_GRAMMER_EXPRESSION_2,
    TP_PARSE_TREE_GRAMMER_TERM_1,
    TP_PARSE_TREE_GRAMMER_TERM_2,
    TP_PARSE_TREE_GRAMMER_FACTOR_1,
    TP_PARSE_TREE_GRAMMER_FACTOR_2,
    TP_PARSE_TREE_GRAMMER_FACTOR_3
}TP_PARSE_TREE_GRAMMER;

typedef struct tp_parse_tree_{
    TP_PARSE_TREE_GRAMMER member_grammer;
    size_t member_element_num;
    TP_PARSE_TREE_ELEMENT* member_element;
}TP_PARSE_TREE;

// semantic analysis section:

#define TP_GRAMMER_TYPE_INDEX_STATEMENT_1 0
#define TP_GRAMMER_TYPE_INDEX_STATEMENT_2 1
#define TP_GRAMMER_TYPE_INDEX_EXPRESSION_1 2
#define TP_GRAMMER_TYPE_INDEX_EXPRESSION_2 3
#define TP_GRAMMER_TYPE_INDEX_TERM_1 4
#define TP_GRAMMER_TYPE_INDEX_TERM_2 5
#define TP_GRAMMER_TYPE_INDEX_FACTOR_1 6
#define TP_GRAMMER_TYPE_INDEX_FACTOR_2 7
#define TP_GRAMMER_TYPE_INDEX_FACTOR_3 8
#define TP_GRAMMER_TYPE_INDEX_NULL 9

#define TP_PARSE_TREE_TYPE_MAX_NUM1 5
#define TP_PARSE_TREE_TYPE_MAX_NUM2 (TP_GRAMMER_TYPE_INDEX_NULL + 1)

typedef enum register_object_type_ {
    NOTHING_REGISTER_OBJECT = 0,
    DEFINED_REGISTER_OBJECT,
    UNDEFINED_REGISTER_OBJECT,
}REGISTER_OBJECT_TYPE;

typedef struct register_object_ {
    REGISTER_OBJECT_TYPE member_register_object_type;
    uint32_t member_var_index;
}REGISTER_OBJECT;

typedef struct sama_hash_data_{
    REGISTER_OBJECT member_register_object;
    uint8_t* member_string;
}SAME_HASH_DATA;

typedef struct register_object_hash_element_{
    SAME_HASH_DATA member_sama_hash_data[UINT8_MAX + 1];
    struct register_object_hash_element_* member_next;
}REGISTER_OBJECT_HASH_ELEMENT;

typedef struct register_object_hash_{
    size_t member_mask;
    REGISTER_OBJECT_HASH_ELEMENT member_hash_table[UINT8_MAX + 1];
}REGISTER_OBJECT_HASH;

// wasm section:

#define TP_WASM_OPCODE_GET_LOCAL 0x20
#define TP_WASM_OPCODE_SET_LOCAL 0x21
#define TP_WASM_OPCODE_TEE_LOCAL 0x22
#define TP_WASM_OPCODE_I32_CONST 0x41
#define TP_WASM_OPCODE_I32_ADD 0x6a
#define TP_WASM_OPCODE_I32_SUB 0x6b
#define TP_WASM_OPCODE_I32_MUL 0x6c
#define TP_WASM_OPCODE_I32_DIV 0x6d
#define TP_WASM_OPCODE_I32_XOR 0x73
#define TP_WASM_OPCODE_END 0x0b
#define TP_WASM_OPCODE_I32_VALUE 0xFF41 // pseudo opcode.

#define TP_WASM_MODULE_MAGIC_NUMBER "\0asm"
#define TP_WASM_MODULE_VERSION 0x1

#define TP_WASM_MODULE_SECTION_TYPE_COUNT 1
#define TP_WASM_MODULE_SECTION_TYPE_FORM_FUNC 0x60
#define TP_WASM_MODULE_SECTION_TYPE_PARAM_COUNT 0
#define TP_WASM_MODULE_SECTION_TYPE_RETURN_COUNT 1
#define TP_WASM_MODULE_SECTION_TYPE_RETURN_TYPE_I32 0x7f

#define TP_WASM_MODULE_SECTION_FUNCTION_COUNT 1
#define TP_WASM_MODULE_SECTION_FUNCTION_TYPES 0

#define TP_WASM_MODULE_SECTION_TABLE_COUNT 1
#define TP_WASM_MODULE_SECTION_TABLE_ELEMENT_TYPE_ANYFUNC 0x70
#define TP_WASM_MODULE_SECTION_TABLE_FLAGS 0
#define TP_WASM_MODULE_SECTION_TABLE_INITIAL 0

#define TP_WASM_MODULE_SECTION_MEMORY_COUNT 1
#define TP_WASM_MODULE_SECTION_MEMORY_FLAGS 0
#define TP_WASM_MODULE_SECTION_MEMORY_INITIAL 1

#define TP_WASM_MODULE_SECTION_EXPORT_COUNT 2
#define TP_WASM_MODULE_SECTION_EXPORT_NAME_LENGTH_1 6
#define TP_WASM_MODULE_SECTION_EXPORT_NAME_1 "memory"
#define TP_WASM_MODULE_SECTION_EXPORT_ITEM_INDEX_1 0
#define TP_WASM_MODULE_SECTION_EXPORT_NAME_LENGTH_2 4
#define TP_WASM_MODULE_SECTION_EXPORT_NAME_2 "calc"
#define TP_WASM_MODULE_SECTION_EXPORT_ITEM_INDEX_2 0

#define TP_WASM_MODULE_SECTION_CODE_COUNT 1
#define TP_WASM_MODULE_SECTION_CODE_LOCAL_COUNT 1
#define TP_WASM_MODULE_SECTION_CODE_VAR_COUNT 2
#define TP_WASM_MODULE_SECTION_CODE_VAR_TYPE_I32 0x7f

typedef enum tp_wasm_section_type_{
    TP_WASM_SECTION_TYPE_CUSTOM = 0,
    TP_WASM_SECTION_TYPE_TYPE,
    TP_WASM_SECTION_TYPE_IMPORT,
    TP_WASM_SECTION_TYPE_FUNCTION,
    TP_WASM_SECTION_TYPE_TABLE,
    TP_WASM_SECTION_TYPE_MEMORY,
    TP_WASM_SECTION_TYPE_GLOBAL,
    TP_WASM_SECTION_TYPE_EXPORT,
    TP_WASM_SECTION_TYPE_START,
    TP_WASM_SECTION_TYPE_ELEMENT,
    TP_WASM_SECTION_TYPE_CODE,
    TP_WASM_SECTION_TYPE_DATA
}TP_WASM_SECTION_TYPE;

typedef enum tp_wasm_section_kind_{
    TP_WASM_SECTION_KIND_FUNCTION = 0,
    TP_WASM_SECTION_KIND_TABLE,
    TP_WASM_SECTION_KIND_MEMORY,
    TP_WASM_SECTION_KIND_GLOBAL
}TP_WASM_SECTION_KIND;

typedef struct tp_wasm_module_section_{
    uint32_t member_section_size;
    uint32_t member_id;
    uint32_t member_payload_len;
//  NOTE: Not implemented.
//  name_len: 0 == member_id
//  name: 0 == member_id
    uint8_t* member_name_len_name_payload_data;
}TP_WASM_MODULE_SECTION;

typedef struct tp_wasm_module_content_{
    uint32_t member_magic_number;
    uint32_t member_version;
    uint8_t member_payload[];
}TP_WASM_MODULE_CONTENT;

typedef struct tp_wasm_module_{
    uint32_t member_section_num;
    TP_WASM_MODULE_SECTION** member_section;
    uint32_t member_content_size;
    TP_WASM_MODULE_CONTENT* member_module_content;
}TP_WASM_MODULE;

// x64 section:

#define TP_WASM_STACK_EMPTY -1
#define TP_WASM_STACK_SIZE_ALLOCATE_UNIT 256
#define TP_WASM_LOCAL_VARIABLE_MAX_DEFAULT 1024
#define TP_WASM_TEMPORARY_VARIABLE_MAX_DEFAULT 1024

#define TP_PADDING_MASK (16 - 1)

#define TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size) \
\
    do{ \
        if (0 == (tmp_x64_code_size)){ \
\
            TP_PUT_LOG_MSG( \
                (symbol_table), TP_LOG_TYPE_DISP_FORCE, \
                TP_MSG_FMT("ERROR: 0 == tmp_x64_code_size at %1 function."), \
                TP_LOG_PARAM_STRING(__func__) \
            ); \
\
            return 0; \
        } \
\
        (x64_code_size) += (tmp_x64_code_size); \
    }while (false)

typedef enum tp_x86_32_register_{
    TP_X86_32_REGISTER_EAX = 0,
    TP_X86_32_REGISTER_ECX,
    TP_X86_32_REGISTER_EDX,
    TP_X86_32_REGISTER_EBX,
    TP_X86_32_REGISTER_ESP,
    TP_X86_32_REGISTER_EBP,
    TP_X86_32_REGISTER_ESI,
    TP_X86_32_REGISTER_EDI,
    TP_X86_32_REGISTER_NULL,
    TP_X86_32_REGISTER_NUM = 8
}TP_X86_32_REGISTER;

typedef enum tp_x64_32_register_{
    TP_X64_32_REGISTER_R8D = 0,
    TP_X64_32_REGISTER_R9D,
    TP_X64_32_REGISTER_R10D,
    TP_X64_32_REGISTER_R11D,
    TP_X64_32_REGISTER_R12D,
    TP_X64_32_REGISTER_R13D,
    TP_X64_32_REGISTER_R14D,
    TP_X64_32_REGISTER_R15D,
    TP_X64_32_REGISTER_NULL,
    TP_X64_32_REGISTER_NUM = 8
}TP_X64_32_REGISTER;

typedef enum tp_x64_64_register_{
    TP_X64_64_REGISTER_RAX = 0,
    TP_X64_64_REGISTER_RCX,
    TP_X64_64_REGISTER_RDX,
    TP_X64_64_REGISTER_RBX,
    TP_X64_64_REGISTER_RSP,
    TP_X64_64_REGISTER_INDEX_NONE = 4,
    TP_X64_64_REGISTER_RBP,
    TP_X64_64_REGISTER_RSI,
    TP_X64_64_REGISTER_RDI,
    TP_X64_64_REGISTER_R8,
    TP_X64_64_REGISTER_R9,
    TP_X64_64_REGISTER_R10,
    TP_X64_64_REGISTER_R11,
    TP_X64_64_REGISTER_R12,
    TP_X64_64_REGISTER_R13,
    TP_X64_64_REGISTER_R14,
    TP_X64_64_REGISTER_R15,
    TP_X64_64_REGISTER_NULL
}TP_X64_64_REGISTER;

typedef enum tp_x64_item_kind_{
    TP_X64_ITEM_KIND_NONE = 0,
    TP_X64_ITEM_KIND_X86_32_REGISTER,
    TP_X64_ITEM_KIND_X64_32_REGISTER,
    TP_X64_ITEM_KIND_MEMORY
}TP_X64_ITEM_KIND;

typedef enum tp_x64_item_memory_kind_{
    TP_X64_ITEM_MEMORY_KIND_NONE = 0,
    TP_X64_ITEM_MEMORY_KIND_LOCAL,
    TP_X64_ITEM_MEMORY_KIND_TEMP
}TP_X64_ITEM_MEMORY_KIND;

typedef union tp_x64_item_{
    TP_X86_32_REGISTER member_x86_32_register;
    TP_X64_32_REGISTER member_x64_32_register;
}TP_X64_ITEM;

typedef struct tp_wasm_stack_element_{
    uint32_t member_wasm_opcode;
    uint32_t member_local_index;
    int32_t member_i32;
    TP_X64_ITEM_KIND member_x64_item_kind;
    TP_X64_ITEM member_x64_item;
    TP_X64_ITEM_MEMORY_KIND member_x64_memory_kind;
    int32_t member_offset;
}TP_WASM_STACK_ELEMENT;

typedef enum tp_x64_allocate_mode_{
    TP_X64_ALLOCATE_DEFAULT,
    TP_X64_ALLOCATE_MEMORY
}TP_X64_ALLOCATE_MODE;

typedef enum tp_x64_nv64_register_{
    TP_X64_NV64_REGISTER_NULL = 0,
    TP_X64_NV64_REGISTER_RBX = TP_X64_64_REGISTER_RBX,
    TP_X64_NV64_REGISTER_RSI = TP_X64_64_REGISTER_RSI,
    TP_X64_NV64_REGISTER_RDI,
    TP_X64_NV64_REGISTER_R12 = TP_X64_64_REGISTER_R12,
    TP_X64_NV64_REGISTER_R13,
    TP_X64_NV64_REGISTER_R14,
    TP_X64_NV64_REGISTER_R15,
    TP_X64_NV64_REGISTER_NUM = 7,
    TP_X64_NV64_REGISTER_RBX_INDEX = 0,
    TP_X64_NV64_REGISTER_RSI_INDEX,
    TP_X64_NV64_REGISTER_RDI_INDEX,
    TP_X64_NV64_REGISTER_R12_INDEX,
    TP_X64_NV64_REGISTER_R13_INDEX,
    TP_X64_NV64_REGISTER_R14_INDEX,
    TP_X64_NV64_REGISTER_R15_INDEX
}TP_X64_NV64_REGISTER;

typedef enum tp_x64_{
    TP_X64_MOV,
    TP_X64_ADD,
    TP_X64_SUB,
    TP_X64_IMUL,
    TP_X64_IDIV,
    TP_X64_XOR,
    TP_X64_NULL
}TP_X64;

typedef enum tp_x64_direction_{
    TP_X64_DIRECTION_SOURCE_REGISTER,
    TP_X64_DIRECTION_SOURCE_MEMORY
}TP_X64_DIRECTION;

typedef struct symbol_table_{
// config section:
    bool member_is_output_current_dir;
    bool member_is_output_log_file;
    bool member_is_no_output_messages;
    bool member_is_no_output_files;
    bool member_is_origin_wasm;
    bool member_is_source_cmd_param;
    uint8_t member_source_code[TP_SOURCE_CODE_STRING_BUFFER_SIZE];
    bool member_is_test_mode;
    bool member_is_output_wasm_file;
    bool member_is_output_x64_file;

// message section:
    bool member_log_hide_after_disp;
    uint8_t member_temp_buffer[TP_MESSAGE_BUFFER_SIZE];
    uint8_t member_log_msg_buffer[TP_MESSAGE_BUFFER_SIZE];
    FILE* member_disp_log_file;

// output file section:
    FILE* member_write_log_file;
    FILE* member_parse_tree_file;
    char member_write_log_file_path[_MAX_PATH];
    char member_token_file_path[_MAX_PATH];
    char member_parse_tree_file_path[_MAX_PATH];
    char member_object_hash_file_path[_MAX_PATH];
    char member_wasm_file_path[_MAX_PATH];
    char member_x64_file_path[_MAX_PATH];

// input file section:
    uint8_t member_input_file_path[_MAX_PATH];
    FILE* member_read_file;
    bool member_is_start_of_file;
    bool member_is_end_of_file;
    uint8_t member_read_lines_buffer[TP_BUFFER_SIZE];
    size_t member_read_lines_length;
    uint8_t* member_read_lines_current_position;
    uint8_t* member_read_lines_end_position;

// token section:
    rsize_t member_tp_token_pos;
    rsize_t member_tp_token_size;
    rsize_t member_tp_token_size_allocate_unit;
    TP_TOKEN* member_tp_token;
    TP_TOKEN* member_tp_token_position;
    rsize_t member_nul_num;

// parse tree section:
    uint8_t member_nesting_level_of_expression;
    TP_PARSE_TREE* member_tp_parse_tree;

// semantic analysis section:
    REGISTER_OBJECT_HASH member_object_hash;
    uint32_t member_var_count;
    TP_PARSE_TREE* member_last_statement;
    TP_PARSE_TREE_TYPE member_parse_tree_type[TP_PARSE_TREE_TYPE_MAX_NUM2][TP_PARSE_TREE_TYPE_MAX_NUM1];
    uint32_t member_grammer_statement_1_num;
    uint32_t member_grammer_statement_2_num;
    uint32_t member_grammer_expression_1_num;
    uint32_t member_grammer_expression_2_num;
    uint32_t member_grammer_term_1_num;
    uint32_t member_grammer_term_2_num;
    uint32_t member_grammer_factor_1_num;
    uint32_t member_grammer_factor_2_num;
    uint32_t member_grammer_factor_3_num;

// wasm section:
    TP_WASM_MODULE member_wasm_module;
    size_t member_code_index;
    uint32_t member_code_body_size;
    uint8_t* member_code_section_buffer;

// x64 section:
    TP_WASM_STACK_ELEMENT* member_stack;
    uint8_t* member_wasm_code_body_buffer;
    uint32_t member_wasm_code_body_pos;
    uint32_t member_wasm_code_body_size;
    int32_t member_stack_pos;
    int32_t member_stack_size;
    int32_t member_stack_size_allocate_unit;

    int32_t member_local_variable_size;
    int32_t member_local_variable_size_max;
    int32_t member_padding_local_variable_bytes;

    int32_t member_temporary_variable_size;
    int32_t member_temporary_variable_size_max;
    int32_t member_padding_temporary_variable_bytes;

    TP_WASM_STACK_ELEMENT member_use_X86_32_register[TP_X86_32_REGISTER_NUM];
    TP_WASM_STACK_ELEMENT member_use_X64_32_register[TP_X64_32_REGISTER_NUM];
    TP_X64_NV64_REGISTER member_use_nv_register[TP_X64_NV64_REGISTER_NUM];

    int32_t member_register_bytes;
    int32_t member_padding_register_bytes;

    int32_t member_stack_imm32;
}TP_SYMBOL_TABLE;

bool tp_compiler(int argc, char** argv, uint8_t* msg_buffer, size_t msg_buffer_size);

bool tp_make_token(TP_SYMBOL_TABLE* symbol_table, uint8_t* string, rsize_t string_length);
bool tp_dump_token_main(
    TP_SYMBOL_TABLE* symbol_table, FILE* write_file, TP_TOKEN* token, uint8_t indent_level
);

bool tp_make_parse_tree(TP_SYMBOL_TABLE* symbol_table);
void tp_free_parse_subtree(TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE** parse_subtree);

bool tp_semantic_analysis(TP_SYMBOL_TABLE* symbol_table);
bool tp_search_object(TP_SYMBOL_TABLE* symbol_table, TP_TOKEN* token, REGISTER_OBJECT* register_object);
void tp_free_object_hash(
    TP_SYMBOL_TABLE* symbol_table,
    REGISTER_OBJECT_HASH* object_hash, REGISTER_OBJECT_HASH_ELEMENT* hash_element
);

bool tp_make_wasm(TP_SYMBOL_TABLE* symbol_table, bool is_origin_wasm);

bool tp_make_x64_code(TP_SYMBOL_TABLE* symbol_table, int32_t* return_value);
bool tp_wasm_stack_push(TP_SYMBOL_TABLE* symbol_table, TP_WASM_STACK_ELEMENT* value);
bool tp_get_local_variable_offset(
    TP_SYMBOL_TABLE* symbol_table, uint32_t local_index, int32_t* local_variable_offset
);
bool tp_allocate_temporary_variable(
    TP_SYMBOL_TABLE* symbol_table, TP_X64_ALLOCATE_MODE allocate_mode,
    uint8_t* x64_code_buffer, uint32_t x64_code_offset, uint32_t* x64_code_size,
    TP_WASM_STACK_ELEMENT* wasm_stack_element
);
bool tp_free_register(TP_SYMBOL_TABLE* symbol_table, TP_WASM_STACK_ELEMENT* stack_element);
uint32_t tp_encode_allocate_stack(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    uint32_t var_count, uint32_t var_type
);
uint32_t tp_encode_get_local_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset, uint32_t local_index
);
uint32_t tp_encode_set_local_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset, uint32_t local_index,
    TP_WASM_STACK_ELEMENT* op1
);
uint32_t tp_encode_tee_local_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset, uint32_t local_index,
    TP_WASM_STACK_ELEMENT* op1
);
uint32_t tp_encode_i32_const_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset, int32_t value
);
uint32_t tp_encode_i32_add_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_WASM_STACK_ELEMENT* op1, TP_WASM_STACK_ELEMENT* op2
);
uint32_t tp_encode_i32_sub_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_WASM_STACK_ELEMENT* op1, TP_WASM_STACK_ELEMENT* op2
);
uint32_t tp_encode_i32_mul_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_WASM_STACK_ELEMENT* op1, TP_WASM_STACK_ELEMENT* op2
);
uint32_t tp_encode_i32_div_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_WASM_STACK_ELEMENT* op1, TP_WASM_STACK_ELEMENT* op2
);
uint32_t tp_encode_i32_xor_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_WASM_STACK_ELEMENT* op1, TP_WASM_STACK_ELEMENT* op2
);
uint32_t tp_encode_end_code(TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset);
uint32_t tp_encode_x64_2_operand(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64 x64_op, TP_WASM_STACK_ELEMENT* op1, TP_WASM_STACK_ELEMENT* op2
);

bool tp_open_read_file(TP_SYMBOL_TABLE* symbol_table, char* path, FILE** file_stream);
bool tp_open_write_file(TP_SYMBOL_TABLE* symbol_table, char* path, FILE** file_stream);
bool tp_ftell(TP_SYMBOL_TABLE* symbol_table, FILE* file_stream, long* seek_position);
bool tp_seek(TP_SYMBOL_TABLE* symbol_table, FILE* file_stream, long seek_position, long line_bytes);
bool tp_close_file(TP_SYMBOL_TABLE* symbol_table, FILE** file_stream);
bool tp_write_file(TP_SYMBOL_TABLE* symbol_table, char* path, void* content, uint32_t content_size);

uint32_t tp_encode_si64leb128(uint8_t* buffer, size_t offset, int64_t value);
uint32_t tp_encode_ui32leb128(uint8_t* buffer, size_t offset, uint32_t value);
int32_t tp_decode_si32leb128(uint8_t* buffer, uint32_t* size);
uint32_t tp_decode_ui32leb128(uint8_t* buffer, uint32_t* size);

void tp_free(TP_SYMBOL_TABLE* symbol_table, void** ptr, size_t size, uint8_t* file, uint8_t* func, size_t line_num);
void tp_free2(TP_SYMBOL_TABLE* symbol_table, void*** ptr, size_t size, uint8_t* file, uint8_t* func, size_t line_num);
void tp_get_last_error(TP_SYMBOL_TABLE* symbol_table, uint8_t* file, uint8_t* func, size_t line_num);
void tp_print_crt_error(TP_SYMBOL_TABLE* symbol_table, uint8_t* file, uint8_t* func, size_t line_num);
bool tp_put_log_msg(
    TP_SYMBOL_TABLE* symbol_table, TP_LOG_TYPE log_type,
    uint8_t* format_string, uint8_t* file, uint8_t* func, size_t line_num,
    TP_LOG_PARAM_ELEMENT* log_param_element, size_t log_param_element_num
);

#endif

