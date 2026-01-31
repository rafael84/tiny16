#include "codegen.h"

#include <stdarg.h>
#include <string.h>

// Convert symbol to valid assembly identifier (replace - with _)
static void sanitize_name(char* dest, const char* src, size_t max_len) {
    size_t i;
    for (i = 0; i < max_len - 1 && src[i]; i++) {
        dest[i] = (src[i] == '-') ? '_' : src[i];
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
        if (strcmp(cg->functions[i], name) == 0) return true;
    }
    return false;
}

static int find_local(SeCodegen* cg, const char* name) {
    for (size_t i = 0; i < cg->local_count; i++) {
        if (strcmp(cg->locals[i].name, name) == 0) {
            return cg->locals[i].offset;
        }
    }
    return -1;
}

static bool add_local(SeCodegen* cg, const char* name, int offset) {
    if (cg->local_count >= SE_MAX_LOCALS) return false;
    strncpy(cg->locals[cg->local_count].name, name, SE_MAX_SYMBOL_LEN - 1);
    cg->locals[cg->local_count].name[SE_MAX_SYMBOL_LEN - 1] = '\0';
    cg->locals[cg->local_count].offset = offset;
    cg->local_count++;
    return true;
}

void se_codegen_init(SeCodegen* cg, FILE* output, const char* filename) {
    memset(cg, 0, sizeof(*cg));
    cg->output = output;
    cg->filename = filename;
    cg->label_counter = 0;
    cg->data_base_addr = TINY16_MEMORY_DATA_BEGIN;
    cg->data_current_addr = TINY16_MEMORY_DATA_BEGIN;
}

// Look up a data label's address
static bool is_data_label(SeCodegen* cg, const char* name, int32_t* addr) {
    for (size_t i = 0; i < cg->data_label_count; i++) {
        if (strcmp(cg->data_labels[i].name, name) == 0) {
            if (addr) *addr = cg->data_labels[i].addr;
            return true;
        }
    }
    return false;
}

// Calculate the size of a data body element
static int32_t calc_data_size(AstNode* node) {
    switch (node->kind) {
    case AST_NUMBER:
    case AST_SYMBOL: return 1;
    case AST_STRING: return (int32_t)strlen(node->as.symbol.name);
    case AST_DB: {
        int32_t size = 0;
        for (size_t i = 0; i < node->as.block.expr_count; i++) {
            size += calc_data_size(node->as.block.exprs[i]);
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

    case AST_SYMBOL: {
        int32_t val;
        if (is_constant(cg, node->as.symbol.name, &val)) {
            *result = val;
            return true;
        }
        if (is_data_label(cg, node->as.symbol.name, &val)) {
            *result = val;
            return true;
        }
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

    case AST_AND: {
        int32_t left, right;
        if (!eval_const(cg, node->as.binary.left, &left)) return false;
        if (!eval_const(cg, node->as.binary.right, &right)) return false;
        *result = left & right;
        return true;
    }

    case AST_OR: {
        int32_t left, right;
        if (!eval_const(cg, node->as.binary.left, &left)) return false;
        if (!eval_const(cg, node->as.binary.right, &right)) return false;
        *result = left | right;
        return true;
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
        if (!eval_const(cg, node->as.unary.operand, &val)) return false;
        *result = (val >> 8) & 0xFF;
        return true;
    }

    case AST_LO: {
        int32_t val;
        if (!eval_const(cg, node->as.unary.operand, &val)) return false;
        *result = val & 0xFF;
        return true;
    }

    default: return false;
    }
}

bool se_codegen_collect(SeCodegen* cg, AstProgram* program) {
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
            strncpy(cg->functions[cg->function_count], node->as.defn.name, SE_MAX_SYMBOL_LEN - 1);
            cg->function_count++;
        }
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
            for (size_t j = 0; j < node->as.data.body_count; j++) {
                size += calc_data_size(node->as.data.body[j]);
            }

            // Store data label info
            strncpy(cg->data_labels[cg->data_label_count].name, node->as.data.name,
                    SE_MAX_SYMBOL_LEN - 1);
            cg->data_labels[cg->data_label_count].addr = addr;
            cg->data_labels[cg->data_label_count].size = size;
            cg->data_label_count++;

            // Update current address for next auto-placed data
            if (addr >= cg->data_current_addr) {
                cg->data_current_addr = addr + size;
            }
        }
    }

    return true;
}

static void emit_expr(SeCodegen* cg, AstNode* node);

static void emit_expr(SeCodegen* cg, AstNode* node) {
    if (cg->has_error) return;

    switch (node->kind) {
    case AST_NUMBER:
        if (node->as.number > 255 || node->as.number < -128) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "value 0x%02X out of 8-bit range (0-255); use (hi) or (lo) for addresses",
                     node->as.number);
            set_error(cg, node->line, msg);
            return;
        }
        emit_line(cg, "LOADI R0, 0x%02X", node->as.number & 0xFF);
        break;

    case AST_SYMBOL: {
        int32_t val;
        if (is_constant(cg, node->as.symbol.name, &val)) {
            if (val > 255 || val < -128) {
                char msg[128];
                snprintf(
                    msg, sizeof(msg),
                    "constant '%s' value 0x%02X out of 8-bit range; use (hi) or (lo) for addresses",
                    node->as.symbol.name, val);
                set_error(cg, node->line, msg);
                return;
            }
            emit_line(cg, "LOADI R0, 0x%02X", val & 0xFF);
            return;
        }

        int offset = find_local(cg, node->as.symbol.name);
        if (offset != -1) {
            if (offset < -1000) {
                // Let-bound local: use R2:R3 base
                int actual_offset = -(offset + 1000);
                emit_line(cg, "LOAD R0, [R2:R3 + %d]", actual_offset);
            } else {
                // Function parameter: use R4:R5 (FP) base
                emit_line(cg, "LOAD R0, [R4:R5 + %d]", offset);
            }
            return;
        }

        set_error(cg, node->line, "undefined variable");
        break;
    }

    case AST_ADD:
        emit_expr(cg, node->as.binary.left);
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.binary.right);
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "POP R0");
        emit_line(cg, "ADD R0, R1");
        break;

    case AST_SUB:
        emit_expr(cg, node->as.binary.left);
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.binary.right);
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "POP R0");
        emit_line(cg, "SUB R0, R1");
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

    case AST_AND:
        emit_expr(cg, node->as.binary.left);
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.binary.right);
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "POP R0");
        emit_line(cg, "AND R0, R1");
        break;

    case AST_OR:
        emit_expr(cg, node->as.binary.left);
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.binary.right);
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "POP R0");
        emit_line(cg, "OR R0, R1");
        break;

    case AST_XOR:
        emit_expr(cg, node->as.binary.left);
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.binary.right);
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "POP R0");
        emit_line(cg, "XOR R0, R1");
        break;

    case AST_NOT:
        emit_expr(cg, node->as.unary.operand);
        emit_line(cg, "LOADI R1, 0xFF");
        emit_line(cg, "XOR R0, R1");
        break;

    case AST_SHL:
        emit_expr(cg, node->as.binary.left);
        // For now, only support constant shift amounts
        if (node->as.binary.right->kind == AST_NUMBER) {
            int count = node->as.binary.right->as.number;
            for (int i = 0; i < count && i < 8; i++) {
                emit_line(cg, "SHL R0");
            }
        } else {
            set_error(cg, node->line, "shift amount must be constant");
        }
        break;

    case AST_SHR:
        emit_expr(cg, node->as.binary.left);
        if (node->as.binary.right->kind == AST_NUMBER) {
            int count = node->as.binary.right->as.number;
            for (int i = 0; i < count && i < 8; i++) {
                emit_line(cg, "SHR R0");
            }
        } else {
            set_error(cg, node->line, "shift amount must be constant");
        }
        break;

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
                // General case: loop multiplication
                int lbl_loop = new_label(cg);
                int lbl_done = new_label(cg);
                emit_line(cg, "MOV R1, R0");
                emit_line(cg, "LOADI R0, 0");
                emit_line(cg, "LOADI R3, 0x%02X", const_val & 0xFF);
                emit(cg, "__L%d:\n", lbl_loop);
                emit_line(cg, "OR R3, R3");
                emit_line(cg, "JZ __L%d", lbl_done);
                emit_line(cg, "ADD R0, R1");
                emit_line(cg, "DEC R3");
                emit_line(cg, "JMP __L%d", lbl_loop);
                emit(cg, "__L%d:\n", lbl_done);
            }
        } else {
            // Runtime multiplication
            int lbl_loop = new_label(cg);
            int lbl_done = new_label(cg);
            emit_expr(cg, node->as.binary.right); // multiplier
            emit_line(cg, "PUSH R0");
            emit_expr(cg, node->as.binary.left); // multiplicand
            emit_line(cg, "MOV R1, R0");
            emit_line(cg, "LOADI R0, 0");
            emit_line(cg, "POP R3"); // multiplier count
            emit(cg, "__L%d:\n", lbl_loop);
            emit_line(cg, "OR R3, R3");
            emit_line(cg, "JZ __L%d", lbl_done);
            emit_line(cg, "ADD R0, R1");
            emit_line(cg, "DEC R3");
            emit_line(cg, "JMP __L%d", lbl_loop);
            emit(cg, "__L%d:\n", lbl_done);
        }
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

    case AST_EQ: {
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.binary.left);
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.binary.right);
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "POP R0");
        emit_line(cg, "CMP R0, R1");
        emit_line(cg, "JZ __L%d", lbl_true);
        emit_line(cg, "LOADI R0, 0");
        emit_line(cg, "JMP __L%d", lbl_end);
        emit(cg, "__L%d:\n", lbl_true);
        emit_line(cg, "LOADI R0, 1");
        emit(cg, "__L%d:\n", lbl_end);
        break;
    }

    case AST_NE: {
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.binary.left);
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.binary.right);
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "POP R0");
        emit_line(cg, "CMP R0, R1");
        emit_line(cg, "JNZ __L%d", lbl_true);
        emit_line(cg, "LOADI R0, 0");
        emit_line(cg, "JMP __L%d", lbl_end);
        emit(cg, "__L%d:\n", lbl_true);
        emit_line(cg, "LOADI R0, 1");
        emit(cg, "__L%d:\n", lbl_end);
        break;
    }

    case AST_LT: {
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.binary.left);
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.binary.right);
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "POP R0");
        emit_line(cg, "CMP R0, R1");
        emit_line(cg, "JC __L%d", lbl_true);
        emit_line(cg, "LOADI R0, 0");
        emit_line(cg, "JMP __L%d", lbl_end);
        emit(cg, "__L%d:\n", lbl_true);
        emit_line(cg, "LOADI R0, 1");
        emit(cg, "__L%d:\n", lbl_end);
        break;
    }

    case AST_GT: {
        // a > b means b < a, so swap and use JC
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.binary.right); // eval b first
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.binary.left); // eval a
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "POP R0");
        emit_line(cg, "CMP R0, R1"); // b - a, C if b < a
        emit_line(cg, "JC __L%d", lbl_true);
        emit_line(cg, "LOADI R0, 0");
        emit_line(cg, "JMP __L%d", lbl_end);
        emit(cg, "__L%d:\n", lbl_true);
        emit_line(cg, "LOADI R0, 1");
        emit(cg, "__L%d:\n", lbl_end);
        break;
    }

    case AST_LE: {
        // a <= b means NOT (a > b) means NOT (b < a)
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.binary.left);
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.binary.right);
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "POP R0");
        emit_line(cg, "CMP R0, R1");
        // C or Z means <=
        emit_line(cg, "JC __L%d", lbl_true);
        emit_line(cg, "JZ __L%d", lbl_true);
        emit_line(cg, "LOADI R0, 0");
        emit_line(cg, "JMP __L%d", lbl_end);
        emit(cg, "__L%d:\n", lbl_true);
        emit_line(cg, "LOADI R0, 1");
        emit(cg, "__L%d:\n", lbl_end);
        break;
    }

    case AST_GE: {
        // a >= b means NOT (a < b)
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.binary.left);
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.binary.right);
        emit_line(cg, "MOV R1, R0");
        emit_line(cg, "POP R0");
        emit_line(cg, "CMP R0, R1");
        emit_line(cg, "JNC __L%d", lbl_true); // no carry = >=
        emit_line(cg, "LOADI R0, 0");
        emit_line(cg, "JMP __L%d", lbl_end);
        emit(cg, "__L%d:\n", lbl_true);
        emit_line(cg, "LOADI R0, 1");
        emit(cg, "__L%d:\n", lbl_end);
        break;
    }

    case AST_LNOT: {
        int lbl_true = new_label(cg);
        int lbl_end = new_label(cg);
        emit_expr(cg, node->as.unary.operand);
        emit_line(cg, "OR R0, R0"); // set Z if R0 == 0
        emit_line(cg, "JZ __L%d", lbl_true);
        emit_line(cg, "LOADI R0, 0");
        emit_line(cg, "JMP __L%d", lbl_end);
        emit(cg, "__L%d:\n", lbl_true);
        emit_line(cg, "LOADI R0, 1");
        emit(cg, "__L%d:\n", lbl_end);
        break;
    }

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
        emit_expr(cg, node->as.load.addr); // sets up R6:R7
        emit_line(cg, "LOAD R0, [R6:R7]");
        break;

    case AST_STORE:
        emit_expr(cg, node->as.store.value);
        emit_line(cg, "PUSH R0");
        emit_expr(cg, node->as.store.addr); // sets up R6:R7
        emit_line(cg, "POP R0");
        emit_line(cg, "STORE R0, [R6:R7]");
        break;

    case AST_HI: {
        int32_t val;
        if (eval_const(cg, node->as.unary.operand, &val)) {
            emit_line(cg, "LOADI R0, 0x%02X", (val >> 8) & 0xFF);
        } else {
            set_error(cg, node->line, "hi requires compile-time constant");
        }
        break;
    }

    case AST_LO: {
        int32_t val;
        if (eval_const(cg, node->as.unary.operand, &val)) {
            emit_line(cg, "LOADI R0, 0x%02X", val & 0xFF);
        } else {
            set_error(cg, node->line, "lo requires compile-time constant");
        }
        break;
    }

    case AST_SET: {
        emit_expr(cg, node->as.set.value);
        int offset = find_local(cg, node->as.set.var);
        if (offset == -1) {
            set_error(cg, node->line, "undefined variable in set");
            return;
        }
        if (offset < -1000) {
            // Let-bound local: use R2:R3 base
            int actual_offset = -(offset + 1000);
            emit_line(cg, "STORE R0, [R2:R3 + %d]", actual_offset);
        } else {
            // Function parameter: use R4:R5 (FP) base
            emit_line(cg, "STORE R0, [R4:R5 + %d]", offset);
        }
        break;
    }

    case AST_IF: {
        int lbl_else = new_label(cg);
        int lbl_end = new_label(cg);

        emit_expr(cg, node->as.if_expr.cond);
        emit_line(cg, "OR R0, R0");
        emit_line(cg, "JZ __L%d", lbl_else);

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
        emit_expr(cg, node->as.while_expr.cond);
        emit_line(cg, "OR R0, R0");
        emit_line(cg, "JZ __L%d", lbl_end);

        for (size_t i = 0; i < node->as.while_expr.body_count; i++) {
            emit_expr(cg, node->as.while_expr.body[i]);
        }

        emit_line(cg, "JMP __L%d", lbl_loop);
        emit(cg, "__L%d:\n", lbl_end);
        emit_line(cg, "LOADI R0, 0"); // while returns 0
        break;
    }

    case AST_DO:
        for (size_t i = 0; i < node->as.block.expr_count; i++) {
            emit_expr(cg, node->as.block.exprs[i]);
        }
        break;

    case AST_LET: {
        // Save current local count for cleanup
        size_t saved_local_count = cg->local_count;
        size_t binding_count = node->as.let.binding_count;

        // Save outer let's base pointer (R2:R3) so nested lets don't clobber it
        emit_line(cg, "PUSH R2");
        emit_line(cg, "PUSH R3");

        // Allocate stack space for all locals first (PUSH dummy values)
        for (size_t i = 0; i < binding_count; i++) {
            emit_line(cg, "PUSH R0");
        }

        // Get current SP as base for locals
        emit_line(cg, "MOVSPR R2:R3");

        // Evaluate each binding and store it, registering BEFORE evaluating
        // so that later bindings can reference earlier ones
        for (size_t i = 0; i < binding_count; i++) {
            // Offset for this local from R2:R3 base
            int offset = (int)(binding_count - i);

            // Register this local first (with uninitialized value)
            add_local(cg, node->as.let.vars[i], -1000 - offset);

            // Evaluate the binding expression (can now reference earlier let-locals)
            emit_expr(cg, node->as.let.vals[i]);

            // Store the value
            emit_line(cg, "STORE R0, [R2:R3 + %d]", offset);
        }

        // Evaluate body
        for (size_t i = 0; i < node->as.let.body_count; i++) {
            emit_expr(cg, node->as.let.body[i]);
        }

        // Deallocate locals (POP dummy values)
        for (size_t i = 0; i < binding_count; i++) {
            emit_line(cg, "POP R1");
        }

        // Restore outer let's base pointer (R2:R3)
        emit_line(cg, "POP R3");
        emit_line(cg, "POP R2");

        // Restore local scope
        cg->local_count = saved_local_count;
        break;
    }

    case AST_CALL: {
        // Save R2:R3 (let-local base) and R4:R5 (frame pointer) before call
        emit_line(cg, "PUSH R2");
        emit_line(cg, "PUSH R3");
        emit_line(cg, "PUSH R4");
        emit_line(cg, "PUSH R5");

        // Push arguments in reverse order
        for (int i = (int)node->as.call.arg_count - 1; i >= 0; i--) {
            emit_expr(cg, node->as.call.args[i]);
            emit_line(cg, "PUSH R0");
        }

        char func_name[SE_MAX_SYMBOL_LEN];
        sanitize_name(func_name, node->as.call.func, SE_MAX_SYMBOL_LEN);
        emit_line(cg, "CALL %s", func_name);

        // Pop arguments
        for (size_t i = 0; i < node->as.call.arg_count; i++) {
            emit_line(cg, "POP R1"); // discard into R1
        }

        // Restore R4:R5 and R2:R3
        emit_line(cg, "POP R5");
        emit_line(cg, "POP R4");
        emit_line(cg, "POP R3");
        emit_line(cg, "POP R2");
        break;
    }

    case AST_ASM:
        // Emit raw assembly code
        emit(cg, "    %s\n", node->as.symbol.name);
        break;

    case AST_INCLUDE:
        // Include directives should be handled at top level, not in expressions
        set_error(cg, node->line, "include cannot be used inside expressions");
        break;

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
        for (size_t i = 0; i < node->as.block.expr_count; i++) {
            emit_data_value(cg, node->as.block.exprs[i]);
        }
        break;

    case AST_REPEAT:
        emit(cg, "    TIMES %d ", node->as.repeat.count);
        // For TIMES, we need inline DB
        if (node->as.repeat.form->kind == AST_DB) {
            emit(cg, "DB ");
            for (size_t i = 0; i < node->as.repeat.form->as.block.expr_count; i++) {
                if (i > 0) emit(cg, ", ");
                AstNode* val = node->as.repeat.form->as.block.exprs[i];
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

    // Parameters are at FP + 3 + i
    // Because: FP points to SP after CALL, return address is at SP+1 and SP+2
    // Arguments are pushed in reverse order, so first arg (args[0]) is pushed last
    // and ends up at SP+3, second arg at SP+4, etc.
    for (size_t i = 0; i < node->as.defn.param_count; i++) {
        int offset = 3 + (int)i;
        add_local(cg, node->as.defn.params[i], offset);
    }

    for (size_t i = 0; i < node->as.defn.body_count; i++) {
        emit_expr(cg, node->as.defn.body[i]);
    }

    emit_line(cg, "RET");
    emit(cg, "\n");
}

static void emit_data(SeCodegen* cg, AstNode* node) {
    if (cg->has_error) return;

    char name[SE_MAX_SYMBOL_LEN];
    sanitize_name(name, node->as.data.name, SE_MAX_SYMBOL_LEN);

    // Look up the computed address from data_labels
    int32_t addr = -1;
    is_data_label(cg, node->as.data.name, &addr);

    if (addr >= 0) {
        emit(cg, "    ORG 0x%04X\n", addr);
    }
    emit(cg, "%s:\n", name);

    for (size_t i = 0; i < node->as.data.body_count; i++) {
        emit_data_value(cg, node->as.data.body[i]);
    }
    emit(cg, "\n");
}

bool se_codegen_emit(SeCodegen* cg, AstProgram* program) {
    emit(cg, "; Generated by tiny16se compiler\n");
    emit(cg, "; Source: %s\n\n", cg->filename);

    emit(cg, "; Constants\n");
    for (size_t i = 0; i < cg->constant_count; i++) {
        if (!cg->constants[i].is_function) {
            char name[SE_MAX_SYMBOL_LEN];
            sanitize_name(name, cg->constants[i].name, SE_MAX_SYMBOL_LEN);
            emit(cg, "%s = 0x%02X\n", name, cg->constants[i].value);
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

    bool has_data = false;
    for (size_t i = 0; i < program->node_count; i++) {
        if (program->nodes[i]->kind == AST_DATA) {
            if (!has_data) {
                emit(cg, "section .data\n\n");
                has_data = true;
            }
            emit_data(cg, program->nodes[i]);
        }
    }

    // Emit includes at the end (typically contain data like tilesets, fonts)
    for (size_t i = 0; i < program->node_count; i++) {
        if (program->nodes[i]->kind == AST_INCLUDE) {
            emit(cg, ".include \"%s\"\n", program->nodes[i]->as.symbol.name);
        }
    }

    return !cg->has_error;
}

bool se_codegen_has_error(SeCodegen* cg) { return cg->has_error; }

void se_codegen_print_error(SeCodegen* cg) {
    fprintf(stderr, "%s:%zu: error: %s\n", cg->filename, cg->error_line, cg->error_msg);
}
