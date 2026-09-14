///******************************************************************************/
///**
// * @file        Log.h
// * @project     Pulse Protocol
// * @author      Reginald Lew Yee Ren
// * @brief		Provides initialization and access to spdlog-based core, client,
//                and crash loggers for engine and game debugging.
// * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
// *              Reproduction or disclosure of this file or its contents without the
// *              prior written consent of DigiPen Institute of Technology is prohibited.
// */
///******************************************************************************/
//
//#pragma once
//#include "pch/pch.h"
//#include <spdlog/sinks/stdout_color_sinks.h>
//#include <spdlog/sinks/basic_file_sink.h>
//
//namespace GAM200graphics {
//    class Log
//    {
//    public:
//        static void Init();
//
//        inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
//        inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
//
//        // Optional: helper to handle fatal/critical logs
//        static void CriticalAndAbort(const std::string& message);
//    private:
//        static std::shared_ptr<spdlog::logger> s_CoreLogger;
//        static std::shared_ptr<spdlog::logger> s_ClientLogger;
//    };
//
//}
//
//// core log macros
//#define GAM200_CORE_TRACE(...)     ::GAM200graphics::Log::GetCoreLogger()->trace(__VA_ARGS__)
//#define GAM200_CORE_INFO(...)      ::GAM200graphics::Log::GetCoreLogger()->info(__VA_ARGS__)
//#define GAM200_CORE_WARN(...)      ::GAM200graphics::Log::GetCoreLogger()->warn(__VA_ARGS__)
//#define GAM200_CORE_ERROR(...)     ::GAM200graphics::Log::GetCoreLogger()->error(__VA_ARGS__)
//#define GAM200_CORE_CRITICAL(...)  ::GAM200graphics::Log::CriticalAndAbort(fmt::format(__VA_ARGS__))
//
//// client log macros
//#define GAM200_TRACE(...)          ::GAM200graphics::Log::GetClientLogger()->trace(__VA_ARGS__)
//#define GAM200_INFO(...)           ::GAM200graphics::Log::GetClientLogger()->info(__VA_ARGS__)
//#define GAM200_WARN(...)           ::GAM200graphics::Log::GetClientLogger()->warn(__VA_ARGS__)
//#define GAM200_ERROR(...)          ::GAM200graphics::Log::GetClientLogger()->error(__VA_ARGS__)
//#define GAM200_CRITICAL(...)       ::GAM200graphics::Log::CriticalAndAbort(fmt::format(__VA_ARGS__))