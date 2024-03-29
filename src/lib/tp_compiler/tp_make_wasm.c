
// (C) Shin'ichi Ichikawa. Released under the MIT license.

#include "tp_compiler.h"

// Convert parse tree to WebAssembly.
//
// Example:
// int32_t value1 = (1 + 2) * 3;
// int32_t value2 = 2 + (3 * value1);
// value1 = value2 + 100;
//
// WebAssembly:
// (module
//   (type (;0;) (func (result i32)))
//   (func (;0;) (type 0) (result i32)
//     (local i32 i32)
//     i32.const 1
//     i32.const 2
//     i32.add
//     i32.const 3
//     i32.mul
//     set_local 0
//     i32.const 2
//     i32.const 3
//     get_local 0
//     i32.mul
//     i32.add
//     set_local 1
//     get_local 1
//     i32.const 100
//     i32.add
//     tee_local 0)
//   (table (;0;) 0 anyfunc)
//   (memory (;0;) 1)
//   (export "memory" (memory 0))
//   (export "calc" (func 0)))

#define TP_SECTION_NUM 6

#define TP_MAKE_WASM_SECTION_BUFFER(symbol_table, section, section_buffer, id, payload_len) \
\
    do{ \
        uint32_t size = tp_encode_ui32leb128(NULL, 0, (id)); \
        size += tp_encode_ui32leb128(NULL, 0, payload_len); \
        size += payload_len; \
\
        (section) = \
            (TP_WASM_MODULE_SECTION*)calloc(1, sizeof(TP_WASM_MODULE_SECTION)); \
\
        if (NULL == section){ \
\
            TP_PRINT_CRT_ERROR(symbol_table); \
\
            return NULL; \
        } \
\
        (section)->member_section_size = size; \
        (section)->member_id = (id); \
        (section)->member_payload_len = payload_len; \
\
        (section_buffer) = (section)->member_name_len_name_payload_data = (uint8_t*)calloc(size, sizeof(uint8_t)); \
\
        if (NULL == (section_buffer)){ \
\
            TP_PRINT_CRT_ERROR(symbol_table); \
\
            TP_FREE(symbol_table, &section, sizeof(TP_WASM_MODULE_SECTION)); \
\
            return NULL; \
        } \
\
        break; \
\
    }while (false)

#define TP_MAKE_ULEB128_CODE(buffer, offset, opcode, value) \
\
    do{ \
        uint32_t size = 0; \
\
        if (buffer){ \
\
            ((buffer) + (offset))[0] = (opcode); \
            size = tp_encode_ui32leb128((buffer), (offset) + 1, (value)) + sizeof(uint8_t); \
        }else{ \
\
            size = tp_encode_ui32leb128(NULL, 0, (value)) + sizeof(uint8_t); \
        } \
\
        return size; \
\
    }while (false)

#define TP_MAKE_SLEB128_CODE(buffer, offset, opcode, value) \
\
    do{ \
        uint32_t size = 0; \
\
        if (buffer){ \
\
            ((buffer) + (offset))[0] = (opcode); \
            size = tp_encode_si64leb128((buffer), (offset) + 1, (value)) + sizeof(uint8_t); \
        }else{ \
\
            size = tp_encode_si64leb128(NULL, 0, (value)) + sizeof(uint8_t); \
        } \
\
        return size; \
\
    }while (false)

#define TP_MAKE_OPCODE(buffer, offset, opcode) \
\
    do{ \
        uint32_t size = 1; \
\
        if (buffer){ \
\
            ((buffer) + (offset))[0] = (opcode); \
        } \
\
        return size; \
\
    }while (false)

static bool wasm_gen(TP_SYMBOL_TABLE* symbol_table, bool is_origin_wasm);
static TP_WASM_MODULE_SECTION* make_section_type(TP_SYMBOL_TABLE* symbol_table);
static TP_WASM_MODULE_SECTION* make_section_function(TP_SYMBOL_TABLE* symbol_table);
static TP_WASM_MODULE_SECTION* make_section_table(TP_SYMBOL_TABLE* symbol_table);
static TP_WASM_MODULE_SECTION* make_section_memory(TP_SYMBOL_TABLE* symbol_table);
static TP_WASM_MODULE_SECTION* make_section_export(TP_SYMBOL_TABLE* symbol_table);
static TP_WASM_MODULE_SECTION* make_section_code_origin_wasm(TP_SYMBOL_TABLE* symbol_table);
static TP_WASM_MODULE_SECTION* make_section_code(TP_SYMBOL_TABLE* symbol_table);
static bool search_parse_tree(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section
);
static bool make_section_code_content(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section
);
static bool wasm_gen_statement_1_and_2(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section
);
static bool wasm_gen_expression_1_and_2(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section
);
static bool wasm_gen_term_1_and_2(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section
);
static bool wasm_gen_factor_1(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section
);
static bool wasm_gen_factor_2_and_3(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section
);
static bool get_var_value(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, size_t index, uint32_t* var_value
);
static uint32_t make_get_local_code(uint8_t* buffer, size_t offset, uint32_t value);
static uint32_t make_set_local_code(uint8_t* buffer, size_t offset, uint32_t value);
static uint32_t make_tee_local_code(uint8_t* buffer, size_t offset, uint32_t value);
static uint32_t make_i32_const_code(uint8_t* buffer, size_t offset, int32_t value);
static uint32_t make_i32_add_code(uint8_t* buffer, size_t offset);
static uint32_t make_i32_sub_code(uint8_t* buffer, size_t offset);
static uint32_t make_i32_mul_code(uint8_t* buffer, size_t offset);
static uint32_t make_i32_div_code(uint8_t* buffer, size_t offset);
static uint32_t make_i32_xor_code(uint8_t* buffer, size_t offset);
static uint32_t make_end_code(uint8_t* buffer, size_t offset);

bool tp_make_wasm(TP_SYMBOL_TABLE* symbol_table, bool is_origin_wasm)
{
    if ( ! wasm_gen(symbol_table, is_origin_wasm)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    return true;
}

static bool wasm_gen(TP_SYMBOL_TABLE* symbol_table, bool is_origin_wasm)
{
    TP_WASM_MODULE* module = &(symbol_table->member_wasm_module);

    TP_WASM_MODULE_SECTION** section =
        (TP_WASM_MODULE_SECTION**)calloc(TP_SECTION_NUM, sizeof(TP_WASM_MODULE_SECTION*));

    if (NULL == section){

        TP_PRINT_CRT_ERROR(symbol_table);

        goto error_proc;
    }

    section[0] = make_section_type(symbol_table);
    section[1] = make_section_function(symbol_table);
    section[2] = make_section_table(symbol_table);
    section[3] = make_section_memory(symbol_table);
    section[4] = make_section_export(symbol_table);

    if (is_origin_wasm){

        section[5] = make_section_code_origin_wasm(symbol_table);
    }else{

        section[5] = make_section_code(symbol_table);
    }

    if (section[0] && section[1] && section[2] && section[3] && section[4] && section[5]){

        module->member_section = section;
        module->member_section_num = TP_SECTION_NUM;

        for (size_t i = 0; TP_SECTION_NUM > i; ++i){

            module->member_content_size += section[i]->member_section_size;
        }

        module->member_content_size +=
            (sizeof(module->member_module_content->member_magic_number) +
            sizeof(module->member_module_content->member_version));

        {
            TP_WASM_MODULE_CONTENT* tmp = (TP_WASM_MODULE_CONTENT*)calloc(
                sizeof(TP_WASM_MODULE_CONTENT) + module->member_content_size, sizeof(uint8_t)
            );

            if (NULL == tmp){

                TP_PRINT_CRT_ERROR(symbol_table);

                goto error_proc;
            }

            module->member_module_content = (TP_WASM_MODULE_CONTENT*)tmp;

            memcpy(&(module->member_module_content->member_magic_number), TP_WASM_MODULE_MAGIC_NUMBER, sizeof(uint32_t));
            module->member_module_content->member_version = TP_WASM_MODULE_VERSION;

            {
                uint32_t prev_content_size = 0;

                for (size_t i = 0; TP_SECTION_NUM > i; ++i){

                    memcpy(
                        module->member_module_content->member_payload + prev_content_size,
                        section[i]->member_name_len_name_payload_data, section[i]->member_section_size
                    );

                    prev_content_size += section[i]->member_section_size;
                }
            }
        }

        if ((false ==  symbol_table->member_is_no_output_files) ||
            (symbol_table->member_is_no_output_files && symbol_table->member_is_output_wasm_file)){

            if ( ! tp_write_file(
                symbol_table, symbol_table->member_wasm_file_path,
                module->member_module_content, module->member_content_size)){

                TP_PUT_LOG_MSG_TRACE(symbol_table);

                goto error_proc;
            }
        }

        return true;
    }

error_proc:

    if (section){

        for (size_t i = 0; TP_SECTION_NUM > i; ++i){

            if (section[i]){

                TP_FREE(symbol_table, &(section[i]->member_name_len_name_payload_data), section[i]->member_section_size);

                TP_FREE(symbol_table, &(section[i]), sizeof(TP_WASM_MODULE_SECTION));
            }
        }

        TP_FREE2(symbol_table, &section, TP_SECTION_NUM * sizeof(TP_WASM_MODULE_SECTION*));
    }

    module->member_section = NULL;

    module->member_section_num = 0;

    if (module->member_module_content){

        TP_FREE(symbol_table, &(module->member_module_content), sizeof(TP_WASM_MODULE_CONTENT) + module->member_content_size);
    }

    return false;
}

static TP_WASM_MODULE_SECTION* make_section_type(TP_SYMBOL_TABLE* symbol_table)
{
    uint32_t count = TP_WASM_MODULE_SECTION_TYPE_COUNT;
    uint32_t form = TP_WASM_MODULE_SECTION_TYPE_FORM_FUNC;
    uint32_t param_count = TP_WASM_MODULE_SECTION_TYPE_PARAM_COUNT;
//  uint32_t param_types;
    uint32_t return_count = TP_WASM_MODULE_SECTION_TYPE_RETURN_COUNT;
    uint32_t return_type = TP_WASM_MODULE_SECTION_TYPE_RETURN_TYPE_I32;

    uint32_t payload_len = tp_encode_ui32leb128(NULL, 0, count);
    payload_len += tp_encode_ui32leb128(NULL, 0, form);
    payload_len += tp_encode_ui32leb128(NULL, 0, param_count);
//  payload_len += tp_encode_si64leb128(NULL, 0, param_types);
    payload_len += tp_encode_ui32leb128(NULL, 0, return_count);
    payload_len += tp_encode_ui32leb128(NULL, 0, return_type);

    TP_WASM_MODULE_SECTION* section = NULL;
    uint8_t* section_buffer = NULL;

    TP_MAKE_WASM_SECTION_BUFFER(
        symbol_table, section, section_buffer, TP_WASM_SECTION_TYPE_TYPE, payload_len
    );

    size_t index = tp_encode_ui32leb128(section_buffer, 0, TP_WASM_SECTION_TYPE_TYPE);
    index += tp_encode_ui32leb128(section_buffer, index, payload_len);
    index += tp_encode_ui32leb128(section_buffer, index, count);
    index += tp_encode_ui32leb128(section_buffer, index, form);
    index += tp_encode_ui32leb128(section_buffer, index, param_count);
//  index += tp_encode_si64leb128(section_buffer, index, param_types);
    index += tp_encode_ui32leb128(section_buffer, index, return_count);
    (void)tp_encode_ui32leb128(section_buffer, index, return_type);

    return section;
}

static TP_WASM_MODULE_SECTION* make_section_function(TP_SYMBOL_TABLE* symbol_table)
{
    uint32_t count = TP_WASM_MODULE_SECTION_FUNCTION_COUNT;
    uint32_t types = TP_WASM_MODULE_SECTION_FUNCTION_TYPES;

    uint32_t payload_len = tp_encode_ui32leb128(NULL, 0, count);
    payload_len += tp_encode_ui32leb128(NULL, 0, types);

    TP_WASM_MODULE_SECTION* section = NULL;
    uint8_t* section_buffer = NULL;

    TP_MAKE_WASM_SECTION_BUFFER(
        symbol_table, section, section_buffer, TP_WASM_SECTION_TYPE_FUNCTION, payload_len
    );

    size_t index = tp_encode_ui32leb128(section_buffer, 0, TP_WASM_SECTION_TYPE_FUNCTION);
    index += tp_encode_ui32leb128(section_buffer, index, payload_len);
    index += tp_encode_ui32leb128(section_buffer, index, count);
    (void)tp_encode_ui32leb128(section_buffer, index, types);

    return section;
}

static TP_WASM_MODULE_SECTION* make_section_table(TP_SYMBOL_TABLE* symbol_table)
{
    uint32_t count = TP_WASM_MODULE_SECTION_TABLE_COUNT;
    uint32_t element_type = TP_WASM_MODULE_SECTION_TABLE_ELEMENT_TYPE_ANYFUNC;
    uint32_t flags = TP_WASM_MODULE_SECTION_TABLE_FLAGS;
    uint32_t initial = TP_WASM_MODULE_SECTION_TABLE_INITIAL;
//  uint32_t maximum;

    uint32_t payload_len = tp_encode_ui32leb128(NULL, 0, count);
    payload_len += tp_encode_ui32leb128(NULL, 0, element_type);
    payload_len += tp_encode_ui32leb128(NULL, 0, flags);
    payload_len += tp_encode_ui32leb128(NULL, 0, initial);
//  payload_len += tp_encode_ui32leb128(NULL, 0, maximum);

    TP_WASM_MODULE_SECTION* section = NULL;
    uint8_t* section_buffer = NULL;

    TP_MAKE_WASM_SECTION_BUFFER(
        symbol_table, section, section_buffer, TP_WASM_SECTION_TYPE_TABLE, payload_len
    );

    size_t index = tp_encode_ui32leb128(section_buffer, 0, TP_WASM_SECTION_TYPE_TABLE);
    index += tp_encode_ui32leb128(section_buffer, index, payload_len);
    index += tp_encode_ui32leb128(section_buffer, index, count);
    index += tp_encode_ui32leb128(section_buffer, index, element_type);
    index += tp_encode_ui32leb128(section_buffer, index, flags);
    (void)tp_encode_ui32leb128(section_buffer, index, initial);
//  (void)tp_encode_ui32leb128(section_buffer, index, maximum);

    return section;
}

static TP_WASM_MODULE_SECTION* make_section_memory(TP_SYMBOL_TABLE* symbol_table)
{
    uint32_t count = TP_WASM_MODULE_SECTION_MEMORY_COUNT;
    uint32_t flags = TP_WASM_MODULE_SECTION_MEMORY_FLAGS;
    uint32_t initial = TP_WASM_MODULE_SECTION_MEMORY_INITIAL;
//  uint32_t maximum;

    uint32_t payload_len = tp_encode_ui32leb128(NULL, 0, count);
    payload_len += tp_encode_ui32leb128(NULL, 0, flags);
    payload_len += tp_encode_ui32leb128(NULL, 0, initial);
//  payload_len += tp_encode_ui32leb128(NULL, 0, maximum);

    TP_WASM_MODULE_SECTION* section = NULL;
    uint8_t* section_buffer = NULL;

    TP_MAKE_WASM_SECTION_BUFFER(
        symbol_table, section, section_buffer, TP_WASM_SECTION_TYPE_MEMORY, payload_len
    );

    size_t index = tp_encode_ui32leb128(section_buffer, 0, TP_WASM_SECTION_TYPE_MEMORY);
    index += tp_encode_ui32leb128(section_buffer, index, payload_len);
    index += tp_encode_ui32leb128(section_buffer, index, count);
    index += tp_encode_ui32leb128(section_buffer, index, flags);
    (void)tp_encode_ui32leb128(section_buffer, index, initial);
//  (void)tp_encode_ui32leb128(section_buffer, index, maximum);

    return section;
}

static TP_WASM_MODULE_SECTION* make_section_export(TP_SYMBOL_TABLE* symbol_table)
{
    uint32_t count = TP_WASM_MODULE_SECTION_EXPORT_COUNT;
    uint32_t name_length_1 = TP_WASM_MODULE_SECTION_EXPORT_NAME_LENGTH_1;
    uint8_t* name_1 = TP_WASM_MODULE_SECTION_EXPORT_NAME_1;
    uint8_t kind_1 = TP_WASM_SECTION_KIND_MEMORY;
    uint32_t item_index_1 = TP_WASM_MODULE_SECTION_EXPORT_ITEM_INDEX_1;

    uint32_t name_length_2 = TP_WASM_MODULE_SECTION_EXPORT_NAME_LENGTH_2;
    uint8_t* name_2 = TP_WASM_MODULE_SECTION_EXPORT_NAME_2;
    uint8_t kind_2 = TP_WASM_SECTION_KIND_FUNCTION;
    uint32_t item_index_2 = TP_WASM_MODULE_SECTION_EXPORT_ITEM_INDEX_2;

    uint32_t payload_len = tp_encode_ui32leb128(NULL, 0, count);
    payload_len += tp_encode_ui32leb128(NULL, 0, name_length_1);
    payload_len += name_length_1;
    payload_len += sizeof(uint8_t);
    payload_len += tp_encode_ui32leb128(NULL, 0, item_index_1);
    payload_len += tp_encode_ui32leb128(NULL, 0, name_length_2);
    payload_len += name_length_2;
    payload_len += sizeof(uint8_t);
    payload_len += tp_encode_ui32leb128(NULL, 0, item_index_2);

    TP_WASM_MODULE_SECTION* section = NULL;
    uint8_t* section_buffer = NULL;

    TP_MAKE_WASM_SECTION_BUFFER(
        symbol_table, section, section_buffer, TP_WASM_SECTION_TYPE_EXPORT, payload_len
    );

    size_t index = tp_encode_ui32leb128(section_buffer, 0, TP_WASM_SECTION_TYPE_EXPORT);
    index += tp_encode_ui32leb128(section_buffer, index, payload_len);
    index += tp_encode_ui32leb128(section_buffer, index, count);
    index += tp_encode_ui32leb128(section_buffer, index, name_length_1);
    memcpy(section_buffer + index, name_1, name_length_1);
    index += name_length_1;
    section_buffer[index] = kind_1;
    index += sizeof(uint8_t);
    index += tp_encode_ui32leb128(section_buffer, index, item_index_1);
    index += tp_encode_ui32leb128(section_buffer, index, name_length_2);
    memcpy(section_buffer + index, name_2, name_length_2);
    index += name_length_2;
    section_buffer[index] = kind_2;
    index += sizeof(uint8_t);
    (void)tp_encode_ui32leb128(section_buffer, index, item_index_2);

    return section;
}

static TP_WASM_MODULE_SECTION* make_section_code_origin_wasm(TP_SYMBOL_TABLE* symbol_table)
{
    uint32_t count = TP_WASM_MODULE_SECTION_CODE_COUNT;
    uint32_t local_count = TP_WASM_MODULE_SECTION_CODE_LOCAL_COUNT;
    uint32_t var_count = TP_WASM_MODULE_SECTION_CODE_VAR_COUNT;
    uint32_t var_type = TP_WASM_MODULE_SECTION_CODE_VAR_TYPE_I32;
    uint32_t body_size = tp_encode_ui32leb128(NULL, 0, local_count);
    body_size += tp_encode_ui32leb128(NULL, 0, var_count);
    body_size += tp_encode_ui32leb128(NULL, 0, var_type);
    body_size += make_i32_const_code(NULL, 0, 1);
    body_size += make_i32_const_code(NULL, 0, 2);
    body_size += make_i32_add_code(NULL, 0);
    body_size += make_i32_const_code(NULL, 0, 3);
    body_size += make_i32_mul_code(NULL, 0);
    body_size += make_set_local_code(NULL, 0, 0);
    body_size += make_i32_const_code(NULL, 0, 2);
    body_size += make_i32_const_code(NULL, 0, 3);
    body_size += make_get_local_code(NULL, 0, 0);
    body_size += make_i32_mul_code(NULL, 0);
    body_size += make_i32_add_code(NULL, 0);
    body_size += make_set_local_code(NULL, 0, 1);
    body_size += make_get_local_code(NULL, 0, 1);
    body_size += make_i32_const_code(NULL, 0, 100);
    body_size += make_i32_add_code(NULL, 0);
    body_size += make_tee_local_code(NULL, 0, 0);
    body_size += make_end_code(NULL, 0);

    uint32_t payload_len = tp_encode_ui32leb128(NULL, 0, count);
    payload_len += tp_encode_ui32leb128(NULL, 0, body_size);
    payload_len += body_size;

    TP_WASM_MODULE_SECTION* section = NULL;
    uint8_t* section_buffer = NULL;

    TP_MAKE_WASM_SECTION_BUFFER(
        symbol_table, section, section_buffer, TP_WASM_SECTION_TYPE_CODE, payload_len
    );

    size_t index = tp_encode_ui32leb128(section_buffer, 0, TP_WASM_SECTION_TYPE_CODE);
    index += tp_encode_ui32leb128(section_buffer, index, payload_len);
    index += tp_encode_ui32leb128(section_buffer, index, count);
    index += tp_encode_ui32leb128(section_buffer, index, body_size);
    index += tp_encode_ui32leb128(section_buffer, index, local_count);
    index += tp_encode_ui32leb128(section_buffer, index, var_count);
    index += tp_encode_ui32leb128(section_buffer, index, var_type);
    index += make_i32_const_code(section_buffer, index, 1);
    index += make_i32_const_code(section_buffer, index, 2);
    index += make_i32_add_code(section_buffer, index);
    index += make_i32_const_code(section_buffer, index, 3);
    index += make_i32_mul_code(section_buffer, index);
    index += make_set_local_code(section_buffer, index, 0);
    index += make_i32_const_code(section_buffer, index, 2);
    index += make_i32_const_code(section_buffer, index, 3);
    index += make_get_local_code(section_buffer, index, 0);
    index += make_i32_mul_code(section_buffer, index);
    index += make_i32_add_code(section_buffer, index);
    index += make_set_local_code(section_buffer, index, 1);
    index += make_get_local_code(section_buffer, index, 1);
    index += make_i32_const_code(section_buffer, index, 100);
    index += make_i32_add_code(section_buffer, index);
    index += make_tee_local_code(section_buffer, index, 0);
    (void)make_end_code(section_buffer, index);

    return section;
}

static TP_WASM_MODULE_SECTION* make_section_code(TP_SYMBOL_TABLE* symbol_table)
{
    uint32_t count = TP_WASM_MODULE_SECTION_CODE_COUNT;
    uint32_t local_count = TP_WASM_MODULE_SECTION_CODE_LOCAL_COUNT;
    uint32_t var_count = symbol_table->member_var_count; // Calculated by semantic analysis.
    uint32_t var_type = TP_WASM_MODULE_SECTION_CODE_VAR_TYPE_I32;

    uint32_t body_size = tp_encode_ui32leb128(NULL, 0, local_count);
    body_size += tp_encode_ui32leb128(NULL, 0, var_count);
    body_size += tp_encode_ui32leb128(NULL, 0, var_type);
    symbol_table->member_code_body_size = body_size;

    if ( ! search_parse_tree(symbol_table, symbol_table->member_tp_parse_tree, NULL)){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return NULL;
    }

    symbol_table->member_code_body_size += make_end_code(NULL, 0);

    uint32_t payload_len = tp_encode_ui32leb128(NULL, 0, count);
    payload_len += tp_encode_ui32leb128(NULL, 0, symbol_table->member_code_body_size);
    payload_len += symbol_table->member_code_body_size;

    TP_WASM_MODULE_SECTION* section = NULL;
    symbol_table->member_code_section_buffer = NULL;

    TP_MAKE_WASM_SECTION_BUFFER(
        symbol_table, section, symbol_table->member_code_section_buffer,
        TP_WASM_SECTION_TYPE_CODE, payload_len
    );

    uint8_t* section_buffer = symbol_table->member_code_section_buffer;
    size_t index = tp_encode_ui32leb128(section_buffer, 0, TP_WASM_SECTION_TYPE_CODE);
    index += tp_encode_ui32leb128(section_buffer, index, payload_len);
    index += tp_encode_ui32leb128(section_buffer, index, count);
    index += tp_encode_ui32leb128(section_buffer, index, symbol_table->member_code_body_size);
    index += tp_encode_ui32leb128(section_buffer, index, local_count);
    index += tp_encode_ui32leb128(section_buffer, index, var_count);
    index += tp_encode_ui32leb128(section_buffer, index, var_type);
    symbol_table->member_code_index = index;

    if ( ! search_parse_tree(symbol_table, symbol_table->member_tp_parse_tree, section)){

        if (symbol_table->member_code_section_buffer){

            TP_FREE(symbol_table, &(symbol_table->member_code_section_buffer), payload_len);
        }

        if (section){

            TP_FREE(symbol_table, &section, sizeof(TP_WASM_MODULE_SECTION));
        }

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return NULL;
    }

    (void)make_end_code(symbol_table->member_code_section_buffer, symbol_table->member_code_index);

    return section;
}

static bool search_parse_tree(TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section)
{
    bool is_make_section_code_success = true;

    TP_PARSE_TREE_ELEMENT* element = parse_tree->member_element;

    size_t element_num = parse_tree->member_element_num;

    for (size_t i = 0; element_num > i; ++i){

        if (TP_PARSE_TREE_TYPE_NULL == element[i].member_type){

            break;
        }

        if (TP_PARSE_TREE_TYPE_NODE == element[i].member_type){

            if ( ! search_parse_tree(symbol_table, (TP_PARSE_TREE*)(element[i].member_body.member_child), section)){

                is_make_section_code_success = false;
            }
        }
    }

    if ( ! make_section_code_content(symbol_table, parse_tree, section)){

        is_make_section_code_success = false;
    }

    return is_make_section_code_success;
}

static bool make_section_code_content(TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section)
{
    switch (parse_tree->member_grammer){
    case TP_PARSE_TREE_GRAMMER_PROGRAM:
        break;
    case TP_PARSE_TREE_GRAMMER_STATEMENT_1:
//      break;
    case TP_PARSE_TREE_GRAMMER_STATEMENT_2:
        if ( ! wasm_gen_statement_1_and_2(symbol_table, parse_tree, section)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            return false;
        }
        break;
    case TP_PARSE_TREE_GRAMMER_EXPRESSION_1:
//      break;
    case TP_PARSE_TREE_GRAMMER_EXPRESSION_2:
        if ( ! wasm_gen_expression_1_and_2(symbol_table, parse_tree, section)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            return false;
        }
        break;
    case TP_PARSE_TREE_GRAMMER_TERM_1:
//      break;
    case TP_PARSE_TREE_GRAMMER_TERM_2:
        if ( ! wasm_gen_term_1_and_2(symbol_table, parse_tree, section)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            return false;
        }
        break;
    case TP_PARSE_TREE_GRAMMER_FACTOR_1:
        if ( ! wasm_gen_factor_1(symbol_table, parse_tree, section)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            return false;
        }
        break;
    case TP_PARSE_TREE_GRAMMER_FACTOR_2:
//      break;
    case TP_PARSE_TREE_GRAMMER_FACTOR_3:
        if ( ! wasm_gen_factor_2_and_3(symbol_table, parse_tree, section)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            return false;
        }
        break;
    default:

        TP_PUT_LOG_MSG_ICE(symbol_table);

        return false;
    }

    return true;
}

static bool wasm_gen_statement_1_and_2(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section)
{
    // Grammer: Statement -> Type? variable '=' Expression ';'

    uint32_t var_value = 0;

    switch (parse_tree->member_grammer){
    // Grammer: Statement -> variable '=' Expression ';'
    case TP_PARSE_TREE_GRAMMER_STATEMENT_1:
        if (symbol_table->member_grammer_statement_1_num != parse_tree->member_element_num){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
        if ( ! get_var_value(symbol_table, parse_tree, 0, &var_value)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            return false;
        }
        break;
    // Grammer: Statement -> Type variable '=' Expression ';'
    case TP_PARSE_TREE_GRAMMER_STATEMENT_2:
        if (symbol_table->member_grammer_statement_2_num != parse_tree->member_element_num){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
        if ( ! get_var_value(symbol_table, parse_tree, 1, &var_value)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            return false;
        }
        break;
    default:

        TP_PUT_LOG_MSG_ICE(symbol_table);

        return false;
    }

    if (NULL == symbol_table->member_last_statement){ // Setup by semantic analysis.

        TP_PUT_LOG_MSG_ICE(symbol_table);

        return false;
    }

    if (symbol_table->member_last_statement == parse_tree){

        if (section){

            symbol_table->member_code_index += make_tee_local_code(
                symbol_table->member_code_section_buffer, symbol_table->member_code_index, var_value
            );
        }else{

            symbol_table->member_code_body_size += make_tee_local_code(NULL, 0, var_value);
        }
    }else{

        if (section){

            symbol_table->member_code_index += make_set_local_code(
                symbol_table->member_code_section_buffer, symbol_table->member_code_index, var_value
            );
        }else{

            symbol_table->member_code_body_size += make_set_local_code(NULL, 0, var_value);
        }
    }

    return true;
}

static bool wasm_gen_expression_1_and_2(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section)
{
    // Grammer: Expression -> Term (('+' | '-') Term)*

    switch (parse_tree->member_grammer){
    case TP_PARSE_TREE_GRAMMER_EXPRESSION_1:
        if (symbol_table->member_grammer_expression_1_num != parse_tree->member_element_num){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
        if (TP_PARSE_TREE_TYPE_TOKEN != symbol_table->member_parse_tree_type[TP_GRAMMER_TYPE_INDEX_EXPRESSION_1][1]){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
        break;
    case TP_PARSE_TREE_GRAMMER_EXPRESSION_2:
        if (symbol_table->member_grammer_expression_2_num != parse_tree->member_element_num){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
        if (TP_PARSE_TREE_TYPE_TOKEN != symbol_table->member_parse_tree_type[TP_GRAMMER_TYPE_INDEX_EXPRESSION_2][1]){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
        break;
    default:

        TP_PUT_LOG_MSG_ICE(symbol_table);

        return false;
    }

    bool is_add = (TP_SYMBOL_PLUS == parse_tree->member_element[1].member_body.member_tp_token->member_symbol);

    if (section){

        if (is_add){

            symbol_table->member_code_index += make_i32_add_code(
                symbol_table->member_code_section_buffer, symbol_table->member_code_index
            );
        }else{

            symbol_table->member_code_index += make_i32_sub_code(
                symbol_table->member_code_section_buffer, symbol_table->member_code_index
            );
        }
    }else{

        if (is_add){

            symbol_table->member_code_body_size += make_i32_add_code(NULL, 0);
        }else{

            symbol_table->member_code_body_size += make_i32_sub_code(NULL, 0);
        }
    }

    return true;
}

static bool wasm_gen_term_1_and_2(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section)
{
    // Grammer: Term -> Factor (('*' | '/') Factor)*

    switch (parse_tree->member_grammer){
    case TP_PARSE_TREE_GRAMMER_TERM_1:
        if (symbol_table->member_grammer_term_1_num != parse_tree->member_element_num){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
        if (TP_PARSE_TREE_TYPE_TOKEN != symbol_table->member_parse_tree_type[TP_GRAMMER_TYPE_INDEX_TERM_1][1]){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
        break;
    case TP_PARSE_TREE_GRAMMER_TERM_2:
        if (symbol_table->member_grammer_term_2_num != parse_tree->member_element_num){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
        if (TP_PARSE_TREE_TYPE_TOKEN != symbol_table->member_parse_tree_type[TP_GRAMMER_TYPE_INDEX_TERM_2][1]){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
        break;
    default:

        TP_PUT_LOG_MSG_ICE(symbol_table);

        return false;
    }

    bool is_mul = (TP_SYMBOL_MUL == parse_tree->member_element[1].member_body.member_tp_token->member_symbol);

    if (section){

        if (is_mul){

            symbol_table->member_code_index += make_i32_mul_code(
                symbol_table->member_code_section_buffer, symbol_table->member_code_index
            );
        }else{

            symbol_table->member_code_index += make_i32_div_code(
                symbol_table->member_code_section_buffer, symbol_table->member_code_index
            );
        }
    }else{

        if (is_mul){

            symbol_table->member_code_body_size += make_i32_mul_code(NULL, 0);
        }else{

            symbol_table->member_code_body_size += make_i32_div_code(NULL, 0);
        }
    }

    return true;
}

static bool wasm_gen_factor_1(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section)
{
    // Factor -> '(' Expression ')'

    switch (parse_tree->member_grammer){
    case TP_PARSE_TREE_GRAMMER_FACTOR_1:
        if (symbol_table->member_grammer_factor_1_num != parse_tree->member_element_num){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
        // NOTE: Skip left paren and right_paren.
        break;
    default:

        TP_PUT_LOG_MSG_ICE(symbol_table);

        return false;
    }

    return true;
}

static bool wasm_gen_factor_2_and_3(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, TP_WASM_MODULE_SECTION* section)
{
    // Factor -> ('+' | '-')? (variable | constant)

    bool is_minus = false;
    bool is_const = false;
    int32_t const_value = 0;
    uint32_t var_value = 0;

    switch (parse_tree->member_grammer){
    // Factor -> ('+' | '-') (variable | constant)
    case TP_PARSE_TREE_GRAMMER_FACTOR_2:
        if (symbol_table->member_grammer_factor_2_num != parse_tree->member_element_num){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
        if ((TP_PARSE_TREE_TYPE_TOKEN != symbol_table->member_parse_tree_type[TP_GRAMMER_TYPE_INDEX_FACTOR_2][0]) ||
            (TP_PARSE_TREE_TYPE_TOKEN != symbol_table->member_parse_tree_type[TP_GRAMMER_TYPE_INDEX_FACTOR_2][1])){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }

        is_minus = (TP_SYMBOL_MINUS == parse_tree->member_element[0].member_body.member_tp_token->member_symbol);
        is_const = (TP_SYMBOL_CONST_VALUE == parse_tree->member_element[1].member_body.member_tp_token->member_symbol);

        if (is_const){

            const_value = parse_tree->member_element[1].member_body.member_tp_token->member_i32_value;
        }else{

            if ( ! get_var_value(symbol_table, parse_tree, 1, &var_value)){

                TP_PUT_LOG_MSG_ICE(symbol_table);

                return false;
            }
        }
        break;
    // Factor -> variable | constant
    case TP_PARSE_TREE_GRAMMER_FACTOR_3:
        if (symbol_table->member_grammer_factor_3_num != parse_tree->member_element_num){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
        if (TP_PARSE_TREE_TYPE_TOKEN != symbol_table->member_parse_tree_type[TP_GRAMMER_TYPE_INDEX_FACTOR_3][0]){

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }

        is_const = (TP_SYMBOL_CONST_VALUE == parse_tree->member_element[0].member_body.member_tp_token->member_symbol);

        if (is_const){

            const_value = parse_tree->member_element[0].member_body.member_tp_token->member_i32_value;
        }else{

            if ( ! get_var_value(symbol_table, parse_tree, 0, &var_value)){

                TP_PUT_LOG_MSG_TRACE(symbol_table);

                return false;
            }
        }
        break;
    default:

        TP_PUT_LOG_MSG_ICE(symbol_table);

        return false;
    }

    uint8_t* p = symbol_table->member_code_section_buffer;
    size_t code_index = symbol_table->member_code_index;
    uint32_t code_body_size = symbol_table->member_code_body_size;

    if (is_const){

        if (section){

            code_index += make_i32_const_code(p, code_index, (is_minus ? -const_value : const_value));
        }else{

            code_body_size += make_i32_const_code(NULL, 0, (is_minus ? -const_value : const_value));
        }
    }else{

        if (is_minus){

            // Change of sign.
            if (section){

                code_index += make_i32_const_code(p, code_index, -1);
                code_index += make_get_local_code(p, code_index, var_value);
                code_index += make_i32_xor_code(p, code_index);
                code_index += make_i32_const_code(p, code_index, 1);
                code_index += make_i32_add_code(p, code_index);
            }else{

                code_body_size += make_i32_const_code(NULL, 0, -1);
                code_body_size += make_get_local_code(NULL, 0, var_value);
                code_body_size += make_i32_xor_code(NULL, 0);
                code_body_size += make_i32_const_code(NULL, 0, 1);
                code_body_size += make_i32_add_code(NULL, 0);
            }
        }else{

            if (section){

                code_index += make_get_local_code(p, code_index, var_value);
            }else{

                code_body_size += make_get_local_code(NULL, 0, var_value);
            }
        }
    }

    symbol_table->member_code_index = code_index;
    symbol_table->member_code_body_size = code_body_size;

    return true;
}

static bool get_var_value(
    TP_SYMBOL_TABLE* symbol_table, TP_PARSE_TREE* parse_tree, size_t index, uint32_t* var_value)
{
    REGISTER_OBJECT register_object = { 0 };

    TP_TOKEN* token = parse_tree->member_element[index].member_body.member_tp_token;

    if (TP_SYMBOL_ID != token->member_symbol){

        TP_PUT_LOG_MSG_ICE(symbol_table);

        return false;
    }

    if (tp_search_object(symbol_table, token, &register_object)){

        switch (register_object.member_register_object_type){
        case DEFINED_REGISTER_OBJECT:

            *var_value = register_object.member_var_index; // Calculated by semantic analysis.

            break;
        case UNDEFINED_REGISTER_OBJECT:

            TP_PUT_LOG_MSG(
                symbol_table, TP_LOG_TYPE_DISP_FORCE,
                TP_MSG_FMT("ERROR: use undefined symbol(%1)."),
                TP_LOG_PARAM_STRING(token->member_string)
            );

            return false;
        default:

            TP_PUT_LOG_MSG_ICE(symbol_table);

            return false;
        }
    }else{

        TP_PUT_LOG_MSG_ICE(symbol_table);

        return false;
    }

    return true;
}

static uint32_t make_get_local_code(uint8_t* buffer, size_t offset, uint32_t value)
{
    TP_MAKE_ULEB128_CODE(buffer, offset, TP_WASM_OPCODE_GET_LOCAL, value);
}

static uint32_t make_set_local_code(uint8_t* buffer, size_t offset, uint32_t value)
{
    TP_MAKE_ULEB128_CODE(buffer, offset, TP_WASM_OPCODE_SET_LOCAL, value);
}

static uint32_t make_tee_local_code(uint8_t* buffer, size_t offset, uint32_t value)
{
    TP_MAKE_ULEB128_CODE(buffer, offset, TP_WASM_OPCODE_TEE_LOCAL, value);
}

static uint32_t make_i32_const_code(uint8_t* buffer, size_t offset, int32_t value)
{
    TP_MAKE_SLEB128_CODE(buffer, offset, TP_WASM_OPCODE_I32_CONST, value);
}

static uint32_t make_i32_add_code(uint8_t* buffer, size_t offset)
{
    TP_MAKE_OPCODE(buffer, offset, TP_WASM_OPCODE_I32_ADD);
}

static uint32_t make_i32_sub_code(uint8_t* buffer, size_t offset)
{
    TP_MAKE_OPCODE(buffer, offset, TP_WASM_OPCODE_I32_SUB);
}

static uint32_t make_i32_mul_code(uint8_t* buffer, size_t offset)
{
    TP_MAKE_OPCODE(buffer, offset, TP_WASM_OPCODE_I32_MUL);
}

static uint32_t make_i32_div_code(uint8_t* buffer, size_t offset)
{
    TP_MAKE_OPCODE(buffer, offset, TP_WASM_OPCODE_I32_DIV);
}

static uint32_t make_i32_xor_code(uint8_t* buffer, size_t offset)
{
    TP_MAKE_OPCODE(buffer, offset, TP_WASM_OPCODE_I32_XOR);
}

static uint32_t make_end_code(uint8_t* buffer, size_t offset)
{
    TP_MAKE_OPCODE(buffer, offset, TP_WASM_OPCODE_END);
}

