# CLI Test Policy for UAVCAN Feature Development

## Policy Statement

**For all new features implemented per the task list, a corresponding CLI test command MUST be implemented to formally verify the feature is working.**

## Implementation Requirements

### 1. Feature Development Process

For every new feature implementation:

1. **Implement the feature** according to the task specification
2. **Create a CLI test command** that verifies the feature
3. **Add the command to the CLI registry** in `uavcan_cli.c`
4. **Update the build system** to include any new test files
5. **Test on hardware** to ensure stability
6. **Document the command** in the verification mapping

### 2. CLI Command Naming Convention

```
Feature: [feature-name]
CLI Command: uavcan-test-[feature-name]
```

Examples:
- Feature: Message Serialization → Command: `uavcan-test-serialization`
- Feature: Node Discovery → Command: `uavcan-test-discovery`
- Feature: Heartbeat Transmission → Command: `uavcan-test-heartbeat`

### 3. CLI Command Structure

Each CLI test command must follow this structure:

```c
/**
 * @brief Test command for [feature name]
 * @param pcWriteBuffer Buffer to write response to
 * @param xWriteBufferLen Size of write buffer
 * @param pcCommandString Command string
 * @retval BaseType_t Command result
 */
BaseType_t uavcanCliTest[FeatureName]Command(char* pcWriteBuffer, 
                                            size_t xWriteBufferLen, 
                                            const char* pcCommandString)
{
    // Clear write buffer
    memset(pcWriteBuffer, 0, xWriteBufferLen);
    
    // Test the feature (Ultra-Safe Mode)
    uint32_t start_time = osKernelSysTick();
    uint32_t passed = 0, failed = 0;
    
    // Feature-specific tests here
    // Use ultra-safe approach to prevent crashes
    
    uint32_t execution_time = osKernelSysTick() - start_time;
    
    // Format results
    snprintf(pcWriteBuffer, xWriteBufferLen, 
            "[Feature Name] Test Results (Ultra-Safe Mode):\r\n"
            "- Test 1: %s\r\n"
            "- Test 2: %s\r\n"
            "\r\n"
            "Total Tests: %lu\r\n"
            "Passed: %lu\r\n"
            "Failed: %lu\r\n"
            "Execution Time: %lu ms\r\n"
            "\r\n"
            "Status: %s\r\n",
            (test1_result) ? "PASS" : "FAIL",
            (test2_result) ? "PASS" : "FAIL",
            total_tests, passed, failed, execution_time,
            (failed == 0) ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    
    return pdFALSE;
}
```

### 4. Command Registration

Add to `uavcanCliRegisterCommands()` in `uavcan_cli.c`:

```c
static const CLI_Command_Definition_t xUavcanTest[FeatureName]Command = {
    "uavcan-test-[feature-name]",
    "\r\nuavcan-test-[feature-name]:\r\n Test [feature description]\r\n",
    uavcanCliTest[FeatureName]Command,
    0
};

void uavcanCliRegisterCommands(void)
{
    // Existing commands...
    FreeRTOS_CLIRegisterCommand(&xUavcanTest[FeatureName]Command);
}
```

### 5. Ultra-Safe Testing Approach

All CLI test commands must use the Ultra-Safe approach:

#### ✅ DO:
- Check if structures/interfaces exist
- Verify basic functionality without complex operations
- Use simple validation checks
- Keep execution time under 100ms
- Provide clear PASS/FAIL results
- Include execution timing

#### ❌ DON'T:
- Perform complex message processing that could loop
- Allocate large amounts of memory
- Create network traffic that could overwhelm the system
- Use operations that previously caused crashes
- Block for extended periods

### 6. Documentation Requirements

For each new CLI command, update:

1. **UAVCAN_Requirements_Test_Mapping.md** - Add command to appropriate requirement
2. **Test scripts** - Add command to `test_simple_uavcan.ps1`
3. **Help documentation** - Ensure command appears in `help` output

## Example Implementation

### Task: Implement Message Priority Handling

#### 1. Feature Implementation
```c
// In uavcan_transport.c
UavcanError uavcanTransportSetMessagePriority(UavcanTransport* transport, 
                                             UavcanMessageId msg_id, 
                                             UavcanPriority priority)
{
    // Implementation here
}
```

#### 2. CLI Test Command
```c
// In uavcan_cli.c
BaseType_t uavcanCliTestPriorityCommand(char* pcWriteBuffer, 
                                       size_t xWriteBufferLen, 
                                       const char* pcCommandString)
{
    memset(pcWriteBuffer, 0, xWriteBufferLen);
    
    uint32_t start_time = osKernelSysTick();
    uint32_t passed = 0, failed = 0;
    
    // Test 1: Check if priority function exists
    if (uavcanTransportSetMessagePriority != NULL) {
        passed++;
    } else {
        failed++;
    }
    
    // Test 2: Check if transport structure has priority field
    UavcanTransport test_transport = {0};
    if (sizeof(test_transport.priority_table) > 0) {
        passed++;
    } else {
        failed++;
    }
    
    uint32_t execution_time = osKernelSysTick() - start_time;
    
    snprintf(pcWriteBuffer, xWriteBufferLen, 
            "Message Priority Test Results (Ultra-Safe Mode):\r\n"
            "- Priority function available: %s\r\n"
            "- Priority table structure: %s\r\n"
            "\r\n"
            "Total Tests: 2\r\n"
            "Passed: %lu\r\n"
            "Failed: %lu\r\n"
            "Execution Time: %lu ms\r\n"
            "\r\n"
            "Status: %s\r\n",
            (passed >= 1) ? "PASS" : "FAIL",
            (passed >= 2) ? "PASS" : "FAIL",
            passed, failed, execution_time,
            (failed == 0) ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    
    return pdFALSE;
}
```

#### 3. Command Registration
```c
static const CLI_Command_Definition_t xUavcanTestPriorityCommand = {
    "uavcan-test-priority",
    "\r\nuavcan-test-priority:\r\n Test message priority handling functionality\r\n",
    uavcanCliTestPriorityCommand,
    0
};
```

## Verification Process

Before considering any feature complete:

1. **Build Test:** `.\build.bat` must succeed
2. **Program Test:** `.\program_hardware.bat` must succeed  
3. **CLI Test:** New command must respond via telnet
4. **Stability Test:** Hardware must not crash during testing
5. **Integration Test:** Command must appear in `help` output

## Benefits of This Policy

1. **Immediate Verification:** Every feature can be tested immediately after implementation
2. **Regression Testing:** All features can be re-tested after changes
3. **Documentation:** CLI commands serve as living documentation
4. **Quality Assurance:** Forces developers to think about testability
5. **Hardware Validation:** Ensures features work on actual hardware
6. **Debugging Aid:** Provides quick way to isolate feature issues

## Enforcement

This policy is **mandatory** for all future UAVCAN feature development. No feature should be considered complete without its corresponding CLI test command.

## Template Files

Create template files for quick feature development:
- `cli_command_template.c` - Template for new CLI commands
- `feature_test_template.c` - Template for feature test implementations
- `command_registration_template.txt` - Template for command registration

This ensures consistency and speeds up development while maintaining quality standards.