#pragma once

#include <string>
#include <functional>

class IDaemon;

class IMessaging
{
public:
  virtual ~IMessaging() {};
  virtual void sendMessage(const std::basic_string<unsigned char>& message) = 0;
  virtual void setDaemon(IDaemon* daemon) = 0;
  virtual void start() = 0;
  virtual void stop() = 0;
};