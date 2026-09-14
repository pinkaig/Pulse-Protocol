///******************************************************************************/
///**
// * @file            Log.cpp
// * @project         Pulse Protocol
// * @author          Reginald Lew Yee Ren
// * @brief           Capturing of crash logs into txt through SPDLOG
// * @copyright       Copyright (C) 2026 DigiPen Institute of Technology.
//Reproduction or disclosure of this file or its contents without the
//prior written consent of DigiPen Institute of Technology is prohibited.
// */
///******************************************************************************/
//
//#include "Log.h"
//
//namespace GAM200graphics {
//    std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
//    std::shared_ptr<spdlog::logger> Log::s_ClientLogger;
//
//    void Log::Init()
//    {
//        try {
//            spdlog::set_pattern("%^[%T] %n: %v%$");
//
//            // Create sinks: console + file
//            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
//            //auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/FullLog.txt", true);
//            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("Logs/Engine.log", 1024 * 1024 * 5, 3); // 5 MB max per file, keep 3 backups
//            std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };
//
//            // Core logger (engine logs)
//            s_CoreLogger = std::make_shared<spdlog::logger>("GAM200graphics", sinks.begin(), sinks.end());
//            s_CoreLogger->set_level(spdlog::level::trace);
//            spdlog::register_logger(s_CoreLogger);
//
//            // Client logger (game logs)
//            s_ClientLogger = std::make_shared<spdlog::logger>("Game", sinks.begin(), sinks.end());
//            s_ClientLogger->set_level(spdlog::level::trace);
//            spdlog::register_logger(s_ClientLogger);
//
//            // Optional: separate crash/error-only file logger
//            //auto crash_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/CrashLog.txt", true);
//            auto crash_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("Logs/CrashLog.log", 1024 * 1024 * 5, 3); // 5 MB max per file, keep 3 backups
//            auto file_logger = std::make_shared<spdlog::logger>("FileLogger", crash_sink);
//            file_logger->set_level(spdlog::level::trace);
//            file_logger->flush_on(spdlog::level::err);
//            spdlog::register_logger(file_logger);
//        }
//        catch (const spdlog::spdlog_ex& ex) {
//            printf("Log initialization failed: %s\n", ex.what());
//        }
//    }
//
//    void Log::CriticalAndAbort(const std::string& message)
//    {
//        // Log to console
//        if (s_CoreLogger) s_CoreLogger->critical(message);
//        if (s_ClientLogger) s_ClientLogger->critical(message);
//
//        // Log to file
//        auto file_logger = spdlog::get("FileLogger");
//        if (file_logger) {
//            file_logger->critical("CRASH LOG: {}", message);
//            file_logger->flush(); // ensure it writes immediately
//        }
//
//        // Immediately terminate program
//        std::abort();
//    }
//
//}