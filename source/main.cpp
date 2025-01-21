#include "application.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

int main()
{
    spdlog::set_default_logger(spdlog::stderr_color_st("logger"));
    spdlog::set_pattern("[%H:%M:%S %z] [%^-%L-%$] %v");
    spdlog::set_level(spdlog::level::info);

    auto application = Application{ 8080 };
    application.run();
}
