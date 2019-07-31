
// Copyright (C) 2018, 2019 Shin'ichi Ichikawa. Released under the MIT license.

#include "tp_compiler.h"

#define TP_DECODE_UI32LEB128_GET_VALUE(symbol_table, section, buffer, offset, value) \
\
    do{ \
        uint32_t size = 0; \
\
        value = tp_decode_ui32leb128(((buffer) + (offset)), &size); \
\
        (offset) += size; \
\
        if ((section)->member_section_size < (offset)){ \
\
            TP_PUT_LOG_MSG( \
                (symbol_table), TP_LOG_TYPE_DISP_FORCE, \
                TP_MSG_FMT("ERROR: section->member_section_size(%1) < offset(%2) at %3 function."), \
                TP_LOG_PARAM_UINT64_VALUE((section)->member_section_size), \
                TP_LOG_PARAM_UINT64_VALUE(offset), \
                TP_LOG_PARAM_STRING(__func__) \
            ); \
\
            return false; \
        } \
    }while (false)

#define TP_DECODE_UI32LEB128_CHECK_VALUE(symbol_table, section, buffer, offset, kind) \
\
    do{ \
        uint32_t size = 0; \
\
        uint32_t value = tp_decode_ui32leb128(((buffer) + (offset)), &size); \
\
        if ((kind) != value){ \
\
            TP_PUT_LOG_MSG( \
                (symbol_table), TP_LOG_TYPE_DISP_FORCE, \
                TP_MSG_FMT("ERROR: kind(%1) != value(%2) at %3 function."), \
                TP_LOG_PARAM_UINT64_VALUE(kind), \
                TP_LOG_PARAM_UINT64_VALUE(value), \
                TP_LOG_PARAM_STRING(__func__) \
            ); \
\
            return false; \
        } \
\
        (offset) += size; \
\
        if ((section)->member_section_size < (offset)){ \
\
            TP_PUT_LOG_MSG( \
                (symbol_table), TP_LOG_TYPE_DISP_FORCE, \
                TP_MSG_FMT("ERROR: section->member_section_size(%1) < offset(%2) at %3 function."), \
                TP_LOG_PARAM_UINT64_VALUE((section)->member_section_size), \
                TP_LOG_PARAM_UINT64_VALUE(offset), \
                TP_LOG_PARAM_STRING(__func__) \
            ); \
\
            return false; \
        } \
    }while (false)

#define TP_WASM_CHECK_STRING(symbol_table, section, buffer, offset, name, name_length, is_match) \
\
    do{ \
        (is_match) = (0 == strncmp((name), (buffer) + (offset), (name_length))); \
\
        (offset) += (name_length); \
\
        if ((section)->member_section_size < (offset)){ \
\
            TP_PUT_LOG_MSG( \
                (symbol_table), TP_LOG_TYPE_DISP_FORCE, \
                TP_MSG_FMT("ERROR: section->member_section_size(%1) < offset(%2) at %3 function."), \
                TP_LOG_PARAM_UINT64_VALUE((section)->member_section_size), \
                TP_LOG_PARAM_UINT64_VALUE(offset), \
                TP_LOG_PARAM_STRING(__func__) \
            ); \
\
            return false; \
        } \
    }while (false)

static uint32_t convert_section_code_content2x64(TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer);

static bool wasm_stack_and_use_register_init(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* wasm_code_body_buffer, uint32_t wasm_code_body_size
);
static bool wasm_stack_and_wasm_code_is_empty(TP_SYMBOL_TABLE* symbol_table);
static bool wasm_stack_is_empty(TP_SYMBOL_TABLE* symbol_table);

typedef enum tp_wasm_stack_pop_{
    TP_WASM_STACK_POP_MODE_DEFAULT,
    TP_WASM_STACK_POP_MODE_PARAM
}TP_WASM_STACK_POP_MODE;

static TP_WASM_STACK_ELEMENT wasm_stack_pop(TP_SYMBOL_TABLE* symbol_table, TP_WASM_STACK_POP_MODE pop_mode);

static bool allocate_variable_common(
    TP_SYMBOL_TABLE* symbol_table, TP_X64_ALLOCATE_MODE allocate_mode,
    uint8_t* x64_code_buffer, uint32_t x64_code_offset, uint32_t* x64_code_size,
    TP_WASM_STACK_ELEMENT* wasm_stack_element
);
static bool get_free_register(
    TP_SYMBOL_TABLE* symbol_table,
    TP_X86_32_REGISTER* x86_32_register, TP_X64_32_REGISTER* x64_32_register, bool* is_zero_free_register
);
static bool spilling_variable(
    TP_SYMBOL_TABLE* symbol_table,
    uint8_t* x64_code_buffer, uint32_t x64_code_offset, uint32_t* x64_code_size,
    TP_X86_32_REGISTER* x86_32_register, TP_X64_32_REGISTER* x64_32_register,
    TP_WASM_STACK_ELEMENT* wasm_stack_element
);
static bool get_free_register_in_wasm_stack(
    TP_SYMBOL_TABLE* symbol_table,
    TP_X86_32_REGISTER* free_x86_32_register, TP_X64_32_REGISTER* free_x64_32_register,
    bool* is_zero_free_register_in_wasm_stack
);
static bool get_spill_register(
    TP_SYMBOL_TABLE* symbol_table,
    TP_X86_32_REGISTER* x86_32_register, TP_X64_32_REGISTER* x64_32_register
);
static bool set_nv_register(
    TP_SYMBOL_TABLE* symbol_table,
    TP_X86_32_REGISTER x86_32_register, TP_X64_32_REGISTER x64_32_register
);

static bool get_wasm_export_code_section(
    TP_SYMBOL_TABLE* symbol_table, TP_WASM_MODULE_SECTION** code_section, uint32_t* return_type
);
static bool make_wasm_module_section(
    TP_SYMBOL_TABLE* symbol_table, TP_WASM_MODULE* module, TP_WASM_MODULE_SECTION*** section
);
static TP_WASM_MODULE_SECTION** allocate_wasm_module_section(
    TP_SYMBOL_TABLE* symbol_table, TP_WASM_MODULE* module
);
static void free_wasm_module_section(
    TP_SYMBOL_TABLE* symbol_table, TP_WASM_MODULE* module, TP_WASM_MODULE_SECTION*** section
);
static bool get_wasm_module_code_section(
    TP_SYMBOL_TABLE* symbol_table,
    TP_WASM_MODULE_SECTION** section, uint32_t section_num, uint32_t* code_section_index, uint32_t* return_type
);
static bool get_wasm_module_export_section_item_index(
    TP_SYMBOL_TABLE* symbol_table,
    TP_WASM_MODULE_SECTION** section, uint32_t section_num, uint8_t* name, uint8_t kind, uint32_t* item_index
);
static bool get_wasm_module_function_section_type(
    TP_SYMBOL_TABLE* symbol_table,
    TP_WASM_MODULE_SECTION** section, uint32_t section_num, uint32_t item_index, uint32_t* type
);
static bool get_wasm_module_type_section_return_type(
    TP_SYMBOL_TABLE* symbol_table,
    TP_WASM_MODULE_SECTION** section, uint32_t section_num, uint32_t type, uint32_t* return_type
);
static bool get_wasm_module_code_section_index(
    TP_SYMBOL_TABLE* symbol_table,
    TP_WASM_MODULE_SECTION** section, uint32_t section_num, uint32_t* code_section_index
);

typedef int (*x64_jit_func)(void);

bool tp_make_x64_code(TP_SYMBOL_TABLE* symbol_table, int32_t* return_value)
{
    uint8_t* x64_code_buffer = NULL;

    uint32_t x64_code_buffer_size1 = convert_section_code_content2x64(symbol_table, NULL);

    if (0 == x64_code_buffer_size1){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("%1"), TP_LOG_PARAM_STRING("ERROR: 0 == x64_code_buffer_size1")
        );

        goto convert_error;
    }

    x64_code_buffer = (uint8_t*)VirtualAlloc(
        NULL, x64_code_buffer_size1,
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE
    );

    if (NULL == x64_code_buffer){

        TP_GET_LAST_ERROR(symbol_table);

        goto convert_error;
    }

    symbol_table->member_padding_temporary_variable_bytes =
        ((-(symbol_table->member_temporary_variable_size)) & TP_PADDING_MASK);

    uint32_t x64_code_buffer_size2 = convert_section_code_content2x64(symbol_table, x64_code_buffer);

    if (0 == x64_code_buffer_size2){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("%1"), TP_LOG_PARAM_STRING("ERROR: 0 == x64_code_buffer_size2")
        );

        goto convert_error;
    }

    if (x64_code_buffer_size1 != x64_code_buffer_size2){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("%1"), TP_LOG_PARAM_STRING("ERROR: x64_code_buffer_size1 != x64_code_buffer_size2")
        );

        goto convert_error;
    }

    if ((false ==  symbol_table->member_is_no_output_files) ||
        (symbol_table->member_is_no_output_files && symbol_table->member_is_output_x64_file)){

        if ( ! tp_write_file(
            symbol_table, symbol_table->member_x64_file_path, x64_code_buffer, x64_code_buffer_size2)){

            goto convert_error;
        }
    }

    DWORD old_protect = 0;

    if ( ! VirtualProtect(x64_code_buffer, x64_code_buffer_size2, PAGE_EXECUTE_READ, &old_protect)){

        TP_GET_LAST_ERROR(symbol_table);

        goto convert_error;
    }

    x64_jit_func func = (x64_jit_func)x64_code_buffer;

    int value = func();

    if ( ! symbol_table->member_is_no_output_messages){

        printf("x64_jit_func() = %d\n", value);
    }

    if (return_value){

        *return_value = value;
    }

    errno_t err = _set_errno(0);

    if ( ! VirtualFree(x64_code_buffer, 0, MEM_RELEASE)){

        TP_GET_LAST_ERROR(symbol_table);

        return false;
    }

    x64_code_buffer = NULL;

    return true;

convert_error:

    if (x64_code_buffer){

        if ( ! VirtualFree(x64_code_buffer, 0, MEM_RELEASE)){

            TP_GET_LAST_ERROR(symbol_table);
        }

        x64_code_buffer = NULL;
    }

    return false;
}

static uint32_t convert_section_code_content2x64(TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer)
{
    uint32_t x64_code_size = 0;

    TP_WASM_MODULE_SECTION* code_section = NULL;
    uint32_t return_type = 0;

    if ( ! get_wasm_export_code_section(symbol_table, &code_section, &return_type)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        goto error_proc;
    }

    uint8_t* payload = code_section->member_name_len_name_payload_data;
    uint32_t offset = 0;

    TP_DECODE_UI32LEB128_CHECK_VALUE(symbol_table, code_section, payload, offset, TP_WASM_SECTION_TYPE_CODE);

    uint32_t payload_len = 0;
    TP_DECODE_UI32LEB128_GET_VALUE(symbol_table, code_section, payload, offset, payload_len);

    TP_DECODE_UI32LEB128_CHECK_VALUE(symbol_table, code_section, payload, offset, TP_WASM_MODULE_SECTION_CODE_COUNT);

    uint32_t body_size = 0;
    TP_DECODE_UI32LEB128_GET_VALUE(symbol_table, code_section, payload, offset, body_size);

    if (0 == body_size){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("%1"), TP_LOG_PARAM_STRING("ERROR: 0 == body_size")
        );

        goto error_proc;
    }

    uint32_t local_count = TP_WASM_MODULE_SECTION_CODE_LOCAL_COUNT;
    uint32_t var_type = TP_WASM_MODULE_SECTION_CODE_VAR_TYPE_I32;

    TP_DECODE_UI32LEB128_CHECK_VALUE(symbol_table, code_section, payload, offset, local_count);

    uint32_t var_count = 0;
    TP_DECODE_UI32LEB128_GET_VALUE(symbol_table, code_section, payload, offset, var_count);

    TP_DECODE_UI32LEB128_CHECK_VALUE(symbol_table, code_section, payload, offset, var_type);

    uint32_t wasm_code_body_size = body_size;

    wasm_code_body_size -= tp_encode_ui32leb128(NULL, 0, local_count);
    wasm_code_body_size -= tp_encode_ui32leb128(NULL, 0, var_count);
    wasm_code_body_size -= tp_encode_ui32leb128(NULL, 0, var_type);

    uint8_t* wasm_code_body_buffer = payload + offset;

    if ( ! wasm_stack_and_use_register_init(symbol_table, wasm_code_body_buffer, wasm_code_body_size)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        goto error_proc;
    }

    uint32_t tmp_x64_code_size = tp_encode_allocate_stack(
        symbol_table, x64_code_buffer, x64_code_size, var_count, var_type
    );

    TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);

    do{
        TP_WASM_STACK_ELEMENT op1 = { 0 };
        TP_WASM_STACK_ELEMENT op2 = { 0 };

        TP_WASM_STACK_ELEMENT opcode = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_DEFAULT);

        switch (opcode.member_wasm_opcode){
        case TP_WASM_OPCODE_GET_LOCAL:
            tmp_x64_code_size = tp_encode_get_local_code(
                symbol_table, x64_code_buffer, x64_code_size, opcode.member_local_index
            );
            break;
        case TP_WASM_OPCODE_SET_LOCAL:
            op1 = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_PARAM);
            tmp_x64_code_size = tp_encode_set_local_code(
                symbol_table, x64_code_buffer, x64_code_size, opcode.member_local_index, &op1
            );
            break;
        case TP_WASM_OPCODE_TEE_LOCAL:
            op1 = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_PARAM);
            tmp_x64_code_size = tp_encode_tee_local_code(
                symbol_table, x64_code_buffer, x64_code_size, opcode.member_local_index, &op1
            );
            break;
        case TP_WASM_OPCODE_I32_CONST:
            tmp_x64_code_size = tp_encode_i32_const_code(symbol_table, x64_code_buffer, x64_code_size, opcode.member_i32);
            break;
        case TP_WASM_OPCODE_I32_ADD:
            op2 = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_PARAM);
            op1 = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_PARAM);
            tmp_x64_code_size = tp_encode_i32_add_code(symbol_table, x64_code_buffer, x64_code_size, &op1, &op2);
            break;
        case TP_WASM_OPCODE_I32_SUB:
            op2 = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_PARAM);
            op1 = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_PARAM);
            tmp_x64_code_size = tp_encode_i32_sub_code(symbol_table, x64_code_buffer, x64_code_size, &op1, &op2);
            break;
        case TP_WASM_OPCODE_I32_MUL:
            op2 = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_PARAM);
            op1 = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_PARAM);
            tmp_x64_code_size = tp_encode_i32_mul_code(symbol_table, x64_code_buffer, x64_code_size, &op1, &op2);
            break;
        case TP_WASM_OPCODE_I32_DIV:
            op2 = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_PARAM);
            op1 = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_PARAM);
            tmp_x64_code_size = tp_encode_i32_div_code(symbol_table, x64_code_buffer, x64_code_size, &op1, &op2);
            break;
        case TP_WASM_OPCODE_I32_XOR:
            op2 = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_PARAM);
            op1 = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_PARAM);
            tmp_x64_code_size = tp_encode_i32_xor_code(symbol_table, x64_code_buffer, x64_code_size, &op1, &op2);
            break;
        case TP_WASM_OPCODE_END:

            tmp_x64_code_size = tp_encode_end_code(symbol_table, x64_code_buffer, x64_code_size);

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);

            op1 = wasm_stack_pop(symbol_table, TP_WASM_STACK_POP_MODE_PARAM);

            if ( ! wasm_stack_and_wasm_code_is_empty(symbol_table)){

                TP_PUT_LOG_MSG_TRACE(symbol_table);

                goto error_proc;
            }

            return x64_code_size;
        default:

            TP_PUT_LOG_MSG_ICE(symbol_table);

            goto error_proc;
        }

        TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);

    }while (true);

error_proc:

    return 0;
}

static bool wasm_stack_and_use_register_init(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* wasm_code_body_buffer, uint32_t wasm_code_body_size)
{
    // wasm_stack_init

    if (symbol_table->member_stack){

        TP_FREE(symbol_table, &(symbol_table->member_stack), symbol_table->member_stack_size);

        symbol_table->member_stack_pos = TP_WASM_STACK_EMPTY;
        symbol_table->member_stack_size = 0;
    }

    symbol_table->member_stack = (TP_WASM_STACK_ELEMENT*)calloc(
        symbol_table->member_stack_size_allocate_unit, sizeof(TP_WASM_STACK_ELEMENT)
    );

    if (NULL == symbol_table->member_stack){

        TP_PRINT_CRT_ERROR(symbol_table);

        symbol_table->member_wasm_code_body_buffer = NULL;
        symbol_table->member_wasm_code_body_size = 0;
        symbol_table->member_wasm_code_body_pos = 0;

        symbol_table->member_stack_pos = TP_WASM_STACK_EMPTY;
        symbol_table->member_stack_size = 0;

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("%1"), TP_LOG_PARAM_STRING("ERROR: NULL == symbol_table->member_stack")
        );

        return false;
    }

    symbol_table->member_wasm_code_body_buffer = wasm_code_body_buffer;
    symbol_table->member_wasm_code_body_size = wasm_code_body_size;
    symbol_table->member_wasm_code_body_pos = 0;

    symbol_table->member_stack_pos = TP_WASM_STACK_EMPTY;
    symbol_table->member_stack_size =
        symbol_table->member_stack_size_allocate_unit * sizeof(TP_WASM_STACK_ELEMENT);

    // use_register_init

    symbol_table->member_local_variable_size = 0;
    symbol_table->member_local_variable_size_max = TP_WASM_LOCAL_VARIABLE_MAX_DEFAULT;
    symbol_table->member_padding_local_variable_bytes = 0;

//  symbol_table->member_temporary_variable_size = 0;
    symbol_table->member_temporary_variable_size_max = TP_WASM_TEMPORARY_VARIABLE_MAX_DEFAULT;
    symbol_table->member_padding_temporary_variable_bytes = 0;

    memset(symbol_table->member_use_X86_32_register, 0, sizeof(symbol_table->member_use_X86_32_register));
    memset(symbol_table->member_use_X64_32_register, 0, sizeof(symbol_table->member_use_X64_32_register));
    memset(symbol_table->member_use_nv_register, 0, sizeof(symbol_table->member_use_nv_register));

    symbol_table->member_register_bytes = 0;
    symbol_table->member_padding_register_bytes = 0;

    symbol_table->member_stack_imm32 = 0;

    return true;
}

static bool wasm_stack_and_wasm_code_is_empty(TP_SYMBOL_TABLE* symbol_table)
{
    if (TP_WASM_STACK_EMPTY == symbol_table->member_stack_pos){

        if (symbol_table->member_wasm_code_body_pos == symbol_table->member_wasm_code_body_size){

            return true;
        }
    }

    return false;
}

static bool wasm_stack_is_empty(TP_SYMBOL_TABLE* symbol_table)
{
    if (TP_WASM_STACK_EMPTY == symbol_table->member_stack_pos){

        return true;
    }

    return false;
}

bool tp_wasm_stack_push(TP_SYMBOL_TABLE* symbol_table, TP_WASM_STACK_ELEMENT* value)
{
    if (symbol_table->member_stack_pos ==
        ((symbol_table->member_stack_size / sizeof(TP_WASM_STACK_ELEMENT)) - 1)){

        int32_t wasm_stack_size_allocate_unit =
            symbol_table->member_stack_size_allocate_unit * sizeof(TP_WASM_STACK_ELEMENT);

        int32_t wasm_stack_size = symbol_table->member_stack_size + wasm_stack_size_allocate_unit;

        if (symbol_table->member_stack_size > wasm_stack_size){

            TP_PUT_LOG_MSG(
                symbol_table, TP_LOG_TYPE_DISP_FORCE,
                TP_MSG_FMT("ERROR: symbol_table->member_stack_size(%1) > wasm_stack_size(%2)"),
                TP_LOG_PARAM_UINT64_VALUE(symbol_table->member_stack_size),
                TP_LOG_PARAM_UINT64_VALUE(wasm_stack_size)
            );

            goto error_out;
        }

        TP_WASM_STACK_ELEMENT* wasm_stack = (TP_WASM_STACK_ELEMENT*)realloc(
            symbol_table->member_stack, wasm_stack_size
        );

        if (NULL == wasm_stack){

            TP_PRINT_CRT_ERROR(symbol_table);

            goto error_out;
        }

        memset(
            ((uint8_t*)wasm_stack) + symbol_table->member_stack_size, 0,
            wasm_stack_size_allocate_unit
        );

        symbol_table->member_stack = wasm_stack;
        symbol_table->member_stack_size = wasm_stack_size;
    }

    ++(symbol_table->member_stack_pos);

    symbol_table->member_stack[symbol_table->member_stack_pos] = *value;

    return true;

error_out:

    if (symbol_table->member_stack){

        TP_FREE(symbol_table, &(symbol_table->member_stack), symbol_table->member_stack_size);
    }

    symbol_table->member_stack_pos = TP_WASM_STACK_EMPTY;
    symbol_table->member_stack_size = 0;

    return false;
}

static TP_WASM_STACK_ELEMENT wasm_stack_pop(TP_SYMBOL_TABLE* symbol_table, TP_WASM_STACK_POP_MODE pop_mode)
{
    TP_WASM_STACK_ELEMENT result = { 0 };

    if (wasm_stack_and_wasm_code_is_empty(symbol_table)){

        return result;
    }

    if (TP_WASM_STACK_EMPTY < symbol_table->member_stack_pos){

        if (TP_WASM_STACK_POP_MODE_PARAM == pop_mode){

            result = symbol_table->member_stack[symbol_table->member_stack_pos];

            --(symbol_table->member_stack_pos);

            return result;
        }
    }

    result.member_wasm_opcode = symbol_table->member_wasm_code_body_buffer[symbol_table->member_wasm_code_body_pos];

    ++(symbol_table->member_wasm_code_body_pos);

    uint32_t param_size = 0;

    switch (result.member_wasm_opcode){
    case TP_WASM_OPCODE_GET_LOCAL:
//      break;
    case TP_WASM_OPCODE_SET_LOCAL:
//      break;
    case TP_WASM_OPCODE_TEE_LOCAL:

        result.member_local_index = tp_decode_ui32leb128(
            &(symbol_table->member_wasm_code_body_buffer[symbol_table->member_wasm_code_body_pos]),
            &param_size
        );

        if ((symbol_table->member_wasm_code_body_pos + param_size) >= symbol_table->member_wasm_code_body_size){

            uint64_t param1 = (uint64_t)(symbol_table->member_wasm_code_body_pos);
            uint64_t param2 = (uint64_t)param_size;

            TP_PUT_LOG_MSG(
                symbol_table, TP_LOG_TYPE_DISP_FORCE,
                TP_MSG_FMT(
                    "ERROR: TP_WASM_OPCODE_GET/SET/TEE_LOCAL: symbol_table->member_wasm_code_body_pos + param_size: %1 >= "
                    "symbol_table->member_wasm_code_body_size: %2"
                ),
                TP_LOG_PARAM_UINT64_VALUE(param1 + param2),
                TP_LOG_PARAM_UINT64_VALUE(symbol_table->member_wasm_code_body_size)
            );

            goto error_out;
        }

        symbol_table->member_wasm_code_body_pos += param_size;

        break;
    case TP_WASM_OPCODE_I32_CONST:

        result.member_i32 = tp_decode_si32leb128(
            &(symbol_table->member_wasm_code_body_buffer[symbol_table->member_wasm_code_body_pos]),
            &param_size
        );

        if ((symbol_table->member_wasm_code_body_pos + param_size) >= symbol_table->member_wasm_code_body_size){

            uint64_t param1 = (uint64_t)(symbol_table->member_wasm_code_body_pos);
            uint64_t param2 = (uint64_t)param_size;

            TP_PUT_LOG_MSG(
                symbol_table, TP_LOG_TYPE_DISP_FORCE,
                TP_MSG_FMT(
                    "ERROR: TP_WASM_OPCODE_I32_CONST: symbol_table->member_wasm_code_body_pos + param_size: %1 >= "
                    "symbol_table->member_wasm_code_body_size: %2"
                ),
                TP_LOG_PARAM_UINT64_VALUE(param1 + param2),
                TP_LOG_PARAM_UINT64_VALUE(symbol_table->member_wasm_code_body_size)
            );

            goto error_out;
        }

        symbol_table->member_wasm_code_body_pos += param_size;

        break;
    default:
        break;
    }

    return result;

error_out:

    memset(&result, 0, sizeof(result));

    return result;
}

bool tp_get_local_variable_offset(
    TP_SYMBOL_TABLE* symbol_table, uint32_t local_index, int32_t* local_variable_offset)
{
    int32_t offset = ((int32_t)local_index) * sizeof(int32_t);

    if (symbol_table->member_local_variable_size_max < offset){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("ERROR: symbol_table->member_local_variable_size_max(%1) < offset(%2)"),
            TP_LOG_PARAM_INT32_VALUE(symbol_table->member_local_variable_size_max),
            TP_LOG_PARAM_INT32_VALUE(offset)
        );

        return false;
    }

    *local_variable_offset = offset;

    return true;
}

bool tp_allocate_temporary_variable(
    TP_SYMBOL_TABLE* symbol_table, TP_X64_ALLOCATE_MODE allocate_mode,
    uint8_t* x64_code_buffer, uint32_t x64_code_offset, uint32_t* x64_code_size,
    TP_WASM_STACK_ELEMENT* wasm_stack_element)
{
    int32_t offset =
          symbol_table->member_local_variable_size
        + symbol_table->member_padding_local_variable_bytes
        + symbol_table->member_temporary_variable_size;

    int32_t max_size = 
        symbol_table->member_temporary_variable_size_max + symbol_table->member_local_variable_size_max;

    if (max_size < (offset + sizeof(int32_t))){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("ERROR: max_size: %1 < (offset + sizeof(int32_t)): %2"),
            TP_LOG_PARAM_INT32_VALUE(max_size),
            TP_LOG_PARAM_INT32_VALUE(offset + sizeof(int32_t))
        );

        return false;
    }

    wasm_stack_element->member_x64_memory_kind = TP_X64_ITEM_MEMORY_KIND_TEMP;

    wasm_stack_element->member_offset = offset;

    symbol_table->member_temporary_variable_size += sizeof(int32_t);

    if ( ! allocate_variable_common(symbol_table, allocate_mode,
        x64_code_buffer, x64_code_offset, x64_code_size, wasm_stack_element)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    return true;
}

static bool allocate_variable_common(
    TP_SYMBOL_TABLE* symbol_table, TP_X64_ALLOCATE_MODE allocate_mode,
    uint8_t* x64_code_buffer, uint32_t x64_code_offset, uint32_t* x64_code_size,
    TP_WASM_STACK_ELEMENT* wasm_stack_element)
{
    switch (allocate_mode){
    case TP_X64_ALLOCATE_DEFAULT:
        break;
    case TP_X64_ALLOCATE_MEMORY:
        wasm_stack_element->member_x64_item_kind = TP_X64_ITEM_KIND_MEMORY;
        return true;
    default:
        TP_PUT_LOG_MSG_ICE(symbol_table);
        return false;
    }

    bool is_zero_free_register = true;

    TP_X86_32_REGISTER x86_32_register = TP_X86_32_REGISTER_NULL;
    TP_X64_32_REGISTER x64_32_register = TP_X64_32_REGISTER_NULL;

    if ( ! get_free_register(symbol_table, &x86_32_register, &x64_32_register, &is_zero_free_register)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    if (is_zero_free_register){

        if ( ! spilling_variable(
            symbol_table, x64_code_buffer, x64_code_offset, x64_code_size,
            &x86_32_register, &x64_32_register, wasm_stack_element)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            return false;
        }
    }

    if (TP_X86_32_REGISTER_NULL != x86_32_register){

        wasm_stack_element->member_x64_item_kind = TP_X64_ITEM_KIND_X86_32_REGISTER;
        wasm_stack_element->member_x64_item.member_x86_32_register = x86_32_register;

        symbol_table->member_use_X86_32_register[x86_32_register] = *wasm_stack_element;

    }else if (TP_X64_32_REGISTER_NULL != x64_32_register){

        wasm_stack_element->member_x64_item_kind = TP_X64_ITEM_KIND_X64_32_REGISTER;
        wasm_stack_element->member_x64_item.member_x64_32_register = x64_32_register;

        symbol_table->member_use_X64_32_register[x64_32_register] = *wasm_stack_element;
    }else{

        return true;
    }

    if ( ! set_nv_register(symbol_table, x86_32_register, x64_32_register)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    return true;
}

static bool get_free_register(
    TP_SYMBOL_TABLE* symbol_table,
    TP_X86_32_REGISTER* x86_32_register, TP_X64_32_REGISTER* x64_32_register, bool* is_zero_free_register)
{
    for (uint32_t i = 0; TP_X86_32_REGISTER_NUM > i; ++i){

        TP_WASM_STACK_ELEMENT* use_x86_32_register = &(symbol_table->member_use_X86_32_register[i]);

        if (TP_X64_ITEM_KIND_X86_32_REGISTER != use_x86_32_register->member_x64_item_kind){

            TP_X64_ITEM* x64_item = &(use_x86_32_register->member_x64_item);

            if ((TP_X86_32_REGISTER_ESP == x64_item->member_x86_32_register) ||
                (TP_X86_32_REGISTER_EBP == x64_item->member_x86_32_register)){

                continue;
            }

            *is_zero_free_register = false;

            *x86_32_register = (TP_X86_32_REGISTER)i;

            break;
        }
    }

    if (*is_zero_free_register){

        for (uint32_t i = 0; TP_X64_32_REGISTER_NUM > i; ++i){

            TP_WASM_STACK_ELEMENT* use_x64_32_register = &(symbol_table->member_use_X64_32_register[i]);

            if (TP_X64_ITEM_KIND_X64_32_REGISTER != use_x64_32_register->member_x64_item_kind){

                *is_zero_free_register = false;

                *x64_32_register = (TP_X64_32_REGISTER)i;

                break;
            }
        }
    }

    return true;
}

static bool spilling_variable(
    TP_SYMBOL_TABLE* symbol_table,
    uint8_t* x64_code_buffer, uint32_t x64_code_offset, uint32_t* x64_code_size,
    TP_X86_32_REGISTER* x86_32_register, TP_X64_32_REGISTER* x64_32_register,
    TP_WASM_STACK_ELEMENT* wasm_stack_element)
{
    TP_X86_32_REGISTER spill_x86_32_register = TP_X86_32_REGISTER_NULL;
    TP_X64_32_REGISTER spill_x64_32_register = TP_X64_32_REGISTER_NULL;

    if ( ! wasm_stack_is_empty(symbol_table)){

        bool is_zero_free_register_in_wasm_stack = true;

        if ( ! get_free_register_in_wasm_stack(
            symbol_table, &spill_x86_32_register, &spill_x64_32_register,
            &is_zero_free_register_in_wasm_stack)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            return false;
        }

        if (is_zero_free_register_in_wasm_stack){

            wasm_stack_element->member_x64_item_kind = TP_X64_ITEM_KIND_MEMORY;

            return true;
        }
    }

    if ((TP_X86_32_REGISTER_NULL == spill_x86_32_register) &&
        (TP_X64_32_REGISTER_NULL == spill_x64_32_register)){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("%1"),
            TP_LOG_PARAM_STRING(
                "ERROR: (TP_X86_32_REGISTER_NULL == spill_x86_32_register) && "
                "(TP_X64_32_REGISTER_NULL == spill_x64_32_register)"
            )
        );

        return false;
    }

    TP_WASM_STACK_ELEMENT dst = { 0 };
    TP_WASM_STACK_ELEMENT* src = NULL;

    if (TP_X86_32_REGISTER_NULL != spill_x86_32_register){

        dst = symbol_table->member_use_X86_32_register[spill_x86_32_register];
        src = &(symbol_table->member_use_X86_32_register[spill_x86_32_register]);
    }else if (TP_X64_32_REGISTER_NULL != spill_x64_32_register){

        dst = symbol_table->member_use_X64_32_register[spill_x64_32_register];
        src = &(symbol_table->member_use_X64_32_register[spill_x64_32_register]);
    }else{

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT(
                "ERROR: (false == (TP_X86_32_REGISTER_NULL != spill_x86_32_register: %1)) &&"
                "(false == (TP_X64_32_REGISTER_NULL != spill_x64_32_register: %2))"
            ),
            TP_LOG_PARAM_INT32_VALUE(spill_x86_32_register),
            TP_LOG_PARAM_INT32_VALUE(spill_x64_32_register)
        );

        return false;
    }

    dst.member_wasm_opcode = TP_WASM_OPCODE_I32_VALUE;
    dst.member_x64_item_kind = TP_X64_ITEM_KIND_MEMORY;

    src->member_wasm_opcode = TP_WASM_OPCODE_I32_VALUE;

    // NOTE: Call tp_free_register(src) in tp_encode_x64_2_operand().
    uint32_t mov_code_size = tp_encode_x64_2_operand(
        symbol_table, x64_code_buffer, x64_code_offset,
        TP_X64_MOV, &dst, src
    );

    if (0 == mov_code_size){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("%1"),
            TP_LOG_PARAM_STRING("ERROR: 0 == mov_code_size")
        );

        return false;
    }

    *x64_code_size += mov_code_size;

    src->member_x64_item_kind = TP_X64_ITEM_KIND_MEMORY;

    return true;
}

static bool get_free_register_in_wasm_stack(
    TP_SYMBOL_TABLE* symbol_table,
    TP_X86_32_REGISTER* free_x86_32_register, TP_X64_32_REGISTER* free_x64_32_register, bool* is_zero_free_register_in_wasm_stack)
{
    bool x86_32_register[TP_X86_32_REGISTER_NUM] = { false };
    bool x64_32_register[TP_X64_32_REGISTER_NUM] = { false };

    for (size_t i = 0; symbol_table->member_stack_size > i; ++i){

        TP_WASM_STACK_ELEMENT* wasm_stack_element = &(symbol_table->member_stack[i]);

        switch (wasm_stack_element->member_x64_item_kind){
        case TP_X64_ITEM_KIND_X86_32_REGISTER:{

            TP_X86_32_REGISTER temp_x86_32_register = wasm_stack_element->member_x64_item.member_x86_32_register;

            if (TP_X86_32_REGISTER_NULL <= temp_x86_32_register){

                TP_PUT_LOG_MSG(
                    symbol_table, TP_LOG_TYPE_DISP_FORCE,
                    TP_MSG_FMT("ERROR: TP_X86_32_REGISTER_NULL <= temp_x86_32_register(%1)"),
                    TP_LOG_PARAM_INT32_VALUE(temp_x86_32_register)
                );

                return false;
            }

            x86_32_register[temp_x86_32_register] = true;

            break;
        }
        case TP_X64_ITEM_KIND_X64_32_REGISTER:{

            TP_X64_32_REGISTER temp_x64_32_register = wasm_stack_element->member_x64_item.member_x64_32_register;

            if (TP_X64_32_REGISTER_NULL <= temp_x64_32_register){

                TP_PUT_LOG_MSG(
                    symbol_table, TP_LOG_TYPE_DISP_FORCE,
                    TP_MSG_FMT("ERROR: TP_X64_32_REGISTER_NULL <= temp_x64_32_register(%1)"),
                    TP_LOG_PARAM_INT32_VALUE(temp_x64_32_register)
                );

                return false;
            }

            x64_32_register[temp_x64_32_register] = true;

            break;
        }
        default:
            break;
        }
    }

    bool is_zero_free_x86_32_register = true;

    const int32_t bad_index = -1;
    int32_t free_x86_32_index = bad_index;

    for (int32_t i = 0; TP_X86_32_REGISTER_NUM > i; ++i){

        if (false == x86_32_register[i]){

            is_zero_free_x86_32_register = false;

            free_x86_32_index = i;

            break;
        }
    }

    bool is_zero_free_x64_32_register = true;
    int32_t free_x64_32_index = bad_index;

    for (int32_t i = 0; TP_X64_32_REGISTER_NUM > i; ++i){

        if (false == x64_32_register[i]){

            is_zero_free_x64_32_register = false;

            free_x64_32_index = i;

            break;
        }
    }

    if (is_zero_free_x86_32_register && is_zero_free_x64_32_register){

        *is_zero_free_register_in_wasm_stack = true;

        return true;
    }

    if ((false == is_zero_free_x86_32_register) && (bad_index != free_x86_32_index)){

        *free_x86_32_register = free_x86_32_index;

        return true;
    }

    if ((false == is_zero_free_x64_32_register) && (bad_index != free_x64_32_index)){

        *free_x64_32_register = free_x64_32_index;

        return true;
    }

    TP_PUT_LOG_MSG(
        symbol_table, TP_LOG_TYPE_DISP_FORCE,
        TP_MSG_FMT("ERROR: at %1"), TP_LOG_PARAM_STRING(__func__)
    );

    return false;
}

static bool set_nv_register(
    TP_SYMBOL_TABLE* symbol_table,
    TP_X86_32_REGISTER x86_32_register, TP_X64_32_REGISTER x64_32_register)
{
    switch (x86_32_register){
    case TP_X86_32_REGISTER_EBX:
        symbol_table->member_use_nv_register[TP_X64_NV64_REGISTER_RBX_INDEX] = TP_X64_NV64_REGISTER_RBX;
        break;
    case TP_X86_32_REGISTER_ESI:
        symbol_table->member_use_nv_register[TP_X64_NV64_REGISTER_RSI_INDEX] = TP_X64_NV64_REGISTER_RSI;
        break;
    case TP_X86_32_REGISTER_EDI:
        symbol_table->member_use_nv_register[TP_X64_NV64_REGISTER_RDI_INDEX] = TP_X64_NV64_REGISTER_RDI;
        break;
    default:
        break;
    }

    switch (x64_32_register){
    case TP_X64_32_REGISTER_R12D:
        symbol_table->member_use_nv_register[TP_X64_NV64_REGISTER_R12_INDEX] = TP_X64_NV64_REGISTER_R12;
        break;
    case TP_X64_32_REGISTER_R13D:
        symbol_table->member_use_nv_register[TP_X64_NV64_REGISTER_R13_INDEX] = TP_X64_NV64_REGISTER_R13;
        break;
    case TP_X64_32_REGISTER_R14D:
        symbol_table->member_use_nv_register[TP_X64_NV64_REGISTER_R14_INDEX] = TP_X64_NV64_REGISTER_R14;
        break;
    case TP_X64_32_REGISTER_R15D:
        symbol_table->member_use_nv_register[TP_X64_NV64_REGISTER_R15_INDEX] = TP_X64_NV64_REGISTER_R15;
        break;
    default:
        break;
    }

    return true;
}

bool tp_free_register(TP_SYMBOL_TABLE* symbol_table, TP_WASM_STACK_ELEMENT* stack_element)
{
    switch (stack_element->member_x64_item_kind){
    case TP_X64_ITEM_KIND_X86_32_REGISTER:{

        TP_X86_32_REGISTER x86_32_register = stack_element->member_x64_item.member_x86_32_register;

        if (TP_X86_32_REGISTER_NULL <= x86_32_register){

            TP_PUT_LOG_MSG(
                symbol_table, TP_LOG_TYPE_DISP_FORCE,
                TP_MSG_FMT("ERROR: TP_X86_32_REGISTER_NULL <= x86_32_register(%1)"),
                TP_LOG_PARAM_INT32_VALUE(x86_32_register)
            );

            return false;
        }

        symbol_table->member_use_X86_32_register[x86_32_register].member_x64_item_kind = TP_X64_ITEM_KIND_MEMORY;
        symbol_table->member_use_X86_32_register[x86_32_register].member_x64_item.member_x86_32_register
            = TP_X86_32_REGISTER_NULL;

        stack_element->member_x64_item_kind = TP_X64_ITEM_KIND_MEMORY;
        stack_element->member_x64_item.member_x86_32_register = TP_X86_32_REGISTER_NULL;
        break;
    }
    case TP_X64_ITEM_KIND_X64_32_REGISTER:{

        TP_X64_32_REGISTER x64_32_register = stack_element->member_x64_item.member_x64_32_register;

        if (TP_X86_32_REGISTER_NULL <= x64_32_register){

            TP_PUT_LOG_MSG(
                symbol_table, TP_LOG_TYPE_DISP_FORCE,
                TP_MSG_FMT("ERROR: TP_X86_32_REGISTER_NULL <= x64_32_register(%1)"),
                TP_LOG_PARAM_INT32_VALUE(x64_32_register)
            );

            return false;
        }

        symbol_table->member_use_X64_32_register[x64_32_register].member_x64_item_kind = TP_X64_ITEM_KIND_MEMORY;
        symbol_table->member_use_X86_32_register[x64_32_register].member_x64_item.member_x64_32_register
            = TP_X64_32_REGISTER_NULL;

        stack_element->member_x64_item_kind = TP_X64_ITEM_KIND_MEMORY;
        stack_element->member_x64_item.member_x64_32_register = TP_X64_32_REGISTER_NULL;
        break;
    }
    case TP_X64_ITEM_KIND_MEMORY:
        break;
    default:
        TP_PUT_LOG_MSG_ICE(symbol_table);
        return false;
    }

    return true;
}

static bool get_wasm_export_code_section(
    TP_SYMBOL_TABLE* symbol_table, TP_WASM_MODULE_SECTION** code_section, uint32_t* return_type)
{
    TP_WASM_MODULE* module = &(symbol_table->member_wasm_module);

    TP_WASM_MODULE_CONTENT* module_content = module->member_module_content;

    uint8_t magic_number[sizeof(TP_WASM_MODULE_MAGIC_NUMBER)];
    memset(magic_number, 0, sizeof(magic_number));

    memcpy(magic_number, &(module_content->member_magic_number), sizeof(uint32_t));
    magic_number[sizeof(TP_WASM_MODULE_MAGIC_NUMBER) - 1] = '\0';

    if ((0 != strcmp(TP_WASM_MODULE_MAGIC_NUMBER, magic_number)) ||
        (TP_WASM_MODULE_VERSION != module_content->member_version)){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT(
                "ERROR: Bad value. module_content->member_magic_number(%1), "
                "module_content->member_version(%2)."
            ),
            TP_LOG_PARAM_STRING(magic_number),
            TP_LOG_PARAM_UINT64_VALUE(module_content->member_version)
        );

        return false;
    }

    if (NULL == module->member_section){

        if ( ! make_wasm_module_section(symbol_table, module, NULL)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            return false;
        }

        TP_WASM_MODULE_SECTION** section = NULL;

        if ( ! make_wasm_module_section(symbol_table, module, &section)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            return false;
        }

        module->member_section = section;
    }

    uint32_t code_section_index = 0;

    if ( ! get_wasm_module_code_section(
        symbol_table,
        module->member_section, module->member_section_num, &code_section_index, return_type)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    *code_section = module->member_section[code_section_index];

    return true;
}

static bool make_wasm_module_section(
    TP_SYMBOL_TABLE* symbol_table, TP_WASM_MODULE* module, TP_WASM_MODULE_SECTION*** section)
{
    if (section){

        TP_WASM_MODULE_SECTION** tmp_section = allocate_wasm_module_section(symbol_table, module);

        if (NULL == tmp_section){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            return false;
        }

        *section = tmp_section;

    }else if (0 != module->member_section_num){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("ERROR: 0 != module->member_section_num(%1)"),
            TP_LOG_PARAM_UINT64_VALUE(module->member_section_num)
        );

        return false;
    }

    TP_WASM_MODULE_CONTENT* module_content = module->member_module_content;

    uint8_t* module_content_payload = module_content->member_payload;

    uint32_t current_size = (sizeof(module_content->member_magic_number) +
            sizeof(module_content->member_version));

    uint32_t section_num = 0;

    do{
        uint32_t size_id = 0;

        uint32_t id = tp_decode_ui32leb128(module_content_payload, &size_id);

        if (TP_WASM_SECTION_TYPE_CUSTOM == id){

            // NOTE: Not implemented.

            TP_PUT_LOG_MSG(
                symbol_table, TP_LOG_TYPE_DISP_FORCE,
                TP_MSG_FMT("%1"),
                TP_LOG_PARAM_STRING("ERROR: TP_WASM_SECTION_TYPE_CUSTOM is not implemented.")
            );

            return false;
        }

        uint32_t size_payload_len = 0;

        uint32_t payload_len = tp_decode_ui32leb128(module_content_payload + size_id, &size_payload_len);

        uint32_t section_size = size_id + size_payload_len + payload_len;

        if (section){

            (*section)[section_num]->member_section_size = section_size;
            (*section)[section_num]->member_id = id;
            (*section)[section_num]->member_payload_len = payload_len;

            // NOTE: Not implemented.
            // name_len: 0 == member_id
            // name: 0 == member_id

            uint8_t* tmp_payload = (uint8_t*)calloc(payload_len, sizeof(uint8_t));

            if (NULL == tmp_payload){

                TP_PRINT_CRT_ERROR(symbol_table);

                free_wasm_module_section(symbol_table, module, section);

                return false;
            }

            (*section)[section_num]->member_name_len_name_payload_data = tmp_payload;

            memcpy((*section)[section_num]->member_name_len_name_payload_data,
                module_content_payload + size_id + size_payload_len, payload_len
            );
        }

        current_size += section_size;

        if (module->member_content_size < current_size){

            TP_PUT_LOG_MSG(
                symbol_table, TP_LOG_TYPE_DISP_FORCE,
                TP_MSG_FMT("ERROR: module->member_content_size(%1) < current_size(%2)"),
                TP_LOG_PARAM_UINT64_VALUE(module->member_content_size),
                TP_LOG_PARAM_UINT64_VALUE(current_size)
            );

            return false;
        }

        module_content_payload -= section_size;

        ++section_num;

    }while (module->member_content_size != current_size);

    module->member_section_num = section_num;

    return true;
}

static TP_WASM_MODULE_SECTION** allocate_wasm_module_section(
    TP_SYMBOL_TABLE* symbol_table, TP_WASM_MODULE* module)
{
    TP_WASM_MODULE_SECTION** tmp_section =
        (TP_WASM_MODULE_SECTION**)calloc(module->member_section_num, sizeof(TP_WASM_MODULE_SECTION*));

    if (NULL == tmp_section){

        module->member_section_num = 0;

        TP_PRINT_CRT_ERROR(symbol_table);

        return false;
    }

    bool status = true;

    for (uint32_t i = 0; module->member_section_num > i; ++i){

        tmp_section[i] = (TP_WASM_MODULE_SECTION*)calloc(1, sizeof(TP_WASM_MODULE_SECTION));

        if (NULL == tmp_section[i]){

            status = false;

            TP_PRINT_CRT_ERROR(symbol_table);
        }
    }

    if ( ! status){

        free_wasm_module_section(symbol_table, module, &tmp_section);

        return false;
    }

    return  tmp_section;
}

static void free_wasm_module_section(
    TP_SYMBOL_TABLE* symbol_table, TP_WASM_MODULE* module, TP_WASM_MODULE_SECTION*** section)
{
    if (NULL == section){

        return;
    }

    for (uint32_t i = 0; module->member_section_num > i; ++i){

        if (section[i]){

            TP_FREE(symbol_table, &((*section)[i]->member_name_len_name_payload_data), (*section)[i]->member_section_size);

            TP_FREE(symbol_table, &(*section)[i], sizeof(TP_WASM_MODULE_SECTION));
        }
    }

    TP_FREE2(symbol_table, section, module->member_section_num * sizeof(TP_WASM_MODULE_SECTION*));

    module->member_section_num = 0;
}

static bool get_wasm_module_code_section(
    TP_SYMBOL_TABLE* symbol_table,
    TP_WASM_MODULE_SECTION** section, uint32_t section_num, uint32_t* code_section_index, uint32_t* return_type)
{
    uint32_t item_index = 0;

    if ( ! get_wasm_module_export_section_item_index(
        symbol_table,
        section, section_num, TP_WASM_MODULE_SECTION_EXPORT_NAME_2, TP_WASM_SECTION_KIND_FUNCTION, &item_index)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    uint32_t type = 0;

    if ( ! get_wasm_module_function_section_type(symbol_table, section, section_num, item_index, &type)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    if ( ! get_wasm_module_type_section_return_type(symbol_table, section, section_num, type, return_type)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    if ( ! get_wasm_module_code_section_index(symbol_table, section, section_num, code_section_index)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    return true;
}

static bool get_wasm_module_export_section_item_index(
    TP_SYMBOL_TABLE* symbol_table,
    TP_WASM_MODULE_SECTION** section, uint32_t section_num, uint8_t* name, uint8_t kind, uint32_t* item_index)
{
    TP_WASM_MODULE_SECTION* export_section = NULL;

    for (uint32_t i = 0; section_num > i; ++i){

        if (TP_WASM_SECTION_TYPE_EXPORT == section[i]->member_id){

            export_section = section[i];

            break;
        }
    }

    if (NULL == export_section){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("%1"),
            TP_LOG_PARAM_STRING("ERROR: NULL == export_section")
        );

        return false;
    }

    uint8_t* payload = export_section->member_name_len_name_payload_data;
    uint32_t offset = 0;

    TP_DECODE_UI32LEB128_CHECK_VALUE(symbol_table, export_section, payload, offset, TP_WASM_SECTION_TYPE_EXPORT);

    uint32_t payload_len = 0;
    TP_DECODE_UI32LEB128_GET_VALUE(symbol_table, export_section, payload, offset, payload_len);

    uint32_t count = 0;
    TP_DECODE_UI32LEB128_GET_VALUE(symbol_table, export_section, payload, offset, count);

    if (0 == count){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("%1"),
            TP_LOG_PARAM_STRING("ERROR: 0 == count")
        );

        return false;
    }

    for (uint32_t i = 0; count > i; ++i){

        uint32_t name_length = 0;
        TP_DECODE_UI32LEB128_GET_VALUE(symbol_table, export_section, payload, offset, name_length);

        bool is_match = false;
        TP_WASM_CHECK_STRING(symbol_table, export_section, payload, offset, name, name_length, is_match);

        uint32_t export_kind = 0;
        TP_DECODE_UI32LEB128_GET_VALUE(symbol_table, export_section, payload, offset, export_kind);

        uint32_t export_item_index = 0;
        TP_DECODE_UI32LEB128_GET_VALUE(symbol_table, export_section, payload, offset, export_item_index);

        if (is_match && (kind == export_kind)){

            *item_index = export_item_index;

            return true;
        }
    }

    TP_PUT_LOG_MSG(
        symbol_table, TP_LOG_TYPE_DISP_FORCE,
        TP_MSG_FMT("%1"),
        TP_LOG_PARAM_STRING("ERROR: WASM module export section item not found.")
    );

    return false;
}

static bool get_wasm_module_function_section_type(
    TP_SYMBOL_TABLE* symbol_table,
    TP_WASM_MODULE_SECTION** section, uint32_t section_num, uint32_t item_index, uint32_t* type)
{
    TP_WASM_MODULE_SECTION* function_section = NULL;

    for (uint32_t i = 0; section_num > i; ++i){

        if (TP_WASM_SECTION_TYPE_FUNCTION == section[i]->member_id){

            function_section = section[i];

            break;
        }
    }

    if (NULL == function_section){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("%1"),
            TP_LOG_PARAM_STRING("ERROR: WASM module type section function item not found.")
        );

        return false;
    }

    uint8_t* payload = function_section->member_name_len_name_payload_data;
    uint32_t offset = 0;

    TP_DECODE_UI32LEB128_CHECK_VALUE(symbol_table, function_section, payload, offset, TP_WASM_SECTION_TYPE_FUNCTION);

    uint32_t payload_len = 0;
    TP_DECODE_UI32LEB128_GET_VALUE(symbol_table, function_section, payload, offset, payload_len);

    TP_DECODE_UI32LEB128_CHECK_VALUE(
        symbol_table, function_section, payload, offset, TP_WASM_MODULE_SECTION_FUNCTION_COUNT
    );
    TP_DECODE_UI32LEB128_CHECK_VALUE(
        symbol_table, function_section, payload, offset, TP_WASM_MODULE_SECTION_FUNCTION_TYPES
    );

    *type = 0;

    return true;
}

static bool get_wasm_module_type_section_return_type(
    TP_SYMBOL_TABLE* symbol_table,
    TP_WASM_MODULE_SECTION** section, uint32_t section_num, uint32_t type, uint32_t* return_type)
{
    TP_WASM_MODULE_SECTION* type_section = NULL;

    for (uint32_t i = 0; section_num > i; ++i){

        if (TP_WASM_SECTION_TYPE_TYPE == section[i]->member_id){

            type_section = section[i];

            break;
        }
    }

    if (NULL == type_section){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("%1"),
            TP_LOG_PARAM_STRING("ERROR: WASM module type section type item not found.")
        );

        return false;
    }

    uint8_t* payload = type_section->member_name_len_name_payload_data;
    uint32_t offset = 0;

    TP_DECODE_UI32LEB128_CHECK_VALUE(symbol_table, type_section, payload, offset, TP_WASM_SECTION_TYPE_TYPE);

    uint32_t payload_len = 0;
    TP_DECODE_UI32LEB128_GET_VALUE(symbol_table, type_section, payload, offset, payload_len);

    TP_DECODE_UI32LEB128_CHECK_VALUE(symbol_table, type_section, payload, offset, TP_WASM_MODULE_SECTION_TYPE_COUNT);
    TP_DECODE_UI32LEB128_CHECK_VALUE(symbol_table, type_section, payload, offset, TP_WASM_MODULE_SECTION_TYPE_FORM_FUNC);
    TP_DECODE_UI32LEB128_CHECK_VALUE(symbol_table, type_section, payload, offset, TP_WASM_MODULE_SECTION_TYPE_PARAM_COUNT);

//  uint32_t param_types;

    TP_DECODE_UI32LEB128_CHECK_VALUE(symbol_table, type_section, payload, offset, TP_WASM_MODULE_SECTION_TYPE_RETURN_COUNT);
    TP_DECODE_UI32LEB128_CHECK_VALUE(symbol_table, type_section, payload, offset, TP_WASM_MODULE_SECTION_TYPE_RETURN_TYPE_I32);

    *return_type = TP_WASM_MODULE_SECTION_TYPE_RETURN_TYPE_I32;

    return true;
}

static bool get_wasm_module_code_section_index(
    TP_SYMBOL_TABLE* symbol_table,
    TP_WASM_MODULE_SECTION** section, uint32_t section_num, uint32_t* code_section_index)
{
    for (uint32_t i = 0; section_num > i; ++i){

        if (TP_WASM_SECTION_TYPE_CODE == section[i]->member_id){

            *code_section_index = i;

            return true;
        }
    }

    TP_PUT_LOG_MSG(
        symbol_table, TP_LOG_TYPE_DISP_FORCE,
        TP_MSG_FMT("%1"),
        TP_LOG_PARAM_STRING("ERROR: WASM module code section item not found.")
    );

    return false;
}

