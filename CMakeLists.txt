cmake_minimum_required(VERSION 3.20)

project("h723_cyclone_tcp_test0" C CXX ASM)

include(cmake/st-project.cmake)

add_executable(${PROJECT_NAME})
add_st_target_properties(${PROJECT_NAME})

# Add UAVCAN source files
target_sources(${PROJECT_NAME} PRIVATE
    "Core/Src/uavcan/uavcan_transport.c"
    "Core/Src/uavcan/uavcan_config.c"
    "Core/Src/uavcan/uavcan_node.c"
    "Core/Src/uavcan/uavcan_test.c"
    "Core/Src/uavcan/uavcan_requirements_test.c"
    "Core/Src/uavcan/uavcan_simple_verify.c"
    "Core/Src/uavcan/uavcan_cli.c"
    "Core/Src/uavcan/uavcan.c"
)

# Add stack monitoring and FreeRTOS hooks
target_sources(${PROJECT_NAME} PRIVATE
    "Core/Src/stack_monitor_cli.c"
    "Core/Src/freertos_hooks.c"
)