#include "codegen.h"

#include <limits.h>
#include <stdarg.h>
#include <string.h>

// Convert symbol to valid assembly identifier (replace special chars with _)
static void sanitize_name(char* dest, const char* src, size_t max_len) {
    size_t i;
    for (i = 0; i < max_len - 1 && src[i]; i++) {
        switch (src[i]) {
        case '-':
        case '/':
        case '.':
        case '?': dest[i] = '_'; break;
        default: dest[i] = src[i]; break;
        }
    }
    dest[i] = '\0';
}

static void set_error(SeCodegen* cg, size_t line, const char* msg) {
    if (cg->has_error) return;
    cg->has_error = true;
    cg->error_line = line;
    strncpy(cg->error_msg, msg, sizeof(cg->error_msg) - 1);
    cg->error_msg[sizeof(cg->error_msg) - 1] = '\0';
}

static void emit(SeCodegen* cg, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(cg->output, fmt, args);
    va_end(args);
}

static void emit_line(SeCodegen* cg, const char* fmt, ...) {
    fprintf(cg->output, "    ");
    va_list args;
    va_start(args, fmt);
    vfprintf(cg->output, fmt, args);
    va_end(args);
    fprintf(cg->output, "\n");
}

static int new_label(SeCodegen* cg) { return cg->label_counter++; }

static bool is_constant(SeCodegen* cg, const char* name, int32_t* value) {
    for (size_t i = 0; i < cg->constant_count; i++) {
        if (strcmp(cg->constants[i].name, name) == 0 && !cg->constants[i].is_function) {
            if (value) *value = cg->constants[i].value;
            return true;
        }
    }
    return false;
}

__attribute__((unused)) static bool is_function(SeCodegen* cg, const char* name) {
    for (size_t i = 0; i < cg->function_count; i++) {
        if (strcmp(cg->functions[i].name, name) == 0) return true;
    }
    return false;
}

// Look up function info by name (returns NULL if not found)
static SeFunctionInfo* find_function_info(SeCodegen* cg, const char* name) {
    for (size_t i = 0; i < cg->function_count; i++) {
        if (strcmp(cg->functions[i].name, name) == 0) return &cg->functions[i];
    }
    return NULL;
}

// Look up a record type definition
static SeRecordType* find_record_type(SeCodegen* cg, const char* name) {
    for (size_t i = 0; i < cg->record_count; i++) {
        if (strcmp(cg->records[i].name, name) == 0) {
            return &cg->records[i];
        }
    }
    return NULL;
}

// Look up a field in a record type by keyword (e.g., ":x")
static SeRecordField* find_record_field(SeRecordType* rec, const char* keyword) {
    // keyword includes ':', e.g. ":x" - field name is stored without ':'
    const char* field_name = keyword;
    if (field_name[0] == ':') field_name++;

    for (size_t i = 0; i < rec->field_count; i++) {
        if (strcmp(rec->fields[i].name, field_name) == 0) {
            return &rec->fields[i];
        }
    }
    return NULL;
}

// Look up a data label by name (returns full struct)
static SeDataLabel* find_data_label(SeCodegen* cg, const char* name) {
    for (size_t i = 0; i < cg->data_label_count; i++) {
        if (strcmp(cg->data_labels[i].name, name) == 0) {
            return &cg->data_labels[i];
        }
    }
    return NULL;
}

// Look up which record type a data label is an instance of
static SeRecordType* find_var_record_type(SeCodegen* cg, const char* var_name) {
    for (size_t i = 0; i < cg->data_label_count; i++) {
        if (strcmp(cg->data_labels[i].name, var_name) == 0 &&
            cg->data_labels[i].record_type[0] != '\0') {
            return find_record_type(cg, cg->data_labels[i].record_type);
        }
    }
    return NULL;
}

static SeLocal* find_local_info(SeCodegen* cg, const char* name) {
    for (size_t i = 0; i < cg->local_count; i++) {
        if (strcmp(cg->locals[i].name, name) == 0) {
            return &cg->locals[i];
        }
    }
    return NULL;
}

// Load local variable into R0 (and R1 if 16-bit). Handles param (offset >= 3) vs let-local.
static void emit_local_load(SeCodegen* cg, SeLocal* local) {
    int offset = local->offset;
    if (offset >= 3) {
        emit_line(cg, "LOAD R0, [R4:R5 + %d]", offset);
        if (local->is_16bit) {
            emit_line(cg, "LOAD R1, [R4:R5 + %d]", offset + 1);
        }
    } else {
        if (offset >= 0) {
            emit_line(cg, "LOAD R0, [R2:R3 + %d]", offset);
            if (local->is_16bit) {
                emit_line(cg, "LOAD R1, [R2:R3 + %d]", offset + 1);
            }
        } else {
            emit_line(cg, "LOAD R0, [R2:R3 - %d]", -offset);
            if (local->is_16bit) {
                emit_line(cg, "LOAD R1, [R2:R3 - %d]", -(offset - 1));
            }
        }
    }
}

// Store R0 (and R1 if 16-bit) into local. Caller must have emitted value into R0/R1 first.
static void emit_local_store(SeCodegen* cg, SeLocal* local) {
    int offset = local->offset;
    if (offset >= 3) {
        emit_line(cg, "STORE R0, [R4:R5 + %d]", offset);
        if (local->is_16bit) {
            emit_line(cg, "STORE R1, [R4:R5 + %d]", offset + 1);
        }
    } else {
        if (offset >= 0) {
            emit_line(cg, "STORE R0, [R2:R3 + %d]", offset);
            if (local->is_16bit) {
                emit_line(cg, "STORE R1, [R2:R3 + %d]", offset + 1);
            }
        } else {
            emit_line(cg, "STORE R0, [R2:R3 - %d]", -offset);
            if (local->is_16bit) {
                emit_line(cg, "STORE R1, [R2:R3 - %d]", -(offset - 1));
            }
        }
    }
}

// Forward declaration for eval_const (used by expr_is_16bit)
static bool eval_const(SeCodegen* cg, AstNode* node, int32_t* result);

// Resolve the record type for a record expression (symbol or (nth arr i))
static SeRecordType* resolve_record_type(SeCodegen* cg, AstNode* record_node) {
    if (record_node->kind == AST_SYMBOL) {
        return find_var_record_type(cg, record_node->as.symbol.name);
    }
    if (record_node->kind == AST_NTH) {
        AstNode* arr = record_node->as.binary.left;
        if (arr->kind == AST_SYMBOL) {
            SeDataLabel* label = find_data_label(cg, arr->as.symbol.name);
            if (label && label->record_type[0] != '\0') {
                return find_record_type(cg, label->record_type);
            }
        }
    }
    return NULL;
}

// Determine if an expression produces a 16-bit result
// Returns true if the expression should be treated as 16-bit (u16/i16)
static bool expr_is_16bit(SeCodegen* cg, AstNode* node) {
    if (!node) return false;
    switch (node->kind) {
    case AST_NUMBER: return node->as.number > 255 || node->as.number < -128;
    case AST_SYMBOL: {
        // Check locals
        SeLocal* local = find_local_info(cg, node->as.symbol.name);
        if (local) return local->is_16bit;
        // Check data labels
        SeDataLabel* dl = find_data_label(cg, node->as.symbol.name);
        if (dl) {
            // Data blocks evaluate to their 16-bit address
            if (dl->is_data_block) return true;
            return dl->is_16bit;
        }
        // Check constants
        int32_t val;
        if (is_constant(cg, node->as.symbol.name, &val)) {
            return val > 255 || val < -128;
        }
        return false;
    }
    case AST_ADD:
    case AST_SUB:
    case AST_MUL:
    case AST_BAND:
    case AST_BOR:
    case AST_XOR:
    case AST_SHR:
        return expr_is_16bit(cg, node->as.binary.left) || expr_is_16bit(cg, node->as.binary.right);
    case AST_SHL: {
        // Left shift: if shift amount is a compile-time constant >= 8,
        // the result always needs 16 bits (even if the operand is 8-bit)
        if (expr_is_16bit(cg, node->as.binary.left)) return true;
        int32_t shift_amt;
        if (eval_const(cg, node->as.binary.right, &shift_amt) && shift_amt >= 8) return true;
        return expr_is_16bit(cg, node->as.binary.right);
    }
    case AST_NEG:
    case AST_INC:
    case AST_DEC:
    case AST_BNOT: return expr_is_16bit(cg, node->as.unary.operand);
    case AST_CAST_U8:
    case AST_CAST_I8: return false; // Casts explicitly narrow to 8-bit
    case AST_HI:
    case AST_LO: return false; // hi/lo always return 8-bit
    case AST_FIELD_GET: {
        // Field access: check if the field is 16-bit
        // Supports both (:field var) and (:field (nth arr i))
        SeRecordType* rec = resolve_record_type(cg, node->as.field_get.record);
        if (rec) {
            SeRecordField* field = find_record_field(rec, node->as.field_get.field);
            if (field) return field->is_16bit;
        }
        return false;
    }
    case AST_CALL:
        // Function calls: we don't track return width yet, default to 8-bit
        return false;
    case AST_EQ:
    case AST_NE:
    case AST_LT:
    case AST_GT:
    case AST_LE:
    case AST_GE:
    case AST_LNOT:
    case AST_LOGIC_NOT:
    case AST_LOGIC_AND:
    case AST_LOGIC_OR:
    case AST_NILP:
    case AST_ZEROP:
    case AST_POSP:
    case AST_NEGP: return false; // Comparisons and logic always return 8-bit bool
    default: return false;
    }
}

// Determine if an expression produces a signed result (i8/i16)
static bool expr_is_signed(SeCodegen* cg, AstNode* node) {
    if (!node) return false;
    switch (node->kind) {
    case AST_NUMBER: return node->as.number < 0;
    case AST_SYMBOL: {
        SeLocal* local = find_local_info(cg, node->as.symbol.name);
        if (local) return local->is_signed;
        SeDataLabel* dl = find_data_label(cg, node->as.symbol.name);
        if (dl) return dl->is_signed;
        int32_t val;
        if (is_constant(cg, node->as.symbol.name, &val)) return val < 0;
        return false;
    }
    case AST_ADD:
    case AST_SUB:
    case AST_MUL:
    case AST_BAND:
    case AST_BOR:
    case AST_XOR:
        return expr_is_signed(cg, node->as.binary.left) ||
               expr_is_signed(cg, node->as.binary.right);
    case AST_NEG: return true; // Negation always produces a signed value
    case AST_INC:
    case AST_DEC:
    case AST_BNOT: return expr_is_signed(cg, node->as.unary.operand);
    case AST_CAST_I8: return true;
    case AST_CAST_U8: return false;
    case AST_FIELD_GET: {
        // Check defrecord field hint
        // Supports both (:field var) and (:field (nth arr i))
        SeRecordType* rec = resolve_record_type(cg, node->as.field_get.record);
        if (rec) {
            SeRecordField* field = find_record_field(rec, node->as.field_get.field);
            if (field) return field->is_signed;
        }
        return false;
    }
    default: return false;
    }
}

static bool add_local_full(SeCodegen* cg, const char* name, int offset, bool is_16bit,
                           bool is_signed) {
    if (cg->local_count >= SE_MAX_LOCALS) return false;
    strncpy(cg->locals[cg->local_count].name, name, SE_MAX_SYMBOL_LEN - 1);
    cg->locals[cg->local_count].name[SE_MAX_SYMBOL_LEN - 1] = '\0';
    cg->locals[cg->local_count].offset = offset;
    cg->locals[cg->local_count].is_16bit = is_16bit;
    cg->locals[cg->local_count].is_signed = is_signed;
    cg->local_count++;
    return true;
}

static bool add_local(SeCodegen* cg, const char* name, int offset) {
    return add_local_full(cg, name, offset, false, false);
}

void se_codegen_init(SeCodegen* cg, FILE* output, const char* filename) {
    memset(cg, 0, sizeof(*cg));
    cg->output = output;
    cg->filename = filename;
    cg->label_counter = 0;
    cg->data_base_addr = TINY16_MEMORY_DATA_BEGIN;
    cg->data_current_addr = TINY16_MEMORY_DATA_BEGIN;
}

static void collect_keywords_child_cb(AstNode** child_ptr, void* ctx) {
    SeCodegen* cg = (SeCodegen*)ctx;
    if (!*child_ptr || cg->has_error) return;
    AstNode* n = *child_ptr;
    if (n->kind == AST_KEYWORD) {
        for (size_t i = 0; i < cg->keyword_count; i++) {
            if (strcmp(cg->keywords[i], n->as.symbol.name) == 0) return;
        }
        if (cg->keyword_count >= SE_MAX_KEYWORDS) {
            set_error(cg, n->line, "too many keywords");
            return;
        }
        strncpy(cg->keywords[cg->keyword_count], n->as.symbol.name, SE_MAX_SYMBOL_LEN - 1);
        cg->keywords[cg->keyword_count][SE_MAX_SYMBOL_LEN - 1] = '\0';
        cg->keyword_count++;
        return;
    }
    ast_for_each_child(n, collect_keywords_child_cb, ctx);
}

// Recursively collect keyword names from AST for interning
static void collect_keywords_from_node(SeCodegen* cg, AstNode* node) {
    if (!node || cg->has_error) return;
    if (node->kind == AST_KEYWORD) {
        for (size_t i = 0; i < cg->keyword_count; i++) {
            if (strcmp(cg->keywords[i], node->as.symbol.name) == 0) return;
        }
        if (cg->keyword_count >= SE_MAX_KEYWORDS) {
            set_error(cg, node->line, "too many keywords");
            return;
        }
        strncpy(cg->keywords[cg->keyword_count], node->as.symbol.name, SE_MAX_SYMBOL_LEN - 1);
        cg->keywords[cg->keyword_count][SE_MAX_SYMBOL_LEN - 1] = '\0';
        cg->keyword_count++;
        return;
    }
    ast_for_each_child(node, collect_keywords_child_cb, cg);
}

static bool eval_const(SeCodegen* cg, AstNode* node, int32_t* result);
static void emit_expr(SeCodegen* cg, AstNode* node);
static void emit_literal(SeCodegen* cg, AstNode* node);
static void emit_arithmetic(SeCodegen* cg, AstNode* node);
static void emit_bitwise(SeCodegen* cg, AstNode* node);
static void emit_comparison(SeCodegen* cg, AstNode* node);
static void emit_control_flow(SeCodegen* cg, AstNode* node);
static void emit_memory(SeCodegen* cg, AstNode* node);
static void emit_variables(SeCodegen* cg, AstNode* node);
static void emit_records(SeCodegen* cg, AstNode* node);
static void emit_call_expr(SeCodegen* cg, AstNode* node);

// Emit code to set R6:R7 to 16-bit address from expression (constant or (+ const offset)).
static void emit_addr_to_r6r7(SeCodegen* cg, AstNode* addr_node) {
    if (addr_node->kind == AST_SYMBOL) {
        SeDataLabel* dl = find_data_label(cg, addr_node->as.symbol.name);
        if (dl) {
            emit_line(cg, "LOADI R6, 0x%02X", (dl->addr >> 8) & 0xFF);
            emit_line(cg, "LOADI R7, 0x%02X", dl->addr & 0xFF);
            return;
        }
    }
    int32_t val;
    if (eval_const(cg, addr_node, &val)) {
        emit_line(cg, "LOADI R6, 0x%02X", (val >> 8) & 0xFF);
        emit_line(cg, "LOADI R7, 0x%02X", val & 0xFF);
        return;
    }
    if (addr_node->kind == AST_ADD) {
        int32_t base;
        int lbl_skip = new_label(cg);
        // Optimization: (+ constant u8_expr) — only valid when right operand is 8-bit
        if (eval_const(cg, addr_node->as.binary.left, &base) &&
            !expr_is_16bit(cg, addr_node->as.binary.right)) {
            emit_line(cg, "LOADI R6, 0x%02X", (base >> 8) & 0xFF);
            emit_line(cg, "LOADI R7, 0x%02X", base & 0xFF);
            emit_expr(cg, addr_node->as.binary.right);
            emit_line(cg, "ADD R7, R0");
            emit_line(cg, "JNC __L%d", lbl_skip);
            emit_line(cg, "INC R6");
            emit(cg, "__L%d:\n", lbl_skip);
            return;
        }
        if (eval_const(cg, addr_node->as.binary.right, &base) &&
            !expr_is_16bit(cg, addr_node->as.binary.left)) {
            emit_expr(cg, addr_node->as.binary.left);
            emit_line(cg, "PUSH R0");
            emit_line(cg, "LOADI R6, 0x%02X", (base >> 8) & 0xFF);
            emit_line(cg, "LOADI R7, 0x%02X", base & 0xFF);
            emit_line(cg, "POP R0");
            emit_line(cg, "ADD R7, R0");
            emit_line(cg, "JNC __L%d", lbl_skip);
            emit_line(cg, "INC R6");
            emit(cg, "__L%d:\n", lbl_skip);
            return;
        }
    }
    // Fallback: evaluate as a 16-bit expression (R0=hi, R1=lo), move to R6:R7
    if (expr_is_16bit(cg, addr_node)) {
        emit_expr(cg, addr_node);
        emit_line(cg, "MOV R6, R0");
        emit_line(cg, "MOV R7, R1");
        return;
    }
    set_error(cg, addr_node->line,
              "load/store address must be constant, (+ const offset), or 16-bit expression");
}

// Calculate the size of a data body element
static int32_t calc_data_size(AstNode* node) {
    switch (node->kind) {
    case AST_NUMBER:
    case AST_SYMBOL: return 1;
    case AST_STRING: return (int32_t)strlen(node->as.symbol.name);
    case AST_DB: {
        int32_t size = 0;
        for (size_t i = 0; i < node->as.block.exprs.count; i++) {
            size += calc_data_size(node->as.block.exprs.items[i]);
        }
        return size;
    }
    case AST_REPEAT: return node->as.repeat.count * calc_data_size(node->as.repeat.form);
    default: return 1; // Assume constant expression = 1 byte
    }
}

// Evaluate compile-time constant expression
static bool eval_const(SeCodegen* cg, AstNode* node, int32_t* result) {
    switch (node->kind) {
    case AST_NUMBER: *result = node->as.number; return true;
    case AST_NIL: *result = 0xFF; return true;
    case AST_TRUE: *result = 1; return true;
    case AST_FALSE: *result = 0; return true;

    case AST_SYMBOL: {
        int32_t val;
        if (is_constant(cg, node->as.symbol.name, &val)) {
            *result = val;
            return true;
        }
        // Note: data labels (var, data) are NOT compile-time constants.
        // Their addresses are only available via (hi label) / (lo label).
        return false;
    }

    case AST_ADD: {
        int32_t left, right;
        if (!eval_const(cg, node->as.binary.left, &left)) return false;
        if (!eval_const(cg, node->as.binary.right, &right)) return false;
        *result = (left + right) & 0xFFFF;
        return true;
    }

    case AST_SUB: {
        int32_t left, right;
        if (!eval_const(cg, node->as.binary.left, &left)) return false;
        if (!eval_const(cg, node->as.binary.right, &right)) return false;
        *result = (left - right) & 0xFFFF;
        return true;
    }

    case AST_BAND: {
        int32_t left, right;
        if (!eval_const(cg, node->as.binary.left, &left)) return false;
        if (!eval_const(cg, node->as.binary.right, &right)) return false;
        *result = left & right;
        return true;
    }

    case AST_BOR: {
        int32_t left, right;
        if (!eval_const(cg, node->as.binary.left, &left)) return false;
        if (!eval_const(cg, node->as.binary.right, &right)) return false;
        *result = left | right;
        return true;
    }

    case AST_KEYWORD: {
        for (size_t i = 0; i < cg->keyword_count; i++) {
            if (strcmp(cg->keywords[i], node->as.symbol.name) == 0) {
                *result = (int32_t)i;
                return true;
            }
        }
        return false;
    }

    case AST_SHL: {
        int32_t left, right;
        if (!eval_const(cg, node->as.binary.left, &left)) return false;
        if (!eval_const(cg, node->as.binary.right, &right)) return false;
        *result = (left << right) & 0xFFFF;
        return true;
    }

    case AST_SHR: {
        int32_t left, right;
        if (!eval_const(cg, node->as.binary.left, &left)) return false;
        if (!eval_const(cg, node->as.binary.right, &right)) return false;
        *result = (left >> right) & 0xFFFF;
        return true;
    }

    case AST_MUL: {
        int32_t left, right;
        if (!eval_const(cg, node->as.binary.left, &left)) return false;
        if (!eval_const(cg, node->as.binary.right, &right)) return false;
        *result = (left * right) & 0xFFFF;
        return true;
    }

    case AST_DIV: {
        int32_t left, right;
        if (!eval_const(cg, node->as.binary.left, &left)) return false;
        if (!eval_const(cg, node->as.binary.right, &right)) return false;
        if (right == 0) return false;
        *result = left / right;
        return true;
    }

    case AST_MOD: {
        int32_t left, right;
        if (!eval_const(cg, node->as.binary.left, &left)) return false;
        if (!eval_const(cg, node->as.binary.right, &right)) return false;
        if (right == 0) return false;
        *result = left % right;
        return true;
    }

    case AST_HI: {
        int32_t val;
        if (node->as.unary.operand->kind == AST_SYMBOL) {
            SeDataLabel* dl = find_data_label(cg, node->as.unary.operand->as.symbol.name);
            if (dl) {
                *result = (dl->addr >> 8) & 0xFF;
                return true;
            }
        }
        if (!eval_const(cg, node->as.unary.operand, &val)) return false;
        *result = (val >> 8) & 0xFF;
        return true;
    }

    case AST_LO: {
        int32_t val;
        if (node->as.unary.operand->kind == AST_SYMBOL) {
            SeDataLabel* dl = find_data_label(cg, node->as.unary.operand->as.symbol.name);
            if (dl) {
                *result = dl->addr & 0xFF;
                return true;
            }
        }
        if (!eval_const(cg, node->as.unary.operand, &val)) return false;
        *result = val & 0xFF;
        return true;
    }

    case AST_LEN: {
        // (len array) returns the element count as compile-time constant
        if (node->as.unary.operand->kind != AST_SYMBOL) return false;
        SeDataLabel* label = find_data_label(cg, node->as.unary.operand->as.symbol.name);
        if (!label || label->element_count == 0) return false;
        *result = label->element_count;
        return true;
    }

    case AST_CAST_U8: {
        int32_t val;
        if (!eval_const(cg, node->as.unary.operand, &val)) return false;
        *result = val & 0xFF;
        return true;
    }
    case AST_CAST_I8: {
        int32_t val;
        if (!eval_const(cg, node->as.unary.operand, &val)) return false;
        int8_t signed_val = (int8_t)(val & 0xFF);
        *result = signed_val;
        return true;
    }

    default: return false;
    }
}

static void collect_anon_fns_child_cb(AstNode** child_ptr, void* ctx) {
    SeCodegen* cg = (SeCodegen*)ctx;
    if (!*child_ptr || cg->has_error) return;
    AstNode* n = *child_ptr;
    if (n->kind == AST_FN) {
        if (cg->anon_fn_count >= SE_MAX_FUNCTIONS) {
            set_error(cg, n->line, "too many anonymous functions");
            return;
        }
        cg->anon_fns[cg->anon_fn_count++] = n;
        if (cg->function_count >= SE_MAX_FUNCTIONS) {
            set_error(cg, n->line, "too many functions");
            return;
        }
        strncpy(cg->functions[cg->function_count].name, n->as.defn.name, SE_MAX_SYMBOL_LEN - 1);
        cg->functions[cg->function_count].param_count = n->as.defn.param_count;
        for (size_t j = 0; j < n->as.defn.param_count; j++) {
            cg->functions[cg->function_count].param_hints[j] = n->as.defn.param_hints[j];
        }
        cg->function_count++;
    }
    ast_for_each_child(n, collect_anon_fns_child_cb, ctx);
}

// Recursively find all AST_FN nodes in the tree and register them
static void collect_anon_fns(SeCodegen* cg, AstNode* node) {
    if (!node || cg->has_error) return;
    if (node->kind == AST_FN) {
        if (cg->anon_fn_count >= SE_MAX_FUNCTIONS) {
            set_error(cg, node->line, "too many anonymous functions");
            return;
        }
        cg->anon_fns[cg->anon_fn_count++] = node;
        if (cg->function_count >= SE_MAX_FUNCTIONS) {
            set_error(cg, node->line, "too many functions");
            return;
        }
        strncpy(cg->functions[cg->function_count].name, node->as.defn.name, SE_MAX_SYMBOL_LEN - 1);
        cg->functions[cg->function_count].param_count = node->as.defn.param_count;
        for (size_t j = 0; j < node->as.defn.param_count; j++) {
            cg->functions[cg->function_count].param_hints[j] = node->as.defn.param_hints[j];
        }
        cg->function_count++;
    }
    ast_for_each_child(node, collect_anon_fns_child_cb, cg);
}

bool se_codegen_collect(SeCodegen* cg, AstProgram* program) {
    // 0th pass: collect all keywords so eval_const can resolve them
    for (size_t i = 0; i < program->node_count; i++) {
        collect_keywords_from_node(cg, program->nodes[i]);
        if (cg->has_error) return false;
    }

    // 0.5th pass: collect record type definitions
    for (size_t i = 0; i < program->node_count; i++) {
        AstNode* node = program->nodes[i];
        if (node->kind == AST_DEFRECORD) {
            if (cg->record_count >= SE_MAX_RECORDS) {
                set_error(cg, node->line, "too many record types");
                return false;
            }
            SeRecordType* rec = &cg->records[cg->record_count];
            strncpy(rec->name, node->as.defrecord.name, SE_MAX_SYMBOL_LEN - 1);
            rec->name[SE_MAX_SYMBOL_LEN - 1] = '\0';
            rec->field_count = node->as.defrecord.field_count;

            int32_t offset = 0;
            for (size_t f = 0; f < node->as.defrecord.field_count; f++) {
                strncpy(rec->fields[f].name, node->as.defrecord.fields[f], SE_MAX_SYMBOL_LEN - 1);
                rec->fields[f].name[SE_MAX_SYMBOL_LEN - 1] = '\0';
                rec->fields[f].is_16bit = node->as.defrecord.field_is_16bit[f];
                rec->fields[f].is_signed = node->as.defrecord.field_is_signed[f];
                rec->fields[f].offset = offset;
                offset += rec->fields[f].is_16bit ? 2 : 1;
            }
            rec->total_size = offset;
            cg->record_count++;
        }
    }

    // 1st pass: collect constants (needed for data address evaluation)
    for (size_t i = 0; i < program->node_count; i++) {
        AstNode* node = program->nodes[i];

        if (node->kind == AST_DEF) {
            if (cg->constant_count >= SE_MAX_CONSTANTS) {
                set_error(cg, node->line, "too many constants");
                return false;
            }

            int32_t value;
            if (!eval_const(cg, node->as.def.value, &value)) {
                set_error(cg, node->line, "def value must be compile-time constant");
                return false;
            }

            strncpy(cg->constants[cg->constant_count].name, node->as.def.name,
                    SE_MAX_SYMBOL_LEN - 1);
            cg->constants[cg->constant_count].value = value;
            cg->constants[cg->constant_count].is_function = false;
            cg->constant_count++;
        } else if (node->kind == AST_DEFN) {
            if (cg->function_count >= SE_MAX_FUNCTIONS) {
                set_error(cg, node->line, "too many functions");
                return false;
            }
            strncpy(cg->functions[cg->function_count].name, node->as.defn.name,
                    SE_MAX_SYMBOL_LEN - 1);
            cg->functions[cg->function_count].param_count = node->as.defn.param_count;
            for (size_t j = 0; j < node->as.defn.param_count; j++) {
                cg->functions[cg->function_count].param_hints[j] = node->as.defn.param_hints[j];
            }
            cg->function_count++;
        } else if (node->kind == AST_NS || node->kind == AST_REQUIRE ||
                   node->kind == AST_DEFRECORD) {
            continue;
        }
    }

    // 1.5th pass: collect anonymous functions from the entire AST
    for (size_t i = 0; i < program->node_count; i++) {
        collect_anon_fns(cg, program->nodes[i]);
        if (cg->has_error) return false;
    }

    // 2nd pass: collect data labels and compute addresses
    for (size_t i = 0; i < program->node_count; i++) {
        AstNode* node = program->nodes[i];

        if (node->kind == AST_DATA) {
            if (cg->data_label_count >= SE_MAX_DATA_LABELS) {
                set_error(cg, node->line, "too many data labels");
                return false;
            }

            // Check if explicit address is specified
            int32_t addr = cg->data_current_addr;
            if (node->as.data.addr >= 0) {
                addr = node->as.data.addr;
            } else if (node->as.data.addr_expr) {
                if (!eval_const(cg, node->as.data.addr_expr, &addr)) {
                    // Not a constant - use auto address
                    addr = cg->data_current_addr;
                }
            }

            // Calculate size of this data block
            int32_t size = 0;
            for (size_t j = 0; j < node->as.data.body.count; j++) {
                size += calc_data_size(node->as.data.body.items[j]);
            }

            // Store data label info
            strncpy(cg->data_labels[cg->data_label_count].name, node->as.data.name,
                    SE_MAX_SYMBOL_LEN - 1);
            cg->data_labels[cg->data_label_count].name[SE_MAX_SYMBOL_LEN - 1] = '\0';
            cg->data_labels[cg->data_label_count].addr = addr;
            cg->data_labels[cg->data_label_count].size = size;
            cg->data_labels[cg->data_label_count].record_type[0] = '\0';
            cg->data_labels[cg->data_label_count].element_count = 0;
            cg->data_labels[cg->data_label_count].element_size = 0;
            cg->data_labels[cg->data_label_count].is_data_block = true;
            cg->data_label_count++;

            // Update current address for next auto-placed data
            if (addr >= cg->data_current_addr) {
                cg->data_current_addr = addr + size;
            }
        } else if (node->kind == AST_VAR) {
            if (cg->data_label_count >= SE_MAX_DATA_LABELS) {
                set_error(cg, node->line, "too many data labels");
                return false;
            }
            int32_t addr = cg->data_current_addr;
            int32_t size = 1;
            char record_type[SE_MAX_SYMBOL_LEN] = {0};
            int32_t element_count = 0;
            int32_t element_size = 0;
            bool var_is_16bit = false;
            bool var_is_signed = false;

            // Check type hint for 16-bit and signedness
            if (se_hint_is_16bit(node->as.var.type_hint)) {
                var_is_16bit = true;
                size = 2;
            }
            if (se_hint_is_signed(node->as.var.type_hint)) {
                var_is_signed = true;
            }

            // Check if the value is an array declaration
            if (node->as.var.value->kind == AST_ARRAY) {
                AstNode* arr = node->as.var.value;
                int32_t count;
                if (!eval_const(cg, arr->as.array_expr.count, &count)) {
                    set_error(cg, node->line, "array count must be compile-time constant");
                    return false;
                }
                element_count = count;

                // Determine element size
                if (arr->as.array_expr.value->kind == AST_CALL) {
                    SeRecordType* rec =
                        find_record_type(cg, arr->as.array_expr.value->as.call.func);
                    if (rec) {
                        element_size = rec->total_size;
                        strncpy(record_type, rec->name, SE_MAX_SYMBOL_LEN - 1);
                    } else {
                        element_size = 1; // Unknown call, assume scalar
                    }
                } else {
                    element_size = 1; // Scalar value
                }
                size = count * element_size;
            }
            // Check if the value is a string literal
            else if (node->as.var.value->kind == AST_STRING) {
                int32_t str_len = (int32_t)strlen(node->as.var.value->as.symbol.name);
                size = str_len + 1; // +1 for null terminator
                element_count = str_len;
                element_size = 1;
            }
            // Check if the value is an anonymous function or function reference
            else if (node->as.var.value->kind == AST_FN) {
                size = 2; // 16-bit function address
            } else if (node->as.var.value->kind == AST_SYMBOL &&
                       is_function(cg, node->as.var.value->as.symbol.name)) {
                size = 2; // 16-bit function address
            }
            // Check if the value is a record constructor call
            else if (node->as.var.value->kind == AST_CALL) {
                SeRecordType* rec = find_record_type(cg, node->as.var.value->as.call.func);
                if (rec) {
                    size = rec->total_size;
                    strncpy(record_type, rec->name, SE_MAX_SYMBOL_LEN - 1);
                }
            }

            strncpy(cg->data_labels[cg->data_label_count].name, node->as.var.name,
                    SE_MAX_SYMBOL_LEN - 1);
            cg->data_labels[cg->data_label_count].addr = addr;
            cg->data_labels[cg->data_label_count].size = size;
            strncpy(cg->data_labels[cg->data_label_count].record_type, record_type,
                    SE_MAX_SYMBOL_LEN - 1);
            cg->data_labels[cg->data_label_count].element_count = element_count;
            cg->data_labels[cg->data_label_count].element_size = element_size;
            cg->data_labels[cg->data_label_count].is_16bit = var_is_16bit;
            cg->data_labels[cg->data_label_count].is_signed = var_is_signed;
            cg->data_label_count++;
            cg->data_current_addr = addr + size;
        }
    }

    return true;
}

// Emit code to compute address of nth element into R6:R7.
// Returns the data label for the array (caller can check record_type, element_size, etc.)
static SeDataLabel* emit_nth_addr(SeCodegen* cg, AstNode* node) {
    // node uses binary layout: left = array expr, right = index expr
    if (node->as.binary.left->kind != AST_SYMBOL) {
        set_error(cg, node->line, "nth requires a variable name for array");
        return NULL;
    }
    const char* arr_name = node->as.binary.left->as.symbol.name;
    SeDataLabel* label = find_data_label(cg, arr_name);
    if (!label) {
        char msg[128];
        snprintf(msg, sizeof(msg), "undefined array variable '%s'", arr_name);
        set_error(cg, node->line, msg);
        return NULL;
    }
    if (label->element_count == 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "variable '%s' is not an array", arr_name);
        set_error(cg, node->line, msg);
        return NULL;
    }

    int32_t base_addr = label->addr;
    int32_t elem_size = label->element_size;

    // Try constant index optimization
    // Note: we can't use eval_const directly because it treats data labels (vars) as
    // addresses, but we want the index VALUE. Only use it for actual constants.
    int32_t const_idx;
    bool is_const_idx = false;
    if (node->as.binary.right->kind == AST_NUMBER) {
        const_idx = node->as.binary.right->as.number;
        is_const_idx = true;
    } else if (node->as.binary.right->kind == AST_SYMBOL &&
               is_constant(cg, node->as.binary.right->as.symbol.name, &const_idx)) {
        is_const_idx = true;
    }

    if (is_const_idx) {
        // Compile-time address computation
        int32_t addr = base_addr + const_idx * elem_size;
        emit_line(cg, "LOADI R6, 0x%02X", (addr >> 8) & 0xFF);
        emit_line(cg, "LOADI R7, 0x%02X", addr & 0xFF);
    } else {
        // Runtime index computation
        emit_expr(cg, node->as.binary.right); // index → R0

        // Multiply index by element_size if > 1
        if (elem_size == 2) {
            emit_line(cg, "SHL R0");
        } else if (elem_size == 4) {
            emit_line(cg, "SHL R0");
            emit_line(cg, "SHL R0");
        } else if (elem_size == 8) {
            emit_line(cg, "SHL R0");
            emit_line(cg, "SHL R0");
            emit_line(cg, "SHL R0");
        } else if (elem_size > 1) {
            // General case: multiply R0 by elem_size using shift-and-add
            int lbl_loop = new_label(cg);
            int lbl_skip = new_label(cg);
            int lbl_done = new_label(cg);
            emit_line(cg, "MOV R1, R0");                         // R1 = index (multiplicand)
            emit_line(cg, "LOADI R0, 0");                        // R0 = result
            emit_line(cg, "LOADI R6, 0x%02X", elem_size & 0xFF); // R6 = multiplier
            emit(cg, "__L%d:\n", lbl_loop);
            emit_line(cg, "OR R6, R6");
            emit_line(cg, "JZ __L%d", lbl_done);
            emit_line(cg, "PUSH R6");
            emit_line(cg, "LOADI R7, 1");
            emit_line(cg, "AND R6, R7");
            emit_line(cg, "JZ __L%d", lbl_skip);
            emit_line(cg, "ADD R0, R1");
            emit(cg, "__L%d:\n", lbl_skip);
            emit_line(cg, "SHL R1");
            emit_line(cg, "POP R6");
            emit_line(cg, "SHR R6");
            emit_line(cg, "JMP __L%d", lbl_loop);
            emit(cg, "__L%d:\n", lbl_done);
        }
        // else elem_size == 1: R0 = index, already correct

        // Add base address: R6:R7 = base + R0 (byte offset)
        int lbl_nc = new_label(cg);
        emit_line(cg, "LOADI R6, 0x%02X", (base_addr >> 8) & 0xFF);
        emit_line(cg, "LOADI R7, 0x%02X", base_addr & 0xFF);
        emit_line(cg, "ADD R7, R0");
        emit_line(cg, "JNC __L%d", lbl_nc);
        emit_line(cg, "INC R6");
        emit(cg, "__L%d:\n", lbl_nc);
    }

    return label;
}

// Check if a node is a "simple" expression that can be evaluated into R1 without
// PUSH/POP overhead (constants, simple variable references).
static bool is_simple_operand(SeCodegen* cg, AstNode* node) {
    if (!node) return false;
    if (node->kind == AST_NUMBER) return true;
    if (node->kind == AST_NIL || node->kind == AST_TRUE || node->kind == AST_FALSE) return true;
    if (node->kind == AST_KEYWORD) return true;
    if (node->kind == AST_SYMBOL) {
        int32_t val;
        if (is_constant(cg, node->as.symbol.name, &val)) return true;
    }
    return false;
}

// Emit a simple expression directly into the specified register (R0 or R1).
// Only call this when is_simple_operand() returns true.
static void emit_simple_to_reg(SeCodegen* cg, AstNode* node, const char* reg) {
    if (node->kind == AST_NUMBER) {
        emit_line(cg, "LOADI %s, 0x%02X", reg, node->as.number & 0xFF);
        return;
    }
    if (node->kind == AST_NIL) {
        emit_line(cg, "LOADI %s, 0xFF", reg);
        return;
    }
    if (node->kind == AST_TRUE) {
        emit_line(cg, "LOADI %s, 1", reg);
        return;
    }
    if (node->kind == AST_FALSE) {
        emit_line(cg, "LOADI %s, 0", reg);
        return;
    }
    if (node->kind == AST_KEYWORD) {
        for (size_t i = 0; i < cg->keyword_count; i++) {
            if (strcmp(cg->keywords[i], node->as.symbol.name) == 0) {
                emit_line(cg, "LOADI %s, 0x%02X", reg, i & 0xFF);
                return;
            }
        }
    }
    if (node->kind == AST_SYMBOL) {
        int32_t val;
        if (is_constant(cg, node->as.symbol.name, &val)) {
            emit_line(cg, "LOADI %s, 0x%02X", reg, val & 0xFF);
            return;
        }
    }
}

// Emit 8-bit commutative binop: left and right in R0/R1, then instr R0, R1.
static void emit_commutative_8bit_binop(SeCodegen* cg, AstNode* node, const char* instr) {
    if (is_simple_operand(cg, node->as.binary.right)) {
        emit_expr(cg, node->as.binary.left);
        emit_simple_to_reg(cg, node->as.binary.right, "R1");
    } else if (is_simple_operand(cg, node->as.binary.left)) {
        emit_expr(cg, node->as.binary.right);
        emit_simple_to_reg(cg, node->as.binary.left, "R1");
    } else {
        emit_expr(cg, node->as.binary.left);
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.binary.right);
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "POP R0");
    }
    emit_line(cg, "%s R0, R1", instr);
}

// Emit boolean result tail: R0=0, JMP end, lbl_true: R0=1, lbl_end:.
static void emit_bool_materialize(SeCodegen* cg, int lbl_true, int lbl_end) {
    emit_line(cg, "LOADI R0, 0");
    emit_line(cg, "JMP __L%d", lbl_end);
    emit(cg, "__L%d:\n", lbl_true);
    emit_line(cg, "LOADI R0, 1");
    emit(cg, "__L%d:\n", lbl_end);
}

// Emit comparison operands: left into R0, right into R1, with CMP.
// For GT and LE, operand order is swapped (eval right first).
// For signed comparisons, XOR 0x80 bias is applied.
// Returns: after CMP, the appropriate jump instruction depends on the comparison type.
static void emit_cmp_operands(SeCodegen* cg, AstNode* node, bool swap) {
    AstNode* first = swap ? node->as.binary.right : node->as.binary.left;
    AstNode* second = swap ? node->as.binary.left : node->as.binary.right;
    bool is_sgn =
        expr_is_signed(cg, node->as.binary.left) || expr_is_signed(cg, node->as.binary.right);

    if (!expr_is_16bit(cg, first) && !expr_is_16bit(cg, second)) {
        // 8-bit comparison - optimized paths
        if (is_simple_operand(cg, second)) {
            emit_expr(cg, first);
            emit_simple_to_reg(cg, second, "R1");
        } else if (is_simple_operand(cg, first)) {
            emit_expr(cg, second);
            emit_line(cg, "MOV R1, R0");
            emit_simple_to_reg(cg, first, "R0");
        } else {
            emit_expr(cg, first);
            emit_line(cg, "PUSH R0");
            emit_expr(cg, second);
            emit_line(cg, "MOV R1, R0");
            emit_line(cg, "POP R0");
        }
        if (is_sgn) {
            emit_line(cg, "PUSH R6");
            emit_line(cg, "LOADI R6, 0x80");
            emit_line(cg, "XOR R0, R6");
            emit_line(cg, "XOR R1, R6");
            emit_line(cg, "POP R6");
        }
        emit_line(cg, "CMP R0, R1");
    } else {
        // Fallback to non-fused for 16-bit (emit as expression producing 0/1)
        // This path shouldn't normally be reached for branch fusion.
        emit_expr(cg, first);
        emit_line(cg, "PUSH R0");
        emit_expr(cg, second);
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "POP R0");
        if (is_sgn) {
            emit_line(cg, "PUSH R6");
            emit_line(cg, "LOADI R6, 0x80");
            emit_line(cg, "XOR R0, R6");
            emit_line(cg, "XOR R1, R6");
            emit_line(cg, "POP R6");
        }
        emit_line(cg, "CMP R0, R1");
    }
}

// Emit code that jumps to target_label when the condition matches when_true.
// when_true: jump when condition is true; !when_true: jump when condition is false.
// Fuses comparison with conditional jump where possible.
static void emit_branch(SeCodegen* cg, AstNode* cond, int target_label, bool when_true) {
    if (cg->has_error) return;
    if (!cond) return;

    bool is_16 = false;
    const char* jump_instr = NULL;

    switch (cond->kind) {
    case AST_EQ:
        is_16 = expr_is_16bit(cg, cond->as.binary.left) || expr_is_16bit(cg, cond->as.binary.right);
        if (!is_16) {
            emit_cmp_operands(cg, cond, false);
            jump_instr = when_true ? "JZ" : "JNZ";
            goto do_jump;
        }
        break;
    case AST_NE:
        is_16 = expr_is_16bit(cg, cond->as.binary.left) || expr_is_16bit(cg, cond->as.binary.right);
        if (!is_16) {
            emit_cmp_operands(cg, cond, false);
            jump_instr = when_true ? "JNZ" : "JZ";
            goto do_jump;
        }
        break;
    case AST_LT:
        is_16 = expr_is_16bit(cg, cond->as.binary.left) || expr_is_16bit(cg, cond->as.binary.right);
        if (!is_16) {
            emit_cmp_operands(cg, cond, false);
            jump_instr = when_true ? "JC" : "JNC";
            goto do_jump;
        }
        break;
    case AST_GE:
        is_16 = expr_is_16bit(cg, cond->as.binary.left) || expr_is_16bit(cg, cond->as.binary.right);
        if (!is_16) {
            emit_cmp_operands(cg, cond, false);
            jump_instr = when_true ? "JNC" : "JC";
            goto do_jump;
        }
        break;
    case AST_GT:
        is_16 = expr_is_16bit(cg, cond->as.binary.left) || expr_is_16bit(cg, cond->as.binary.right);
        if (!is_16) {
            emit_cmp_operands(cg, cond, true);
            jump_instr = when_true ? "JC" : "JNC";
            goto do_jump;
        }
        break;
    case AST_LE:
        is_16 = expr_is_16bit(cg, cond->as.binary.left) || expr_is_16bit(cg, cond->as.binary.right);
        if (!is_16) {
            emit_cmp_operands(cg, cond, true);
            jump_instr = when_true ? "JNC" : "JC";
            goto do_jump;
        }
        break;
    do_jump:
        if (jump_instr) {
            emit_line(cg, "%s __L%d", jump_instr, target_label);
            return;
        }
        break;

    case AST_LOGIC_AND:
        if (!when_true) {
            emit_branch(cg, cond->as.binary.left, target_label, false);
            emit_branch(cg, cond->as.binary.right, target_label, false);
        }
        return;
    case AST_LOGIC_OR:
        if (when_true) {
            emit_branch(cg, cond->as.binary.left, target_label, true);
            emit_branch(cg, cond->as.binary.right, target_label, true);
        }
        return;
    case AST_LOGIC_NOT:
        emit_expr(cg, cond->as.unary.operand);
        emit_line(cg, "%s __L%d", when_true ? "JFALSE" : "JTRUE", target_label);
        return;
    case AST_NILP:
        emit_expr(cg, cond->as.unary.operand);
        emit_line(cg, "LOADI R1, 0xFF");
        emit_line(cg, "CMP R0, R1");
        emit_line(cg, "%s __L%d", when_true ? "JZ" : "JNZ", target_label);
        return;
    case AST_ZEROP:
        emit_expr(cg, cond->as.unary.operand);
        emit_line(cg, "OR R0, R0");
        emit_line(cg, "%s __L%d", when_true ? "JZ" : "JNZ", target_label);
        return;
    case AST_TRUE:
        if (when_true) emit_line(cg, "JMP __L%d", target_label);
        return;
    case AST_FALSE:
    case AST_NIL:
        if (!when_true) emit_line(cg, "JMP __L%d", target_label);
        return;
    default: break;
    }
    emit_expr(cg, cond);
    emit_line(cg, "%s __L%d", when_true ? "JTRUE" : "JFALSE", target_label);
}

static void emit_literal(SeCodegen* cg, AstNode* node) {
    switch (node->kind) {
    case AST_NUMBER:
        if (node->as.number > 255 || node->as.number < -128) {
            int32_t val = node->as.number;
            emit_line(cg, "LOADI R0, 0x%02X", (val >> 8) & 0xFF);
            emit_line(cg, "LOADI R1, 0x%02X", val & 0xFF);
        } else {
            emit_line(cg, "LOADI R0, 0x%02X", node->as.number & 0xFF);
        }
        break;
    case AST_SYMBOL: {
        int32_t val;
        if (is_constant(cg, node->as.symbol.name, &val)) {
            if (val > 255 || val < -128) {
                emit_line(cg, "LOADI R0, 0x%02X", (val >> 8) & 0xFF);
                emit_line(cg, "LOADI R1, 0x%02X", val & 0xFF);
            } else {
                emit_line(cg, "LOADI R0, 0x%02X", val & 0xFF);
            }
            return;
        }
        SeLocal* local = find_local_info(cg, node->as.symbol.name);
        if (local) {
            emit_local_load(cg, local);
            return;
        }
        SeDataLabel* dl = find_data_label(cg, node->as.symbol.name);
        if (dl) {
            int32_t addr = dl->addr;
            if (dl->is_data_block) {
                emit_line(cg, "LOADI R0, 0x%02X", (addr >> 8) & 0xFF);
                emit_line(cg, "LOADI R1, 0x%02X", addr & 0xFF);
            } else {
                emit_line(cg, "LOADI R6, 0x%02X", (addr >> 8) & 0xFF);
                emit_line(cg, "LOADI R7, 0x%02X", addr & 0xFF);
                if (dl->is_16bit) {
                    emit_line(cg, "LOAD R0, [R6:R7]+");
                    emit_line(cg, "LOAD R1, [R6:R7]");
                } else {
                    emit_line(cg, "LOAD R0, [R6:R7]");
                }
            }
            return;
        }
        if (is_function(cg, node->as.symbol.name)) {
            char fn_name[SE_MAX_SYMBOL_LEN];
            sanitize_name(fn_name, node->as.symbol.name, SE_MAX_SYMBOL_LEN);
            emit_line(cg, "LOADI R0, %s >> 8", fn_name);
            emit_line(cg, "LOADI R1, %s & 0xFF", fn_name);
            return;
        }
        set_error(cg, node->line, "undefined variable");
        break;
    }
    case AST_NIL: emit_line(cg, "LOADI R0, 0xFF"); break;
    case AST_TRUE: emit_line(cg, "LOADI R0, 1"); break;
    case AST_FALSE: emit_line(cg, "LOADI R0, 0"); break;
    case AST_KEYWORD: {
        for (size_t i = 0; i < cg->keyword_count; i++) {
            if (strcmp(cg->keywords[i], node->as.symbol.name) == 0) {
                emit_line(cg, "LOADI R0, 0x%02X", i & 0xFF);
                return;
            }
        }
        set_error(cg, node->line, "keyword not interned");
        break;
    }
    default: break;
    }
}

static void emit_arithmetic(SeCodegen* cg, AstNode* node) {
    switch (node->kind) {
    case AST_ADD:
        // 8-bit optimized path: avoid PUSH/POP when one operand is simple
        if (!expr_is_16bit(cg, node->as.binary.left) && !expr_is_16bit(cg, node->as.binary.right)) {
            if (is_simple_operand(cg, node->as.binary.right)) {
                emit_expr(cg, node->as.binary.left);
                emit_simple_to_reg(cg, node->as.binary.right, "R1");
                emit_line(cg, "ADD R0, R1");
                break;
            }
            if (is_simple_operand(cg, node->as.binary.left)) {
                emit_expr(cg, node->as.binary.right);
                emit_simple_to_reg(cg, node->as.binary.left, "R1");
                emit_line(cg, "ADD R0, R1"); // commutative
                break;
            }
        }
        if (expr_is_16bit(cg, node->as.binary.left) || expr_is_16bit(cg, node->as.binary.right)) {
            // 16-bit addition: R0:R1 = left + right using ADD (low) + ADC (high)
            emit_expr(cg, node->as.binary.left);
            if (!expr_is_16bit(cg, node->as.binary.left)) {
                // Promote 8-bit to 16-bit: R0 is value, R1=0 (unsigned)
                emit_line(cg, "MOV R1, R0");
                emit_line(cg, "LOADI R0, 0");
            }
            emit_line(cg, "PUSH R0"); // save high byte of left
            emit_line(cg, "PUSH R1"); // save low byte of left
            emit_expr(cg, node->as.binary.right);
            if (!expr_is_16bit(cg, node->as.binary.right)) {
                emit_line(cg, "MOV R1, R0");
                emit_line(cg, "LOADI R0, 0");
            }
            // R0:R1 = right (hi:lo), stack has left hi, left lo
            emit_line(cg, "POP R6"); // R6 = left lo
            emit_line(cg, "POP R7"); // R7 = left hi
            // low byte: R6 + R1
            emit_line(cg, "PUSH R0"); // save right hi
            emit_line(cg, "MOV R0, R6");
            emit_line(cg, "ADD R0, R1"); // R0 = left_lo + right_lo, carry set if overflow
            emit_line(cg, "MOV R1, R0"); // R1 = result low byte
            // high byte: R7 + right_hi + carry
            emit_line(cg, "POP R0");     // R0 = right hi
            emit_line(cg, "ADC R7, R0"); // R7 = left_hi + right_hi + carry
            emit_line(cg, "MOV R0, R7"); // R0 = result high byte
        } else {
            emit_expr(cg, node->as.binary.left);
            emit_line(cg, "PUSH R0");
            emit_expr(cg, node->as.binary.right);
            emit_line(cg, "MOV R1, R0");
            emit_line(cg, "POP R0");
            emit_line(cg, "ADD R0, R1");
        }
        break;

    case AST_SUB:
        // 8-bit optimized path: avoid PUSH/POP when right operand is simple
        if (!expr_is_16bit(cg, node->as.binary.left) && !expr_is_16bit(cg, node->as.binary.right)) {
            if (is_simple_operand(cg, node->as.binary.right)) {
                emit_expr(cg, node->as.binary.left);
                emit_simple_to_reg(cg, node->as.binary.right, "R1");
                emit_line(cg, "SUB R0, R1");
                break;
            }
        }
        if (expr_is_16bit(cg, node->as.binary.left) || expr_is_16bit(cg, node->as.binary.right)) {
            // 16-bit subtraction: R0:R1 = left - right using SUB (low) + SBC (high)
            emit_expr(cg, node->as.binary.left);
            if (!expr_is_16bit(cg, node->as.binary.left)) {
                emit_line(cg, "MOV R1, R0");
                emit_line(cg, "LOADI R0, 0");
            }
            emit_line(cg, "PUSH R0"); // save left hi
            emit_line(cg, "PUSH R1"); // save left lo
            emit_expr(cg, node->as.binary.right);
            if (!expr_is_16bit(cg, node->as.binary.right)) {
                emit_line(cg, "MOV R1, R0");
                emit_line(cg, "LOADI R0, 0");
            }
            // R0:R1 = right, stack has left hi, left lo
            emit_line(cg, "POP R6");  // R6 = left lo
            emit_line(cg, "POP R7");  // R7 = left hi
            emit_line(cg, "PUSH R0"); // save right hi
            // low byte: left_lo - right_lo
            emit_line(cg, "MOV R0, R6");
            emit_line(cg, "SUB R0, R1"); // R0 = left_lo - right_lo, borrow set if underflow
            emit_line(cg, "MOV R1, R0"); // R1 = result low byte
            // high byte: left_hi - right_hi - borrow
            emit_line(cg, "POP R0");     // R0 = right hi
            emit_line(cg, "SBC R7, R0"); // SBC subtracts with borrow (carry flag)
            emit_line(cg, "MOV R0, R7"); // R0 = result high byte
        } else {
            emit_expr(cg, node->as.binary.left);
            emit_line(cg, "PUSH R0");
            emit_expr(cg, node->as.binary.right);
            emit_line(cg, "MOV R1, R0");
            emit_line(cg, "POP R0");
            emit_line(cg, "SUB R0, R1");
        }
        break;

    case AST_NEG:
        emit_expr(cg, node->as.unary.operand);
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "LOADI R0, 0");
        emit_line(cg, "SUB R0, R1");
        break;

    case AST_INC:
        emit_expr(cg, node->as.unary.operand);
        emit_line(cg, "INC R0");
        break;

    case AST_DEC:
        emit_expr(cg, node->as.unary.operand);
        emit_line(cg, "DEC R0");
        break;

    case AST_BAND: emit_commutative_8bit_binop(cg, node, "AND"); break;

    case AST_BOR: emit_commutative_8bit_binop(cg, node, "OR"); break;

    case AST_XOR: emit_commutative_8bit_binop(cg, node, "XOR"); break;

    case AST_BNOT:
        emit_expr(cg, node->as.unary.operand);
        // XOR with 0xFF to flip all bits
        emit_line(cg, "LOADI R1, 0xFF");
        emit_line(cg, "XOR R0, R1");
        break;

    case AST_SHL: {
        bool left_16 = expr_is_16bit(cg, node->as.binary.left);
        bool result_16 = expr_is_16bit(cg, node);
        emit_expr(cg, node->as.binary.left);
        if (node->as.binary.right->kind != AST_NUMBER) {
            set_error(cg, node->line, "shift amount must be constant");
            break;
        }
        int count = node->as.binary.right->as.number;
        if (result_16 && !left_16) {
            // Promote 8-bit operand to 16-bit: value goes to low byte (R1), high byte (R0) = 0
            emit_line(cg, "MOV R1, R0");
            emit_line(cg, "LOADI R0, 0");
            left_16 = true;
        }
        if (left_16) {
            // 16-bit shift: shift R0:R1 left (R0=hi, R1=lo)
            for (int i = 0; i < count && i < 16; i++) {
                emit_line(cg, "SHL R1");     // shift low byte, carry = old bit 7
                emit_line(cg, "ADC R0, R0"); // shift high byte left and add carry (same as ROL+ADC
                                             // trick: R0 = R0 + R0 + carry)
            }
        } else {
            // 8-bit shift
            for (int i = 0; i < count && i < 8; i++) {
                emit_line(cg, "SHL R0");
            }
        }
        break;
    }

    case AST_SHR: {
        bool is_sgn = expr_is_signed(cg, node->as.binary.left);
        emit_expr(cg, node->as.binary.left);
        if (node->as.binary.right->kind == AST_NUMBER) {
            int count = node->as.binary.right->as.number;
            if (is_sgn) {
                // Arithmetic shift right: preserve sign bit
                for (int i = 0; i < count && i < 8; i++) {
                    // Save sign bit, shift, restore sign bit
                    emit_line(cg, "PUSH R0"); // save original
                    emit_line(cg, "SHR R0");  // logical shift right
                    emit_line(cg, "POP R1");  // R1 = original
                    emit_line(cg, "LOADI R6, 0x80");
                    emit_line(cg, "AND R1, R6"); // R1 = sign bit only (0x80 or 0x00)
                    emit_line(cg, "OR R0, R1");  // OR sign bit back into shifted result
                }
            } else {
                for (int i = 0; i < count && i < 8; i++) {
                    emit_line(cg, "SHR R0");
                }
            }
        } else {
            set_error(cg, node->line, "shift amount must be constant");
        }
        break;
    }

    case AST_MUL: {
        // Check for constant multiplier (optimize powers of 2)
        int32_t const_val;
        if (eval_const(cg, node->as.binary.right, &const_val)) {
            emit_expr(cg, node->as.binary.left);
            // Optimize small multipliers using shifts and adds
            if (const_val == 0) {
                emit_line(cg, "LOADI R0, 0");
            } else if (const_val == 1) {
                // No-op
            } else if (const_val == 2) {
                emit_line(cg, "SHL R0");
            } else if (const_val == 4) {
                emit_line(cg, "SHL R0");
                emit_line(cg, "SHL R0");
            } else if (const_val == 8) {
                emit_line(cg, "SHL R0");
                emit_line(cg, "SHL R0");
                emit_line(cg, "SHL R0");
            } else if (const_val == 16) {
                emit_line(cg, "SHL R0");
                emit_line(cg, "SHL R0");
                emit_line(cg, "SHL R0");
                emit_line(cg, "SHL R0");
            } else {
                // General case: shift-and-add multiplication (O(8) iterations)
                // R1 = multiplicand, R6 = multiplier, R0 = result
                // NOTE: Must NOT use R2/R3 (local var base) or R4/R5 (frame pointer)
                int lbl_loop = new_label(cg);
                int lbl_skip = new_label(cg);
                int lbl_done = new_label(cg);
                emit_line(cg, "MOV R1, R0");                         // R1 = multiplicand
                emit_line(cg, "LOADI R0, 0");                        // R0 = result
                emit_line(cg, "LOADI R6, 0x%02X", const_val & 0xFF); // R6 = multiplier
                emit(cg, "__L%d:\n", lbl_loop);
                emit_line(cg, "OR R6, R6"); // test if multiplier == 0
                emit_line(cg, "JZ __L%d", lbl_done);
                emit_line(cg, "PUSH R6");            // save multiplier
                emit_line(cg, "LOADI R7, 1");        // R7 = 1 for bit test
                emit_line(cg, "AND R6, R7");         // R6 = multiplier & 1
                emit_line(cg, "JZ __L%d", lbl_skip); // if bit is 0, skip add
                emit_line(cg, "ADD R0, R1");         // result += multiplicand
                emit(cg, "__L%d:\n", lbl_skip);
                emit_line(cg, "SHL R1"); // multiplicand *= 2
                emit_line(cg, "POP R6"); // restore multiplier
                emit_line(cg, "SHR R6"); // multiplier /= 2
                emit_line(cg, "JMP __L%d", lbl_loop);
                emit(cg, "__L%d:\n", lbl_done);
            }
        } else {
            // Runtime multiplication using shift-and-add (O(8) iterations)
            // NOTE: Must NOT use R2/R3 (local var base) or R4/R5 (frame pointer)
            int lbl_loop = new_label(cg);
            int lbl_skip = new_label(cg);
            int lbl_done = new_label(cg);
            emit_expr(cg, node->as.binary.right); // multiplier -> R0
            emit_line(cg, "PUSH R0");             // save multiplier
            emit_expr(cg, node->as.binary.left);  // multiplicand -> R0
            emit_line(cg, "MOV R1, R0");          // R1 = multiplicand
            emit_line(cg, "LOADI R0, 0");         // R0 = result
            emit_line(cg, "POP R6");              // R6 = multiplier
            emit(cg, "__L%d:\n", lbl_loop);
            emit_line(cg, "OR R6, R6"); // test if multiplier == 0
            emit_line(cg, "JZ __L%d", lbl_done);
            emit_line(cg, "PUSH R6");            // save multiplier
            emit_line(cg, "LOADI R7, 1");        // R7 = 1 for bit test
            emit_line(cg, "AND R6, R7");         // R6 = multiplier & 1
            emit_line(cg, "JZ __L%d", lbl_skip); // if bit is 0, skip add
            emit_line(cg, "ADD R0, R1");         // result += multiplicand
            emit(cg, "__L%d:\n", lbl_skip);
            emit_line(cg, "SHL R1"); // multiplicand *= 2
            emit_line(cg, "POP R6"); // restore multiplier
            emit_line(cg, "SHR R6"); // multiplier /= 2
            emit_line(cg, "JMP __L%d", lbl_loop);
            emit(cg, "__L%d:\n", lbl_done);
        }
        break;
    }

    case AST_DIV: {
        // Division using repeated subtraction
        // a / b: count how many times b fits into a
        // NOTE: Must NOT use R2/R3 (local var base) or R4/R5 (frame pointer)
        int lbl_loop = new_label(cg);
        int lbl_done = new_label(cg);
        emit_expr(cg, node->as.binary.right); // divisor -> R0
        emit_line(cg, "PUSH R0");             // save divisor
        emit_expr(cg, node->as.binary.left);  // dividend -> R0
        emit_line(cg, "POP R1");              // R1 = divisor, R0 = dividend
        emit_line(cg, "MOV R6, R0");          // R6 = dividend (working copy)
        emit_line(cg, "LOADI R0, 0");         // R0 = quotient
        emit(cg, "__L%d:\n", lbl_loop);
        emit_line(cg, "CMP R6, R1");         // compare dividend with divisor
        emit_line(cg, "JC __L%d", lbl_done); // if dividend < divisor, done
        emit_line(cg, "SUB R6, R1");         // dividend -= divisor
        emit_line(cg, "INC R0");             // quotient++
        emit_line(cg, "JMP __L%d", lbl_loop);
        emit(cg, "__L%d:\n", lbl_done);
        break;
    }

    case AST_MOD: {
        // Modulo: a % b = a - (a / b) * b
        // Since we don't have division, use repeated subtraction
        int lbl_loop = new_label(cg);
        int lbl_done = new_label(cg);
        emit_expr(cg, node->as.binary.right); // divisor
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.binary.left); // dividend
        emit_line(cg, "POP R1");             // R1 = divisor, R0 = dividend
        emit(cg, "__L%d:\n", lbl_loop);
        emit_line(cg, "CMP R0, R1");
        emit_line(cg, "JC __L%d", lbl_done); // if R0 < R1, done
        emit_line(cg, "SUB R0, R1");
        emit_line(cg, "JMP __L%d", lbl_loop);
        emit(cg, "__L%d:\n", lbl_done);
        break;
    }
    default: break;
    }
}

static void emit_bitwise(SeCodegen* cg, AstNode* node) {
    switch (node->kind) {
    case AST_BAND: emit_commutative_8bit_binop(cg, node, "AND"); break;
    case AST_BOR: emit_commutative_8bit_binop(cg, node, "OR"); break;
    case AST_XOR: emit_commutative_8bit_binop(cg, node, "XOR"); break;
    case AST_BNOT:
        emit_expr(cg, node->as.unary.operand);
        emit_line(cg, "LOADI R1, 0xFF");
        emit_line(cg, "XOR R0, R1");
        break;
    case AST_SHL: {
        bool left_16 = expr_is_16bit(cg, node->as.binary.left);
        bool result_16 = expr_is_16bit(cg, node);
        emit_expr(cg, node->as.binary.left);
        if (node->as.binary.right->kind != AST_NUMBER) {
            set_error(cg, node->line, "shift amount must be constant");
            break;
        }
        int count = node->as.binary.right->as.number;
        if (result_16 && !left_16) {
            emit_line(cg, "MOV R1, R0");
            emit_line(cg, "LOADI R0, 0");
            left_16 = true;
        }
        if (left_16) {
            for (int i = 0; i < count && i < 16; i++) {
                emit_line(cg, "SHL R1");
                emit_line(cg, "ADC R0, R0");
            }
        } else {
            for (int i = 0; i < count && i < 8; i++) {
                emit_line(cg, "SHL R0");
            }
        }
        break;
    }
    case AST_SHR: {
        bool is_sgn = expr_is_signed(cg, node->as.binary.left);
        emit_expr(cg, node->as.binary.left);
        if (node->as.binary.right->kind == AST_NUMBER) {
            int count = node->as.binary.right->as.number;
            if (is_sgn) {
                for (int i = 0; i < count && i < 8; i++) {
                    emit_line(cg, "PUSH R0");
                    emit_line(cg, "SHR R0");
                    emit_line(cg, "POP R1");
                    emit_line(cg, "LOADI R6, 0x80");
                    emit_line(cg, "AND R1, R6");
                    emit_line(cg, "OR R0, R1");
                }
            } else {
                for (int i = 0; i < count && i < 8; i++) {
                    emit_line(cg, "SHR R0");
                }
            }
        } else {
            set_error(cg, node->line, "shift amount must be constant");
        }
        break;
    }
    default: break;
    }
}

static void emit_comparison(SeCodegen* cg, AstNode* node) {
    switch (node->kind) {
    case AST_EQ: {
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        if (!expr_is_16bit(cg, node->as.binary.left) && !expr_is_16bit(cg, node->as.binary.right)) {
            emit_cmp_operands(cg, node, false);
        } else {
            emit_expr(cg, node->as.binary.left);
            emit_line(cg, "PUSH R0");
            emit_expr(cg, node->as.binary.right);
            emit_line(cg, "MOV R1, R0");
            emit_line(cg, "POP R0");
            emit_line(cg, "CMP R0, R1");
        }
        emit_line(cg, "JZ __L%d", lbl_true);
        emit_bool_materialize(cg, lbl_true, lbl_end);
        break;
    }

    case AST_NE: {
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        if (!expr_is_16bit(cg, node->as.binary.left) && !expr_is_16bit(cg, node->as.binary.right)) {
            emit_cmp_operands(cg, node, false);
        } else {
            emit_expr(cg, node->as.binary.left);
            emit_line(cg, "PUSH R0");
            emit_expr(cg, node->as.binary.right);
            emit_line(cg, "MOV R1, R0");
            emit_line(cg, "POP R0");
            emit_line(cg, "CMP R0, R1");
        }
        emit_line(cg, "JNZ __L%d", lbl_true);
        emit_bool_materialize(cg, lbl_true, lbl_end);
        break;
    }

    case AST_LT: {
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_cmp_operands(cg, node, false);
        emit_line(cg, "JC __L%d", lbl_true);
        emit_bool_materialize(cg, lbl_true, lbl_end);
        break;
    }

    case AST_GT: {
        // a > b: swap operands, CMP b,a, then C=1 means b<a (=a>b)
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_cmp_operands(cg, node, true);
        emit_line(cg, "JC __L%d", lbl_true);
        emit_bool_materialize(cg, lbl_true, lbl_end);
        break;
    }

    case AST_LE: {
        // a <= b: swap -> CMP b,a, JNC (true when b >= a = a <= b)
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_cmp_operands(cg, node, true);
        emit_line(cg, "JNC __L%d", lbl_true);
        emit_bool_materialize(cg, lbl_true, lbl_end);
        break;
    }

    case AST_GE: {
        // a >= b: CMP a,b, JNC (true when a >= b, no carry)
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_cmp_operands(cg, node, false);
        emit_line(cg, "JNC __L%d", lbl_true);
        emit_bool_materialize(cg, lbl_true, lbl_end);
        break;
    }

    case AST_LNOT: {
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.unary.operand);
        emit_line(cg, "OR R0, R0");
        emit_line(cg, "JZ __L%d", lbl_true);
        emit_bool_materialize(cg, lbl_true, lbl_end);
        break;
    }
    default: break;
    }
}

static void emit_memory(SeCodegen* cg, AstNode* node) {
    switch (node->kind) {
    case AST_ADDR:
        // Set up R6:R7 with the address
        emit_expr(cg, node->as.addr.hi);
        emit_line(cg, "MOV R6, R0");
        emit_expr(cg, node->as.addr.lo);
        emit_line(cg, "MOV R7, R0");
        break;

    case AST_ADDR16: {
        // Set up R6:R7 from a 16-bit value
        int32_t addr_val;
        if (eval_const(cg, node->as.unary.operand, &addr_val)) {
            // Compile-time constant: split into hi/lo immediately
            emit_line(cg, "LOADI R6, 0x%02X", (addr_val >> 8) & 0xFF);
            emit_line(cg, "LOADI R7, 0x%02X", addr_val & 0xFF);
            emit_line(cg, "MOV R0, R7"); // R0 = lo byte
        } else if (node->as.unary.operand->kind == AST_ADDR ||
                   node->as.unary.operand->kind == AST_ADDR16) {
            // Address expression: load 16-bit value from that memory location
            // This enables pointer dereferencing: (addr16 (addr ptr-hi ptr-lo))
            emit_expr(cg, node->as.unary.operand); // sets up R6:R7 with source address
            emit_line(cg, "LOAD R0, [R6:R7]+");    // load hi byte, auto-increment R7
            emit_line(cg, "PUSH R0");              // save hi byte
            emit_line(cg, "LOAD R0, [R6:R7]");     // load lo byte
            emit_line(cg, "MOV R7, R0");           // R7 = lo byte
            emit_line(cg, "POP R6");               // R6 = hi byte
        } else {
            set_error(cg, node->line,
                      "addr16 requires compile-time constant or address expression. "
                      "For runtime pointers, use (addr16 (addr hi lo))");
        }
        break;
    }

    case AST_LOAD:
        emit_addr_to_r6r7(cg, node->as.load.addr);
        if (!cg->has_error) emit_line(cg, "LOAD R0, [R6:R7]");
        break;

    case AST_STORE:
        emit_expr(cg, node->as.store.value);
        emit_line(cg, "PUSH R0");
        emit_addr_to_r6r7(cg, node->as.store.addr);
        if (!cg->has_error) {
            emit_line(cg, "POP R0");
            emit_line(cg, "STORE R0, [R6:R7]");
        }
        break;

    case AST_HI: {
        int32_t val;
        if (node->as.unary.operand->kind == AST_SYMBOL) {
            SeDataLabel* dl = find_data_label(cg, node->as.unary.operand->as.symbol.name);
            if (dl) {
                emit_line(cg, "LOADI R0, 0x%02X", (dl->addr >> 8) & 0xFF);
                break;
            }
        }
        if (eval_const(cg, node->as.unary.operand, &val)) {
            emit_line(cg, "LOADI R0, 0x%02X", (val >> 8) & 0xFF);
        } else if (expr_is_16bit(cg, node->as.unary.operand)) {
            // Runtime 16-bit value: evaluate -> R0=hi, R1=lo. R0 already has hi byte.
            emit_expr(cg, node->as.unary.operand);
        } else {
            // Runtime 8-bit value: hi byte is always 0
            emit_line(cg, "LOADI R0, 0x00");
        }
        break;
    }

    case AST_LO: {
        int32_t val;
        if (node->as.unary.operand->kind == AST_SYMBOL) {
            SeDataLabel* dl = find_data_label(cg, node->as.unary.operand->as.symbol.name);
            if (dl) {
                emit_line(cg, "LOADI R0, 0x%02X", dl->addr & 0xFF);
                break;
            }
        }
        if (eval_const(cg, node->as.unary.operand, &val)) {
            emit_line(cg, "LOADI R0, 0x%02X", val & 0xFF);
        } else if (expr_is_16bit(cg, node->as.unary.operand)) {
            // Runtime 16-bit value: evaluate -> R0=hi, R1=lo. Move lo byte to R0.
            emit_expr(cg, node->as.unary.operand);
            emit_line(cg, "MOV R0, R1");
        } else {
            emit_expr(cg, node->as.unary.operand);
        }
        break;
    }
    default: break;
    }
}

static void emit_variables(SeCodegen* cg, AstNode* node) {
    switch (node->kind) {
    case AST_SET: {
        emit_expr(cg, node->as.set.value);
        SeLocal* local = find_local_info(cg, node->as.set.var);
        if (!local) {
            set_error(cg, node->line, "undefined variable in set");
            return;
        }
        emit_local_store(cg, local);
        break;
    }

    case AST_SET_BANG: {
        // Check if this is a field set: (set! (:field record) value)
        if (node->as.set.target_expr != NULL && node->as.set.var[0] == ':') {
            // Field mutation - target_expr is record expression (symbol or nth)
            SeRecordType* rec = NULL;

            if (node->as.set.target_expr->kind == AST_SYMBOL) {
                // (set! (:field var) value) - simple record variable
                const char* var_name = node->as.set.target_expr->as.symbol.name;
                rec = find_var_record_type(cg, var_name);
                if (!rec) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "variable '%s' is not a record", var_name);
                    set_error(cg, node->line, msg);
                    break;
                }
                SeRecordField* field = find_record_field(rec, node->as.set.var);
                if (!field) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "record '%s' has no field '%s'", rec->name,
                             node->as.set.var);
                    set_error(cg, node->line, msg);
                    break;
                }
                SeDataLabel* dl_addr = find_data_label(cg, var_name);
                if (!dl_addr) {
                    set_error(cg, node->line, "cannot resolve record address");
                    break;
                }
                int32_t addr = dl_addr->addr;
                if (!field->is_16bit && is_simple_operand(cg, node->as.set.value)) {
                    // Optimized: set up address first, then load value
                    emit_line(cg, "LOADI R6, 0x%02X", (addr >> 8) & 0xFF);
                    emit_line(cg, "LOADI R7, 0x%02X", addr & 0xFF);
                    emit_expr(cg, node->as.set.value);
                    emit_line(cg, "STORE R0, [R6:R7 + %d]", field->offset);
                } else {
                    emit_expr(cg, node->as.set.value);
                    emit_line(cg, "LOADI R6, 0x%02X", (addr >> 8) & 0xFF);
                    emit_line(cg, "LOADI R7, 0x%02X", addr & 0xFF);
                    if (field->is_16bit) {
                        emit_line(cg, "STORE R0, [R6:R7 + %d]", field->offset);
                        emit_line(cg, "STORE R1, [R6:R7 + %d]", field->offset + 1);
                    } else {
                        emit_line(cg, "STORE R0, [R6:R7 + %d]", field->offset);
                    }
                }
            } else if (node->as.set.target_expr->kind == AST_NTH) {
                // (set! (:field (nth arr i)) value) - field of array element
                AstNode* nth_node = node->as.set.target_expr;
                if (nth_node->as.binary.left->kind != AST_SYMBOL) {
                    set_error(cg, node->line, "nth array must be a variable name");
                    break;
                }
                SeDataLabel* label = find_data_label(cg, nth_node->as.binary.left->as.symbol.name);
                if (!label || label->record_type[0] == '\0') {
                    set_error(cg, node->line, "nth target is not a record array");
                    break;
                }
                rec = find_record_type(cg, label->record_type);
                if (!rec) {
                    set_error(cg, node->line, "cannot find record type for array");
                    break;
                }
                SeRecordField* field = find_record_field(rec, node->as.set.var);
                if (!field) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "record '%s' has no field '%s'", rec->name,
                             node->as.set.var);
                    set_error(cg, node->line, msg);
                    break;
                }
                // Evaluate value first, save on stack
                emit_expr(cg, node->as.set.value);
                emit_line(cg, "PUSH R0");
                if (field->is_16bit) {
                    emit_line(cg, "PUSH R1");
                }
                // Compute element address → R6:R7
                emit_nth_addr(cg, nth_node);
                if (cg->has_error) break;
                // Store at field offset
                if (field->is_16bit) {
                    emit_line(cg, "POP R1");
                    emit_line(cg, "POP R0");
                    emit_line(cg, "STORE R0, [R6:R7 + %d]", field->offset);
                    emit_line(cg, "STORE R1, [R6:R7 + %d]", field->offset + 1);
                } else {
                    emit_line(cg, "POP R0");
                    emit_line(cg, "STORE R0, [R6:R7 + %d]", field->offset);
                }
            } else {
                set_error(cg, node->line,
                          "set! field target must be a variable or (nth array index)");
                break;
            }
        } else if (node->as.set.target_expr != NULL && node->as.set.target_expr->kind == AST_NTH) {
            // (set! (nth arr i) value) - array element mutation
            AstNode* nth_node = node->as.set.target_expr;
            if (nth_node->as.binary.left->kind != AST_SYMBOL) {
                set_error(cg, node->line, "nth array must be a variable name");
                break;
            }
            SeDataLabel* label = find_data_label(cg, nth_node->as.binary.left->as.symbol.name);
            if (!label || label->element_count == 0) {
                set_error(cg, node->line, "set! nth target is not an array");
                break;
            }
            bool is_record_arr = (label->record_type[0] != '\0');

            if (is_record_arr) {
                // Writing a full record to array element
                // Value should be a record constructor call
                SeRecordType* rec = find_record_type(cg, label->record_type);
                if (!rec) {
                    set_error(cg, node->line, "cannot find record type for array");
                    break;
                }
                if (node->as.set.value->kind != AST_CALL) {
                    set_error(cg, node->line,
                              "set! record array element requires record constructor");
                    break;
                }
                // Compute element address → R6:R7
                emit_nth_addr(cg, nth_node);
                if (cg->has_error) break;
                emit_line(cg, "PUSH R6");
                emit_line(cg, "PUSH R7");
                // Emit each field value
                for (size_t f = 0; f < rec->field_count; f++) {
                    int32_t val;
                    if (!eval_const(cg, node->as.set.value->as.call.args[f], &val)) {
                        set_error(cg, node->line,
                                  "record constructor args must be compile-time constants");
                        return;
                    }
                    emit_line(cg, "POP R7");
                    emit_line(cg, "POP R6");
                    emit_line(cg, "PUSH R6");
                    emit_line(cg, "PUSH R7");
                    if (rec->fields[f].is_16bit) {
                        emit_line(cg, "LOADI R0, 0x%02X", (val >> 8) & 0xFF);
                        emit_line(cg, "STORE R0, [R6:R7 + %d]", rec->fields[f].offset);
                        emit_line(cg, "LOADI R0, 0x%02X", val & 0xFF);
                        emit_line(cg, "STORE R0, [R6:R7 + %d]", rec->fields[f].offset + 1);
                    } else {
                        emit_line(cg, "LOADI R0, 0x%02X", val & 0xFF);
                        emit_line(cg, "STORE R0, [R6:R7 + %d]", rec->fields[f].offset);
                    }
                }
                emit_line(cg, "POP R7");
                emit_line(cg, "POP R6");
            } else {
                // Scalar array element: (set! (nth scores i) value)
                emit_expr(cg, node->as.set.value);
                emit_line(cg, "PUSH R0");
                emit_nth_addr(cg, nth_node);
                if (cg->has_error) break;
                emit_line(cg, "POP R0");
                emit_line(cg, "STORE R0, [R6:R7]");
            }
        } else {
            // Simple variable set: (set! var value)
            // First check if target is a local
            SeLocal* local = find_local_info(cg, node->as.set.var);
            if (local) {
                emit_expr(cg, node->as.set.value);
                emit_local_store(cg, local);
                break;
            }

            SeDataLabel* label = find_data_label(cg, node->as.set.var);
            if (!label) {
                set_error(cg, node->line, "set!: target must be a var");
                return;
            }
            int32_t addr = label->addr;

            // Check if target is a 2-byte variable (function pointer or 16-bit)
            if (label->size == 2 && label->element_count == 0 && label->record_type[0] == '\0') {
                // 16-bit variable: R0 = hi, R1 = lo
                emit_expr(cg, node->as.set.value);
                emit_line(cg, "LOADI R6, 0x%02X", (addr >> 8) & 0xFF);
                emit_line(cg, "LOADI R7, 0x%02X", addr & 0xFF);
                emit_line(cg, "STORE R0, [R6:R7]+"); // hi byte, auto-increment
                emit_line(cg, "STORE R1, [R6:R7]");  // lo byte
            } else {
                // For simple values, avoid PUSH/POP: set up addr first, then eval
                if (is_simple_operand(cg, node->as.set.value)) {
                    emit_line(cg, "LOADI R6, 0x%02X", (addr >> 8) & 0xFF);
                    emit_line(cg, "LOADI R7, 0x%02X", addr & 0xFF);
                    emit_expr(cg, node->as.set.value);
                    emit_line(cg, "STORE R0, [R6:R7]");
                } else {
                    emit_expr(cg, node->as.set.value);
                    emit_line(cg, "LOADI R6, 0x%02X", (addr >> 8) & 0xFF);
                    emit_line(cg, "LOADI R7, 0x%02X", addr & 0xFF);
                    emit_line(cg, "STORE R0, [R6:R7]");
                }
            }
        }
        break;
    }
    default: break;
    }
}

static void emit_control_flow(SeCodegen* cg, AstNode* node) {
    switch (node->kind) {
    case AST_IF: {
        int lbl_else = new_label(cg);
        int lbl_end = new_label(cg);

        emit_branch(cg, node->as.if_expr.cond, lbl_else, false);

        emit_expr(cg, node->as.if_expr.then_branch);
        emit_line(cg, "JMP __L%d", lbl_end);

        emit(cg, "__L%d:\n", lbl_else);
        emit_expr(cg, node->as.if_expr.else_branch);

        emit(cg, "__L%d:\n", lbl_end);
        break;
    }

    case AST_WHILE: {
        int lbl_loop = new_label(cg);
        int lbl_end = new_label(cg);

        emit(cg, "__L%d:\n", lbl_loop);
        emit_branch(cg, node->as.while_expr.cond, lbl_end, false);

        for (size_t i = 0; i < node->as.while_expr.body.count; i++) {
            emit_expr(cg, node->as.while_expr.body.items[i]);
        }

        emit_line(cg, "JMP __L%d", lbl_loop);
        emit(cg, "__L%d:\n", lbl_end);
        emit_line(cg, "LOADI R0, 0xFF"); // while returns nil
        break;
    }

    case AST_COND: {
        size_t n = node->as.cond.clause_count;
        bool same_var_const = (n >= 1);
        const char* var_name = NULL;
        if (same_var_const) {
            for (size_t i = 0; i < n; i++) {
                AstNode* t = node->as.cond.tests[i];
                if (!t || t->kind != AST_EQ || !t->as.binary.left ||
                    t->as.binary.left->kind != AST_SYMBOL) {
                    same_var_const = false;
                    break;
                }
                int32_t k;
                if (!eval_const(cg, t->as.binary.right, &k)) {
                    same_var_const = false;
                    break;
                }
                if (expr_is_16bit(cg, t->as.binary.left)) {
                    same_var_const = false;
                    break;
                }
                const char* name = t->as.binary.left->as.symbol.name;
                if (var_name && strcmp(var_name, name) != 0) {
                    same_var_const = false;
                    break;
                }
                var_name = name;
            }
        }
        if (same_var_const && var_name) {
            emit_expr(cg, node->as.cond.tests[0]->as.binary.left);
            int lbl_end = new_label(cg);
            int lbl_no_match = new_label(cg);
            int body_labels[SE_MAX_COND_CLAUSES];
            for (size_t i = 0; i < n; i++) {
                body_labels[i] = new_label(cg);
                int32_t k;
                eval_const(cg, node->as.cond.tests[i]->as.binary.right, &k);
                emit_line(cg, "LOADI R1, 0x%02X", k & 0xFF);
                emit_line(cg, "CMP R0, R1");
                emit_line(cg, "JZ __L%d", body_labels[i]);
            }
            emit_line(cg, "JMP __L%d", lbl_no_match);
            emit(cg, "__L%d:\n", lbl_no_match);
            emit_line(cg, "LOADI R0, 0xFF");
            emit_line(cg, "JMP __L%d", lbl_end);
            for (size_t i = 0; i < n; i++) {
                emit(cg, "__L%d:\n", body_labels[i]);
                for (size_t j = 0; j < node->as.cond.bodies[i].count; j++) {
                    emit_expr(cg, node->as.cond.bodies[i].items[j]);
                }
                emit_line(cg, "JMP __L%d", lbl_end);
            }
            emit(cg, "__L%d:\n", lbl_end);
        } else {
            int lbl_end = new_label(cg);
            for (size_t i = 0; i < n; i++) {
                int lbl_next = new_label(cg);
                emit_branch(cg, node->as.cond.tests[i], lbl_next, false);
                for (size_t j = 0; j < node->as.cond.bodies[i].count; j++) {
                    emit_expr(cg, node->as.cond.bodies[i].items[j]);
                }
                emit_line(cg, "JMP __L%d", lbl_end);
                emit(cg, "__L%d:\n", lbl_next);
            }
            emit_line(cg, "LOADI R0, 0xFF");
            emit(cg, "__L%d:\n", lbl_end);
        }
        break;
    }

    case AST_WHEN: {
        int lbl_end = new_label(cg);
        emit_branch(cg, node->as.when_expr.cond, lbl_end, false);
        for (size_t i = 0; i < node->as.when_expr.body.count; i++) {
            emit_expr(cg, node->as.when_expr.body.items[i]);
        }
        emit(cg, "__L%d:\n", lbl_end);
        break;
    }

    case AST_UNLESS: {
        int lbl_end = new_label(cg);
        // Unless = when NOT cond, so branch TRUE to skip body
        emit_branch(cg, node->as.when_expr.cond, lbl_end, true);
        for (size_t i = 0; i < node->as.when_expr.body.count; i++) {
            emit_expr(cg, node->as.when_expr.body.items[i]);
        }
        emit(cg, "__L%d:\n", lbl_end);
        break;
    }

    case AST_LOGIC_AND: {
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.binary.left);
        emit_line(cg, "JFALSE __L%d", lbl_end);
        emit_expr(cg, node->as.binary.right);
        emit(cg, "__L%d:\n", lbl_end);
        break;
    }

    case AST_LOGIC_OR: {
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.binary.left);
        emit_line(cg, "JTRUE __L%d", lbl_end);
        emit_expr(cg, node->as.binary.right);
        emit(cg, "__L%d:\n", lbl_end);
        break;
    }

    case AST_LOGIC_NOT: {
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.unary.operand);
        emit_line(cg, "JFALSE __L%d", lbl_true);
        emit_bool_materialize(cg, lbl_true, lbl_end);
        break;
    }

    case AST_FOR: {
        int lbl_loop = new_label(cg);
        int lbl_done = new_label(cg);
        int lbl_continue = new_label(cg);
        size_t saved_local_count = cg->local_count;
        int saved_stack_depth = cg->let_stack_depth;
        bool is_outermost = (cg->let_depth == 0);
        cg->let_depth++;

        if (is_outermost) {
            emit_line(cg, "PUSH R2");
            emit_line(cg, "PUSH R3");
            emit_line(cg, "MOVSPR R2:R3");
            cg->let_stack_depth = 0;
        }

        AstNode* coll_node = node->as.for_expr.collection;
        int32_t start_val = 0, end_val = 0;
        bool const_range = (coll_node->kind == AST_RANGE &&
                            eval_const(cg, coll_node->as.range.start, &start_val) &&
                            eval_const(cg, coll_node->as.range.end, &end_val));
        bool dyn_range = (coll_node->kind == AST_RANGE && !const_range);

        // Check if the collection is an array variable (symbol that resolves to a data label
        // with element_count > 0)
        bool is_array = false;
        SeDataLabel* arr_label = NULL;
        if (coll_node->kind == AST_SYMBOL) {
            arr_label = find_data_label(cg, coll_node->as.symbol.name);
            if (arr_label && arr_label->element_count > 0) {
                is_array = true;
            }
        }

        if (is_array) {
            // Array iteration: (for (e arr) ...)
            // Push loop counter (index), starting at 0
            emit_line(cg, "LOADI R0, 0");
            emit_line(cg, "PUSH R0");
            // The binding var refers to the current element (computed from index)
            // We store the index on stack and compute element address in the body
            add_local(cg, node->as.for_expr.var, 0);
            cg->let_stack_depth = 1;
        } else if (const_range) {
            emit_line(cg, "LOADI R0, 0x%02X", start_val & 0xFF);
            emit_line(cg, "PUSH R0");
            add_local(cg, node->as.for_expr.var, 0);
            cg->let_stack_depth = 1;
        } else if (dyn_range) {
            emit_expr(cg, coll_node->as.range.start);
            emit_line(cg, "PUSH R0");
            emit_expr(cg, coll_node->as.range.end);
            emit_line(cg, "PUSH R0");
            add_local(cg, node->as.for_expr.var, -1);
            cg->let_stack_depth = 2;
        } else {
            set_error(cg, node->line, "for collection must be a range or array variable");
            break;
        }

        emit(cg, "__L%d:\n", lbl_loop);
        if (is_array) {
            // Compare index against element count
            emit_line(cg, "LOAD R0, [R2:R3]");
            emit_line(cg, "LOADI R1, 0x%02X", arr_label->element_count & 0xFF);
            emit_line(cg, "CMP R0, R1");
            emit_line(cg, "JNC __L%d", lbl_done);

            // Compute element address into R6:R7: base + index * elem_size
            int32_t base_addr = arr_label->addr;
            int32_t elem_size = arr_label->element_size;
            if (elem_size <= 0) elem_size = 1;
            emit_line(cg, "LOAD R0, [R2:R3]"); // reload index

            if (elem_size == 1) {
                // R0 = byte offset already
            } else if (elem_size == 2) {
                emit_line(cg, "SHL R0");
            } else if (elem_size == 4) {
                emit_line(cg, "SHL R0");
                emit_line(cg, "SHL R0");
            } else {
                // General multiply by elem_size using shift-and-add
                int lbl_ml = new_label(cg);
                int lbl_ms = new_label(cg);
                int lbl_md = new_label(cg);
                emit_line(cg, "MOV R1, R0");
                emit_line(cg, "LOADI R0, 0");
                emit_line(cg, "LOADI R6, 0x%02X", elem_size & 0xFF);
                emit(cg, "__L%d:\n", lbl_ml);
                emit_line(cg, "OR R6, R6");
                emit_line(cg, "JZ __L%d", lbl_md);
                emit_line(cg, "PUSH R6");
                emit_line(cg, "LOADI R7, 1");
                emit_line(cg, "AND R6, R7");
                emit_line(cg, "JZ __L%d", lbl_ms);
                emit_line(cg, "ADD R0, R1");
                emit(cg, "__L%d:\n", lbl_ms);
                emit_line(cg, "SHL R1");
                emit_line(cg, "POP R6");
                emit_line(cg, "SHR R6");
                emit_line(cg, "JMP __L%d", lbl_ml);
                emit(cg, "__L%d:\n", lbl_md);
            }
            // R0 = byte offset from array base
            int lbl_nc = new_label(cg);
            emit_line(cg, "LOADI R6, 0x%02X", (base_addr >> 8) & 0xFF);
            emit_line(cg, "LOADI R7, 0x%02X", base_addr & 0xFF);
            emit_line(cg, "ADD R7, R0");
            emit_line(cg, "JNC __L%d", lbl_nc);
            emit_line(cg, "INC R6");
            emit(cg, "__L%d:\n", lbl_nc);
            // R6:R7 = address of current element
            // For scalar arrays, load value into R0
            if (arr_label->record_type[0] == '\0') {
                emit_line(cg, "LOAD R0, [R6:R7]");
            }
            // For record arrays, R6:R7 is the element reference (used by :field accessors)
        } else if (const_range) {
            emit_line(cg, "LOAD R0, [R2:R3]");
            emit_line(cg, "LOADI R1, 0x%02X", end_val & 0xFF);
            emit_line(cg, "CMP R0, R1");
            emit_line(cg, "JNC __L%d", lbl_done);
        } else { // dyn_range
            emit_line(cg, "LOAD R0, [R2:R3 - 1]");
            emit_line(cg, "LOAD R1, [R2:R3]");
            emit_line(cg, "CMP R0, R1");
            emit_line(cg, "JNC __L%d", lbl_done);
        }
        if (node->as.for_expr.when_cond) {
            emit_branch(cg, node->as.for_expr.when_cond, lbl_continue, false);
        }
        for (size_t i = 0; i < node->as.for_expr.body.count; i++) {
            emit_expr(cg, node->as.for_expr.body.items[i]);
        }
        emit(cg, "__L%d:\n", lbl_continue);
        if (is_array || const_range) {
            // Increment index at [R2:R3]
            emit_line(cg, "LOAD R0, [R2:R3]");
            emit_line(cg, "INC R0");
            emit_line(cg, "STORE R0, [R2:R3]");
        } else { // dyn_range
            emit_line(cg, "LOAD R0, [R2:R3 - 1]");
            emit_line(cg, "INC R0");
            emit_line(cg, "STORE R0, [R2:R3 - 1]");
        }
        emit_line(cg, "JMP __L%d", lbl_loop);
        emit(cg, "__L%d:\n", lbl_done);

        emit_line(cg, "POP R1");
        if (dyn_range) emit_line(cg, "POP R1");
        if (is_outermost) {
            emit_line(cg, "POP R3");
            emit_line(cg, "POP R2");
        }
        cg->let_depth--;
        cg->let_stack_depth = saved_stack_depth;
        cg->local_count = saved_local_count;
        emit_line(cg, "LOADI R0, 0xFF"); // for returns nil
        break;
    }

    case AST_RANGE: emit_expr(cg, node->as.range.start); break;

    case AST_DO:
        for (size_t i = 0; i < node->as.block.exprs.count; i++) {
            emit_expr(cg, node->as.block.exprs.items[i]);
        }
        break;

    case AST_LET: {
        // Save current local count for cleanup
        size_t saved_local_count = cg->local_count;
        size_t binding_count = node->as.let.binding_count;
        int saved_stack_depth = cg->let_stack_depth;
        bool is_outermost = (cg->let_depth == 0);
        int total_pushes = 0; // Track total stack slots pushed for cleanup

        cg->let_depth++;

        if (is_outermost) {
            // Only outermost let saves R2:R3 and establishes new base
            emit_line(cg, "PUSH R2");
            emit_line(cg, "PUSH R3");
            emit_line(cg, "MOVSPR R2:R3");
            cg->let_stack_depth = 0;
        }

        // Evaluate each binding, push it, and register it
        for (size_t i = 0; i < binding_count; i++) {
            bool is_16 =
                se_hint_is_16bit(node->as.let.hints[i]) || expr_is_16bit(cg, node->as.let.vals[i]);
            bool is_sgn = se_hint_is_signed(node->as.let.hints[i]) ||
                          expr_is_signed(cg, node->as.let.vals[i]);

            emit_expr(cg, node->as.let.vals[i]);

            if (is_16) {
                // 16-bit: push high byte first, then low byte
                // R0=hi, R1=lo
                if (!expr_is_16bit(cg, node->as.let.vals[i])) {
                    // Promote 8-bit to 16-bit
                    emit_line(cg, "MOV R1, R0");
                    emit_line(cg, "LOADI R0, 0");
                }
                emit_line(cg, "PUSH R0"); // hi byte
                emit_line(cg, "PUSH R1"); // lo byte
                add_local_full(cg, node->as.let.vars[i], -cg->let_stack_depth, true, is_sgn);
                cg->let_stack_depth += 2;
                total_pushes += 2;
            } else {
                emit_line(cg, "PUSH R0");
                add_local_full(cg, node->as.let.vars[i], -cg->let_stack_depth, false, is_sgn);
                cg->let_stack_depth++;
                total_pushes++;
            }
        }

        // Evaluate body
        for (size_t i = 0; i < node->as.let.body.count; i++) {
            emit_expr(cg, node->as.let.body.items[i]);
        }

        // Deallocate locals
        for (int i = 0; i < total_pushes; i++) {
            emit_line(cg, "POP R1");
        }

        if (is_outermost) {
            // Restore outer R2:R3
            emit_line(cg, "POP R3");
            emit_line(cg, "POP R2");
        }

        // Restore state
        cg->let_depth--;
        cg->let_stack_depth = saved_stack_depth;
        cg->local_count = saved_local_count;
        break;
    }
    default: break;
    }
}

static void emit_call_expr(SeCodegen* cg, AstNode* node) {
    switch (node->kind) {
    case AST_CALL: {
        bool direct = is_function(cg, node->as.call.func);
        bool save_let_base = (cg->let_depth > 0);

        // Save R2:R3 (let-local base) only when inside a let scope
        if (save_let_base) {
            emit_line(cg, "PUSH R2");
            emit_line(cg, "PUSH R3");
        }
        // Always save R4:R5 (frame pointer) - callee will set FP
        emit_line(cg, "PUSH R4");
        emit_line(cg, "PUSH R5");

        // Look up callee param hints for auto-promotion
        SeFunctionInfo* callee_info = find_function_info(cg, node->as.call.func);

        // Push arguments in reverse order
        // For 16-bit args, push lo then hi so that hi ends up at the lower
        // stack address.  The callee reads [FP+offset] -> R0 (hi) and
        // [FP+offset+1] -> R1 (lo), and since the stack grows downward the
        // last-pushed byte occupies the lowest address.
        // Auto-promote 8-bit args to 16-bit when callee expects ^u16
        for (int i = (int)node->as.call.arg_count - 1; i >= 0; i--) {
            emit_expr(cg, node->as.call.args[i]);
            bool arg_is_16 = expr_is_16bit(cg, node->as.call.args[i]);
            bool callee_wants_16 = callee_info && (size_t)i < callee_info->param_count &&
                                   se_hint_is_16bit(callee_info->param_hints[i]);
            if (arg_is_16 || callee_wants_16) {
                if (!arg_is_16) {
                    // Promote 8-bit to 16-bit: R0 has value, move to R1 (lo), set R0=0 (hi)
                    emit_line(cg, "MOV R1, R0");
                    emit_line(cg, "LOADI R0, 0");
                }
                emit_line(cg, "PUSH R1"); // lo byte first (higher stack address)
                emit_line(cg, "PUSH R0"); // hi byte last  (lower stack address = FP+offset)
            } else {
                emit_line(cg, "PUSH R0");
            }
        }

        if (direct) {
            char func_name[SE_MAX_SYMBOL_LEN];
            sanitize_name(func_name, node->as.call.func, SE_MAX_SYMBOL_LEN);
            emit_line(cg, "CALL %s", func_name);
        } else {
            // Indirect call: load function address from variable and patch trampoline
            // First, resolve the function address from the call target variable
            SeLocal* local = find_local_info(cg, node->as.call.func);
            if (local) {
                emit_local_load(cg, local);
            } else {
                SeDataLabel* dl = find_data_label(cg, node->as.call.func);
                if (dl) {
                    int32_t addr = dl->addr;
                    emit_line(cg, "LOADI R6, 0x%02X", (addr >> 8) & 0xFF);
                    emit_line(cg, "LOADI R7, 0x%02X", addr & 0xFF);
                    emit_line(cg, "LOAD R0, [R6:R7]");
                    emit_line(cg, "LOADI R7, 0x%02X", (addr + 1) & 0xFF);
                    emit_line(cg, "LOAD R1, [R6:R7]");
                } else {
                    set_error(cg, node->line, "undefined function or variable");
                    return;
                }
            }

            // R0:R1 now holds the target function address (hi:lo)
            // Patch the __call_indirect trampoline's CALL instruction
            cg->needs_indirect_call = true;
            emit_line(cg, "LOADI R6, __call_indirect >> 8");
            emit_line(cg, "LOADI R7, (__call_indirect & 0xFF) + 1");
            emit_line(cg, "STORE R0, [R6:R7]");
            emit_line(cg, "LOADI R7, (__call_indirect & 0xFF) + 2");
            emit_line(cg, "STORE R1, [R6:R7]");
            emit_line(cg, "CALL __call_indirect");
        }

        // Pop arguments (accounting for 16-bit args and auto-promotion)
        for (size_t i = 0; i < node->as.call.arg_count; i++) {
            bool arg_is_16 = expr_is_16bit(cg, node->as.call.args[i]);
            bool callee_wants_16 = callee_info && i < callee_info->param_count &&
                                   se_hint_is_16bit(callee_info->param_hints[i]);
            if (arg_is_16 || callee_wants_16) {
                emit_line(cg, "POP R1"); // hi byte (was pushed last)
                emit_line(cg, "POP R1"); // lo byte (was pushed first)
            } else {
                emit_line(cg, "POP R1"); // discard into R1
            }
        }

        // Restore R4:R5 and R2:R3
        emit_line(cg, "POP R5");
        emit_line(cg, "POP R4");
        if (save_let_base) {
            emit_line(cg, "POP R3");
            emit_line(cg, "POP R2");
        }
        break;
    }
    default: break;
    }
}

static void emit_records(SeCodegen* cg, AstNode* node) {
    switch (node->kind) {
    case AST_FIELD_GET: {
        // (:field record-expr) - keyword as accessor
        SeRecordType* rec = NULL;

        if (node->as.field_get.record->kind == AST_SYMBOL) {
            // Simple case: (:field var)
            const char* var_name = node->as.field_get.record->as.symbol.name;
            rec = find_var_record_type(cg, var_name);
            if (!rec) {
                char msg[128];
                snprintf(msg, sizeof(msg), "variable '%s' is not a record", var_name);
                set_error(cg, node->line, msg);
                break;
            }
            SeRecordField* field = find_record_field(rec, node->as.field_get.field);
            if (!field) {
                char msg[128];
                snprintf(msg, sizeof(msg), "record '%s' has no field '%s'", rec->name,
                         node->as.field_get.field);
                set_error(cg, node->line, msg);
                break;
            }
            SeDataLabel* dl_fg = find_data_label(cg, var_name);
            if (!dl_fg) {
                set_error(cg, node->line, "cannot resolve record address");
                break;
            }
            int32_t addr = dl_fg->addr;
            emit_line(cg, "LOADI R6, 0x%02X", (addr >> 8) & 0xFF);
            emit_line(cg, "LOADI R7, 0x%02X", addr & 0xFF);
            if (field->is_16bit) {
                emit_line(cg, "LOAD R0, [R6:R7 + %d]", field->offset);
                emit_line(cg, "LOAD R1, [R6:R7 + %d]", field->offset + 1);
            } else {
                emit_line(cg, "LOAD R0, [R6:R7 + %d]", field->offset);
            }
        } else if (node->as.field_get.record->kind == AST_NTH) {
            // Array element field access: (:field (nth arr i))
            AstNode* nth_node = node->as.field_get.record;
            if (nth_node->as.binary.left->kind != AST_SYMBOL) {
                set_error(cg, node->line, "nth array must be a variable name");
                break;
            }
            SeDataLabel* label = find_data_label(cg, nth_node->as.binary.left->as.symbol.name);
            if (!label || label->record_type[0] == '\0') {
                set_error(cg, node->line, "nth target is not a record array");
                break;
            }
            rec = find_record_type(cg, label->record_type);
            if (!rec) {
                set_error(cg, node->line, "cannot find record type for array");
                break;
            }
            SeRecordField* field = find_record_field(rec, node->as.field_get.field);
            if (!field) {
                char msg[128];
                snprintf(msg, sizeof(msg), "record '%s' has no field '%s'", rec->name,
                         node->as.field_get.field);
                set_error(cg, node->line, msg);
                break;
            }
            // Compute element base address → R6:R7
            emit_nth_addr(cg, nth_node);
            if (cg->has_error) break;
            // Load field at offset
            if (field->is_16bit) {
                emit_line(cg, "LOAD R0, [R6:R7 + %d]", field->offset);
                emit_line(cg, "LOAD R1, [R6:R7 + %d]", field->offset + 1);
            } else {
                emit_line(cg, "LOAD R0, [R6:R7 + %d]", field->offset);
            }
        } else {
            set_error(cg, node->line, "keyword accessor requires a variable or (nth array index)");
        }
        break;
    }

    case AST_NTH: {
        // (nth array index) - access element by index
        SeDataLabel* label = emit_nth_addr(cg, node);
        if (!label || cg->has_error) break;
        if (label->record_type[0] != '\0') {
            // Record array: address is in R6:R7 - that IS the result (a reference)
            // Caller (field_get, set!) will access via R6:R7
        } else {
            // Scalar array: load the value
            emit_line(cg, "LOAD R0, [R6:R7]");
        }
        break;
    }

    case AST_LEN: {
        // (len array) - compile-time constant element count
        int32_t count;
        if (eval_const(cg, node, &count)) {
            if (count > 255 || count < 0) {
                set_error(cg, node->line, "array length out of 8-bit range");
                break;
            }
            emit_line(cg, "LOADI R0, 0x%02X", count & 0xFF);
        } else {
            set_error(cg, node->line, "len requires a known array variable");
        }
        break;
    }

    case AST_ARRAY:
        // (array ...) should only appear as value in (var ...), not as standalone expression
        set_error(cg, node->line, "array can only be used in var declarations");
        break;

    case AST_DEFRECORD: break;
    default: break;
    }
}

static void emit_expr(SeCodegen* cg, AstNode* node) {
    if (cg->has_error) return;
    switch (node->kind) {
    case AST_NUMBER:
    case AST_SYMBOL:
    case AST_NIL:
    case AST_TRUE:
    case AST_FALSE:
    case AST_KEYWORD: emit_literal(cg, node); break;
    case AST_ADD:
    case AST_SUB:
    case AST_NEG:
    case AST_INC:
    case AST_DEC:
    case AST_MUL:
    case AST_DIV:
    case AST_MOD: emit_arithmetic(cg, node); break;
    case AST_BAND:
    case AST_BOR:
    case AST_XOR:
    case AST_BNOT:
    case AST_SHL:
    case AST_SHR: emit_bitwise(cg, node); break;
    case AST_EQ:
    case AST_NE:
    case AST_LT:
    case AST_GT:
    case AST_LE:
    case AST_GE:
    case AST_LNOT: emit_comparison(cg, node); break;
    case AST_ADDR:
    case AST_ADDR16:
    case AST_LOAD:
    case AST_STORE:
    case AST_HI:
    case AST_LO: emit_memory(cg, node); break;
    case AST_SET:
    case AST_SET_BANG: emit_variables(cg, node); break;
    case AST_IF:
    case AST_WHILE:
    case AST_COND:
    case AST_WHEN:
    case AST_UNLESS:
    case AST_LOGIC_AND:
    case AST_LOGIC_OR:
    case AST_LOGIC_NOT:
    case AST_FOR:
    case AST_RANGE:
    case AST_DO:
    case AST_LET: emit_control_flow(cg, node); break;
    case AST_CALL: emit_call_expr(cg, node); break;
    case AST_ASM: emit(cg, "    %s\n", node->as.symbol.name); break;
    case AST_IMPORT: set_error(cg, node->line, "include cannot be used inside expressions"); break;
    case AST_FIELD_GET:
    case AST_NTH:
    case AST_LEN:
    case AST_ARRAY:
    case AST_DEFRECORD: emit_records(cg, node); break;
    case AST_NILP: {
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.unary.operand);
        emit_line(cg, "LOADI R1, 0xFF");
        emit_line(cg, "CMP R0, R1");
        emit_line(cg, "JZ __L%d", lbl_true);
        emit_bool_materialize(cg, lbl_true, lbl_end);
        break;
    }
    case AST_ZEROP: {
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.unary.operand);
        emit_line(cg, "OR R0, R0");
        emit_line(cg, "JZ __L%d", lbl_true);
        emit_bool_materialize(cg, lbl_true, lbl_end);
        break;
    }
    case AST_POSP: {
        int lbl_false = new_label(cg);
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.unary.operand);
        emit_line(cg, "OR R0, R0");
        emit_line(cg, "JZ __L%d", lbl_false);
        emit_line(cg, "LOADI R1, 0x80");
        emit_line(cg, "AND R1, R0");
        emit_line(cg, "JNZ __L%d", lbl_false);
        emit_line(cg, "LOADI R0, 1");
        emit_line(cg, "JMP __L%d", lbl_end);
        emit(cg, "__L%d:\n", lbl_false);
        emit_line(cg, "LOADI R0, 0");
        emit(cg, "__L%d:\n", lbl_end);
        break;
    }
    case AST_NEGP: {
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.unary.operand);
        emit_line(cg, "LOADI R1, 0x80");
        emit_line(cg, "AND R0, R1");
        emit_line(cg, "JNZ __L%d", lbl_true);
        emit_bool_materialize(cg, lbl_true, lbl_end);
        break;
    }
    case AST_CAST_U8:
    case AST_CAST_I8: emit_expr(cg, node->as.unary.operand); break;
    case AST_FN: {
        char fn_name[SE_MAX_SYMBOL_LEN];
        sanitize_name(fn_name, node->as.defn.name, SE_MAX_SYMBOL_LEN);
        emit_line(cg, "LOADI R0, %s >> 8", fn_name);
        emit_line(cg, "LOADI R1, %s & 0xFF", fn_name);
        break;
    }
    case AST_NS: set_error(cg, node->line, "ns cannot be used inside expressions"); break;

    case AST_REQUIRE: set_error(cg, node->line, "require cannot be used inside expressions"); break;

    default: set_error(cg, node->line, "unsupported expression"); break;
    }
}

static void emit_data_value(SeCodegen* cg, AstNode* node) {
    if (cg->has_error) return;

    switch (node->kind) {
    case AST_NUMBER:
        if (node->as.number > 255 || node->as.number < -128) {
            char msg[128];
            snprintf(msg, sizeof(msg), "data value 0x%02X out of 8-bit range (0-255)",
                     node->as.number);
            set_error(cg, node->line, msg);
            return;
        }
        emit(cg, "    DB 0x%02X\n", node->as.number & 0xFF);
        break;

    case AST_STRING: {
        emit(cg, "    DB ");
        for (size_t i = 0; node->as.symbol.name[i]; i++) {
            if (i > 0) emit(cg, ", ");
            emit(cg, "0x%02X", (unsigned char)node->as.symbol.name[i]);
        }
        emit(cg, "\n");
        break;
    }

    case AST_DB:
        for (size_t i = 0; i < node->as.block.exprs.count; i++) {
            emit_data_value(cg, node->as.block.exprs.items[i]);
        }
        break;

    case AST_REPEAT:
        emit(cg, "    TIMES %d ", node->as.repeat.count);
        // For TIMES, we need inline DB
        if (node->as.repeat.form->kind == AST_DB) {
            emit(cg, "DB ");
            for (size_t i = 0; i < node->as.repeat.form->as.block.exprs.count; i++) {
                if (i > 0) emit(cg, ", ");
                AstNode* val = node->as.repeat.form->as.block.exprs.items[i];
                if (val->kind == AST_NUMBER) {
                    emit(cg, "0x%02X", val->as.number & 0xFF);
                }
            }
            emit(cg, "\n");
        } else {
            emit_data_value(cg, node->as.repeat.form);
        }
        break;

    default: {
        // Try to evaluate as constant
        int32_t val;
        if (eval_const(cg, node, &val)) {
            if (val > 255 || val < -128) {
                char msg[128];
                snprintf(msg, sizeof(msg), "data value 0x%02X out of 8-bit range (0-255)", val);
                set_error(cg, node->line, msg);
                return;
            }
            emit(cg, "    DB 0x%02X\n", val & 0xFF);
        } else {
            set_error(cg, node->line, "invalid data value");
        }
        break;
    }
    }
}

static void emit_function(SeCodegen* cg, AstNode* node) {
    if (cg->has_error) return;

    char name[SE_MAX_SYMBOL_LEN];
    sanitize_name(name, node->as.defn.name, SE_MAX_SYMBOL_LEN);
    emit(cg, "%s:\n", name);

    // reset locals for this function
    cg->local_count = 0;
    cg->param_count = node->as.defn.param_count;

    // set up frame pointer
    emit_line(cg, "MOVSPR R4:R5");

    // Parameters are at FP + 3 + cumulative offset
    // Because: FP points to SP after CALL, return address is at SP+1 and SP+2
    // Arguments are pushed in reverse order, so first arg (args[0]) is pushed last
    // and ends up at SP+3. For 16-bit params, 2 stack slots are used.
    {
        int param_offset = 3;
        for (size_t i = 0; i < node->as.defn.param_count; i++) {
            bool is_16 = se_hint_is_16bit(node->as.defn.param_hints[i]);
            bool is_sgn = se_hint_is_signed(node->as.defn.param_hints[i]);
            add_local_full(cg, node->as.defn.params[i], param_offset, is_16, is_sgn);
            param_offset += is_16 ? 2 : 1;
        }
    }

    for (size_t i = 0; i < node->as.defn.body.count; i++) {
        emit_expr(cg, node->as.defn.body.items[i]);
    }

    emit_line(cg, "RET");
    emit(cg, "\n");
}

static void emit_data(SeCodegen* cg, AstNode* node) {
    if (cg->has_error) return;

    char name[SE_MAX_SYMBOL_LEN];
    sanitize_name(name, node->as.data.name, SE_MAX_SYMBOL_LEN);

    SeDataLabel* dl = find_data_label(cg, node->as.data.name);
    int32_t addr = dl ? dl->addr : -1;

    if (addr >= 0) {
        emit(cg, "    ORG 0x%04X\n", addr);
    }
    emit(cg, "%s:\n", name);

    for (size_t i = 0; i < node->as.data.body.count; i++) {
        emit_data_value(cg, node->as.data.body.items[i]);
    }
    emit(cg, "\n");
}

static void emit_var(SeCodegen* cg, AstNode* node) {
    if (cg->has_error) return;

    char name[SE_MAX_SYMBOL_LEN];
    sanitize_name(name, node->as.var.name, SE_MAX_SYMBOL_LEN);

    SeDataLabel* dl_var = find_data_label(cg, node->as.var.name);
    int32_t addr = dl_var ? dl_var->addr : -1;

    // Check if this is a string literal: (var title "HELLO")
    if (node->as.var.value->kind == AST_STRING) {
        if (addr >= 0) {
            emit(cg, "    ORG 0x%04X\n", addr);
        }
        emit(cg, "%s:\n", name);
        const char* str = node->as.var.value->as.symbol.name;
        emit(cg, "    DB ");
        for (size_t i = 0; str[i]; i++) {
            if (i > 0) emit(cg, ", ");
            emit(cg, "0x%02X", (unsigned char)str[i]);
        }
        emit(cg, ", 0x00\n\n"); // null terminator
        return;
    }

    // Check if this is an array declaration: (var scores (array 4 0))
    if (node->as.var.value->kind == AST_ARRAY) {
        AstNode* arr = node->as.var.value;
        int32_t count;
        if (!eval_const(cg, arr->as.array_expr.count, &count)) {
            set_error(cg, node->line, "array count must be compile-time constant");
            return;
        }

        if (addr >= 0) {
            emit(cg, "    ORG 0x%04X\n", addr);
        }
        emit(cg, "%s:\n", name);

        // Check if the value is a record constructor
        if (arr->as.array_expr.value->kind == AST_CALL) {
            SeRecordType* rec = find_record_type(cg, arr->as.array_expr.value->as.call.func);
            if (rec) {
                if (arr->as.array_expr.value->as.call.arg_count != rec->field_count) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "record '%s' expects %zu fields, got %zu", rec->name,
                             rec->field_count, arr->as.array_expr.value->as.call.arg_count);
                    set_error(cg, node->line, msg);
                    return;
                }
                // Emit count copies of the record
                for (int32_t elem = 0; elem < count; elem++) {
                    for (size_t f = 0; f < rec->field_count; f++) {
                        int32_t val;
                        if (!eval_const(cg, arr->as.array_expr.value->as.call.args[f], &val)) {
                            set_error(cg, node->line,
                                      "record field initial value must be compile-time constant");
                            return;
                        }
                        if (rec->fields[f].is_16bit) {
                            emit(cg, "    DB 0x%02X, 0x%02X\n", (val >> 8) & 0xFF, val & 0xFF);
                        } else {
                            emit(cg, "    DB 0x%02X\n", val & 0xFF);
                        }
                    }
                }
                emit(cg, "\n");
                return;
            }
        }

        // Scalar array
        int32_t val;
        if (!eval_const(cg, arr->as.array_expr.value, &val)) {
            set_error(cg, node->line, "array initial value must be compile-time constant");
            return;
        }
        if (val > 255 || val < -128) {
            set_error(cg, node->line, "array initial value out of 8-bit range");
            return;
        }
        // Use TIMES directive for efficient emission
        emit(cg, "    TIMES %d DB 0x%02X\n\n", count, val & 0xFF);
        return;
    }

    // Check if this is a record constructor call: (var player (entity 10 20 3 :alive))
    if (node->as.var.value->kind == AST_CALL) {
        SeRecordType* rec = find_record_type(cg, node->as.var.value->as.call.func);
        if (rec) {
            if (node->as.var.value->as.call.arg_count != rec->field_count) {
                char msg[128];
                snprintf(msg, sizeof(msg), "record '%s' expects %zu fields, got %zu", rec->name,
                         rec->field_count, node->as.var.value->as.call.arg_count);
                set_error(cg, node->line, msg);
                return;
            }

            if (addr >= 0) {
                emit(cg, "    ORG 0x%04X\n", addr);
            }
            emit(cg, "%s:\n", name);

            // Emit each field's initial value
            for (size_t f = 0; f < rec->field_count; f++) {
                int32_t val;
                if (!eval_const(cg, node->as.var.value->as.call.args[f], &val)) {
                    set_error(cg, node->line,
                              "record field initial value must be compile-time constant");
                    return;
                }
                if (rec->fields[f].is_16bit) {
                    emit(cg, "    DB 0x%02X, 0x%02X\n", (val >> 8) & 0xFF, val & 0xFF);
                } else {
                    if (val > 255 || val < -128) {
                        char msg[128];
                        snprintf(msg, sizeof(msg), "field '%s' value 0x%02X out of 8-bit range",
                                 rec->fields[f].name, val);
                        set_error(cg, node->line, msg);
                        return;
                    }
                    emit(cg, "    DB 0x%02X\n", val & 0xFF);
                }
            }
            emit(cg, "\n");
            return;
        }
    }

    // Check if value is an anonymous function (fn expression)
    if (node->as.var.value->kind == AST_FN) {
        if (addr >= 0) {
            emit(cg, "    ORG 0x%04X\n", addr);
        }
        emit(cg, "%s:\n", name);
        char fn_name[SE_MAX_SYMBOL_LEN];
        sanitize_name(fn_name, node->as.var.value->as.defn.name, SE_MAX_SYMBOL_LEN);
        // Emit 2 bytes: hi and lo of the function address
        // These are assembler expressions resolved at link time
        emit(cg, "    DB %s >> 8, %s & 0xFF\n\n", fn_name, fn_name);
        return;
    }

    // Check if value is a function reference (symbol naming a known function)
    if (node->as.var.value->kind == AST_SYMBOL &&
        is_function(cg, node->as.var.value->as.symbol.name)) {
        if (addr >= 0) {
            emit(cg, "    ORG 0x%04X\n", addr);
        }
        emit(cg, "%s:\n", name);
        char fn_name[SE_MAX_SYMBOL_LEN];
        sanitize_name(fn_name, node->as.var.value->as.symbol.name, SE_MAX_SYMBOL_LEN);
        emit(cg, "    DB %s >> 8, %s & 0xFF\n\n", fn_name, fn_name);
        return;
    }

    // Simple scalar var
    int32_t val;
    if (!eval_const(cg, node->as.var.value, &val)) {
        set_error(cg, node->line, "var initial value must be compile-time constant");
        return;
    }

    SeDataLabel* dl = find_data_label(cg, node->as.var.name);
    bool is_16 = (dl && dl->is_16bit);

    if (!is_16 && (val > 255 || val < -128)) {
        set_error(cg, node->line, "var initial value out of 8-bit range");
        return;
    }

    if (addr >= 0) {
        emit(cg, "    ORG 0x%04X\n", addr);
    }
    emit(cg, "%s:\n", name);
    if (is_16) {
        emit(cg, "    DB 0x%02X, 0x%02X\n\n", (val >> 8) & 0xFF, val & 0xFF);
    } else {
        emit(cg, "    DB 0x%02X\n\n", val & 0xFF);
    }
}

bool se_codegen_emit(SeCodegen* cg, AstProgram* program) {
    emit(cg, "; Generated by tiny16se compiler\n");
    emit(cg, "; Source: %s\n\n", cg->filename);

    // Emit compiler support macros
    emit(cg, "; Compiler macros\n");

    // JFALSE target: jump if R0 is falsy (0x00=false or 0xFF=nil)
    emit(cg, ".macro JFALSE target\n");
    emit(cg, "    LOADI R1, 0\n");
    emit(cg, "    CMP R0, R1\n");
    emit(cg, "    JZ target\n");
    emit(cg, "    LOADI R1, 0xFF\n");
    emit(cg, "    CMP R0, R1\n");
    emit(cg, "    JZ target\n");
    emit(cg, ".endmacro\n\n");

    // JTRUE target: jump if R0 is truthy (not 0x00 and not 0xFF)
    emit(cg, ".macro JTRUE target\n");
    emit(cg, "    LOADI R1, 0\n");
    emit(cg, "    CMP R0, R1\n");
    emit(cg, "    JZ @skip\n");
    emit(cg, "    LOADI R1, 0xFF\n");
    emit(cg, "    CMP R0, R1\n");
    emit(cg, "    JZ @skip\n");
    emit(cg, "    JMP target\n");
    emit(cg, "@skip:\n");
    emit(cg, ".endmacro\n\n");

    emit(cg, "; Constants\n");
    for (size_t i = 0; i < cg->constant_count; i++) {
        if (!cg->constants[i].is_function) {
            char name[SE_MAX_SYMBOL_LEN];
            sanitize_name(name, cg->constants[i].name, SE_MAX_SYMBOL_LEN);
            int32_t val = cg->constants[i].value;
            if (val < 0 || val > 0xFF) {
                emit(cg, "%s = 0x%04X\n", name, (unsigned)(val & 0xFFFF));
            } else {
                emit(cg, "%s = 0x%02X\n", name, (unsigned)(val & 0xFF));
            }
        }
    }

    emit(cg, "\n");
    emit(cg, "section .code\n\n");
    emit(cg, "; Entry point\n");
    emit(cg, "_start:\n");
    emit_line(cg, "CALL main");
    emit_line(cg, "HALT");
    emit(cg, "\n");

    for (size_t i = 0; i < program->node_count; i++) {
        if (program->nodes[i]->kind == AST_DEFN) {
            emit_function(cg, program->nodes[i]);
        }
    }

    // Emit anonymous function bodies
    for (size_t i = 0; i < cg->anon_fn_count; i++) {
        emit_function(cg, cg->anon_fns[i]);
    }

    // Emit indirect call trampoline if needed
    if (cg->needs_indirect_call) {
        emit(cg, "; Indirect call trampoline (self-modifying)\n");
        emit(cg, "__call_indirect:\n");
        emit_line(cg, "CALL 0x0000");
        emit_line(cg, "RET");
        emit(cg, "\n");
    }

    bool has_data = false;
    for (size_t i = 0; i < program->node_count; i++) {
        if (program->nodes[i]->kind == AST_DATA) {
            if (!has_data) {
                emit(cg, "section .data\n\n");
                has_data = true;
            }
            emit_data(cg, program->nodes[i]);
        } else if (program->nodes[i]->kind == AST_VAR) {
            if (!has_data) {
                emit(cg, "section .data\n\n");
                has_data = true;
            }
            emit_var(cg, program->nodes[i]);
        }
    }

    // Emit includes at the end (typically contain data like tilesets, fonts)
    for (size_t i = 0; i < program->node_count; i++) {
        if (program->nodes[i]->kind == AST_IMPORT) {
            emit(cg, ".include \"%s\"\n", program->nodes[i]->as.symbol.name);
        }
    }

    return !cg->has_error;
}

bool se_codegen_has_error(SeCodegen* cg) { return cg->has_error; }

void se_codegen_print_error(SeCodegen* cg) {
    fprintf(stderr, "%s:%zu: error: %s\n", cg->filename, cg->error_line, cg->error_msg);
}
