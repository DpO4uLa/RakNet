#pragma once

/* modified by AdCKuY_DpO4uLa */
/* https://www.blast.hk/members/229228/ */
/* https://github.com/DpO4uLa */
/* this class support only SOCKS5 UDP proxy (no auth + auth) */

/* SOCKS5 RFC */
/* https://www.rfc-es.org/rfc/rfc1928-es.txt */
/* https://www.rfc-es.org/rfc/rfc1929-es.txt */

#include <iostream>
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "WS2_32.lib")

//define this if you want to log the status of the connection to the proxy
//#define SOCKS5_LOG

namespace SOCKS5
{
#pragma pack(push, 1)
	struct AuthRequestHeader
	{
		std::uint8_t		byteVersion;
		std::uint8_t		byteAuthMethodsCount;
		std::uint8_t		byteMethods[1];
	};
	struct AuthRespondHeader
	{
		std::uint8_t	byteVersion;
		std::uint8_t		byteAuthMethod;
	};
	struct AuthRespondHeaderAuth //only for proxy with auth
	{
		std::uint8_t		byteVersion;
		std::uint8_t		byteStatus;
	};
	struct ConnectRequestHeader
	{
		std::uint8_t		byteVersion;
		std::uint8_t		byteCommand;
		std::uint8_t		byteReserved;
		std::uint8_t		byteAddressType;
		std::uint32_t		ulAddressIPv4;
		std::uint16_t		usPort;
	};
	struct ConnectRespondHeader
	{
		std::uint8_t		byteVersion;
		std::uint8_t		byteResult;
		std::uint8_t		byteReserved;
		std::uint8_t		byteAddressType;
		std::uint32_t		ulAddressIPv4;
		std::uint16_t		usPort;
	};
	struct UDPDatagramHeader
	{
		std::uint16_t		usReserved;
		std::uint8_t		byteFragment;
		std::uint8_t		byteAddressType;
		std::uint32_t		ulAddressIPv4;
		std::uint16_t		usPort;
	};
#pragma pack( pop )
//	//start winsock
//	auto StartWinSock(void) -> void
//	{
//		WSADATA wsa;
//		auto res = WSAStartup(MAKEWORD(2, 1), &wsa);
//		if (res == 0)
//		{
//#if defined(SOCKS5_LOG)
//			printf("[SOCKS5]: WinSock2 успешно инициализирован.\n");
//#endif
//		}
//		else
//		{
//#if defined(SOCKS5_LOG)
//			printf("[SOCKS5->Error]: Не удалось инициализировать WinSock2. (WSAError: %d)\n", WSAGetLastError());
//#endif
//		}
//	};
//	//stop winsock
//	auto StopWinSock(void) -> void
//	{
//		auto res = WSACleanup();
//		if (res == 0)
//		{
//#if defined(SOCKS5_LOG)
//			printf("[SOCKS5]: Использование WinSock2 успешно прекращено.\n");
//#endif
//
//		}
//		else
//		{
//#if defined(SOCKS5_LOG)
//			printf("[SOCKS5->Error]: Не удалось прекратить использование WinSock2. (WSAError: %d)\n", WSAGetLastError());
//#endif
//		}
//	};
	//error codes
	enum class SOCKS5Err
	{
		SOCKS5_NONE,
		SOCKS5_FAILED_TO_CREATE_SOCKET,
		SOCKS5_FAILED_TO_CONNECT_TO_SERVER,
		SOCKS5_AUTHENTICATION_ERROR,
		SOCKS5_INVALID_VERSION_OR_METHOD,
		SOCKS5_AUTHORIZATION_ERROR,
		SOCKS5_INVALID_VERSION_OR_STATUS,
		SOCKS5_CONNECTION_ERROR,
		SOCKS5_INVALID_VERSION_OR_RESULT,
		SOCKS5_UNKNOWN_ERROR,
		SOCKS5_INITIALIZED_SUCCESSFULLY
	};
	class SOCKS5 {
	private:
		SOCKET m_sockTCP;//socket of proxy
		sockaddr_in m_proxyServerAddr;//public addr and port of proxy
		std::uint32_t m_proxyIP;//network addr of proxy
		std::uint16_t m_proxyPort;//network port of proxy
		std::string m_thisIP;//connected proxy ip
		std::string m_thisPORT;//connected proxy port
		std::string m_thisLogin;//connected proxy login (only auth)
		std::string m_thisPassword;//connected proxy password (only auth)
		bool m_bIsStarted;//is proxy connected
		bool m_bIsValidReceiving;//set true if you successfully connected to server
		bool m_bIsReceivingByProxy;//set true if you want to redirect traffic to proxy
	public:
		SOCKS5(const std::string ProxyIP, const std::string ProxyPort, const std::string ProxyLogin, const std::string ProxyPassword) :
			m_sockTCP(INVALID_SOCKET), 
			m_proxyServerAddr{},
			m_proxyIP{},
			m_proxyPort{},
			m_thisIP{"NULL"},
			m_thisPORT{"NULL"},
			m_thisLogin{"NLL"},
			m_thisPassword{"NULL"},
			m_bIsStarted(false),
			m_bIsValidReceiving(false),
			m_bIsReceivingByProxy(false)
		{
			m_proxyIP = inet_addr(ProxyIP.c_str());
			m_proxyPort = htons(std::atoi(ProxyPort.c_str()));
			m_thisIP = ProxyIP;
			m_thisPORT = ProxyPort;
			m_thisLogin = ProxyLogin;
			m_thisPassword = ProxyPassword;
		};
		SOCKS5(const std::string ProxyIP, const std::string ProxyPort) :
			m_sockTCP(INVALID_SOCKET),
			m_proxyServerAddr{},
			m_proxyIP{},
			m_proxyPort{},
			m_thisIP{ "NULL" },
			m_thisPORT{ "NULL" },
			m_thisLogin{ "NLL" },
			m_thisPassword{ "NULL" },
			m_bIsStarted(false),
			m_bIsValidReceiving(false),
			m_bIsReceivingByProxy(false)
		{
			m_proxyIP = inet_addr(ProxyIP.c_str());
			m_proxyPort = htons(std::atoi(ProxyPort.c_str()));
			m_thisIP = ProxyIP;
			m_thisPORT = ProxyPort;
		};
		SOCKS5() :
			m_sockTCP(INVALID_SOCKET),
			m_proxyServerAddr{},
			m_proxyIP{},
			m_proxyPort{},
			m_thisIP{ "NULL" },
			m_thisPORT{ "NULL" },
			m_thisLogin{ "NLL" },
			m_thisPassword{ "NULL" },
			m_bIsStarted(false),
			m_bIsValidReceiving(false),
			m_bIsReceivingByProxy(false)
		{

		};
		~SOCKS5() {
			if (m_bIsStarted)
				Shutdown();
		};
		SOCKS5& operator= (const SOCKS5& prox) 
		{
			this->m_sockTCP = prox.m_sockTCP;
			this->m_proxyServerAddr = prox.m_proxyServerAddr;
			this->m_proxyIP = prox.m_proxyIP;
			this->m_proxyPort = prox.m_proxyPort;
			this->m_thisIP = prox.m_thisIP;
			this->m_thisPORT = prox.m_thisPORT;
			this->m_thisLogin = prox.m_thisLogin;
			this->m_thisPassword = prox.m_thisPassword;
			this->m_bIsStarted = prox.m_bIsStarted;
			this->m_bIsValidReceiving = prox.m_bIsValidReceiving;
			this->m_bIsReceivingByProxy = prox.m_bIsReceivingByProxy;
			return *this;
		};
		//after start (log + pass)
		auto StartWithAuth(void)
		{
			return Start(m_thisIP, m_thisPORT, m_thisLogin, m_thisPassword);
		};
		//after start (no auth)
		auto StartWithoutAuth(void)
		{
			return Start(m_thisIP, m_thisPORT);
		}
		//not auth
		auto Start(const std::string ProxyIP, const std::string ProxyPort) -> std::pair<bool, SOCKS5Err>
		{
			m_proxyIP = inet_addr(ProxyIP.c_str());
			m_proxyPort = htons(std::atoi(ProxyPort.c_str()));
			m_thisIP = ProxyIP;
			m_thisPORT = ProxyPort;

#if defined(SOCKS5_LOG)
			printf("[CProxy::Start]: Запуск прокси для хоста: %s:%s\n", m_thisIP.c_str(), m_thisPORT.c_str());
#endif
			
			if (m_sockTCP != INVALID_SOCKET)
				closesocket(m_sockTCP);

			if ((m_sockTCP = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
			{
#if defined (SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Не удалось создать сокет. (WSAError: %d)\n", WSAGetLastError());
#endif
				return { false, SOCKS5Err::SOCKS5_FAILED_TO_CREATE_SOCKET };
			}

			sockaddr_in sa{};
			sa.sin_addr.S_un.S_addr = m_proxyIP;
			sa.sin_family = AF_INET;
			sa.sin_port = m_proxyPort;

			std::uint32_t timeout = 5000;//timeout of connect to proxy server

			setsockopt(m_sockTCP, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

			if (connect(m_sockTCP, (sockaddr*)&sa, sizeof(sa)) == INVALID_SOCKET)
			{
#if defined (SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Не удалось подключиться к серверу. (WSAError: %d)\n", WSAGetLastError());
#endif
				return { false, SOCKS5Err::SOCKS5_FAILED_TO_CONNECT_TO_SERVER };
			}

			/*
				+----+----------+----------+
				|VER | NMETHODS | METHODS  |
				+----+----------+----------+
				| 1  |    1     | 1 to 255 |
				+----+----------+----------+
			*/
			AuthRequestHeader ahead{};
			ahead.byteVersion = 5;// SOCKS5
			ahead.byteAuthMethodsCount = 1;
			ahead.byteMethods[0] = 0;//no auth

#if defined(SOCKS5_LOG)
			printf("[CProxy::Start]: Аутентификация...\n");
#endif

			send(m_sockTCP, (char*)&ahead, sizeof(AuthRequestHeader), 0);

			/*
				+----+--------+
				|VER | METHOD |
				+----+--------+
				| 1  |   1    |
				+----+--------+
			*/
			AuthRespondHeader arhead{};
			auto res = recv(m_sockTCP, (char*)&arhead, sizeof(AuthRespondHeader), 0);

			if (res == SOCKET_ERROR) 
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Ошибка аутентификации. (WSAError: %d)\n", WSAGetLastError());
#endif
				closesocket(m_sockTCP);
				return { false, SOCKS5Err::SOCKS5_AUTHENTICATION_ERROR };
			}

			if (arhead.byteVersion != 5 || arhead.byteAuthMethod != 0) 
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Невалидная версия или метод -> ver: %d, method: %d\n", arhead.byteVersion, arhead.byteAuthMethod);
#endif
				closesocket(m_sockTCP);
				return { false, SOCKS5Err::SOCKS5_INVALID_VERSION_OR_METHOD };
			}
			else
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start]: Аутентификация выполнена успешно.\n");
#endif
			}
		
			
			/*
				+----+-----+-------+------+----------+----------+
				|VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
				+----+-----+-------+------+----------+----------+
				| 1  |  1  | X'00' |  1   | Variable |    2     |
				+----+-----+-------+------+----------+----------+
			*/
			ConnectRequestHeader head{};
			head.byteVersion = 5; //SOCKS5
			head.byteCommand = 3; // tcp connection = 1, tcp binding = 2,  udp = 3
			head.byteReserved = 0;
			head.byteAddressType = 1; // IPv4=1, domain name = 3, IPv6 = 4
			head.ulAddressIPv4 = 0; 
			head.usPort = 0;

#if defined(SOCKS5_LOG)
			printf("[CProxy::Start]: Подключение...\n");
#endif

			send(m_sockTCP, (char*)&head, sizeof(ConnectRequestHeader), 0);
			
			ConnectRespondHeader rhead{};
			auto res2 = recv(m_sockTCP, (char*)&rhead, sizeof(ConnectRespondHeader), 0);

			if (res2 == SOCKET_ERROR)
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Ошибка подключения. (WSAError: %d)\n", WSAGetLastError());
#endif
				closesocket(m_sockTCP);
				return { false, SOCKS5Err::SOCKS5_CONNECTION_ERROR };
			}

			if (rhead.byteVersion != 5 || rhead.byteResult != 0)
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Невалидная версия или результат -> ver: %d, result: %d\n", rhead.byteVersion, rhead.byteResult);
#endif
				closesocket(m_sockTCP);
				return { false, SOCKS5Err::SOCKS5_INVALID_VERSION_OR_RESULT };
			}
			else
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start]: Подключено.\n");
#endif
				m_proxyServerAddr.sin_family = AF_INET;
				m_proxyServerAddr.sin_port = rhead.usPort;
				m_proxyServerAddr.sin_addr.s_addr = rhead.ulAddressIPv4;
				m_bIsStarted = true;
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start]: Прокси успешно инициализирован.\n");
#endif
				return { true, SOCKS5Err::SOCKS5_INITIALIZED_SUCCESSFULLY };
			}

#if defined(SOCKS5_LOG)
			printf("[CProxy::Start->Error]: Неизвестная ошибка.\n");
#endif
			closesocket(m_sockTCP);
			return { false, SOCKS5Err::SOCKS5_UNKNOWN_ERROR };
		};
		//auth with login and password
		auto Start(const std::string ProxyIP, const std::string ProxyPort, const std::string ProxyLogin, const std::string ProxyPassword) -> std::pair<bool, SOCKS5Err>
		{ 
			m_proxyIP = inet_addr(ProxyIP.c_str());
			m_proxyPort = htons(std::atoi(ProxyPort.c_str()));
			m_thisIP = ProxyIP;
			m_thisPORT = ProxyPort;
			m_thisLogin = ProxyLogin;
			m_thisPassword = ProxyPassword;
			
#if defined(SOCKS5_LOG)
			printf("[CProxy::Start]: Запуск прокси для хоста: %s:%s, логин: %s, пароль: %s\n", m_thisIP.c_str(), m_thisPORT.c_str(), m_thisLogin.c_str(), m_thisPassword.c_str());
#endif
			
			if (m_sockTCP != INVALID_SOCKET)
				closesocket(m_sockTCP);

			if ((m_sockTCP = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
			{
#if defined (SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Не удалось создать сокет. (WSAError: %d)\n", WSAGetLastError());
#endif
				return { false, SOCKS5Err::SOCKS5_FAILED_TO_CREATE_SOCKET };
			}

			sockaddr_in sa{};
			sa.sin_addr.S_un.S_addr = m_proxyIP;
			sa.sin_family = AF_INET;
			sa.sin_port = m_proxyPort;

			std::uint32_t timeout = 5000;//tineout of connect to proxy server

			setsockopt(m_sockTCP, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

			if (connect(m_sockTCP, (sockaddr*)&sa, sizeof(sa)) == INVALID_SOCKET)
			{
#if defined (SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Не удалось подключиться к серверу. (WSAError: %d)\n", WSAGetLastError());
#endif
				return { false, SOCKS5Err::SOCKS5_FAILED_TO_CONNECT_TO_SERVER };
			}

			/*
				+----+----------+----------+
				|VER | NMETHODS | METHODS  |
				+----+----------+----------+
				| 1  |    1     | 1 to 255 |
				+----+----------+----------+
			*/
			AuthRequestHeader ahead{};
			ahead.byteVersion = 5;// SOCKS5
			ahead.byteAuthMethodsCount = 1;
			ahead.byteMethods[0] = 2;//auth with login and password

#if defined(SOCKS5_LOG)
			printf("[CProxy::Start]: Аутентификация...\n");
#endif

			send(m_sockTCP, (char*)&ahead, sizeof(AuthRequestHeader), 0);

			/*
				+----+--------+
				|VER | METHOD |
				+----+--------+
				| 1  |   1    |
				+----+--------+
			*/
			AuthRespondHeader arhead{};
			auto res = recv(m_sockTCP, (char*)&arhead, sizeof(AuthRespondHeader), 0);

			if (res == SOCKET_ERROR) 
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Ошибка аутентификации. (WSAError: %d)\n", WSAGetLastError());
#endif
				closesocket(m_sockTCP);
				return { false, SOCKS5Err::SOCKS5_AUTHENTICATION_ERROR };
			}

			if (arhead.byteVersion != 5 || arhead.byteAuthMethod != 2) 
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Невалидная версия или метод -> ver: %d, method: %d\n", arhead.byteVersion, arhead.byteAuthMethod);
#endif
				closesocket(m_sockTCP);
				return { false, SOCKS5Err::SOCKS5_INVALID_VERSION_OR_METHOD };
			}
			else
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start]: Аутентификация выполнена успешно.\n");
#endif
			}
		
			/*   username/password request looks like
				* +----+------+----------+------+----------+
				* |VER | ULEN |  UNAME   | PLEN |  PASSWD  |
				* +----+------+----------+------+----------+
				* | 1  |  1   | 1 to 255 |  1   | 1 to 255 |
				* +----+------+----------+------+----------+
			*/
			char AuthRequest[1024] = { 0 };
			std::uint16_t AuthRequestLen = 0;
			AuthRequest[AuthRequestLen] = 1; //VER
			AuthRequestLen++;

			AuthRequest[AuthRequestLen] = (std::uint8_t)m_thisLogin.size();//ULEN
			AuthRequestLen++;

			std::memcpy(AuthRequest + AuthRequestLen, m_thisLogin.data(), m_thisLogin.size()); //UNAME
			AuthRequestLen += (std::uint16_t)m_thisLogin.size();

			AuthRequest[AuthRequestLen] = (std::uint8_t)m_thisPassword.size();//PLEN
			AuthRequestLen++;

			std::memcpy(AuthRequest + AuthRequestLen, m_thisPassword.data(), m_thisPassword.size()); //PASSWD
			AuthRequestLen += (std::uint16_t)m_thisPassword.size();

#if defined(SOCKS5_LOG)
			printf("[CProxy::Start]: Авторизация.\n");
#endif

			send(m_sockTCP, AuthRequest, AuthRequestLen, 0);

			AuthRespondHeaderAuth arheada{};
			auto res2 = recv(m_sockTCP, (char*)&arheada, sizeof(AuthRespondHeaderAuth), 0);

			if (res2 == SOCKET_ERROR)
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Ошибка авторизации. (WSAError: %d)\n", WSAGetLastError());
#endif
				closesocket(m_sockTCP);
				return { false, SOCKS5Err::SOCKS5_AUTHORIZATION_ERROR };
			}

			if (arheada.byteVersion != 1 || arheada.byteStatus != 0)
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Невалидная версия или статус -> ver: %d, status: %d\n", arheada.byteVersion, arheada.byteStatus);
#endif
				closesocket(m_sockTCP);
				return { false, SOCKS5Err::SOCKS5_INVALID_VERSION_OR_STATUS };
			}
			else
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start]: Авторизация выполнена успешно.\n");
#endif
			}

			/*
				+----+-----+-------+------+----------+----------+
				|VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
				+----+-----+-------+------+----------+----------+
				| 1  |  1  | X'00' |  1   | Variable |    2     |
				+----+-----+-------+------+----------+----------+
			*/
			ConnectRequestHeader head{};
			head.byteVersion = 5; //SOCKS5
			head.byteCommand = 3; // tcp connection = 1, tcp binding = 2,  udp = 3
			head.byteReserved = 0;
			head.byteAddressType = 1; // IPv4=1, domain name = 3, IPv6 = 4
			head.ulAddressIPv4 = 0; 
			head.usPort = 0;

#if defined(SOCKS5_LOG)
			printf("[CProxy::Start]: Подключение...\n");
#endif

			send(m_sockTCP, (char*)&head, sizeof(ConnectRequestHeader), 0);
			
			ConnectRespondHeader rhead{};
			auto res3 = recv(m_sockTCP, (char*)&rhead, sizeof(ConnectRespondHeader), 0);

			if (res3 == SOCKET_ERROR)
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Ошибка подключения. (WSAError: %d)\n", WSAGetLastError());
#endif
				closesocket(m_sockTCP);
				return { false, SOCKS5Err::SOCKS5_CONNECTION_ERROR };
			}

			if (rhead.byteVersion != 5 || rhead.byteResult != 0)
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Невалидная версия или результат -> ver: %d, result: %d\n", rhead.byteVersion, rhead.byteResult);
#endif
				closesocket(m_sockTCP);
				return { false, SOCKS5Err::SOCKS5_INVALID_VERSION_OR_RESULT };
			}
			else
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start]: Подключено.\n");
#endif
				m_proxyServerAddr.sin_family = AF_INET;
				m_proxyServerAddr.sin_port = rhead.usPort;
				m_proxyServerAddr.sin_addr.s_addr = rhead.ulAddressIPv4;
				m_bIsStarted = true;
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start]: Прокси успешно инициализирован.\n");
#endif
				return { true, SOCKS5Err::SOCKS5_INITIALIZED_SUCCESSFULLY};
			}

#if defined(SOCKS5_LOG)
			printf("[CProxy::Start->Error]: Неизвестная ошибка.\n");
#endif
			closesocket(m_sockTCP);
			return { false, SOCKS5Err::SOCKS5_UNKNOWN_ERROR };
		};
		//send datagram
		auto SendTo(SOCKET socket, char* data, std::int32_t dataLength, std::int32_t flags, sockaddr_in* to, std::int32_t tolen) -> std::int32_t
		{
			const std::int32_t data_len = sizeof(UDPDatagramHeader) + dataLength;
			std::uint8_t* proxyData = new std::uint8_t[data_len];
			UDPDatagramHeader* udph = (UDPDatagramHeader*)proxyData;
			std::memcpy((void*)((std::uintptr_t)proxyData + sizeof(UDPDatagramHeader)), data, dataLength);

			udph->usReserved = 0;
			udph->byteFragment = 0;
			udph->byteAddressType = 1;
			udph->ulAddressIPv4 = to->sin_addr.S_un.S_addr;
			udph->usPort = to->sin_port;

			auto len = sendto(socket, (char*)proxyData, data_len, 0, (const sockaddr*)&m_proxyServerAddr, sizeof(sockaddr_in));
			delete[] proxyData;

			return len;
		};
		//recv datagram
		int RecvFrom(SOCKET socket, char* buffer, std::int32_t bufferLength, std::int32_t flags, sockaddr_in* from, std::int32_t* fromlen) {
			const std::int32_t udphsize = sizeof(UDPDatagramHeader);
			char* data = new char[bufferLength + udphsize];
			auto len = recvfrom(socket, data, bufferLength + udphsize, flags, (sockaddr*)from, fromlen);
			if (len <= 0)
			{
				delete[] data;
				return 0;
			}
			if (len <= udphsize)
			{
				std::memcpy(buffer, data, len);
				delete[] data;
				return len;
			}
			UDPDatagramHeader* udph = (UDPDatagramHeader*)data;
			from->sin_addr.S_un.S_addr = udph->ulAddressIPv4;
			from->sin_port = udph->usPort;
			std::memcpy(buffer, (void*)((std::uintptr_t)data + udphsize), len - udphsize);
			delete[] data;
			return len - udphsize;
		};
		//check if proxy started
		auto IsStarted(void) -> bool
		{
			return m_bIsStarted;
		};
		//restart connection to proxy 
		auto Restart(void) -> void
		{
			if (m_proxyIP == 0 || m_proxyPort == 0)
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Restart->Error]: Прокси еще не запускался. Рестарт не возможен.\n");
#endif
			}
			else
			{
				if (m_thisLogin.empty())
					Start(m_thisIP, m_thisPORT);
				else 
					Start(m_thisIP, m_thisPORT, m_thisLogin, m_thisPassword);
			}
		};
		//get proxy ip
		auto GetProxyIP(void) -> std::string
		{
			return m_thisIP;
		};
		//get proxy port
		auto GetProxyPort(void) -> std::string
		{
			return m_thisPORT;
		};
		//get public proxy ip
		auto GetPublicProxyIP(void) -> std::string
		{
			if (m_bIsStarted)
				return inet_ntoa(m_proxyServerAddr.sin_addr);
			else
				return "";
		};
		//use this if you successfully connected to server from proxy
		auto SetValidProxy(bool status) -> void
		{
			m_bIsValidReceiving = status;
		}
		//check if successfully connect to the server from proxy
		auto IsValidProxy(void) -> bool
		{
			return m_bIsValidReceiving;
		}
		//set true if you want to redirect traffic from proxy
		auto SetReceivingByProxy(bool status) -> void
		{
			m_bIsReceivingByProxy = status;
		};
		//check if traffic redirecting from proxy
		auto IsReceivingByProxy(void) -> bool
		{
			return m_bIsReceivingByProxy;
		}
		//disconnect from proxy
		auto Shutdown(void) -> void
		{
			if (m_sockTCP) {
				closesocket(m_sockTCP);
				m_bIsStarted = false;
			}
		}
	};
};