cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C ASM)
# STM32CubeMX generated symbols (macros)
set(MX_Defines_Syms 
	USE_PWR_LDO_SUPPLY 
	UX_INCLUDE_USER_DEFINE_FILE 
	USE_HAL_DRIVER 
	STM32H743xx 
	STM32_THREAD_SAFE_STRATEGY=2
    $<$<CONFIG:Debug>:DEBUG>
)

# STM32CubeMX generated include paths
set(MX_Include_Dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../USBX/App
    ${CMAKE_CURRENT_SOURCE_DIR}/../../USBX/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/BSP/STM32H7xx_Nucleo
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/inc/
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/ports/generic/inc/
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_device_classes/inc/
)

# STM32CubeMX generated application sources
set(MX_Application_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../USBX/App/ux_device_descriptors.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../USBX/App/ux_device_cdc_acm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../USBX/App/app_usbx_device.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../USBX/App/app_usbx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32h7xx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32h7xx_hal_msp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32h7xx_hal_timebase_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/sysmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/w25q128.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../startup_stm32h743xx.s
)

# STM32 HAL/LL Drivers
set(STM32_Drivers_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/system_stm32h7xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_qspi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_delayblock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usb.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_usart_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/BSP/STM32H7xx_Nucleo/stm32h7xx_nucleo.c
)

# Drivers Midllewares


set(USBX_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_activate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_control_request.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_deactivate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_entry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_initialize.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_ioctl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_read_run.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_tasks_run.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_unitialize.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_write_run.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_write_with_callback.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_callback.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_endpoint_create.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_endpoint_destroy.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_endpoint_reset.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_endpoint_stall.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_endpoint_status.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_frame_number_get.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_function.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_initialize.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_initialize_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_interrupt_handler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_transfer_abort.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_transfer_run.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_uninitialize.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_debug_callback_register.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_debug_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_delay_ms.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_descriptor_pack.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_descriptor_parse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_error_callback_register.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_long_get.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_long_get_big_endian.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_long_put.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_long_put_big_endian.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_allocate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_allocate_add_safe.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_allocate_mulc_safe.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_allocate_mulv_safe.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_compare.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_copy.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_free.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_byte_pool_create.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_byte_pool_search.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_set.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_pci_class_scan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_pci_read.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_pci_write.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_physical_address.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_set_interrupt_handler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_short_get.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_short_get_big_endian.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_short_put.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_short_put_big_endian.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_string_length_check.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_string_length_get.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_string_to_unicode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_unicode_to_string.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_utility_virtual_address.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_system_error_handler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_system_initialize.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_system_tasks_run.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_system_uninitialize.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_alternate_setting_get.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_alternate_setting_set.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_class_register.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_class_unregister.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_clear_feature.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_configuration_get.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_configuration_set.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_control_request_process.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_descriptor_send.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_disconnect.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_endpoint_stall.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_get_status.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_host_wakeup.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_initialize.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_interface_delete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_interface_get.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_interface_set.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_interface_start.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_microsoft_extension_register.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_set_feature.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_tasks_run.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_transfer_abort.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_transfer_all_request_abort.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_transfer_request.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_transfer_run.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/usbx/common/core/src/ux_device_stack_uninitialize.c
)

# Link directories setup
set(MX_LINK_DIRS

)
# Project static libraries
set(MX_LINK_LIBS 
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
    USBX
	
)
# Interface library for includes and symbols
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})

# Create STM32_Drivers static library
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)


# Create USBX static library
add_library(USBX OBJECT)
target_sources(USBX PRIVATE ${USBX_Src})
target_link_libraries(USBX PUBLIC stm32cubemx)

# Add STM32CubeMX generated application sources to the project
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${MX_Application_Src})

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${MX_LINK_DIRS})

# Add libraries to the project
target_link_libraries(${CMAKE_PROJECT_NAME} ${MX_LINK_LIBS})

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
