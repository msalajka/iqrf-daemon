#include "MessageQueue.h"
#include "IqrfLogging.h"

MessageQueue::MessageQueue(ProcessMessageFunc processMessageFunc)
  :m_processMessageFunc(processMessageFunc)
{
  m_messagePushed = false;
  m_runWorkerThread = true;
  m_workerThread = std::thread(&MessageQueue::worker, this);
}

MessageQueue::~MessageQueue()
{
  {
    m_runWorkerThread = false;
    std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
    m_messagePushed = true;
    m_conditionVariable.notify_one();
  }

  if (m_workerThread.joinable())
    m_workerThread.join();
  TRC_DBG("thread stopped");
}

void MessageQueue::pushToQueue(const std::basic_string<unsigned char>& message)
{
  {
    std::lock_guard<std::mutex> lck(m_messageQueueMutex);
    m_messageQueue.push(message);
  }
  {
    std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
    m_messagePushed = true;
    m_conditionVariable.notify_one();
  }
}

void MessageQueue::worker()
{
  TRC_ENTER("thread starts");
  try {
    while (m_runWorkerThread) {

      { //wait for something in the queue
        std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
        m_conditionVariable.wait(lck, [&]{ return m_messagePushed; });
        m_messagePushed = false;
      }

      while (true) {
        std::lock_guard<std::mutex> lck(m_messageQueueMutex);
        if (!m_messageQueue.empty()) {
          m_processMessageFunc(m_messageQueue.front());
          m_messageQueue.pop();
        }
        else {
          break;
        }
      }
    }
  }
  catch (std::exception& e) {
    CATCH_EX("Worker thread error", std::exception, e);
    m_runWorkerThread = false;
  }
  TRC_ENTER("thread stopped");
}