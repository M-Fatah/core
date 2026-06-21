if(NOT INPUT_APK OR NOT EXISTS "${INPUT_APK}")
    message(FATAL_ERROR "INPUT_APK does not exist: ${INPUT_APK}")
endif()

if(NOT NATIVE_LIBRARY OR NOT EXISTS "${NATIVE_LIBRARY}")
    message(FATAL_ERROR "NATIVE_LIBRARY does not exist: ${NATIVE_LIBRARY}")
endif()

if(NOT ANDROID_ABI)
    message(FATAL_ERROR "ANDROID_ABI is required.")
endif()

if(NOT WORK_DIR)
    message(FATAL_ERROR "WORK_DIR is required.")
endif()

if(NOT OUTPUT_APK)
    message(FATAL_ERROR "OUTPUT_APK is required.")
endif()

file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")
file(ARCHIVE_EXTRACT INPUT "${INPUT_APK}" DESTINATION "${WORK_DIR}")

get_filename_component(_core_native_library_name "${NATIVE_LIBRARY}" NAME)
file(MAKE_DIRECTORY "${WORK_DIR}/lib/${ANDROID_ABI}")
file(COPY_FILE "${NATIVE_LIBRARY}" "${WORK_DIR}/lib/${ANDROID_ABI}/${_core_native_library_name}" ONLY_IF_DIFFERENT)

file(REMOVE "${OUTPUT_APK}")
file(GLOB _core_apk_entries RELATIVE "${WORK_DIR}" "${WORK_DIR}/*")
file(ARCHIVE_CREATE OUTPUT "${OUTPUT_APK}" FORMAT zip WORKING_DIRECTORY "${WORK_DIR}" PATHS ${_core_apk_entries})