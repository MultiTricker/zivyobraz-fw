#include "logger.h"

Logger &Logger::getInstance()
{
  static Logger instance;
  return instance;
}

String Logger::replaceFirstToken(const String &str, const String &value) const
{
  // Find first occurrence of {}
  int pos = str.indexOf("{}");
  if (pos >= 0)
  {
    String result = str.substring(0, pos);
    result += value;
    result += str.substring(pos + 2);
    return result;
  }
  return str;
}
