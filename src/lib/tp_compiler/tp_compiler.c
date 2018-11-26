
// Copyright (C) 2018 Shin'ichi Ichikawa. Released under the MIT license.

#include "tp_compiler.h"

static TP_SYMBOL_TABLE init_symbol_table_value = {
// config section:
    .member_is_output_current_dir = false,
    .member_is_output_log_file = false,
    .member_is_no_output_messages = false,
    .member_is_no_output_files = false,
    .member_is_origin_wasm = false,
    .member_is_source_cmd_param = false,
    .member_source_code = { 0 },
    .member_is_test_mode = false,
    .member_is_output_wasm_file = false,
    .member_is_output_x64_file = false,

// message section:
    .member_log_hide_after_disp = false,
    .member_temp_buffer = { 0 },
    .member_log_msg_buffer = { 0 },
    .member_disp_log_file = NULL,

// output file section:
    .member_write_log_file = NULL,
    .member_parse_tree_file = NULL,
    .member_write_log_file_path = { 0 },
    .member_token_file_path = { 0 },
    .member_parse_tree_file_path = { 0 },
    .member_object_hash_file_path = { 0 },
    .member_wasm_file_path = { 0 },
    .member_x64_file_path = { 0 },

// input file section:
    .member_input_file_path = { 0 },
    .member_read_file = NULL,
    .member_is_start_of_file = true,
    .member_is_end_of_file = false,
    .member_read_lines_buffer = { 0 },
    .member_read_lines_length = 0,
    .member_read_lines_current_position = 0,
    .member_read_lines_end_position = 0,

// token section:
    .member_tp_token_pos = 0,
    .member_tp_token_size = 0,
    .member_tp_token_size_allocate_unit = TP_TOKEN_SIZE_ALLOCATE_UNIT,
    .member_tp_token = NULL,
    .member_tp_token_position = NULL,
    .member_nul_num = 0,

// parse tree section:
    .member_nesting_level_of_expression = 0,
    .member_tp_parse_tree = NULL,

// semantic analysis section:
    .member_object_hash.member_mask = UINT8_MAX,
    .member_object_hash.member_hash_table = { 0 },
    .member_var_count = 0,
    .member_last_statement = NULL,
    .member_parse_tree_type = {
        // TP_GRAMMER_TYPE_INDEX_STATEMENT_1, Grammer: Statement -> variable '=' Expression ';'
        { TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_NODE, TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_NULL },
        // TP_GRAMMER_TYPE_INDEX_STATEMENT_2, Grammer: Statement -> Type variable '=' Expression ';'
        { TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_NODE, TP_PARSE_TREE_TYPE_TOKEN },
        // TP_GRAMMER_TYPE_INDEX_EXPRESSION_1, Grammer: Expression -> Term (('+' | '-') Term)*
        { TP_PARSE_TREE_TYPE_NODE, TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_NODE, TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL },
        // TP_GRAMMER_TYPE_INDEX_EXPRESSION_2, Grammer: Expression -> Term (('+' | '-') Term)*
        { TP_PARSE_TREE_TYPE_NODE, TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_NODE, TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL },
        // TP_GRAMMER_TYPE_INDEX_TERM_1, Grammer: Term -> Factor (('*' | '/') Factor)*
        { TP_PARSE_TREE_TYPE_NODE, TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_NODE, TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL },
        // TP_GRAMMER_TYPE_INDEX_TERM_2, Grammer: Term -> Factor (('*' | '/') Factor)*
        { TP_PARSE_TREE_TYPE_NODE, TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_NODE, TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL },
        // TP_GRAMMER_TYPE_INDEX_FACTOR_1, Grammer: Factor -> '(' Expression ')'
        { TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_NODE, TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL },
        // TP_GRAMMER_TYPE_INDEX_FACTOR_2, Grammer: Factor -> ('+' | '-') (variable | constant)
        { TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL },
        // TP_GRAMMER_TYPE_INDEX_FACTOR_3, Grammer: Factor -> variable | constant
        { TP_PARSE_TREE_TYPE_TOKEN, TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL },
        // NULL
        { TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL, TP_PARSE_TREE_TYPE_NULL }
    },
    .member_grammer_statement_1_num = 0,
    .member_grammer_statement_2_num = 0,
    .member_grammer_expression_1_num = 0,
    .member_grammer_expression_2_num = 0,
    .member_grammer_term_1_num = 0,
    .member_grammer_term_2_num = 0,
    .member_grammer_factor_1_num = 0,
    .member_grammer_factor_2_num = 0,
    .member_grammer_factor_3_num = 0,

// wasm section:
    .member_wasm_module = { 0 },
    .member_code_index = 0,
    .member_code_body_size = 0,
    .member_code_section_buffer = NULL,

// x64 section:
    .member_stack = NULL,
    .member_wasm_code_body_buffer = NULL,
    .member_wasm_code_body_pos = 0,
    .member_wasm_code_body_size = 0,
    .member_stack_pos = TP_WASM_STACK_EMPTY,
    .member_stack_size = 0,
    .member_stack_size_allocate_unit = TP_WASM_STACK_SIZE_ALLOCATE_UNIT,

    .member_local_variable_size = 0,
    .member_local_variable_size_max = TP_WASM_LOCAL_VARIABLE_MAX_DEFAULT,
    .member_padding_local_variable_bytes = 0,

    .member_temporary_variable_size = 0,
    .member_temporary_variable_size_max = TP_WASM_TEMPORARY_VARIABLE_MAX_DEFAULT,
    .member_padding_temporary_variable_bytes = 0,

    .member_use_X86_32_register = { 0 },
    .member_use_X64_32_register = { 0 },
    .member_use_nv_register = { TP_X64_NV64_REGISTER_NULL },

    .member_register_bytes = 0,
    .member_padding_register_bytes = 0,

    .member_stack_imm32 = 0
};

typedef struct test_case_table_{
    uint8_t* member_source_code;
    int32_t member_return_value;
}TEST_CASE_TABLE;

static TEST_CASE_TABLE test_case_table[] = {

    { "int32_t value1 = 1 + 2;\n", 3 },
    { "int32_t value1 = 1 - 2;\n", -1 },
    { "int32_t value1 = 1 * 2;\n", 2 },
    { "int32_t value1 = 4 / 2;\n", 2 },

    { "int32_t value1 = 1;\n"
    "int32_t value2 = value1 + 2;\n", 3 },

    { "int32_t value1 = 1;\n"
    "int32_t value2 = value1 - 2;\n", -1 },

    { "int32_t value1 = 1;\n"
    "int32_t value2 = value1 * 2;\n", 2 },

    { "int32_t value1 = 4;\n"
    "int32_t value2 = value1 / 2;\n", 2 },

    { "int32_t value1 = 1;\n"
    "int32_t value2 = 2 + value1;\n", 3 },

    { "int32_t value1 = 1;\n"
    "int32_t value2 = 2 - value1;\n", 1 },

    { "int32_t value1 = 1;\n"
    "int32_t value2 = 2 * value1;\n", 2 },

    { "int32_t value1 = 2;\n"
    "int32_t value2 = 4 / value1;\n", 2 },

    { "int32_t value1 = 1;\n"
    "int32_t value2 = 2;\n"
    "int32_t value3 = value1 + value2;\n", 3 },

    { "int32_t value1 = 1;\n"
    "int32_t value2 = 2;\n"
    "int32_t value3 = value1 - value2;\n", -1 },

    { "int32_t value1 = 1;\n"
    "int32_t value2 = 2;\n"
    "int32_t value3 = value1 * value2;\n", 2 },

    { "int32_t value1 = 4;\n"
    "int32_t value2 = 2;\n"
    "int32_t value3 = value1 / value2;\n", 2 },

    { "int32_t value1 = (1 + 2) * 3;\n"
    "int32_t value2 = 2 + (3 * value1);\n"
    "value1 = value2 + 100;\n", 129 },

    { NULL, 0 }
};

static bool test_compiler(
    int argc, char** argv, uint8_t* msg_buffer, size_t msg_buffer_size,
    char* drive, char* dir, time_t now
);
static bool compiler_main(
    int argc, char** argv, uint8_t* msg_buffer, size_t msg_buffer_size,
    bool* is_test_mode, size_t test_index, int32_t* return_value,
    char* drive, char* dir, time_t now
);
static bool init_symbol_table(
    TP_SYMBOL_TABLE* symbol_table, int argc, char** argv, bool* is_disp_usage,
    bool* is_test, size_t test_index, time_t now, char* drive, char* dir,
    uint8_t* msg_buffer, size_t msg_buffer_size
);
static bool make_path_log_files(
    TP_SYMBOL_TABLE* symbol_table, char* drive, char* dir,
    bool is_test, size_t test_index, time_t now
);
static bool make_path_test_log_files(
    TP_SYMBOL_TABLE* symbol_table, char* drive, char* dir_param,
    bool is_test, size_t test_index, time_t now
);
static bool move_test_log_files(char* drive, char* dir_param, bool is_test, time_t now);
static bool make_path_log_files_main(
    TP_SYMBOL_TABLE* symbol_table, char* drive, char* dir, bool is_test
);
static bool make_path(
    TP_SYMBOL_TABLE* symbol_table, char* drive, char* dir, char* prefix, char* fname, char* ext,
    char* path, size_t path_size
);
static uint32_t calc_grammer_type_num(TP_SYMBOL_TABLE* symbol_table, size_t grammer_type_index);
static bool parse_cmd_line_param(
    int argc, char** argv, TP_SYMBOL_TABLE* symbol_table, bool* is_disp_usage, bool* is_test
);
static void free_memory_and_file(TP_SYMBOL_TABLE** symbol_table);

bool tp_compiler(int argc, char** argv, uint8_t* msg_buffer, size_t msg_buffer_size)
{
    SetLastError(NO_ERROR);

    errno_t err = _set_errno(0);

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);

    bool is_test_mode = false;

    time_t now = time(NULL);

    char drive[_MAX_DRIVE];
    memset(drive, 0, sizeof(drive));

    char dir[_MAX_DIR];
    memset(dir, 0, sizeof(dir));

    if ( ! compiler_main(
        argc, argv, msg_buffer, msg_buffer_size,
        &is_test_mode, 0, NULL, drive, dir, now)){

        _CrtDumpMemoryLeaks();

        return false;
    }

    if (is_test_mode){

        if ( ! test_compiler(
            argc, argv, msg_buffer, msg_buffer_size, drive, dir, now)){

            _CrtDumpMemoryLeaks();

            return false;
        }
    }

    _CrtDumpMemoryLeaks();

    return true;
}

static bool test_compiler(
    int argc, char** argv, uint8_t* msg_buffer, size_t msg_buffer_size,
    char* drive, char* dir, time_t now)
{
    bool status = true;

    bool is_test_mode = true;

    size_t size = (sizeof(test_case_table) / sizeof(TEST_CASE_TABLE));

    for (size_t i = 0; size > i; ++i){

        uint8_t* source_code = test_case_table[i].member_source_code;

        if (NULL == source_code){

            break;
        }

        int32_t return_value = 0;

        if ( ! compiler_main(
            argc, argv, msg_buffer, msg_buffer_size,
            &is_test_mode, i, &return_value, drive, dir, now)){

            status = false;

            fprintf_s(stderr, "ERROR: compile failed. test case No.%03zd.\n", i + 1);
        }else{

            int32_t correct_value = test_case_table[i].member_return_value;

            if (return_value == correct_value){

                fprintf_s(stderr, "SUCCESS: test case No.%03zd.\n", i + 1);
            }else{

                fprintf_s(
                    stderr, "ERROR: test case No.%03zd: "
                    "return value=(%d), correct value=(%d),\n"
                    "source code=(%s).\n",
                    i + 1, return_value, correct_value, source_code
                );
            }
        }
    }

    (void)move_test_log_files(drive, dir, is_test_mode, now);

    return status;
}

static bool compiler_main(
    int argc, char** argv, uint8_t* msg_buffer, size_t msg_buffer_size,
    bool* is_test_mode, size_t test_index, int32_t* return_value,
    char* drive, char* dir, time_t now)
{
    TP_SYMBOL_TABLE* symbol_table = (TP_SYMBOL_TABLE*)calloc(1, sizeof(TP_SYMBOL_TABLE));

    if (NULL == symbol_table){

        TP_PRINT_CRT_ERROR(NULL);

        goto error_proc;
    }

    bool is_disp_usage = false;

    bool is_test = (is_test_mode ? *is_test_mode : false);

    if ( ! init_symbol_table(
        symbol_table, argc, argv, &is_disp_usage,
        is_test_mode, test_index, now, drive, dir, msg_buffer, msg_buffer_size)){

        if ( ! is_disp_usage){

            fprintf(stderr, "ERROR: Initialize failed.\n");
        }

        free_memory_and_file(&symbol_table);

        return false;
    }

    if (is_test_mode){

        if ((false == is_test) && *is_test_mode){

            // switch to test mode.
            free_memory_and_file(&symbol_table);

            return true;
        }
    }

    bool is_origin_wasm = symbol_table->member_is_origin_wasm;

    if (is_origin_wasm){

        if ( ! tp_make_wasm(symbol_table, is_origin_wasm)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            goto error_proc;
        }

        if ( ! tp_make_x64_code(symbol_table, return_value)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            goto error_proc;
        }
    }else{

        if (is_test_mode && *is_test_mode){

            if ( ! tp_make_token(
                symbol_table,
                test_case_table[test_index].member_source_code,
                strlen(test_case_table[test_index].member_source_code))){

                TP_PUT_LOG_MSG_TRACE(symbol_table);

                goto error_proc;
            }
        }else{

            if (symbol_table->member_is_source_cmd_param){

                if ( ! tp_make_token(
                    symbol_table,
                    symbol_table->member_source_code, strlen(symbol_table->member_source_code))){

                    TP_PUT_LOG_MSG_TRACE(symbol_table);

                    goto error_proc;
                }
            }else{

                if ( ! tp_make_token(symbol_table, NULL, 0)){

                    TP_PUT_LOG_MSG_TRACE(symbol_table);

                    goto error_proc;
                }
            }
        }

        if ( ! tp_make_parse_tree(symbol_table)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            goto error_proc;
        }

        if ( ! tp_semantic_analysis(symbol_table)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            goto error_proc;
        }

        if ( ! tp_make_wasm(symbol_table, is_origin_wasm)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            goto error_proc;
        }

        if ( ! tp_make_x64_code(symbol_table, return_value)){

            TP_PUT_LOG_MSG_TRACE(symbol_table);

            goto error_proc;
        }
    }

    free_memory_and_file(&symbol_table);

    return true;

error_proc:

    if (symbol_table){

        TP_PUT_LOG_MSG(
            symbol_table, TP_LOG_TYPE_DISP_FORCE,
            TP_MSG_FMT("%1"), TP_LOG_PARAM_STRING("ERROR: Compile failed.")
        );
    }

    free_memory_and_file(&symbol_table);

    return false;
}

static bool init_symbol_table(
    TP_SYMBOL_TABLE* symbol_table, int argc, char** argv, bool* is_disp_usage,
    bool* is_test, size_t test_index, time_t now, char* drive, char* dir,
    uint8_t* msg_buffer, size_t msg_buffer_size)
{
    errno_t err = 0;

    *symbol_table = init_symbol_table_value;

    symbol_table->member_disp_log_file = stderr;

    if (0 != setvbuf(symbol_table->member_disp_log_file, msg_buffer, _IOFBF, msg_buffer_size)){

        goto error_out;
    }

    symbol_table->member_grammer_statement_1_num = calc_grammer_type_num(symbol_table, TP_GRAMMER_TYPE_INDEX_STATEMENT_1);
    symbol_table->member_grammer_statement_2_num = calc_grammer_type_num(symbol_table, TP_GRAMMER_TYPE_INDEX_STATEMENT_2);
    symbol_table->member_grammer_expression_1_num = calc_grammer_type_num(symbol_table, TP_GRAMMER_TYPE_INDEX_EXPRESSION_1);
    symbol_table->member_grammer_expression_2_num = calc_grammer_type_num(symbol_table, TP_GRAMMER_TYPE_INDEX_EXPRESSION_2);
    symbol_table->member_grammer_term_1_num = calc_grammer_type_num(symbol_table, TP_GRAMMER_TYPE_INDEX_TERM_1);
    symbol_table->member_grammer_term_2_num = calc_grammer_type_num(symbol_table, TP_GRAMMER_TYPE_INDEX_TERM_2);
    symbol_table->member_grammer_factor_1_num = calc_grammer_type_num(symbol_table, TP_GRAMMER_TYPE_INDEX_FACTOR_1);
    symbol_table->member_grammer_factor_2_num = calc_grammer_type_num(symbol_table, TP_GRAMMER_TYPE_INDEX_FACTOR_2);
    symbol_table->member_grammer_factor_3_num = calc_grammer_type_num(symbol_table, TP_GRAMMER_TYPE_INDEX_FACTOR_3);

    bool is_normal = false;

    if (is_test && (false == *is_test)){

        is_normal = true;
    }

    if ( ! parse_cmd_line_param(argc, argv, symbol_table, is_disp_usage, is_test)){

        return false;
    }

    if (is_test && *is_test && is_normal){

        // switch to test mode.
        return true;
    }

    char base_dir[_MAX_PATH];
    memset(base_dir, 0, sizeof(base_dir));

    if (symbol_table->member_is_output_current_dir){

        DWORD status = GetCurrentDirectoryA(sizeof(base_dir), base_dir);

        if (0 == status){

            goto error_out;
        }
    }else{

        HMODULE handle = GetModuleHandleA(NULL);

        if (0 == handle){

            goto error_out;
        }

        DWORD status = GetModuleFileNameA(handle, base_dir, sizeof(base_dir));

        if (0 == status){

            goto error_out;
        }
    }

    err = _splitpath_s(base_dir, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);

    if (err){

        goto error_out;
    }

    if ( ! make_path_log_files(symbol_table, drive, dir, *is_test, test_index, now)){

        return false;
    }

    return true;

error_out:

    if (err){

        TP_PRINT_CRT_ERROR(NULL);
    }else{

        TP_GET_LAST_ERROR(NULL);
    }

    return false;
}

static bool make_path_log_files(
    TP_SYMBOL_TABLE* symbol_table, char* drive, char* dir_param, bool is_test, size_t test_index, time_t now)
{
    if (is_test){

        if ( ! make_path_test_log_files(symbol_table, drive, dir_param, is_test, test_index, now)){

            fprintf_s(stderr, "ERROR: already exists.\n");

            return false;
        }
    }else{

        if ( ! make_path_log_files_main(symbol_table, drive, dir_param, is_test)){

            return false;
        }
    }

    return true;
}

static bool make_path_test_log_files(
    TP_SYMBOL_TABLE* symbol_table, char* drive, char* dir_param, bool is_test, size_t test_index, time_t now)
{
    struct tm local_time = { 0 };

    errno_t err = localtime_s(&local_time, &now);

    if (err){

        TP_PRINT_CRT_ERROR(NULL);

        return false;
    }

    char dir[_MAX_DIR];
    memset(dir, 0, sizeof(dir));

    sprintf_s(
        dir, sizeof(dir), "%s\\test_%04d-%02d-%02d",
        dir_param, local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday
    );

    char path[_MAX_PATH];
    memset(path, 0, sizeof(path));

    err = _makepath_s(path, sizeof(path), drive, dir, NULL, NULL);

    if (err){

        TP_PRINT_CRT_ERROR(NULL);

        return false;
    }

    DWORD attributes = GetFileAttributesA(path);

    if (-1 == attributes){

        SetLastError(NO_ERROR);

        if ( ! CreateDirectoryA(path, NULL)){

            TP_GET_LAST_ERROR(NULL);

            return false;
        }
    }

    if (TP_TEST_FNAME_NUM_MAX < test_index){

        fprintf_s(stderr, "ERROR: overflow test case(%03zd).\n", test_index);

        return false;
    }

    memset(dir, 0, sizeof(dir));

    sprintf_s(
        dir, sizeof(dir), "%s\\test_%04d-%02d-%02d\\test_case_%03zd",
        dir_param, local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday, test_index + 1
    );

    memset(path, 0, sizeof(path));

    err = _makepath_s(path, sizeof(path), drive, dir, NULL, NULL);

    if (err){

        TP_PRINT_CRT_ERROR(NULL);

        return false;
    }

    attributes = GetFileAttributesA(path);

    if (-1 == attributes){

        SetLastError(NO_ERROR);

        if ( ! CreateDirectoryA(path, NULL)){

            TP_GET_LAST_ERROR(NULL);

            return false;
        }

        if ( ! make_path_log_files_main(symbol_table, drive, dir, is_test)){

            return false;
        }

        return true;
    }

    return false;
}

static bool move_test_log_files(char* drive, char* dir_param, bool is_test, time_t now)
{
    if ( ! is_test){

        return true;
    }

    struct tm local_time = { 0 };

    errno_t err = localtime_s(&local_time, &now);

    if (err){

        TP_PRINT_CRT_ERROR(NULL);

        return false;
    }

    char dir[_MAX_DIR];
    memset(dir, 0, sizeof(dir));

    sprintf_s(
        dir, sizeof(dir), "%s\\test_%04d-%02d-%02d",
        dir_param, local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday
    );

    char path[_MAX_PATH];
    memset(path, 0, sizeof(path));

    err = _makepath_s(path, sizeof(path), drive, dir, NULL, NULL);

    if (err){

        TP_PRINT_CRT_ERROR(NULL);

        return false;
    }

    DWORD attributes = GetFileAttributesA(path);

    if (-1 == attributes){

        TP_GET_LAST_ERROR(NULL);

        return false;
    }

    for (size_t i = 1; TP_TEST_FNAME_NUM_MAX >= i; ++i){

        char new_path[_MAX_PATH];
        memset(new_path, 0, sizeof(new_path));

        memset(dir, 0, sizeof(dir));

        sprintf_s(
            dir, sizeof(dir), "%s\\test_%04d-%02d-%02d_%03zd",
            dir_param, local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday, i
        );

        err = _makepath_s(new_path, sizeof(new_path), drive, dir, NULL, NULL);

        if (err){

            TP_PRINT_CRT_ERROR(NULL);

            return false;
        }

        DWORD attributes = GetFileAttributesA(new_path);

        if (-1 != attributes){

            continue;
        }

        SetLastError(NO_ERROR);

        if ( ! MoveFileA(path, new_path)){

            TP_GET_LAST_ERROR(NULL);

            return false;
        }

        return true;
    }

    fprintf_s(stderr, "ERROR: overflow move directory suffix number.\n");

    return false;
}

static bool make_path_log_files_main(TP_SYMBOL_TABLE* symbol_table, char* drive, char* dir, bool is_test)
{
    if ( ! make_path(
        symbol_table, drive, dir, TP_LOG_FILE_PREFIX,
        TP_WRITE_LOG_DEFAULT_FILE_NAME, TP_WRITE_LOG_DEFAULT_EXT_NAME,
        symbol_table->member_write_log_file_path,
        sizeof(symbol_table->member_write_log_file_path))){

        TP_PRINT_CRT_ERROR(NULL);

        return false;
    }

    if (symbol_table->member_is_output_log_file){

        FILE* write_log_file = NULL;

        if ( ! tp_open_write_file(NULL, symbol_table->member_write_log_file_path, &write_log_file)){

            TP_PRINT_CRT_ERROR(NULL);

            return false;
        }

        symbol_table->member_write_log_file = write_log_file;

        if (0 != setvbuf(
            symbol_table->member_write_log_file, symbol_table->member_log_msg_buffer, _IOFBF,
            sizeof(symbol_table->member_log_msg_buffer))){

            TP_PRINT_CRT_ERROR(NULL);

            return false;
        }
    }

    if ( ! make_path(
        symbol_table, drive, dir, TP_LOG_FILE_PREFIX,
        TP_TOKEN_DEFAULT_FILE_NAME, TP_TOKEN_DEFAULT_EXT_NAME,
        symbol_table->member_token_file_path,
        sizeof(symbol_table->member_token_file_path))){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    if ( ! make_path(
        symbol_table, drive, dir, TP_LOG_FILE_PREFIX,
        TP_PARSE_TREE_DEFAULT_FILE_NAME, TP_PARSE_TREE_DEFAULT_EXT_NAME,
        symbol_table->member_parse_tree_file_path,
        sizeof(symbol_table->member_parse_tree_file_path))){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    if ( ! make_path(
        symbol_table, drive, dir, TP_LOG_FILE_PREFIX,
        TP_OBJECT_HASH_DEFAULT_FILE_NAME, TP_OBJECT_HASH_DEFAULT_EXT_NAME,
        symbol_table->member_object_hash_file_path,
        sizeof(symbol_table->member_object_hash_file_path))){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    if ( ! make_path(
        symbol_table, drive, dir, NULL,
        TP_WASM_DEFAULT_FILE_NAME, TP_WASM_DEFAULT_EXT_NAME,
        symbol_table->member_wasm_file_path,
        sizeof(symbol_table->member_wasm_file_path))){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    if ( ! make_path(
        symbol_table, drive, dir, NULL,
        TP_X64_DEFAULT_FILE_NAME, TP_X64_DEFAULT_EXT_NAME,
        symbol_table->member_x64_file_path,
        sizeof(symbol_table->member_x64_file_path))){

        TP_PUT_LOG_MSG_TRACE(symbol_table);

        return false;
    }

    return true;
}

static bool make_path(
    TP_SYMBOL_TABLE* symbol_table, char* drive, char* dir, char* prefix, char* fname, char* ext,
    char* path, size_t path_size)
{
    char tmp_fname[_MAX_FNAME];
    memset(tmp_fname, 0, sizeof(tmp_fname));

    if (prefix){

        sprintf_s(tmp_fname, sizeof(tmp_fname), "%s_%s", prefix, fname);
    }else{

        sprintf_s(tmp_fname, sizeof(tmp_fname), "%s", fname);
    }

    errno_t err = _makepath_s(path, path_size, drive, dir, tmp_fname, ext);

    if (err){

        TP_PRINT_CRT_ERROR(symbol_table);

        return false;
    }

    return true;
}

static uint32_t calc_grammer_type_num(TP_SYMBOL_TABLE* symbol_table, size_t grammer_type_index)
{
    uint32_t grammer_type_num = 0;

    for (size_t i = 0; TP_PARSE_TREE_TYPE_MAX_NUM1 > i; ++i){

        if (TP_PARSE_TREE_TYPE_NULL == symbol_table->member_parse_tree_type[grammer_type_index][i]){

            break;
        }

        ++grammer_type_num;
    }

    return grammer_type_num;
}

static bool parse_cmd_line_param(
    int argc, char** argv, TP_SYMBOL_TABLE* symbol_table, bool* is_disp_usage, bool* is_test)
{
    char* command_line_param = NULL;

    for (int i = 0; argc > i; ++i){

        if (0 == i){

            continue;
        }

        char* param = argv[i];

        if (('/' != param[0]) && ('-' != param[0])){

            if (command_line_param){

                goto error_out;
            }else{

                command_line_param = param;
            }
        }else{

            rsize_t length = strlen(param);

            for (int j = 1; length > j; ++j){

                switch (param[j]){
                case TP_CONFIG_OPTION_IS_OUTPUT_CURRENT_DIR:
                    symbol_table->member_is_output_current_dir = true;
                    break;
                case TP_CONFIG_OPTION_IS_OUTPUT_LOG_FILE:
                    symbol_table->member_is_output_log_file = true;
                    break;
                case TP_CONFIG_OPTION_IS_NO_OUTPUT_MESSAGES:
                    symbol_table->member_is_no_output_messages = true;
                    break;
                case TP_CONFIG_OPTION_IS_NO_OUTPUT_FILES:
                    symbol_table->member_is_no_output_files = true;
                    break;
                case TP_CONFIG_OPTION_IS_ORIGIN_WASM:
                    symbol_table->member_is_origin_wasm = true;
                    break;
                case TP_CONFIG_OPTION_IS_SOURCE_CMD_PARAM:
                    symbol_table->member_is_source_cmd_param = true;
                    break;
                case TP_CONFIG_OPTION_IS_TEST_MODE:
                    *is_test = true;
                    symbol_table->member_is_test_mode = true;
                    symbol_table->member_is_output_log_file = true;
                    break;
                case TP_CONFIG_OPTION_IS_OUTPUT_WASM_FILE:
                    symbol_table->member_is_output_wasm_file = true;
                    break;
                case TP_CONFIG_OPTION_IS_OUTPUT_X64_FILE:
                    symbol_table->member_is_output_x64_file = true;
                    break;
                default:
                    goto error_out;
                }
            }
        }
    }

    if (command_line_param &&
        (symbol_table->member_is_origin_wasm || symbol_table->member_is_test_mode)){

        goto error_out;
    }

    if ((NULL == command_line_param) && (1 == argc)){

        goto error_out;
    }

    if (symbol_table->member_is_source_cmd_param){

        size_t length = strlen(command_line_param);

        if (TP_SOURCE_CODE_STRING_LENGTH_MAX < length){

            goto error_out;
        }

        sprintf_s(
            symbol_table->member_source_code,
            sizeof(symbol_table->member_source_code),
            "%s", command_line_param
        );
    }

    if (command_line_param &&
        ((false == symbol_table->member_is_origin_wasm) &&
        (false == symbol_table->member_is_source_cmd_param))){

        sprintf_s(
            symbol_table->member_input_file_path,
            sizeof(symbol_table->member_input_file_path),
            "%s", command_line_param
        );
    }

    errno_t err = _set_errno(0);

    return true;

error_out:

    *is_disp_usage = true;

    fprintf_s(stderr, "usage: int_calc_compiler [-/][rcmlnwx] [input file] [source code string]\n");
    fprintf_s(stderr, "  -c : set output current directory.\n");
    fprintf_s(stderr, "  -l : set output log file.\n");
    fprintf_s(stderr, "  -m : set no output messages.\n");
    fprintf_s(stderr, "  -n : set no output files.\n");
    fprintf_s(stderr, "  -r : set origin WASM. [input file] is not necessary.\n");
    fprintf_s(stderr, "  -s : set source code command line parameter mode.\n");
    fprintf_s(
        stderr,
        "       need [source code string] up to %d characters. [input file] is not necessary.\n",
        TP_SOURCE_CODE_STRING_LENGTH_MAX
    );
    fprintf_s(stderr, "  -t : set test mode. [input file] is not necessary.\n");
    fprintf_s(stderr, "  -w : set output WASM file.\n");
    fprintf_s(stderr, "  -x : set output x64 file.\n");

    err = _set_errno(0);

    return false;
}

static void free_memory_and_file(TP_SYMBOL_TABLE** symbol_table)
{
    if (NULL == symbol_table){

        return;
    }

    if ((*symbol_table)->member_tp_token){

        TP_FREE(*symbol_table, &((*symbol_table)->member_tp_token), (*symbol_table)->member_tp_token_size);
    }

    if ((*symbol_table)->member_tp_parse_tree){

        tp_free_parse_subtree(*symbol_table, &((*symbol_table)->member_tp_parse_tree));
    }

    tp_free_object_hash(*symbol_table, &((*symbol_table)->member_object_hash), (*symbol_table)->member_object_hash.member_hash_table);

    TP_WASM_MODULE* wasm_module = &((*symbol_table)->member_wasm_module);

    TP_WASM_MODULE_SECTION** section = wasm_module->member_section;

    if (section){

        for (size_t i = 0; wasm_module->member_section_num > i; ++i){

            if (section[i]) {

                TP_FREE(*symbol_table, &(section[i]->member_name_len_name_payload_data), section[i]->member_section_size);

                TP_FREE(*symbol_table, &(section[i]), sizeof(TP_WASM_MODULE_SECTION));
            }
        }

        TP_FREE2(*symbol_table, &section, wasm_module->member_section_num * sizeof(TP_WASM_MODULE_SECTION*));
    }

    if (wasm_module->member_module_content){

        TP_FREE(*symbol_table, &(wasm_module->member_module_content),
            sizeof(TP_WASM_MODULE_CONTENT) + wasm_module->member_content_size);
    }

    if ((*symbol_table)->member_stack){

        TP_FREE(*symbol_table, &((*symbol_table)->member_stack), (*symbol_table)->member_stack_size);
    }

    if ( ! tp_close_file(*symbol_table, &((*symbol_table)->member_read_file))){

        TP_PUT_LOG_MSG_TRACE(*symbol_table);
    }

    if ( ! tp_close_file(*symbol_table, &((*symbol_table)->member_parse_tree_file))){

        TP_PUT_LOG_MSG_TRACE(*symbol_table);
    }

    (void)tp_close_file(NULL, &((*symbol_table)->member_write_log_file));

    TP_FREE(NULL, symbol_table, sizeof(TP_SYMBOL_TABLE));
}

