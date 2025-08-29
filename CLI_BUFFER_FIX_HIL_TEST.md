# CLI Buffer Fix HIL Test Documentation

## Overview

This document describes the Hardware-in-the-Loop (HIL) test created to formally verify the CLI buffer truncation fix. The test ensures that the fix works correctly on actual target hardware and prevents regression.

## Problem Being Tested

**Original Issue**: CLI output was being truncated at 128 characters, preventing users from seeing complete command responses from UAVCAN test commands.

**Root Cause**: `configCOMMAND_INT_MAX_OUTPUT_SIZE` was set to 128 bytes, insufficient for UAVCAN command outputs of 300-600+ characters.

**Fix Applied**: Increased buffer size to 512 bytes and improved CLI command implementations.

## HIL Test Implementation

### Test Script: `test_cli_buffer_fix_hil.ps1`

**Purpose**: Automated verification that CLI buffer truncation fix works on actual hardware.

**Test Approach**:
1. **Build-Flash-Test Cycle**: Automatically builds firmware, flashes hardware, and tests
2. **CLI Connection**: Establishes proper CLI handshake with retry logic
3. **Command Testing**: Tests all UAVCAN CLI commands for complete output
4. **Validation**: Verifies specific fix criteria and checks for regressions

### Test Parameters

```powershell
.\test_cli_buffer_fix_hil.ps1 -ComPort "COM3" -BaudRate 115200 -TimeoutMs 5000
```

- **ComPort**: Serial port for CLI communication (default: COM3)
- **BaudRate**: Serial communication speed (default: 115200)
- **TimeoutMs**: Command response timeout (default: 5000ms)

## Test Cases

### 1. CLI Buffer Test Command
- **Command**: `uavcan-test-buffer`
- **Expected**: 500+ character output with end marker
- **Validates**: Buffer size increase and complete output transmission

### 2. Requirements Verification
- **Command**: `uavcan-verify-requirements`
- **Expected**: 400+ character structured output
- **Validates**: Long command outputs are not truncated

### 3. HIL Test Command
- **Command**: `uavcan-test`
- **Expected**: 300+ character test results
- **Validates**: Original problematic command now works

### 4. System Test Command
- **Command**: `uavcan-system-test`
- **Expected**: 300+ character system test output
- **Validates**: System-level commands work correctly

### 5. Status Command
- **Command**: `uavcan-status`
- **Expected**: 200+ character status information
- **Validates**: Status reporting is complete

### 6. Simple Verify Command
- **Command**: `uavcan-simple-verify`
- **Expected**: 150+ character verification results
- **Validates**: All command types work correctly

## Validation Criteria

For each command, the test validates:

### 1. Length Test
- **Requirement**: Output length >= expected minimum
- **Purpose**: Ensures no truncation occurs
- **Pass Criteria**: Response length meets or exceeds minimum expected length

### 2. End Marker Test
- **Requirement**: Specific end markers present (where applicable)
- **Purpose**: Confirms complete message transmission
- **Pass Criteria**: Expected end markers found in output

### 3. Status Format Test
- **Requirement**: "Status: PASS|FAIL" line present
- **Purpose**: Ensures structured output format compliance
- **Pass Criteria**: Status line matches required format

### 4. Error Format Test
- **Requirement**: "Error: [description]" line present
- **Purpose**: Ensures error reporting format compliance
- **Pass Criteria**: Error line present with correct format

### 5. No 128-byte Truncation Test
- **Requirement**: Output length > 128 characters OR not exactly 128 characters
- **Purpose**: Specifically tests the original truncation issue
- **Pass Criteria**: No evidence of 128-byte truncation

## Test Execution Process

### Automated Execution
1. **Build Firmware**: Executes `build.bat` to compile latest code
2. **Flash Hardware**: Runs `program_hardware.bat` to update target
3. **Hardware Boot**: Waits for hardware to initialize
4. **Serial Connection**: Opens and configures serial port
5. **CLI Handshake**: Establishes CLI connection with retry logic
6. **Command Testing**: Executes each test command and validates output
7. **Result Analysis**: Analyzes all test results and generates report

### Manual Verification
If automated test fails, manual verification can be performed:

1. Build and flash firmware manually
2. Connect to COM3 at 115200 baud using terminal emulator
3. Send carriage return to get '>' prompt
4. Execute each command and verify complete output
5. Look for end markers and structured format

## Expected Results

### Success Criteria
- **All Commands Pass**: Every CLI command test passes all validation criteria
- **No Truncation**: No evidence of 128-byte or other truncation
- **Complete Output**: All expected content visible in command responses
- **Structured Format**: All commands follow required output format
- **End Markers**: Buffer test command shows completion marker

### Failure Indicators
- **Truncated Output**: Command responses cut off at specific character counts
- **Missing End Markers**: Buffer test command missing completion marker
- **Format Violations**: Commands not following structured output format
- **Connection Issues**: Unable to establish CLI connection
- **Build/Flash Failures**: Firmware compilation or programming errors

## Test Results Interpretation

### Pass Result
```
CLI BUFFER FIX HIL TEST: PASS
All CLI commands output complete text without truncation
```

**Meaning**: The CLI buffer fix is working correctly on actual hardware.

### Fail Result
```
CLI BUFFER FIX HIL TEST: FAIL
One or more CLI commands failed buffer tests
```

**Meaning**: The fix is not working correctly or there's a regression.

## Regression Testing

The HIL test also serves as regression testing by:

1. **Testing All Commands**: Ensures fix doesn't break existing functionality
2. **Format Validation**: Confirms structured output format is maintained
3. **Connection Testing**: Verifies CLI handshake still works correctly
4. **Performance Testing**: Ensures no significant performance degradation

## Maintenance

### Updating the Test
When adding new CLI commands:
1. Add new test case to the script
2. Define appropriate validation criteria
3. Update expected character counts if needed
4. Test the updated script on hardware

### Test Script Location
- **File**: `test_cli_buffer_fix_hil.ps1`
- **Purpose**: CLI buffer fix verification
- **Dependencies**: `build.bat`, `program_hardware.bat`, target hardware
- **Requirements**: PowerShell, .NET Serial Port support

## Integration with Development Workflow

This HIL test integrates with the development workflow as follows:

1. **After Fix Implementation**: Run test to verify fix works
2. **Before Code Commits**: Ensure test passes to prevent regressions
3. **CI/CD Integration**: Can be integrated into automated build pipelines
4. **Release Verification**: Run as part of release testing process

## Conclusion

The CLI Buffer Fix HIL Test provides comprehensive verification that the CLI truncation issue has been resolved and continues to work correctly on actual hardware. It serves as both fix verification and regression testing, ensuring the reliability of the CLI interface for UAVCAN testing and diagnostics.