add_library(SPDLOG
    ${TP_DIR}/spdlog/src/async.cpp
    ${TP_DIR}/spdlog/src/bundled_fmtlib_format.cpp
    ${TP_DIR}/spdlog/src/cfg.cpp
    ${TP_DIR}/spdlog/src/color_sinks.cpp
    ${TP_DIR}/spdlog/src/file_sinks.cpp
    ${TP_DIR}/spdlog/src/spdlog.cpp
    ${TP_DIR}/spdlog/src/stdout_sinks.cpp
)

target_include_directories(SPDLOG PRIVATE
    ${TP_DIR}/spdlog/include/
)

target_compile_definitions(SPDLOG PRIVATE
    SPDLOG_COMPILED_LIB
)