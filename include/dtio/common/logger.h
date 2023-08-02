#ifndef LOGGER_H
#define LOGGER_H

#include <mpi.h>
#include <spdlog/spdlog.h>
#include <sstream>

#define kFatal 0 /**< A fatal error has occurred */
#define kError 1 /**< A non-fatal error has occurred */

class Logger
{
public:
  static void
  Log (spdlog::level::level_enum logLevel, int rank,
       const std::string &message)
  {
    static std::once_flag flag;
    std::call_once (flag, [rank] {
      spdlog::set_pattern ("[%H:%M:%S %z] [Rank " + std::to_string (rank)
                           + "] [%^%L%$] [thread %t] %v");
    });
    spdlog::log (logLevel, message);
  }
  static void
  Log (spdlog::level::level_enum logLevel,
       const std::string &message)
  {
    static std::once_flag flag;
    std::call_once (flag, [] {
      spdlog::set_pattern ("[%H:%M:%S %z] [%^%L%$] [thread %t] %v");
    });
    spdlog::log (logLevel, message);
  }
};

#define DTIO_LOG_MESSAGE(logLevel, ...)                                       \
  do                                                                          \
    {                                                                         \
      int rank;                                                               \
      std::stringstream ss;                                                   \
      MPI_Comm_rank (MPI_COMM_WORLD, &rank);                                  \
      ss << __VA_ARGS__;                                                      \
      Logger::Log (logLevel, rank, ss.str ());                                \
    }                                                                         \
  while (0)

#define DTIO_LOG_MESSAGE_RANKLESS(logLevel, ...)			      \
  do                                                                          \
    {                                                                         \
      std::stringstream ss;                                                   \
      ss << __VA_ARGS__;                                                      \
      Logger::Log (logLevel, ss.str ());                                      \
    }                                                                         \
  while (0)

#if LOG_LEVEL >= 1
#define DTIO_LOG_ERROR(...) DTIO_LOG_MESSAGE (spdlog::level::err, __VA_ARGS__)
#else
#define DTIO_LOG_ERROR(...)
#endif

#if LOG_LEVEL >= 2
#define DTIO_LOG_WARN(...) DTIO_LOG_MESSAGE (spdlog::level::warn, __VA_ARGS__)
#else
#define DTIO_LOG_WARN(...)
#endif

#if LOG_LEVEL >= 3
#define DTIO_LOG_INFO(...) DTIO_LOG_MESSAGE (spdlog::level::info, __VA_ARGS__)
#else
#define DTIO_LOG_INFO(...)
#endif

#if LOG_LEVEL >= 4
#define DTIO_LOG_DEBUG(...)                                                   \
  DTIO_LOG_MESSAGE (spdlog::level::debug, __VA_ARGS__)
#define DTIO_LOG_DEBUG_RANKLESS(...)                                          \
  DTIO_LOG_MESSAGE_RANKLESS(spdlog::level::debug, __VA_ARGS__)
#else
#define DTIO_LOG_DEBUG(...)
#define DTIO_LOG_DEBUG_RANKLESS(...)
#endif

#if LOG_LEVEL >= 5
#define DTIO_LOG_TRACE(...)                                                   \
  DTIO_LOG_MESSAGE (spdlog::level::trace, __VA_ARGS__)
#else
#define DTIO_LOG_TRACE(...)
#endif

#endif // LOGGER_H
