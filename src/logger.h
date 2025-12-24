#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <type_traits>

class Logger
{
public:
  enum class Level : uint8_t
  {
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR
  };

// Set minimum log level - messages below this level won't be compiled in
#ifndef LOG_LEVEL_MINIMUM
  static constexpr Level LOG_LEVEL_MINIMUM = Level::DEBUG; // Default: show all levels
#endif

  enum class Topic : uint8_t
  {
    APIKEY = 0,
    BATTERY,
    BOARD,
    BTN,
    DISP,
    HEADER,
    HTTP,
    IMAGE,
    SENS,
    STREAM,
    SYSTEM,
    WIFI
  };

  static Logger &getInstance();

  // Primary template: Level and Topic both specified
  template <Level L, Topic T, typename... Args>
  static typename std::enable_if<(L >= LOG_LEVEL_MINIMUM), void>::type log(const String &format, Args... args)
  {
    getInstance().logMessageImpl<L, T>(format, args...);
  }

  // Overload for filtered-out log levels
  template <Level L, Topic T, typename... Args>
  static typename std::enable_if<(L < LOG_LEVEL_MINIMUM), void>::type log(const String &format, Args... args)
  {
    // Empty - optimized away
  }

  // Convenience wrapper: Only Topic specified, Level defaults to INFO
  template <Topic T, typename... Args>
  static void log(const String &format, Args... args)
  {
    log<Level::INFO, T>(format, args...);
  }

private:
  Logger() = default;
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  // Compile-time optimized version - topic and level known at compile time
  template <Level L, Topic T, typename... Args>
  void logMessageImpl(const String &format, Args... args)
  {
    // All string concatenation can be optimized away by compiler
    constexpr const char *levelStr = getLevelString(L);
    constexpr const char *topicStr = getTopicString(T);

    String message = "[";
    message += levelStr;
    message += "][";

    message += topicStr;
    message += "] ";

    message += formatMessage(format, args...);
    Serial.print(message);
  }

  static constexpr const char *getLevelString(Level level)
  {
    // Fixed-width level strings for alignment
    return level == Level::DEBUG     ? "DEBUG  "
           : level == Level::INFO    ? "INFO   "
           : level == Level::WARNING ? "WARNING"
           : level == Level::ERROR   ? "ERROR  "
                                     : "";
  }

  static constexpr const char *getTopicString(Topic topic)
  {
    // Fixed-width topic strings for alignment
    return topic == Topic::APIKEY    ? "APIKEY "
           : topic == Topic::BATTERY ? "BATTERY"
           : topic == Topic::BOARD   ? "BOARD  "
           : topic == Topic::DISP    ? "DISPLAY"
           : topic == Topic::HEADER  ? "HEADER "
           : topic == Topic::HTTP    ? "HTTP   "
           : topic == Topic::IMAGE   ? "IMAGE  "
           : topic == Topic::SENS    ? "SENSOR "
           : topic == Topic::STREAM  ? "STREAM "
           : topic == Topic::SYSTEM  ? "SYSTEM "
           : topic == Topic::WIFI    ? "WIFI   "
                                     : "";
  }

  // Base case: no arguments
  String formatMessage(const String &format) const { return format; }

  // Recursive case: process arguments one by one
  template <typename T, typename... Rest>
  String formatMessage(const String &format, T arg, Rest... rest) const
  {
    String result = replaceFirstToken(format, String(arg));
    return formatMessage(result, rest...);
  }

  String replaceFirstToken(const String &str, const String &value) const;
};

#endif // LOGGER_H
