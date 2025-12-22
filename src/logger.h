#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

class Logger
{
public:
  enum class Topic : uint8_t
  {
    NOTOPIC = 0,
    APIKEY,
    BATTERY,
    BOARD,
    BTN,
    DISP,
    HEADER,
    HTTP,
    IMAGE,
    SENS,
    SLEEP,
    STATE,
    STREAM,
    SYSTEM,
    WIFI,
  };

  static Logger& getInstance();

  // Static wrapper for cleaner syntax: Logger::log(...)
  template<typename... Args>
  static void log(Topic topic, const String& format, Args... args)
  {
    getInstance().logMessage(topic, format, args...);
  }

  // Instance method (can still be called via getInstance())
  template<typename... Args>
  void logMessage(Topic topic, const String& format, Args... args)
  {
    String message = topic != Topic::NOTOPIC ? "[" : "";
    message += getTopicString(topic);
    message += topic != Topic::NOTOPIC ? "] " : "";
    message += formatMessage(format, args...);
    Serial.print(message);
  }

private:
  Logger() = default;
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  static constexpr const char* getTopicString(Topic topic)
  {
    // Fixed-width topic strings for alignment
    return topic == Topic::NOTOPIC ? "" :
           topic == Topic::APIKEY ? "APIKEY " :
           topic == Topic::BATTERY ? "BATTERY" :
           topic == Topic::BOARD ? "BOARD  " :
           topic == Topic::DISP ? "DISPLAY" :
           topic == Topic::HEADER ? "HEADER"  :
           topic == Topic::HTTP ? "HTTP   " :
           topic == Topic::IMAGE ? "IMAGE  " :
           topic == Topic::SENS ? "SENSOR " :
           topic == Topic::SLEEP ? "SLEEP  " :
           topic == Topic::STATE ? "STATE  " :
           topic == Topic::STREAM ? "STREAM " :
           topic == Topic::WIFI ? "WIFI   " :
           topic == Topic::SYSTEM ? "SYSTEM " :
           "UKNOWN ";
  }
  
  // Base case: no arguments
  String formatMessage(const String& format) const
  {
    return format;
  }
  
  // Recursive case: process arguments one by one
  template<typename T, typename... Rest>
  String formatMessage(const String& format, T arg, Rest... rest) const
  {
    String result = replaceFirstToken(format, String(arg));
    return formatMessage(result, rest...);
  }
  
  String replaceFirstToken(const String& str, const String& value) const;
};

#endif // LOGGER_H
