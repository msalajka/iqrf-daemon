#include "UdpMessaging.h"
//#include "IqrfCdcChannel.h"
//#include "IqrfSpiChannel.h"
#include "UdpMessage.h"
#include "helpers.h"
#include "MessageQueue.h"
#include "IqrfLogging.h"
#include "PlatformDep.h"
#include <climits>
#include <ctime>
#include <ratio>
#include <chrono>

int UdpMessaging::handleMessageFromUdp(const ustring& udpMessage)
{
  TRC_DBG("Received from UDP: " << std::endl << FORM_HEX(udpMessage.data(), udpMessage.size()));

  size_t msgSize = udpMessage.size();
  std::basic_string<unsigned char> message;

  try {
    decodeMessageUdp(udpMessage, message);
  }
  catch (UdpChannelException& e) {
    TRC_DBG("Wrong message: " << e.what());
    return -1;
  }

  switch (udpMessage[cmd])
  {
  case IQRF_UDP_GET_GW_INFO:          // --- Returns GW identification ---
  {
    ustring udpResponse(udpMessage);
    udpResponse[cmd] = udpResponse[cmd] | 0x80;
    ustring msg;
    getGwIdent(msg);
    encodeMessageUdp(udpResponse, msg);
    m_toUdpMessageQueue->pushToQueue(udpResponse);
  }
  return 0;

  case IQRF_UDP_GET_GW_STATUS:          // --- Returns GW status ---
  {
    ustring udpResponse(udpMessage);
    udpResponse[cmd] = udpResponse[cmd] | 0x80;
    ustring msg;
    getGwStatus(msg);
    encodeMessageUdp(udpResponse, msg);
    m_toUdpMessageQueue->pushToQueue(udpResponse);
  }
  return 0;

  case IQRF_UDP_WRITE_IQRF:       // --- Writes data to the TR module ---
  {
    m_toIqrfMessageQueue->pushToQueue(message);

    //send response
    ustring udpResponse(udpMessage.substr(0, IQRF_UDP_HEADER_SIZE));
    udpResponse[cmd] = udpResponse[cmd] | 0x80;
    udpResponse[subcmd] = (unsigned char)IQRF_UDP_ACK;
    encodeMessageUdp(udpResponse);
    //TODO it is required to send back via subcmd write result - implement sync write with appropriate ret code
    m_toUdpMessageQueue->pushToQueue(udpResponse);
  }
  return 0;

  default:
    //not implemented command
    std::basic_string<unsigned char> udpResponse(udpMessage);
    udpResponse[cmd] = udpResponse[cmd] | 0x80;
    udpResponse[subcmd] = (unsigned char)IQRF_UDP_NAK;
    encodeMessageUdp(udpResponse);
    m_toUdpMessageQueue->pushToQueue(udpResponse);
    break;
  }
  return -1;
}

int UdpMessaging::handleMessageFromIqrf(const ustring& iqrfMessage)
{
  TRC_DBG("Received from IQRF: " << std::endl << FORM_HEX(iqrfMessage.data(), iqrfMessage.size()));

  std::basic_string<unsigned char> udpMessage(IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE, '\0');
  std::basic_string<unsigned char> message(iqrfMessage);
  udpMessage[cmd] = (unsigned char)IQRF_UDP_IQRF_SPI_DATA;
  encodeMessageUdp(udpMessage, message);
  m_toUdpMessageQueue->pushToQueue(udpMessage);
  return 0;
}

void UdpMessaging::encodeMessageUdp(ustring& udpMessage, const ustring& message)
{
  unsigned short dlen = (unsigned short)message.size();

  udpMessage.resize(IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE, '\0');
  udpMessage[gwAddr] = IQRF_UDP_GW_ADR;
  udpMessage[dlen_H] = (unsigned char)((dlen >> 8) & 0xFF);
  udpMessage[dlen_L] = (unsigned char)(dlen & 0xFF);

  if (0 < dlen)
    udpMessage.insert(IQRF_UDP_HEADER_SIZE, message);

  uint16_t crc = GetCRC_CCITT((unsigned char*)udpMessage.data(), dlen + IQRF_UDP_HEADER_SIZE);
  udpMessage[dlen + IQRF_UDP_HEADER_SIZE] = (unsigned char)((crc >> 8) & 0xFF);
  udpMessage[dlen + IQRF_UDP_HEADER_SIZE + 1] = (unsigned char)(crc & 0xFF);
}

void UdpMessaging::decodeMessageUdp(const ustring& udpMessage, ustring& message)
{
  unsigned short dlen = 0;

  // Min. packet length check
  if (udpMessage.size() < IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE)
    THROW_EX(UdpChannelException, "Message is too short: " << FORM_HEX(udpMessage.data(), udpMessage.size()));

  // GW_ADR check
  if (udpMessage[gwAddr] != IQRF_UDP_GW_ADR)
    THROW_EX(UdpChannelException, "Message is has wrong GW_ADDR: " << PAR_HEX(udpMessage[gwAddr]));

  //iqrf data length
  dlen = (udpMessage[dlen_H] << 8) + udpMessage[dlen_L];

  // Max. packet length check
  if ((dlen + IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE) > IQRF_UDP_BUFFER_SIZE)
    THROW_EX(UdpChannelException, "Message is too long: " << PAR(dlen));

  // CRC check
  unsigned short crc = (udpMessage[IQRF_UDP_HEADER_SIZE + dlen] << 8) + udpMessage[IQRF_UDP_HEADER_SIZE + dlen + 1];
  if (crc != GetCRC_CCITT((unsigned char*)udpMessage.data(), dlen + IQRF_UDP_HEADER_SIZE))
    THROW_EX(UdpChannelException, "Message has wrong CRC");

  message = udpMessage.substr(IQRF_UDP_HEADER_SIZE, dlen);
}

UdpMessaging::UdpMessaging(const std::string& iqrf_port_name, const std::string& remote_ip_port, const std::string& local_ip_port)
  :m_daemon(nullptr)
  , m_udpChannel(nullptr)
  , m_toUdpMessageQueue(nullptr)
  , m_toIqrfMessageQueue(nullptr)
  , m_iqrfPortName(iqrf_port_name)
{
  m_remotePort = strtoul(remote_ip_port.c_str(), nullptr, 0);
  if (0 == m_remotePort || ULONG_MAX == m_remotePort)
    m_remotePort = 55000;
  m_localPort = strtoul(local_ip_port.c_str(), nullptr, 0);
  if (0 == m_localPort || ULONG_MAX == m_localPort)
    m_localPort = 55300;
}

UdpMessaging::~UdpMessaging()
{
}

void UdpMessaging::getGwIdent(ustring& message)
{
  //1. - GW type e.g.: GW - ETH - 02A
  //2. - FW version e.g. : 2.50
  //3. - MAC address e.g. : 00 11 22 33 44 55
  //4. - TCP / IP Stack version e.g. : 5.42
  //5. - IP address of GW e.g. : 192.168.2.100
  //6. - Net BIOS Name e.g. : iqrf_xxxx (15 characters)
  //7. - IQRF module OS version e.g. : 3.06D
  //8. - Public IP address e.g. : 213.214.215.120

  const char* crlf = "\x0D\x0A";
  const std::string& ipAddr = m_udpChannel->getListeningIpAdress();

  //TODO set correct IP adresses, MAC, OS ver, etc
  std::basic_ostringstream<char> ostring;
  ostring << crlf <<
    "udp-dmn-01" << crlf <<
    "1.00" << crlf <<
    "00 00 00 00 00 00" << crlf <<
    "5.42" << crlf <<
    ipAddr << crlf <<
    "iqrf_xxxx" << crlf <<
    "3.06D" << crlf <<
    ipAddr << crlf;

  ustring res((unsigned char*)ostring.str().data(), ostring.str().size());
  message = res;
}

void UdpMessaging::getGwStatus(ustring& message)
{
  // current date/time based on current system
  time_t now = time(0);
  tm *ltm = localtime(&now);

  message.resize(UdpGwStatus::unused12 + 1, '\0');
  //TODO get channel status to Channel iface
  message[trStatus] = 0x80;   //SPI_IQRF_SPI_READY_COMM = 0x80, see spi_iqrf.h
  message[supplyExt] = 0x01;  //DB3 0x01 supplied from external source
  message[timeSec] = (unsigned char)ltm->tm_sec;    //DB4 GW time – seconds(see Time and date coding)
  message[timeMin] = (unsigned char)ltm->tm_min;    //DB5 GW time – minutes
  message[timeHour] = (unsigned char)ltm->tm_hour;    //DB6 GW time – hours
  message[timeWday] = (unsigned char)ltm->tm_wday;    //DB7 GW date – day of the week
  message[timeMday] = (unsigned char)ltm->tm_mday;    //DB8 GW date – day
  message[timeMon] = (unsigned char)ltm->tm_mon;    //DB9 GW date – month
  message[timeYear] = (unsigned char)(ltm->tm_year % 100);   //DB10 GW date – year
}

/*
void UdpMessaging::watchDog()
{
  TRC_ENTER("");
  m_running = true;
  bool m_cout_IP = false;
  try {
    start();
    while (m_running)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      //TODO
      //watch worker threads and possibly restart
      if (m_udpChannel->isListening()) {
        if (!m_cout_IP) {
          std::cout << "Listening on: " << m_udpChannel->getListeningIpAdress() << ":" << m_udpChannel->getListeningIpPort() << std::endl;
          m_cout_IP = true;
        }
      }
    }
  }
  catch (std::exception& e) {
    CATCH_EX("error", std::exception, e);
  }

  stop();
  TRC_LEAVE("");
}
*/

void UdpMessaging::start()
{
  TRC_ENTER("");
  //size_t found = m_iqrfPortName.find("spi");
  //if (found != std::string::npos)
  //  m_iqrfChannel = ant_new IqrfSpiChannel(m_iqrfPortName);
  //else
  //  m_iqrfChannel = ant_new IqrfCdcChannel(m_iqrfPortName);

  m_udpChannel = ant_new UdpChannel((unsigned short)m_remotePort, (unsigned short)m_localPort, IQRF_UDP_BUFFER_SIZE);

  //Messages from IQRF are sent via MessageQueue to UDP channel
  m_toUdpMessageQueue = ant_new MessageQueue(m_udpChannel);

  //Messages from UDP are sent via MessageQueue to IQRF channel
  //m_toIqrfMessageQueue = ant_new MessageQueue(m_iqrfChannel);

  //Received messages from IQRF channel are pushed to UDP MessageQueue
  //m_iqrfChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
  //  return handleMessageFromIqrf(msg); });

  //Received messages from UDP channel are pushed to IQRF MessageQueue
  m_udpChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromUdp(msg); });

  std::cout << "udp-daemon started" << std::endl;

  TRC_LEAVE("");
}

void UdpMessaging::stop()
{
  TRC_ENTER("");
  std::cout << "udp-daemon stops" << std::endl;
  //delete m_iqrfChannel;
  delete m_udpChannel;
  delete m_toUdpMessageQueue;
  delete m_toIqrfMessageQueue;
  TRC_LEAVE("");
}

void UdpMessaging::exit()
{
  TRC_WAR("exiting ...");
  std::cout << "udp-daemon exits" << std::endl;
  m_running = false;
}