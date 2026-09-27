if(NOT DEFINED TINYUSB_SOURCE_DIR)
    message(FATAL_ERROR "TINYUSB_SOURCE_DIR is required")
endif()

if(NOT DEFINED TINYUSB_PATCH_DIR)
    message(FATAL_ERROR "TINYUSB_PATCH_DIR is required")
endif()

find_package(Git REQUIRED)

set(TINYUSB_PATCHES
    "${TINYUSB_PATCH_DIR}/0001-usbd-event-queue-recovery.patch"
)

foreach(PATCH_FILE IN LISTS TINYUSB_PATCHES)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" apply --check "${PATCH_FILE}"
        WORKING_DIRECTORY "${TINYUSB_SOURCE_DIR}"
        RESULT_VARIABLE APPLY_CHECK_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )

    if(APPLY_CHECK_RESULT EQUAL 0)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" apply "${PATCH_FILE}"
            WORKING_DIRECTORY "${TINYUSB_SOURCE_DIR}"
            RESULT_VARIABLE APPLY_RESULT
        )
        if(NOT APPLY_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to apply TinyUSB patch: ${PATCH_FILE}")
        endif()
    else()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${PATCH_FILE}"
            WORKING_DIRECTORY "${TINYUSB_SOURCE_DIR}"
            RESULT_VARIABLE REVERSE_CHECK_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if(NOT REVERSE_CHECK_RESULT EQUAL 0)
            message(FATAL_ERROR "TinyUSB patch cannot be applied and is not already present: ${PATCH_FILE}")
        endif()
    endif()
endforeach()
