
// Copyright (C) 2018 Shin'ichi Ichikawa. Released under the MIT license.

#include "tp_compiler.h"

static uint32_t encode_x64_32_register_to_x64_32_register(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64 x64_op, TP_WASM_STACK_ELEMENT* dst, TP_WASM_STACK_ELEMENT* src
);
static uint32_t encode_x64_32_register_to_memory_offset(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64 x64_op, TP_WASM_STACK_ELEMENT* dst, TP_WASM_STACK_ELEMENT* src
);
static uint32_t encode_x64_32_memory_offset_to_register(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64 x64_op, TP_WASM_STACK_ELEMENT* dst, TP_WASM_STACK_ELEMENT* src
);
static uint32_t encode_x64_32_memory_offset_common(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64 x64_op, TP_X64_DIRECTION x64_direction, TP_WASM_STACK_ELEMENT* dst, TP_WASM_STACK_ELEMENT* src
);

typedef enum tp_x64_mov_imm_mode_{
    TP_X64_MOV_IMM_MODE_DEFAULT,
    TP_X64_MOV_IMM_MODE_FORCE_IMM32
}TP_X64_MOV_IMM_MODE;

static uint32_t encode_x64_mov_imm(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    int32_t imm, TP_X64_MOV_IMM_MODE x64_mov_imm_mode, TP_WASM_STACK_ELEMENT* result
);

typedef enum tp_x64_add_sub_imm_mode_{
    TP_X64_ADD_SUB_IMM_MODE_DEFAULT,
    TP_X64_ADD_SUB_IMM_MODE_FORCE_IMM32
}TP_X64_ADD_SUB_IMM_MODE;

static uint32_t encode_x64_add_sub_imm(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64 x64_op, TP_X64_64_REGISTER reg64, int32_t imm, TP_X64_ADD_SUB_IMM_MODE x64_add_sub_imm_mode
);

static uint32_t encode_x64_lea(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64_64_REGISTER reg64_dst_reg, TP_X64_64_REGISTER reg64_src_index, TP_X64_64_REGISTER reg64_src_base, int32_t offset
);
static uint32_t encode_x64_push_reg64(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset, TP_X64_64_REGISTER reg64
);
static uint32_t encode_x64_pop_reg64(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset, TP_X64_64_REGISTER reg64
);
static uint32_t encode_x64_1_opcode(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset, uint8_t opcode
);

uint32_t tp_encode_allocate_stack(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    uint32_t var_count, uint32_t var_type)
{
    if (TP_WASM_MODULE_SECTION_CODE_VAR_TYPE_I32 != var_type){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("ERROR: TP_WASM_MODULE_SECTION_CODE_VAR_TYPE_I32(%1) != var_type(%2)"),
            TP_LOG_PARAM_UINT64_VALUE(TP_WASM_MODULE_SECTION_CODE_VAR_TYPE_I32),
            TP_LOG_PARAM_UINT64_VALUE(var_type)
        );

        return 0;
    }

    uint32_t local_variable_size = var_count * sizeof(int32_t);

    if (INT32_MAX < local_variable_size){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("ERROR: INT32_MAX(%1) < temp_local_variable_size(%2)"),
            TP_LOG_PARAM_INT32_VALUE(INT32_MAX),
            TP_LOG_PARAM_UINT64_VALUE(local_variable_size)
        );

        return 0;
    }

    symbol_table->member_local_variable_size = (int32_t)local_variable_size;

    symbol_table->member_padding_local_variable_bytes =
        ((-(symbol_table->member_local_variable_size)) & TP_PADDING_MASK);

    // PUSH – Push Operand onto the Stack
    uint32_t x64_code_size = encode_x64_push_reg64(
        symbol_table, x64_code_buffer, x64_code_offset, TP_X64_64_REGISTER_RBP
    );

    {
        int32_t nv_register_bytes = 0;

        for (int32_t i = TP_X64_NV64_REGISTER_NUM; TP_X64_NV64_REGISTER_NULL < i; --i){

            switch (symbol_table->member_use_nv_register[i]){
            case TP_X64_NV64_REGISTER_NULL:
                break;
            case TP_X64_NV64_REGISTER_RBX:
//              break;
            case TP_X64_NV64_REGISTER_RSI:
//              break;
            case TP_X64_NV64_REGISTER_RDI:
//              break;
            case TP_X64_NV64_REGISTER_R12:
//              break;
            case TP_X64_NV64_REGISTER_R13:
//                break;
            case TP_X64_NV64_REGISTER_R14:
//                break;
            case TP_X64_NV64_REGISTER_R15:
                // PUSH – Push Operand onto the Stack
                x64_code_size += encode_x64_push_reg64(
                    symbol_table, x64_code_buffer, x64_code_offset + x64_code_size,
                    (TP_X64_64_REGISTER)(symbol_table->member_use_nv_register[i])
                );
                ++nv_register_bytes;
                break;
            default:
                TP_PUT_LOG_MSG_ICE(symbol_table);
                return 0;
            }

            symbol_table->member_register_bytes = nv_register_bytes * sizeof(uint64_t);

            symbol_table->member_padding_register_bytes = ((-nv_register_bytes) & TP_PADDING_MASK);
        }
    }

    const int32_t stack_param_size = 32;

    symbol_table->member_stack_imm32 =
        symbol_table->member_local_variable_size +
        symbol_table->member_padding_local_variable_bytes +
        symbol_table->member_temporary_variable_size +
        symbol_table->member_padding_temporary_variable_bytes +
        symbol_table->member_register_bytes +
        symbol_table->member_padding_register_bytes +
        stack_param_size;

    if (x64_code_buffer){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_HIDE,
            TP_MSG_FMT(
                "sub rsp, imm32\n"
                "symbol_table->member_stack_imm32: %1\n"
                "symbol_table->member_local_variable_size: %2\n"
                "symbol_table->member_padding_local_variable_bytes: %3\n"
                "symbol_table->member_temporary_variable_size: %4\n"
                "symbol_table->member_padding_temporary_variable_bytes: %5\n"
                "symbol_table->member_register_bytes: %6\n"
                "symbol_table->member_padding_register_bytes: %7\n"
                "stack_param_size: %8"
            ),
            TP_LOG_PARAM_INT32_VALUE(symbol_table->member_stack_imm32),
            TP_LOG_PARAM_INT32_VALUE(symbol_table->member_local_variable_size),
            TP_LOG_PARAM_INT32_VALUE(symbol_table->member_padding_local_variable_bytes),
            TP_LOG_PARAM_INT32_VALUE(symbol_table->member_temporary_variable_size),
            TP_LOG_PARAM_INT32_VALUE(symbol_table->member_padding_temporary_variable_bytes),
            TP_LOG_PARAM_INT32_VALUE(symbol_table->member_register_bytes),
            TP_LOG_PARAM_INT32_VALUE(symbol_table->member_padding_register_bytes),
            TP_LOG_PARAM_INT32_VALUE(stack_param_size)
        );
    }

    // SUB – Integer Subtraction
    x64_code_size += encode_x64_add_sub_imm(
        symbol_table, x64_code_buffer, x64_code_offset + x64_code_size,
        TP_X64_SUB, TP_X64_64_REGISTER_RSP, symbol_table->member_stack_imm32, TP_X64_ADD_SUB_IMM_MODE_FORCE_IMM32
    );
    // LEA - Load Effective Address
    // lea rbp, QWORD PTR [rsp+32]
    x64_code_size += encode_x64_lea(
        symbol_table, x64_code_buffer, x64_code_offset + x64_code_size,
        TP_X64_64_REGISTER_RBP, TP_X64_64_REGISTER_INDEX_NONE,
        TP_X64_64_REGISTER_RSP, stack_param_size
    );

    return x64_code_size;
}

uint32_t tp_encode_get_local_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset, uint32_t local_index)
{
    TP_WASM_STACK_ELEMENT dst = { .member_wasm_opcode = TP_WASM_OPCODE_I32_VALUE };

    uint32_t x64_code_size = 0;

    if ( ! tp_allocate_temporary_variable(
        symbol_table, TP_X64_ALLOCATE_DEFAULT, x64_code_buffer, x64_code_offset, &x64_code_size, &dst)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    TP_WASM_STACK_ELEMENT src = {
        .member_wasm_opcode = TP_WASM_OPCODE_I32_VALUE,
        .member_local_index = local_index,
        .member_x64_item_kind = TP_X64_ITEM_KIND_MEMORY,
        .member_x64_memory_kind = TP_X64_ITEM_MEMORY_KIND_LOCAL
    };

    int32_t offset = 0;

    if ( ! tp_get_local_variable_offset(symbol_table, local_index, &offset)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    src.member_offset = offset;

    uint32_t tmp_x64_code_size = tp_encode_x64_2_operand(
        symbol_table, x64_code_buffer, x64_code_offset,
        TP_X64_MOV, &dst, &src
    );

    TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);

    if ( ! tp_wasm_stack_push(symbol_table, &dst)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    return x64_code_size;
}

uint32_t tp_encode_set_local_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    uint32_t local_index, TP_WASM_STACK_ELEMENT* op1)
{
    if (TP_WASM_OPCODE_I32_VALUE != op1->member_wasm_opcode){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("ERROR: TP_WASM_OPCODE_I32_VALUE(%1) != op1->member_wasm_opcode(%2)"),
            TP_LOG_PARAM_UINT64_VALUE(TP_WASM_OPCODE_I32_VALUE),
            TP_LOG_PARAM_UINT64_VALUE(op1->member_wasm_opcode)
        );

        return 0;
    }

    TP_WASM_STACK_ELEMENT dst = {
        .member_wasm_opcode = TP_WASM_OPCODE_I32_VALUE,
        .member_local_index = local_index,
        .member_x64_item_kind = TP_X64_ITEM_KIND_MEMORY,
        .member_x64_memory_kind = TP_X64_ITEM_MEMORY_KIND_LOCAL
    };

    int32_t offset = 0;

    if ( ! tp_get_local_variable_offset(symbol_table, local_index, &offset)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    dst.member_offset = offset;

    uint32_t x64_code_size = 0;

    uint32_t tmp_x64_code_size = tp_encode_x64_2_operand(
        symbol_table, x64_code_buffer, x64_code_offset,
        TP_X64_MOV, &dst, op1
    );

    TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);

    return x64_code_size;
}

uint32_t tp_encode_tee_local_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    uint32_t local_index, TP_WASM_STACK_ELEMENT* op1)
{
    uint32_t x64_code_size = tp_encode_set_local_code(
        symbol_table, x64_code_buffer, x64_code_offset,
        local_index, op1
    );

    if (0 == x64_code_size){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    if ( ! tp_wasm_stack_push(symbol_table, op1)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    return x64_code_size;
}

uint32_t tp_encode_i32_const_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset, int32_t value)
{
    TP_WASM_STACK_ELEMENT result = {
        .member_wasm_opcode = TP_WASM_OPCODE_I32_VALUE
    };

    uint32_t x64_code_size = 0;

    if ( ! tp_allocate_temporary_variable(symbol_table, TP_X64_ALLOCATE_DEFAULT,
        x64_code_buffer, x64_code_offset, &x64_code_size, &result)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    uint32_t tmp_x64_code_size = encode_x64_mov_imm(
        symbol_table, x64_code_buffer, x64_code_offset,
        value, TP_X64_MOV_IMM_MODE_FORCE_IMM32, &result
    );

    TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);

    if ( ! tp_wasm_stack_push(symbol_table, &result)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    return x64_code_size;
}

uint32_t tp_encode_i32_add_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_WASM_STACK_ELEMENT* op1, TP_WASM_STACK_ELEMENT* op2)
{
    // ADD
    uint32_t x64_code_size = tp_encode_x64_2_operand(
        symbol_table, x64_code_buffer, x64_code_offset, TP_X64_ADD, op1, op2
    );

    if ( ! tp_wasm_stack_push(symbol_table, op1)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    return x64_code_size;
}

uint32_t tp_encode_i32_sub_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_WASM_STACK_ELEMENT* op1, TP_WASM_STACK_ELEMENT* op2)
{
    // SUB – Integer Subtraction
    uint32_t x64_code_size = tp_encode_x64_2_operand(
        symbol_table, x64_code_buffer, x64_code_offset, TP_X64_SUB, op1, op2
    );

    if ( ! tp_wasm_stack_push(symbol_table, op1)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    return x64_code_size;
}

uint32_t tp_encode_i32_mul_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_WASM_STACK_ELEMENT* op1, TP_WASM_STACK_ELEMENT* op2)
{
    // IMUL – Signed Multiply
    uint32_t x64_code_size = tp_encode_x64_2_operand(
        symbol_table, x64_code_buffer, x64_code_offset, TP_X64_IMUL, op1, op2
    );

    if ( ! tp_wasm_stack_push(symbol_table, op1)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    return x64_code_size;
}

uint32_t tp_encode_i32_div_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_WASM_STACK_ELEMENT* op1, TP_WASM_STACK_ELEMENT* op2)
{
    // IDIV – Signed Divide
    uint32_t x64_code_size = tp_encode_x64_2_operand(
        symbol_table, x64_code_buffer, x64_code_offset, TP_X64_IDIV, op1, op2
    );

    if ( ! tp_wasm_stack_push(symbol_table, op1)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    return x64_code_size;
}

uint32_t tp_encode_i32_xor_code(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_WASM_STACK_ELEMENT* op1, TP_WASM_STACK_ELEMENT* op2)
{
    // XOR – Logical Exclusive OR
    uint32_t x64_code_size = tp_encode_x64_2_operand(
        symbol_table, x64_code_buffer, x64_code_offset, TP_X64_XOR, op1, op2
    );

    if ( ! tp_wasm_stack_push(symbol_table, op1)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    return x64_code_size;
}

uint32_t tp_encode_end_code(TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset)
{
    uint32_t x64_code_size = 0;

    for (int32_t i = 0; TP_X64_NV64_REGISTER_NUM > i; ++i){

        switch (symbol_table->member_use_nv_register[i]){
        case TP_X64_NV64_REGISTER_NULL:
            break;
        case TP_X64_NV64_REGISTER_RBX:
//          break;
        case TP_X64_NV64_REGISTER_RSI:
//          break;
        case TP_X64_NV64_REGISTER_RDI:
//          break;
        case TP_X64_NV64_REGISTER_R12:
//          break;
        case TP_X64_NV64_REGISTER_R13:
//          break;
        case TP_X64_NV64_REGISTER_R14:
//          break;
        case TP_X64_NV64_REGISTER_R15:
            // POP – Pop a Value from the Stack
            x64_code_size += encode_x64_pop_reg64(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size,
                (TP_X64_64_REGISTER)(symbol_table->member_use_nv_register[i])
            );
            break;
        default:
            TP_PUT_LOG_MSG_ICE(symbol_table);
            return 0;
        }
    }

//  // LEA - Load Effective Address
//  // lea rsp, QWORD PTR [rbp+32]
//  x64_code_size += encode_x64_lea(
//      symbol_table, x64_code_buffer, x64_code_offset + x64_code_size,
//      TP_X64_64_REGISTER_RSP, TP_X64_64_REGISTER_INDEX_NONE, TP_X64_64_REGISTER_RBP, stack_param_size
//  );
    // ADD
    x64_code_size += encode_x64_add_sub_imm(
        symbol_table, x64_code_buffer, x64_code_offset + x64_code_size,
        TP_X64_ADD, TP_X64_64_REGISTER_RSP, symbol_table->member_stack_imm32, TP_X64_ADD_SUB_IMM_MODE_FORCE_IMM32
    );

    // POP – Pop a Value from the Stack
    x64_code_size += encode_x64_pop_reg64(
        symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, TP_X64_64_REGISTER_RBP
    );

    // RET - Return from Procedure(near return)
    x64_code_size += encode_x64_1_opcode(
        symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, 0xc3
    );

    return x64_code_size;
}

uint32_t tp_encode_x64_2_operand(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64 x64_op, TP_WASM_STACK_ELEMENT* op1, TP_WASM_STACK_ELEMENT* op2)
{
    uint32_t x64_code_size = 0;
    uint32_t tmp_x64_code_size = 0;

    if ((TP_WASM_OPCODE_I32_VALUE != op1->member_wasm_opcode) ||
        (TP_WASM_OPCODE_I32_VALUE != op2->member_wasm_opcode)){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("ERROR: TP_WASM_OPCODE_I32_VALUE(%1) != (op1(%2) || op2(%3))"),
            TP_LOG_PARAM_UINT64_VALUE(TP_WASM_OPCODE_I32_VALUE),
            TP_LOG_PARAM_UINT64_VALUE(op1->member_wasm_opcode),
            TP_LOG_PARAM_UINT64_VALUE(op2->member_wasm_opcode)
        );

        return 0;
    }

    switch (op1->member_x64_item_kind){
    case TP_X64_ITEM_KIND_X86_32_REGISTER:
//      break;
    case TP_X64_ITEM_KIND_X64_32_REGISTER:
        switch (op2->member_x64_item_kind){
        case TP_X64_ITEM_KIND_X86_32_REGISTER:
//          break;
        case TP_X64_ITEM_KIND_X64_32_REGISTER:
            x64_code_size = encode_x64_32_register_to_x64_32_register(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size,
                x64_op, op1, op2
            );
            break;
        case TP_X64_ITEM_KIND_MEMORY:
            x64_code_size = encode_x64_32_memory_offset_to_register(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size,
                x64_op, op1, op2
            );
            break;
        default:
            TP_PUT_LOG_MSG_ICE(symbol_table);
            return 0;
        }
        break;
    case TP_X64_ITEM_KIND_MEMORY:
        switch (op2->member_x64_item_kind){
        case TP_X64_ITEM_KIND_X86_32_REGISTER:
//          break;
        case TP_X64_ITEM_KIND_X64_32_REGISTER:
            x64_code_size = encode_x64_32_register_to_memory_offset(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size,
                x64_op, op1, op2
            );
            break;
        case TP_X64_ITEM_KIND_MEMORY:
        {
            tmp_x64_code_size = encode_x64_push_reg64(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, TP_X64_64_REGISTER_RAX
            );

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);

            TP_WASM_STACK_ELEMENT eax_op = {
                .member_wasm_opcode = TP_WASM_OPCODE_I32_VALUE,
                .member_x64_item_kind = TP_X64_ITEM_KIND_X86_32_REGISTER,
                .member_x64_item.member_x86_32_register = TP_X86_32_REGISTER_EAX
            };

            tmp_x64_code_size = encode_x64_32_memory_offset_to_register(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size,
                TP_X64_MOV, &eax_op, op1
            );

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);

            tmp_x64_code_size = encode_x64_32_memory_offset_to_register(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size,
                x64_op, &eax_op, op2
            );

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);

            tmp_x64_code_size = encode_x64_32_register_to_memory_offset(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size,
                TP_X64_MOV, op1, &eax_op
            );

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);

            tmp_x64_code_size = encode_x64_pop_reg64(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, TP_X64_64_REGISTER_RAX
            );

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);
            break;
        }
        default:
            TP_PUT_LOG_MSG_ICE(symbol_table);
            return 0;
        }
        break;
    default:
        TP_PUT_LOG_MSG_ICE(symbol_table);
        return 0;
    }

    if ( ! tp_free_register(symbol_table, op2)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return 0;
    }

    return x64_code_size;
}

static uint32_t encode_x64_32_register_to_x64_32_register(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64 x64_op, TP_WASM_STACK_ELEMENT* dst, TP_WASM_STACK_ELEMENT* src)
{
    uint32_t x64_code_size = 0;
    uint32_t tmp_x64_code_size = 0;

    bool is_dst_x86_32_register = (TP_X64_ITEM_KIND_X86_32_REGISTER == dst->member_x64_item_kind);
    bool is_dst_EAX_register = (is_dst_x86_32_register &&
        (TP_X86_32_REGISTER_EAX == dst->member_x64_item.member_x86_32_register)
    );
    bool is_use_EDX_register = (TP_X64_ITEM_KIND_X86_32_REGISTER ==
        symbol_table->member_use_X86_32_register[TP_X86_32_REGISTER_EDX].member_x64_item_kind
    );
    bool is_dst_x64_32_register = (TP_X64_ITEM_KIND_X64_32_REGISTER == dst->member_x64_item_kind);

    bool is_src_x86_32_register = (TP_X64_ITEM_KIND_X86_32_REGISTER == src->member_x64_item_kind);
    bool is_src_x64_32_register = (TP_X64_ITEM_KIND_X64_32_REGISTER == src->member_x64_item_kind);

    if (TP_X64_IDIV == x64_op){

        if (false == is_dst_EAX_register){

            tmp_x64_code_size = encode_x64_push_reg64(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, TP_X64_64_REGISTER_RAX
            );

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);
        }

        if (is_use_EDX_register){

            tmp_x64_code_size = encode_x64_push_reg64(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, TP_X64_64_REGISTER_RDX
            );

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);
        }

        TP_WASM_STACK_ELEMENT edx_op = {
            .member_wasm_opcode = TP_WASM_OPCODE_I32_VALUE,
            .member_x64_item_kind = TP_X64_ITEM_KIND_X86_32_REGISTER,
            .member_x64_item.member_x86_32_register = TP_X86_32_REGISTER_EDX
        };

        // xor edx, edx
        tmp_x64_code_size = tp_encode_x64_2_operand(
            symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, TP_X64_XOR, &edx_op, &edx_op
        );

        TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);
    }

    if (x64_code_buffer){

        if (is_dst_x64_32_register || is_src_x64_32_register){

            switch (x64_op){
            case TP_X64_IMUL:
                if ( ! is_dst_EAX_register){

                    goto rex_common;
                }
//              break;
            case TP_X64_IDIV:
                x64_code_buffer[x64_code_offset + x64_code_size] = (0x40 |
                    /* B */ (is_src_x64_32_register ? 0x01 : 0x00)
                );
                break;
            default:
rex_common:
                x64_code_buffer[x64_code_offset + x64_code_size] = (0x40 |
                    /* R */ (is_dst_x64_32_register ? 0x04 : 0x00) |
                    /* B */ (is_src_x64_32_register ? 0x01 : 0x00)
                );
                break;
            }

            ++x64_code_size;
        }

        switch (x64_op){
        case TP_X64_MOV:
            // MOV – Move Data
            // register2 to register1 1000 101w : 11 reg1 reg2
            x64_code_buffer[x64_code_offset + x64_code_size] = 0x8b;
            break;
        case TP_X64_ADD:
            // ADD
            // register2 to register1 0000 001w : 11 reg1 reg2
            x64_code_buffer[x64_code_offset + x64_code_size] = 0x03;
            break;
        case TP_X64_SUB:
            // SUB – Integer Subtraction
            // register2 to register1 0010 101w : 11 reg1 reg2
            x64_code_buffer[x64_code_offset + x64_code_size] = 0x2b;
            break;
        case TP_X64_IMUL:
            // IMUL – Signed Multiply
            if (is_dst_EAX_register){

                // AL, AX, or EAX with register 1111 011w : 11 101 reg
                x64_code_buffer[x64_code_offset + x64_code_size] = 0xf7;
            }else{

                // register1 with register2 0000 1111 : 1010 1111 : 11 : reg1 reg2
                x64_code_buffer[x64_code_offset + x64_code_size] = 0x0f;

                ++x64_code_size;

                x64_code_buffer[x64_code_offset + x64_code_size] = 0xaf;
            }
            break;
        case TP_X64_IDIV:
            // IDIV – Signed Divide
            // AL, AX, or EAX by register 1111 011w : 11 111 reg
            x64_code_buffer[x64_code_offset + x64_code_size] = 0xf7;
            break;
        case TP_X64_XOR:
            // XOR – Logical Exclusive OR
            // register2 to register1 0011 001w : 11 reg1 reg2
            x64_code_buffer[x64_code_offset + x64_code_size] = 0x33;
            break;
        default:
            TP_PUT_LOG_MSG_ICE(symbol_table);
            return 0;
        }

        ++x64_code_size;

        // ModR/M
        switch (x64_op){
        case TP_X64_IMUL:
            if (is_dst_EAX_register){

                // AL, AX, or EAX with register : 11 101 reg
                x64_code_buffer[x64_code_offset + x64_code_size] = (0xe8 |
                    ((is_src_x86_32_register ? src->member_x64_item.member_x86_32_register :
                    src->member_x64_item.member_x64_32_register) & 0x07)
                );
            }else{

                goto mod_rm_common;
            }
            break;
        case TP_X64_IDIV:
            // AL, AX, or EAX by register : 11 111 reg
            x64_code_buffer[x64_code_offset + x64_code_size] = (0xf8 |
                ((is_src_x86_32_register ? src->member_x64_item.member_x86_32_register :
                src->member_x64_item.member_x64_32_register) & 0x07)
            );
            break;
        default:
mod_rm_common:
            // register2 to register1 : 11 reg1 reg2
            x64_code_buffer[x64_code_offset + x64_code_size] = ((0x03 << 6) |
                (((is_dst_x86_32_register ? dst->member_x64_item.member_x86_32_register :
                dst->member_x64_item.member_x64_32_register) & 0x07) << 3) |
                ((is_src_x86_32_register ? src->member_x64_item.member_x86_32_register :
                src->member_x64_item.member_x64_32_register) & 0x07)
            );
            break;
        }

        ++x64_code_size;
    }else{

        if (is_dst_x64_32_register || is_src_x64_32_register){

            ++x64_code_size;
        }

        x64_code_size += 2;

        if ((TP_X64_IMUL == x64_op) && (false == is_dst_EAX_register)){

            ++x64_code_size;
        }

        if (TP_X64_NULL <= x64_op){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return 0;
        }
    }

    if (TP_X64_IDIV == x64_op){

        if (is_use_EDX_register){

            tmp_x64_code_size = encode_x64_pop_reg64(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, TP_X64_64_REGISTER_RDX
            );

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);
        }

        if (false == is_dst_EAX_register){

            tmp_x64_code_size = encode_x64_pop_reg64(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, TP_X64_64_REGISTER_RAX
            );

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);
        }
    }

    return x64_code_size;
}

static uint32_t encode_x64_32_register_to_memory_offset(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64 x64_op, TP_WASM_STACK_ELEMENT* dst, TP_WASM_STACK_ELEMENT* src)
{
    return encode_x64_32_memory_offset_common(
        symbol_table, x64_code_buffer, x64_code_offset,
        x64_op, TP_X64_DIRECTION_SOURCE_REGISTER, dst, src
    );
}

static uint32_t encode_x64_32_memory_offset_to_register(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64 x64_op, TP_WASM_STACK_ELEMENT* dst, TP_WASM_STACK_ELEMENT* src)
{
    return encode_x64_32_memory_offset_common(
        symbol_table, x64_code_buffer, x64_code_offset,
        x64_op, TP_X64_DIRECTION_SOURCE_MEMORY, dst, src
    );
}

static uint32_t encode_x64_32_memory_offset_common(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64 x64_op, TP_X64_DIRECTION x64_direction, TP_WASM_STACK_ELEMENT* dst, TP_WASM_STACK_ELEMENT* src)
{
    uint32_t x64_code_size = 0;
    uint32_t tmp_x64_code_size = 0;

    if ((TP_X64_ITEM_KIND_MEMORY != dst->member_x64_item_kind) &&
        (TP_X64_ITEM_KIND_MEMORY != src->member_x64_item_kind)){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("TP_X64_ITEM_KIND_MEMORY(%1) != dst_op(%2), src_op(%3)"),
            TP_LOG_PARAM_INT32_VALUE(TP_X64_ITEM_KIND_MEMORY),
            TP_LOG_PARAM_INT32_VALUE(dst->member_x64_item_kind),
            TP_LOG_PARAM_INT32_VALUE(src->member_x64_item_kind)
        );

        return 0;
    }

    bool is_source_memory = (TP_X64_DIRECTION_SOURCE_MEMORY == x64_direction);

    int32_t offset = (is_source_memory ? src->member_offset : dst->member_offset);

    bool is_disp8 = (is_source_memory ? (
        (INT8_MIN <= src->member_offset) &&
        (INT8_MAX >= src->member_offset)
    ) : (
        (INT8_MIN <= dst->member_offset) &&
        (INT8_MAX >= dst->member_offset)
    ));

    bool is_dst_x86_32_register = (TP_X64_ITEM_KIND_X86_32_REGISTER == dst->member_x64_item_kind);
    bool is_dst_EAX_register = (is_dst_x86_32_register &&
        (TP_X86_32_REGISTER_EAX == dst->member_x64_item.member_x86_32_register)
    );
    bool is_use_EDX_register = (TP_X64_ITEM_KIND_X86_32_REGISTER ==
        symbol_table->member_use_X86_32_register[TP_X86_32_REGISTER_EDX].member_x64_item_kind
    );
    bool is_dst_x64_32_register = (TP_X64_ITEM_KIND_X64_32_REGISTER == dst->member_x64_item_kind);

    bool is_src_x86_32_register = (TP_X64_ITEM_KIND_X86_32_REGISTER == src->member_x64_item_kind);
    bool is_src_x64_32_register = (TP_X64_ITEM_KIND_X64_32_REGISTER == src->member_x64_item_kind);

    if ((false == is_source_memory) && (TP_X64_IMUL == x64_op)){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE, TP_MSG_FMT("%1"),
            TP_LOG_PARAM_STRING("ERROR: (false == is_source_memory) && (TP_X64_IMUL == x64_op)")
        );

        return 0;
    }

    if (TP_X64_IDIV == x64_op){

        if (false == is_dst_EAX_register){

            tmp_x64_code_size = encode_x64_push_reg64(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, TP_X64_64_REGISTER_RAX
            );

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);
        }

        if (is_use_EDX_register){

            tmp_x64_code_size = encode_x64_push_reg64(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, TP_X64_64_REGISTER_RDX
            );

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);
        }

        TP_WASM_STACK_ELEMENT edx_op = {
            .member_wasm_opcode = TP_WASM_OPCODE_I32_VALUE,
            .member_x64_item_kind = TP_X64_ITEM_KIND_X86_32_REGISTER,
            .member_x64_item.member_x86_32_register = TP_X86_32_REGISTER_EDX
        };

        // xor edx, edx
        tmp_x64_code_size = tp_encode_x64_2_operand(
            symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, TP_X64_XOR, &edx_op, &edx_op
        );

        TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);
    }

    if (x64_code_buffer){

        if (is_dst_x64_32_register || is_src_x64_32_register){

            switch (x64_op){
            case TP_X64_IMUL:
                if ( ! is_dst_EAX_register){

                    goto rex_common;
                }
//              break;
            case TP_X64_IDIV:
                x64_code_buffer[x64_code_offset + x64_code_size] = (0x40 |
                    /* B */ (is_src_x64_32_register ? 0x01 : 0x00)
                );
                break;
            default:
rex_common:
                x64_code_buffer[x64_code_offset + x64_code_size] = (0x40 |
                    /* R */ (is_dst_x64_32_register ? 0x04 : 0x00) |
                    /* B */ (is_src_x64_32_register ? 0x01 : 0x00)
                );
                break;
            }

            ++x64_code_size;
        }

        switch (x64_op){
        case TP_X64_MOV:
            // MOV – Move Data
            // memory to reg 1000 101w : mod reg r/m
            // reg to memory 1000 100w : mod reg r/m
            x64_code_buffer[x64_code_offset + x64_code_size] = (0x89 | (is_source_memory ? 0x02 : 0x00));
            break;
        case TP_X64_ADD:
            // ADD
            // memory to register 0000 001w : mod reg r/m
            // register to memory 0000 000w : mod reg r/m
            x64_code_buffer[x64_code_offset + x64_code_size] = (0x01 | (is_source_memory ? 0x02 : 0x00));
            break;
        case TP_X64_SUB:
            // SUB – Integer Subtraction
            // memory to register 0010 101w : mod reg r/m
            // register to memory 0010 100w : mod reg r/m
            x64_code_buffer[x64_code_offset + x64_code_size] = (0x29 | (is_source_memory ? 0x02 : 0x00));
            break;
        case TP_X64_IMUL:
            // IMUL – Signed Multiply
            // AL, AX, or EAX with memory 1111 011w : mod 101 reg
            // register with memory 0000 1111 : 1010 1111 : mod reg r/m
            if (is_dst_EAX_register){

                x64_code_buffer[x64_code_offset + x64_code_size] = 0xf7;
            }else{

                x64_code_buffer[x64_code_offset + x64_code_size] = 0x0f;

                ++x64_code_size;

                x64_code_buffer[x64_code_offset + x64_code_size] = 0xaf;
            }
            break;
        case TP_X64_IDIV:
            // IDIV – Signed Divide
            // AL, AX, or EAX by memory 1111 011w : mod 111 r/m
            x64_code_buffer[x64_code_offset + x64_code_size] = 0xf7;
            break;
        case TP_X64_XOR:
            // XOR – Logical Exclusive OR
            // memory to register 0011 001w : mod reg r/m
            // register to memory 0011 000w : mod reg r/m
            x64_code_buffer[x64_code_offset + x64_code_size] = (0x31 | (is_source_memory ? 0x02 : 0x00));
            break;
        default:
            TP_PUT_LOG_MSG_ICE(symbol_table);
            return 0;
        }

        ++x64_code_size;

        // ModR/M
        switch (x64_op){
        case TP_X64_IMUL:
            if (is_dst_EAX_register){

                // AL, AX, or EAX with memory 1111 011w : mod 101 reg
                x64_code_buffer[x64_code_offset + x64_code_size] = (
                    (is_disp8 ? 0x44 : 0x84) | ((TP_X86_32_REGISTER_EAX & 0x07) << 3)
                );
            }else{

                goto mod_rm_common;
            }
            break;
        case TP_X64_IDIV:
            // AL, AX, or EAX by memory 1111 011w : mod 111 r/m
            x64_code_buffer[x64_code_offset + x64_code_size] = (
                (is_disp8 ? 0x44 : 0x84) | ((TP_X86_32_REGISTER_EAX & 0x07) << 3)
            );
            break;
        default:
mod_rm_common:
            // mod reg r/m
            x64_code_buffer[x64_code_offset + x64_code_size] = ((is_disp8 ? 0x44 : 0x84) |
                (((is_dst_x86_32_register ? dst->member_x64_item.member_x86_32_register :
                dst->member_x64_item.member_x64_32_register) & 0x07) << 3) |
                ((is_src_x86_32_register ? src->member_x64_item.member_x86_32_register :
                src->member_x64_item.member_x64_32_register) & 0x07)
            );
            break;
        }

        ++x64_code_size;

        // SIB
        x64_code_buffer[x64_code_offset + x64_code_size] = (
            ((TP_X64_64_REGISTER_INDEX_NONE & 0x07) << 3) | (TP_X64_64_REGISTER_RBP & 0x07)
        );

        ++x64_code_size;

        // Address displacement
        if (is_disp8){

            x64_code_buffer[x64_code_offset + x64_code_size] = (uint8_t)offset;

            x64_code_size += 1;
        }else{

            memcpy(
                &(x64_code_buffer[x64_code_offset + x64_code_size]), &offset,
                sizeof(offset)
            );

            x64_code_size += 4;
        }
    }else{

        if (is_dst_x64_32_register || is_src_x64_32_register){

            ++x64_code_size;
        }

        x64_code_size += 3;

        if ((TP_X64_IMUL == x64_op) && (false == is_dst_EAX_register)){

            ++x64_code_size;
        }

        if (TP_X64_NULL <= x64_op){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return 0;
        }

        if (is_disp8){

            x64_code_size += 1;
        }else{

            x64_code_size += 4;
        }
    }

    if (TP_X64_IDIV == x64_op){

        if (is_use_EDX_register){

            tmp_x64_code_size = encode_x64_pop_reg64(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, TP_X64_64_REGISTER_RDX
            );

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);
        }

        if (false == is_dst_EAX_register){

            tmp_x64_code_size = encode_x64_pop_reg64(
                symbol_table, x64_code_buffer, x64_code_offset + x64_code_size, TP_X64_64_REGISTER_RAX
            );

            TP_X64_CHECK_CODE_SIZE(symbol_table, x64_code_size, tmp_x64_code_size);
        }
    }

    return x64_code_size;
}

static uint32_t encode_x64_mov_imm(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    int32_t imm, TP_X64_MOV_IMM_MODE x64_mov_imm_mode, TP_WASM_STACK_ELEMENT* result)
{
    uint32_t x64_code_size = 0;

    bool is_disp8 = (
        (INT8_MIN <= result->member_offset) &&
        (INT8_MAX >= result->member_offset)
    );

    bool is_imm8 = false;

    switch (x64_mov_imm_mode){
    case TP_X64_MOV_IMM_MODE_DEFAULT:
        is_imm8 = ((INT8_MIN <= imm) && (INT8_MAX >= imm));
        break;
    case TP_X64_MOV_IMM_MODE_FORCE_IMM32:
        break;
    default:
        TP_PUT_LOG_MSG_ICE(symbol_table);
        return 0;
    }

    // MOV – Move Data
    if (x64_code_buffer){

        switch (result->member_x64_item_kind){
        case TP_X64_ITEM_KIND_X86_32_REGISTER:

            // immediate to register (alternate encoding) 1011 w reg : immediate data
            x64_code_buffer[x64_code_offset] = (
                0xb0 | (is_imm8 ? 0x00 : 0x08) |
                (result->member_x64_item.member_x86_32_register & 0x07)
            );

            x64_code_size = 1;

            // Immediate data
            if (is_imm8){

                x64_code_buffer[x64_code_offset + x64_code_size] = (uint8_t)imm;

                x64_code_size += 1;
            }else{

                memcpy(&(x64_code_buffer[x64_code_offset + x64_code_size]), &imm, sizeof(imm));

                x64_code_size += 4;
            }

            break;
        case TP_X64_ITEM_KIND_X64_32_REGISTER:

            // immediate to register (alternate encoding) 0100 000B : 1011 w reg : imm
            x64_code_buffer[x64_code_offset] = 0x41;
            x64_code_buffer[x64_code_offset + 1] = (
                0xb0 | (is_imm8 ? 0x00 : 0x08) |
                (result->member_x64_item.member_x64_32_register & 0x07)
            );

            x64_code_size = 2;

            // Immediate data
            if (is_imm8){

                x64_code_buffer[x64_code_offset + x64_code_size] = (uint8_t)imm;

                x64_code_size += 1;
            }else{

                memcpy(&(x64_code_buffer[x64_code_offset + x64_code_size]), &imm, sizeof(imm));

                x64_code_size += 4;
            }

            break;
        case TP_X64_ITEM_KIND_MEMORY:

            // immediate to memory 1100 011w : mod 000 r/m : immediate data
            x64_code_buffer[x64_code_offset] = (0xc7 | (is_imm8 ? 0x00 : 0x01));

           // ModR/M
            if (is_disp8){

                // disp8
                x64_code_buffer[x64_code_offset + 1] = 0x44;
            }else{

                // disp32
                x64_code_buffer[x64_code_offset + 1] = 0x84;
            }

            // SIB
            x64_code_buffer[x64_code_offset + 2] = (
                ((TP_X64_64_REGISTER_INDEX_NONE & 0x07) << 3) | (TP_X64_64_REGISTER_RBP & 0x07)
            );

            x64_code_size = 3;

            // Address displacement
            if (is_disp8){

                x64_code_buffer[x64_code_offset + x64_code_size] = (uint8_t)(result->member_offset);

                x64_code_size += 1;
            }else{

                memcpy(
                    &(x64_code_buffer[x64_code_offset + x64_code_size]), &(result->member_offset),
                    sizeof(result->member_offset)
                );

                x64_code_size += 4;
            }

            // Immediate data
            if (is_imm8){

                x64_code_buffer[x64_code_offset + x64_code_size] = (uint8_t)imm;

                x64_code_size += 1;
            }else{

                memcpy(&(x64_code_buffer[x64_code_offset + x64_code_size]), &imm, sizeof(imm));

                x64_code_size += 4;
            }

            break;
        default:
            TP_PUT_LOG_MSG_ICE(symbol_table);
            return 0;
        }
    }else{

        switch (result->member_x64_item_kind){
        case TP_X64_ITEM_KIND_X86_32_REGISTER:
            x64_code_size = 1;

            if (is_imm8){

                x64_code_size += 1;
            }else{

                x64_code_size += 4;
            }
            break;
        case TP_X64_ITEM_KIND_X64_32_REGISTER:
            x64_code_size = 2;

            if (is_imm8){

                x64_code_size += 1;
            }else{

                x64_code_size += 4;
            }
            break;
        case TP_X64_ITEM_KIND_MEMORY:
            x64_code_size = 3;

            if (is_disp8){

                x64_code_size += 1;
            }else{

                x64_code_size += 4;
            }

            if (is_imm8){

                x64_code_size += 1;
            }else{

                x64_code_size += 4;
            }
            break;
        default:
            TP_PUT_LOG_MSG_ICE(symbol_table);
            return 0;
        }
    }

    return x64_code_size;
}

static uint32_t encode_x64_add_sub_imm(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64 x64_op, TP_X64_64_REGISTER reg64, int32_t imm, TP_X64_ADD_SUB_IMM_MODE x64_add_sub_imm_mode)
{
    uint32_t x64_code_size = 3;

    bool is_add = false;

    switch (x64_op){
    case TP_X64_ADD:
        is_add = true;
        break;
    case TP_X64_SUB:
        break;
    default:
        TP_PUT_LOG_MSG_ICE(symbol_table);
        return 0;
    }

    bool is_imm8 = false;

    switch (x64_add_sub_imm_mode){
    case TP_X64_ADD_SUB_IMM_MODE_DEFAULT:
        is_imm8 = ((INT8_MIN <= imm) && (INT8_MAX >= imm));
        break;
    case TP_X64_ADD_SUB_IMM_MODE_FORCE_IMM32:
        break;
    default:
        TP_PUT_LOG_MSG_ICE(symbol_table);
        return 0;
    }

    if (is_add){

        is_imm8 = false;
    }

    if (x64_code_buffer){

        x64_code_buffer[x64_code_offset] = (0x48 | ((TP_X64_64_REGISTER_R8 <= reg64) ? 0x01 : 0x00));

        // SUB – Integer Subtraction
        if (is_imm8){

            // SUB: immediate8 from qwordregister 0100 100B 1000 0011 : 11 101 qwordreg : imm8
            x64_code_buffer[x64_code_offset + 1] = 0x83;
            x64_code_buffer[x64_code_offset + 2] = (0xe8 | (reg64 & 0x07));
            x64_code_buffer[x64_code_offset + 3] = (uint8_t)imm;

            ++x64_code_size;
        }else{

            // ADD: immediate32 to qwordregister 0100 100B : 1000 0001 : 11 000 qwordreg : imm
            // SUB: immediate32 from qwordregister 0100 100B 1000 0001 : 11 101 qwordreg : imm32
            x64_code_buffer[x64_code_offset + 1] = 0x81;
            x64_code_buffer[x64_code_offset + 2] = ((is_add ? 0xc0 : 0xe8) | (reg64 & 0x07));
            memcpy(&(x64_code_buffer[x64_code_offset + 3]), &imm, sizeof(imm));

            x64_code_size += 4;
        }
    }else{

        if (is_imm8){

            // imm8
            ++x64_code_size;
        }else{

            // imm32
            x64_code_size += 4;
        }
    }

    return x64_code_size;
}

static uint32_t encode_x64_lea(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset,
    TP_X64_64_REGISTER reg64_dst_reg, TP_X64_64_REGISTER reg64_src_index, TP_X64_64_REGISTER reg64_src_base, int32_t offset)
{
    uint32_t x64_code_size = 4;

    bool is_disp8 = ((INT8_MIN <= offset) && (INT8_MAX >= offset));

    if (x64_code_buffer){

        // LEA - Load Effective Address : 8D /r LEA r64,m 
        // in qwordregister 0100 1RXB : 1000 1101 : modA qwordreg r/m
        x64_code_buffer[x64_code_offset] = (0x48 |
            /* R */ ((TP_X64_64_REGISTER_R8 <= reg64_dst_reg) ? 0x04 : 0x00) |
            /* X */ ((TP_X64_64_REGISTER_R8 <= reg64_src_index) ? 0x02 : 0x00) |
            /* B */ ((TP_X64_64_REGISTER_R8 <= reg64_src_base) ? 0x01 : 0x00)
        );
        x64_code_buffer[x64_code_offset + 1] = 0x8d;

        // ModR/M
        if (0 == offset){

            // disp0
            x64_code_buffer[x64_code_offset + 2] = (((reg64_dst_reg & 0x07) << 3) | 0x04);
        }else if (is_disp8){

            // disp8
            x64_code_buffer[x64_code_offset + 2] = (((reg64_dst_reg & 0x07) << 3) | 0x44);
        }else{

            // disp32
            x64_code_buffer[x64_code_offset + 2] = (((reg64_dst_reg & 0x07) << 3) | 0x84);
        }

        // SIB
        x64_code_buffer[x64_code_offset + 3] = (((reg64_src_index & 0x07) << 3) | (reg64_src_base & 0x07));

        if (0 == offset){

            // disp0
            ;
        }else if (is_disp8){

            // disp8
            x64_code_buffer[x64_code_offset + 4] = (uint8_t)offset;

            ++x64_code_size;
        }else{

            // disp32
            memcpy(&(x64_code_buffer[x64_code_offset + 4]), &offset, sizeof(offset));

            x64_code_size += 4;
        }
    }else{

        if (0 == offset){

            // disp0
            ;
        }else if (is_disp8){

            // disp8
            ++x64_code_size;
        }else{

            // disp32
            x64_code_size += 4;
        }
    }

    return x64_code_size;
}

static uint32_t encode_x64_push_reg64(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset, TP_X64_64_REGISTER reg64)
{
    uint32_t x64_code_size = 0;

    if (x64_code_buffer){

        // PUSH – Push Operand onto the Stack : 50+rd push r64
        // qwordregister (alternate encoding) 0100 W00B : 0101 0 reg64
        if (TP_X64_64_REGISTER_R8 <= reg64){

            x64_code_buffer[x64_code_offset] = (0x48 | 0x01);

            ++x64_code_size;
        }

        x64_code_buffer[x64_code_offset + x64_code_size] = (0x50 | (reg64 & 0x07));

        ++x64_code_size;
    }else{

        if (TP_X64_64_REGISTER_R8 <= reg64){

            ++x64_code_size;
        }

        ++x64_code_size;
    }

    return x64_code_size;
}


static uint32_t encode_x64_pop_reg64(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset, TP_X64_64_REGISTER reg64)
{
    uint32_t x64_code_size = 0;

    if (x64_code_buffer){

        // POP – Pop a Value from the Stack : REX.W + 58+rd pop r64
        // qwordregister (alternate encoding) 0100 W00B : 0101 1 reg64
        if (TP_X64_64_REGISTER_R8 <= reg64){

            x64_code_buffer[x64_code_offset] = (0x48 | 0x01);

            ++x64_code_size;
        }

        x64_code_buffer[x64_code_offset + x64_code_size] = (0x58 | (reg64 & 0x07));

        ++x64_code_size;
    }else{

        if (TP_X64_64_REGISTER_R8 <= reg64){

            ++x64_code_size;
        }

        ++x64_code_size;
    }

    return x64_code_size;
}

static uint32_t encode_x64_1_opcode(
    TP_SYMBOL_TABLE* symbol_table, uint8_t* x64_code_buffer, uint32_t x64_code_offset, uint8_t opcode)
{
    uint32_t x64_code_size = 1;

    if (x64_code_buffer){

        x64_code_buffer[x64_code_offset] = opcode;
    }

    return x64_code_size;
}

