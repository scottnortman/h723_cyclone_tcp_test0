# FreeRTOS Stack Overflow Debugging Guide

## Overview

This guide explains how to use the newly implemented FreeRTOS stack monitoring CLI tools to debug stack overflow issues that cause hardware freezes.

## Problem Description

During HIL testing, the hardware froze, suspected to be caused by a stack overflow. Stack overflows in embedded systems can cause:
- System crashes and freezes
- Unpredictable behavior
- Memory corruption
- Hard faults

## Stack Monitoring Tools Implemented

### 1. Configuration Changes

**FreeRTOS Configuration Updates:**
- `configCHECK_FOR_STACK_OVERFLOW = 2` - Enable stack overflow detection (Method 2)
- `configGENERATE_RUN_TIME_STATS = 1` - Enable runtime statistics
- `configINCLUDE_QUERY_HEAP_COMMAND = 1` - Enable heap monitoring

### 2. CLI Commands Available

| Command | Purpose | Usage |
|---------|---------|-------|
| `task-stats` | Show all task information | Basic FreeRTOS task list |
| `stack-info` | Detailed stack usage for all tasks | Shows usage percentages and warnings |
| `stack-check` | Check for stack overflow conditions | Identifies critical and warning tasks |
| `stack-watch <task>` | Monitor specific task stack | Detailed info for one task |
| `heap-info` | Heap usage statistics | Memory allocation information |
| `memory-info` | Comprehensive memory report | Complete system memory overview |
| `stack-overflow-info` | Stack overflow detection history | Shows if overflows occurred |

### 3. Stack Overflow Detection

**Automatic Detection:**
- FreeRTOS hook function `vApplicationStackOverflowHook()` 
- Triggers when stack overflow is detected
- System halts with LED indication
- Overflow information is logged

**Detection Levels:**
- **OK**: < 75% stack usage
- **WARNING**: 75-90% stack usage  
- **CRITICAL**: > 90% stack usage

## Debugging Workflow

### Step 1: Build and Program with Stack Monitoring

```bash
# Build with new stack monitoring features
./build.bat

# Program the hardware
./program_hardware.bat
```

### Step 2: Run Stack Monitoring Tests

```bash
# Test all stack monitoring commands
./test_stack_monitoring.ps1

# Or use serial connection
./test_stack_monitoring.ps1 -UseSerial
```

### Step 3: Manual CLI Analysis

Connect to the CLI and run diagnostic commands:

```bash
# Connect via telnet
telnet 192.168.0.20 23

# Or via serial
# Use terminal program on COM3, 115200 baud

# Check overall stack health
> stack-check

# Get detailed stack information
> stack-info

# Check for previous overflows
> stack-overflow-info

# Monitor specific problematic task
> stack-watch TaskName
```

### Step 4: Interpret Results

#### Critical Stack Usage Example:
```
> stack-check
Stack Overflow Check Results:
=============================
CRITICAL: UavcanTask - 95% stack usage (1900/2000 bytes)
WARNING: TelnetTask - 80% stack usage (1600/2000 bytes)

Summary: 1 critical, 1 warning tasks found.
Recommendation: Increase stack size for critical tasks.
```

#### Stack Overflow Detection Example:
```
> stack-overflow-info
Stack Overflow Detection Report:
================================
Total Overflows Detected:   1
Last Overflow Task:         UavcanTask
Last Overflow Task Number:  3
Detection Method:           FreeRTOS Hook (Method 2)
Stack Check Enabled:        Yes (Method 2)

Status: OVERFLOW DETECTED - SYSTEM WAS RESET
```

### Step 5: Fix Stack Issues

#### Increase Stack Size

**In `freertos.c` or task creation code:**
```c
// Before (insufficient stack)
xTaskCreate(vUavcanTask, "UavcanTask", 512, NULL, 2, &xUavcanTaskHandle);

// After (increased stack)
xTaskCreate(vUavcanTask, "UavcanTask", 1024, NULL, 2, &xUavcanTaskHandle);
```

#### Optimize Task Code

**Reduce stack usage by:**
- Minimizing local variables
- Avoiding large arrays on stack
- Using dynamic allocation for large buffers
- Reducing function call depth

#### Monitor After Changes

```bash
# After making changes, rebuild and test
./build.bat
./program_hardware.bat
./test_stack_monitoring.ps1
```

## Common Stack Overflow Causes

### 1. Large Local Variables
```c
// BAD: Large array on stack
void badFunction(void) {
    char largeBuffer[2048];  // Uses 2KB of stack!
    // ... use buffer
}

// GOOD: Dynamic allocation or static
void goodFunction(void) {
    char *buffer = pvPortMalloc(2048);
    // ... use buffer
    vPortFree(buffer);
}
```

### 2. Deep Function Calls
```c
// BAD: Deep recursion or call chains
void deepFunction(int level) {
    char localData[100];
    if (level > 0) {
        deepFunction(level - 1);  // Each call uses more stack
    }
}

// GOOD: Iterative approach or limited depth
void iterativeFunction(int count) {
    for (int i = 0; i < count; i++) {
        // Process without recursion
    }
}
```

### 3. Insufficient Stack Allocation
```c
// BAD: Too small stack for task requirements
xTaskCreate(complexTask, "Complex", 256, NULL, 2, NULL);  // Only 256 words

// GOOD: Adequate stack size
xTaskCreate(complexTask, "Complex", 1024, NULL, 2, NULL);  // 1024 words
```

## Monitoring Best Practices

### 1. Regular Monitoring
- Run `stack-check` periodically during development
- Monitor stack usage after adding new features
- Check stack usage under different load conditions

### 2. Safety Margins
- Keep stack usage below 75% in production
- Add 25-50% safety margin to calculated requirements
- Monitor worst-case scenarios

### 3. Automated Testing
- Include stack monitoring in automated test scripts
- Set up alerts for high stack usage
- Test under stress conditions

## Emergency Debugging

### If System Freezes During Testing:

1. **Reset the hardware**
2. **Connect immediately after boot**
3. **Run stack-overflow-info to check history**
4. **Identify the problematic task**
5. **Increase stack size for that task**
6. **Rebuild and test**

### LED Indicators:

- **Red LED solid**: Stack overflow detected (system halted)
- **Red LED flashing**: Memory allocation failure

## Example Debugging Session

```bash
# 1. Connect to CLI
telnet 192.168.0.20 23

# 2. Check for previous overflows
> stack-overflow-info
Total Overflows Detected: 1
Last Overflow Task: UavcanTask

# 3. Check current stack usage
> stack-check
CRITICAL: UavcanTask - 95% stack usage

# 4. Get detailed info on problematic task
> stack-watch UavcanTask
Stack Size: 2048 bytes
Used: 1946 bytes
Usage: 95%
Status: CRITICAL

# 5. Fix: Increase UavcanTask stack size in code
# 6. Rebuild and verify
> stack-check
All tasks have healthy stack usage levels.
```

## Integration with Existing Tests

The stack monitoring commands are integrated with existing test scripts:

```bash
# Enhanced test script includes stack monitoring
./test_simple_uavcan.ps1

# Dedicated stack monitoring test
./test_stack_monitoring.ps1

# All tests now include memory health checks
./test_all_uavcan_requirements.ps1
```

## Conclusion

These stack monitoring tools provide comprehensive visibility into FreeRTOS memory usage and stack overflow conditions. Use them to:

1. **Identify** tasks with high stack usage
2. **Prevent** stack overflows before they occur  
3. **Debug** system freezes and crashes
4. **Optimize** memory usage for better system stability

The tools follow the established CLI testing patterns with ultra-safe mode operation and structured output for automated testing integration.