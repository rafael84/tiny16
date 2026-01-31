# tiny16 Tests

This directory contains automated tests for the tiny16 project.

## Test Suites

### VM Tests (`vm_test.c`)
Tests for the virtual machine core functionality:
- CPU instruction execution
- Memory management
- Stack operations
- Jump and branch instructions

### ASM Tests (`asm_test.c`)
Tests for the assembler:
- Lexer tokenization
- Parser (labels, sections, directives, instructions)
- Expressions and constants
- Macros and preprocessor
- Data directives

### SEC Tests (`sec_test.c`)
Tests for the S-expression compiler (tiny16-se):
- **Lexer tests**: S-expression tokenization (numbers, strings, symbols, parentheses)
- **Parser tests**: AST generation for all language constructs (def, defn, let, if, while, primitives)
- **Codegen tests**: Assembly generation from AST
- **Integration tests**: End-to-end compilation of complete programs

## Running Tests

Run all tests:
```bash
make tests
```

Run individual test suites:
```bash
make tests-vm    # Virtual machine tests
make tests-asm   # Assembler tests
make tests-sec   # S-expression compiler tests
```

## Test Fixtures

The `fixtures/` directory contains sample files for testing:
- `*.asm` - Assembly test programs
- `*.se` - S-expression test programs
- `*.inc` - Include files for testing

## Writing New Tests

Follow the existing test patterns:

```c
void test_feature_name(void) {
    // Arrange: Set up test data
    const char* input = "...";
    
    // Act: Execute the code being tested
    // ...
    
    // Assert: Verify the results
    assert(condition);
}
```

Add new tests to the main function and use the `SEC_TEST()` or `ASM_TEST()` macro.

## Test Output

Tests use a simple format with checkmarks for passed tests:
```
▸ test_name  ✓
```

Failed tests will show assertion failures with file and line number.
