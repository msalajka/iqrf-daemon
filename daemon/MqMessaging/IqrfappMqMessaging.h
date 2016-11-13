#pragma once

#include "TaskQueue.h"
#include "IMessaging.h"
#include <string>

class IDaemon;
class MqChannel;

typedef std::basic_string<unsigned char> ustring;

class IqrfappMqMessaging : public IMessaging
{
public:
  IqrfappMqMessaging();
  virtual ~IqrfappMqMessaging();

  virtual void setDaemon(IDaemon* daemon);
  virtual void start();
  virtual void stop();

private:
  void sendMessageToMq(const std::string& message);
  int handleMessageFromMq(const ustring& mqMessage);

  IDaemon* m_daemon;
  MqChannel* m_mqChannel;
  TaskQueue<ustring>* m_toMqMessageQueue;

  std::string m_localMqName;
  std::string m_remoteMqName;

};