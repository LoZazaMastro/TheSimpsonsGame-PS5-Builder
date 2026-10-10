#pragma once
#include <fmt/format.h>
#include <string>
namespace framegen_test {
void CaptureLog(std::string message);
}
#define REXLOG_INFO(...) framegen_test::CaptureLog(fmt::format(__VA_ARGS__))
#define REXLOG_WARN(...) framegen_test::CaptureLog(fmt::format(__VA_ARGS__))
