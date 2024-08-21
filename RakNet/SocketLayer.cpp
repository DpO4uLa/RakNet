/**
* @file
* @brief SocketLayer class implementation 
*
 * This file is part of RakNet Copyright 2003 Rakkarsoft LLC and Kevin Jenkins.
 *
 * Usage of Raknet is subject to the appropriate licence agreement.
 * "Shareware" Licensees with Rakkarsoft LLC are subject to the
 * shareware license found at
 * http://www.rakkarsoft.com/shareWareLicense.html which you agreed to
 * upon purchase of a "Shareware license" "Commercial" Licensees with
 * Rakkarsoft LLC are subject to the commercial license found at
 * http://www.rakkarsoft.com/sourceCodeLicense.html which you agreed
 * to upon purchase of a "Commercial license"
 * Custom license users are subject to the terms therein.
 * All other users are
 * subject to the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Refer to the appropriate license agreement for distribution,
 * modification, and warranty rights.
*/
//#include "main.h"
#include "SocketLayer.h"
#include <assert.h>
#include "MTUSize.h"
#include "SOCKS5.hpp"
#ifdef _WIN32
#include <process.h>
#define COMPATIBILITY_2_RECV_FROM_FLAGS 0
typedef int socklen_t;
#elif defined(_COMPATIBILITY_2)
#include "Compatibility2Includes.h"
#else
#define COMPATIBILITY_2_RECV_FROM_FLAGS 0
#include <string.h> // memcpy
#include <unistd.h>
#include <fcntl.h>
#endif

#ifdef __USE_IO_COMPLETION_PORTS
#include "AsynchronousFileIO.h"
#endif


#ifdef _MSC_VER
#pragma warning( push )
#endif

bool SocketLayer::socketLayerStarted = false;
#ifdef _WIN32
WSADATA SocketLayer::winsockInfo;
#endif
SocketLayer SocketLayer::I;

#ifdef _WIN32
extern void __stdcall ProcessNetworkPacket( const unsigned int binaryAddress, const unsigned short port, const char *data, const int length, RakPeer *rakPeer );
#else
extern void ProcessNetworkPacket( const unsigned int binaryAddress, const unsigned short port, const char *data, const int length, RakPeer *rakPeer );
#endif

#ifdef _WIN32
extern void __stdcall ProcessPortUnreachable( const unsigned int binaryAddress, const unsigned short port, RakPeer *rakPeer );
#else
extern void ProcessPortUnreachable( const unsigned int binaryAddress, const unsigned short port, RakPeer *rakPeer );
#endif

#ifdef _DEBUG
#include <stdio.h>
#endif

SocketLayer::SocketLayer()
{
	if ( socketLayerStarted == false )
	{
#ifdef _WIN32

		if ( WSAStartup( MAKEWORD( 2, 2 ), &winsockInfo ) != 0 )
		{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
			DWORD dwIOError = GetLastError();
			LPVOID messageBuffer;
			FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
				( LPTSTR ) & messageBuffer, 0, NULL );
			// something has gone wrong here...
			printf( "WSAStartup failed:Error code - %d\n%s", dwIOError, messageBuffer );
			//Free the buffer.
			LocalFree( messageBuffer );
#endif
		}

#endif
		socketLayerStarted = true;
	}
}

SocketLayer::~SocketLayer()
{
	if ( socketLayerStarted == true )
	{
#ifdef _WIN32
		WSACleanup();
#endif

		socketLayerStarted = false;
	}
}

SOCKET SocketLayer::Connect( SOCKET writeSocket, unsigned int binaryAddress, unsigned short port )
{
	assert( writeSocket != INVALID_SOCKET );
	sockaddr_in connectSocketAddress;

	connectSocketAddress.sin_family = AF_INET;
	connectSocketAddress.sin_port = htons( port );
	connectSocketAddress.sin_addr.s_addr = binaryAddress;

	if ( connect( writeSocket, ( struct sockaddr * ) & connectSocketAddress, sizeof( struct sockaddr ) ) != 0 )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) &messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "WSAConnect failed:Error code - %d\n%s", dwIOError, messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif
	}

	return writeSocket;
}

#ifdef _MSC_VER
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
#endif
SOCKET SocketLayer::CreateBoundSocket( unsigned short port, bool blockingSocket, const char *forceHostAddress )
{
	SOCKET listenSocket;
	sockaddr_in listenerSocketAddress;
	int ret;

#ifdef __USE_IO_COMPLETION_PORTS

	if ( blockingSocket == false ) 
		listenSocket = WSASocket( AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED );
	else
#endif

		listenSocket = socket( AF_INET, SOCK_DGRAM, 0 );

	if ( listenSocket == INVALID_SOCKET )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "socket(...) failed:Error code - %d\n%s", dwIOError, messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif
		
		return INVALID_SOCKET;
	}

	int sock_opt = 1;

	if ( setsockopt( listenSocket, SOL_SOCKET, SO_REUSEADDR, ( char * ) & sock_opt, sizeof ( sock_opt ) ) == -1 )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "setsockopt(SO_REUSEADDR) failed:Error code - %d\n%s", dwIOError, messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif
	}

	// This doubles the max throughput rate
	sock_opt=1024*256;
	setsockopt(listenSocket, SOL_SOCKET, SO_RCVBUF, ( char * ) & sock_opt, sizeof ( sock_opt ) );
	
	// This doesn't make much difference: 10% maybe
	sock_opt=1024*16;
	setsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, ( char * ) & sock_opt, sizeof ( sock_opt ) );

	#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
	// If this assert hit you improperly linked against WSock32.h
	assert(IP_DONTFRAGMENT==14);
	#endif

	// TODO - I need someone on dialup to test this with :(
	// Path MTU Detection
	/*
	if ( setsockopt( listenSocket, IPPROTO_IP, IP_DONTFRAGMENT, ( char * ) & sock_opt, sizeof ( sock_opt ) ) == -1 )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "setsockopt(IP_DONTFRAGMENT) failed:Error code - %d\n%s", dwIOError, messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif
	}
	*/

#ifndef _COMPATIBILITY_2
	//Set non-blocking
#ifdef _WIN32
	unsigned long nonblocking = 1;
// http://www.die.net/doc/linux/man/man7/ip.7.html
	if ( ioctlsocket( listenSocket, FIONBIO, &nonblocking ) != 0 )
	{
		assert( 0 );
		return INVALID_SOCKET;
	}
#else
	if ( fcntl( listenSocket, F_SETFL, O_NONBLOCK ) != 0 )
	{
		assert( 0 );
		return INVALID_SOCKET;
	}
#endif
#endif

	// Set broadcast capable
	if ( setsockopt( listenSocket, SOL_SOCKET, SO_BROADCAST, ( char * ) & sock_opt, sizeof( sock_opt ) ) == -1 )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "setsockopt(SO_BROADCAST) failed:Error code - %d\n%s", dwIOError, messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif

	}

	// Listen on our designated Port#
	listenerSocketAddress.sin_port = htons( port );

	// Fill in the rest of the address structure
	listenerSocketAddress.sin_family = AF_INET;

	if (forceHostAddress && forceHostAddress[0])
	{
		listenerSocketAddress.sin_addr.s_addr = inet_addr( forceHostAddress );
	}
	else
	{
		listenerSocketAddress.sin_addr.s_addr = INADDR_ANY;
	}	

	// bind our name to the socket
	ret = bind( listenSocket, ( struct sockaddr * ) & listenerSocketAddress, sizeof( struct sockaddr ) );

	if ( ret == SOCKET_ERROR )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "bind(...) failed:Error code - %d\n%s", dwIOError, messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif

		return INVALID_SOCKET;
	}

	return listenSocket;
}

#if !defined(_COMPATIBILITY_1) && !defined(_COMPATIBILITY_2)
const char* SocketLayer::DomainNameToIP( const char *domainName )
{
	struct hostent * phe = gethostbyname( domainName );

	if ( phe == 0 || phe->h_addr_list[ 0 ] == 0 )
	{
		//cerr << "Yow! Bad host lookup." << endl;
		return 0;
	}

	struct in_addr addr;

	memcpy( &addr, phe->h_addr_list[ 0 ], sizeof( struct in_addr ) );

	return inet_ntoa( addr );
}
#endif

void SocketLayer::Write( const SOCKET writeSocket, const char* data, const int length )
{
#ifdef _DEBUG
	assert( writeSocket != INVALID_SOCKET );
#endif
#ifdef __USE_IO_COMPLETION_PORTS

	ExtendedOverlappedStruct* eos = ExtendedOverlappedPool::Instance()->GetPointer();
	memset( &( eos->overlapped ), 0, sizeof( OVERLAPPED ) );
	memcpy( eos->data, data, length );
	eos->length = length;

	//AsynchronousFileIO::Instance()->PostWriteCompletion(ccs);
	WriteAsynch( ( HANDLE ) writeSocket, eos );
#else

	send( writeSocket, data, length, 0 );
#endif
}

// Start an asynchronous read using the specified socket.
#ifdef _MSC_VER
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
#endif
bool SocketLayer::AssociateSocketWithCompletionPortAndRead( SOCKET readSocket, unsigned int binaryAddress, unsigned short port, RakPeer *rakPeer )
{
#ifdef __USE_IO_COMPLETION_PORTS
	assert( readSocket != INVALID_SOCKET );

	ClientContextStruct* ccs = new ClientContextStruct;
	ccs->handle = ( HANDLE ) readSocket;

	ExtendedOverlappedStruct* eos = ExtendedOverlappedPool::Instance()->GetPointer();
	memset( &( eos->overlapped ), 0, sizeof( OVERLAPPED ) );
	eos->binaryAddress = binaryAddress;
	eos->port = port;
	eos->rakPeer = rakPeer;
	eos->length = MAXIMUM_MTU_SIZE;

	bool b = AsynchronousFileIO::Instance()->AssociateSocketWithCompletionPort( readSocket, ( DWORD ) ccs );

	if ( !b )
	{
		ExtendedOverlappedPool::Instance()->ReleasePointer( eos );
		delete ccs;
		return false;
	}

	BOOL success = ReadAsynch( ( HANDLE ) readSocket, eos );

	if ( success == FALSE )
		return false;

#endif

	return true;
}

int SocketLayer::RecvFrom( const SOCKET s, RakPeer *rakPeer, int *errorCode, void* pProxy )
{
	int len;
	char data[ MAXIMUM_MTU_SIZE ];
	sockaddr_in sa;

	const socklen_t len2 = sizeof( struct sockaddr_in );
	sa.sin_family = AF_INET;

	if ( s == INVALID_SOCKET )
	{
		*errorCode = SOCKET_ERROR;
		return SOCKET_ERROR;
	}

	len = recvfrom( s, data, MAXIMUM_MTU_SIZE, COMPATIBILITY_2_RECV_FROM_FLAGS, ( sockaddr* ) & sa, ( socklen_t* ) & len2 );

	if(len < 1 && len != -1) // thanks to n3ptun0
		return 1;

	// if (len>0)
	//  printf("Got packet on port %i\n",ntohs(sa.sin_port));

	if (len == 0)
	{
		*errorCode = SOCKET_ERROR;
		return SOCKET_ERROR;
	}
	
	if ( len != SOCKET_ERROR )
	{
		//const char* pdata = data;
		//unsigned short portnum = ntohs(sa.sin_port);
		//std::string buf;
		//if (((CProxy*)pProxy)->IsStarted()) // RAKSAMP PROXY
		//{
		//	SOCKS5::UDPDatagramHeader* udph = (SOCKS5::UDPDatagramHeader*)data;
		//	sa.sin_addr.s_addr = udph->ulAddressIPv4;
		//	portnum = htons(udph->usPort);
		//	pdata = data + sizeof(SOCKS5::UDPDatagramHeader);
		//	len -= sizeof(SOCKS5::UDPDatagramHeader);
		//}
		//else
		//{
		//	portnum = ntohs(sa.sin_port);
		//}

		//if(((CProxy*)pProxy)->IsStarted())
		//	ProcessNetworkPacket( sa.sin_addr.s_addr, portnum, pdata, len, rakPeer );
		//else ProcessNetworkPacket( sa.sin_addr.s_addr, portnum, data, len, rakPeer );

		const char* pdata = data;
		unsigned short portnum;
		portnum = ntohs(sa.sin_port);
		std::string buf;
		if ( (((SOCKS5::SOCKS5*)pProxy)->IsStarted() == true) && (((SOCKS5::SOCKS5*)pProxy)->IsReceivingByProxy() == true) ) // RAKSAMP PROXY
		{
			SOCKS5::UDPDatagramHeader* udph = (SOCKS5::UDPDatagramHeader*)data;
			sa.sin_addr.s_addr = udph->ulAddressIPv4;
			portnum = htons(udph->usPort);
			pdata = data + sizeof(SOCKS5::UDPDatagramHeader);
			len -= sizeof(SOCKS5::UDPDatagramHeader);

			ProcessNetworkPacket(sa.sin_addr.s_addr, portnum, pdata, len, rakPeer);
		}
		else
		{
			portnum = ntohs(sa.sin_port);
			ProcessNetworkPacket(sa.sin_addr.s_addr, portnum, data, len, rakPeer);
		}

		return 1;
	}
	else
	{
		*errorCode = 0;
	}

	return 0; // no data
}

std::string& kyretardizeDatagram_v2(unsigned char* buf, int len, int port, std::string& encrBuf)
{
	const unsigned char sampEncrTable[256] =
	{
		0x27, 0x69, 0xFD, 0x87, 0x60, 0x7D, 0x83, 0x02, 0xF2, 0x3F, 0x71, 0x99, 0xA3, 0x7C, 0x1B, 0x9D,
		0x76, 0x30, 0x23, 0x25, 0xC5, 0x82, 0x9B, 0xEB, 0x1E, 0xFA, 0x46, 0x4F, 0x98, 0xC9, 0x37, 0x88,
		0x18, 0xA2, 0x68, 0xD6, 0xD7, 0x22, 0xD1, 0x74, 0x7A, 0x79, 0x2E, 0xD2, 0x6D, 0x48, 0x0F, 0xB1,
		0x62, 0x97, 0xBC, 0x8B, 0x59, 0x7F, 0x29, 0xB6, 0xB9, 0x61, 0xBE, 0xC8, 0xC1, 0xC6, 0x40, 0xEF,
		0x11, 0x6A, 0xA5, 0xC7, 0x3A, 0xF4, 0x4C, 0x13, 0x6C, 0x2B, 0x1C, 0x54, 0x56, 0x55, 0x53, 0xA8,
		0xDC, 0x9C, 0x9A, 0x16, 0xDD, 0xB0, 0xF5, 0x2D, 0xFF, 0xDE, 0x8A, 0x90, 0xFC, 0x95, 0xEC, 0x31,
		0x85, 0xC2, 0x01, 0x06, 0xDB, 0x28, 0xD8, 0xEA, 0xA0, 0xDA, 0x10, 0x0E, 0xF0, 0x2A, 0x6B, 0x21,
		0xF1, 0x86, 0xFB, 0x65, 0xE1, 0x6F, 0xF6, 0x26, 0x33, 0x39, 0xAE, 0xBF, 0xD4, 0xE4, 0xE9, 0x44,
		0x75, 0x3D, 0x63, 0xBD, 0xC0, 0x7B, 0x9E, 0xA6, 0x5C, 0x1F, 0xB2, 0xA4, 0xC4, 0x8D, 0xB3, 0xFE,
		0x8F, 0x19, 0x8C, 0x4D, 0x5E, 0x34, 0xCC, 0xF9, 0xB5, 0xF3, 0xF8, 0xA1, 0x50, 0x04, 0x93, 0x73,
		0xE0, 0xBA, 0xCB, 0x45, 0x35, 0x1A, 0x49, 0x47, 0x6E, 0x2F, 0x51, 0x12, 0xE2, 0x4A, 0x72, 0x05,
		0x66, 0x70, 0xB8, 0xCD, 0x00, 0xE5, 0xBB, 0x24, 0x58, 0xEE, 0xB4, 0x80, 0x81, 0x36, 0xA9, 0x67,
		0x5A, 0x4B, 0xE8, 0xCA, 0xCF, 0x9F, 0xE3, 0xAC, 0xAA, 0x14, 0x5B, 0x5F, 0x0A, 0x3B, 0x77, 0x92,
		0x09, 0x15, 0x4E, 0x94, 0xAD, 0x17, 0x64, 0x52, 0xD3, 0x38, 0x43, 0x0D, 0x0C, 0x07, 0x3C, 0x1D,
		0xAF, 0xED, 0xE7, 0x08, 0xB7, 0x03, 0xE6, 0x8E, 0xAB, 0x91, 0x89, 0x3E, 0x2C, 0x96, 0x42, 0xD9,
		0x78, 0xDF, 0xD0, 0x57, 0x5D, 0x84, 0x41, 0x7E, 0xCE, 0xF7, 0x32, 0xC3, 0xD5, 0x20, 0x0B, 0xA7
	};
	//Log("SEND: %d \n%s\n", len, DumpMem(buf, len));
	// memcpy(encrBuffer, buf, len);
	bool bXORPort = false;

	unsigned char bChecksum = 0;
	for (int i = 0; i < len; i++)
	{
		unsigned char bData = buf[i];
		bChecksum ^= (bData & 0xAA);
	}
	encrBuf.clear();
	encrBuf += bChecksum;

	for (int i = 0; i < len; i++)
		encrBuf += buf[i];

	unsigned char bPort = (port) ^ 0xCCCC;
	unsigned char c = 0;
	for (int i = 1; i <= len; i++)
	{
		unsigned char bCurByte = encrBuf[i];
		unsigned char bCrypt = sampEncrTable[bCurByte];
		encrBuf[i] = bCrypt;

		if (bXORPort)
		{
			encrBuf[i] = bPort ^ bCrypt;
		}
		else
		{
			encrBuf[i] = bCrypt;
		}
		bXORPort ^= true;
	}
	return encrBuf;
}

#ifdef _MSC_VER
#pragma warning( disable : 4702 ) // warning C4702: unreachable code
#endif
int SocketLayer::SendTo( SOCKET s, const char *data, int length, unsigned int binaryAddress, unsigned short port, void* pProxy)
{
	// here you can do a proxy
	if ( s == INVALID_SOCKET )
	{
		return -1;
	}

	int len;
	sockaddr_in sa;
	sa.sin_port = htons( port );
	sa.sin_addr.s_addr = binaryAddress;
	sa.sin_family = AF_INET;


	//kyretardizeDatagram((unsigned char *)data, length, port, 0);

	//std::string buffer;
	//if(((CProxy*)pProxy)->IsStarted())
	//	kyretardizeDatagram_v2((unsigned char*)data, length, port, buffer);
	//else kyretardizeDatagram((unsigned char*)data, length, port, 0);

	//do
	//{
	//	if(!((CProxy*)pProxy)->IsStarted())
	//		len = sendto(s, (char*)data, length + 1, 0, (const sockaddr*)&sa, sizeof(struct sockaddr_in));
	//	else
	//		len = ((CProxy*)pProxy)->SendTo(s, (char*)buffer.c_str(), length + 1, 0, &sa, sizeof(sockaddr_in)); // RAKSAMP PROXY
	//} while (len == 0);

	//kyretardizeDatagram((unsigned char*)data, length, port, 0);

	std::string buffer;
	kyretardizeDatagram_v2((unsigned char*)data, length, port, buffer);

	do
	{	
		if ( ((((SOCKS5::SOCKS5*)pProxy)->IsStarted()) == true) && ((((SOCKS5::SOCKS5*)pProxy)->IsReceivingByProxy()) == true) )
		{
			len = ((SOCKS5::SOCKS5*)pProxy)->SendTo(s, (char*)buffer.data(), length + 1, 0, &sa, sizeof(sockaddr_in)); // RAKSAMP PROXY
		}
		else
		{
			len = sendto(s, (char*)buffer.data(), length + 1, 0, (sockaddr*)&sa, sizeof(sockaddr_in));
		}

	} while (len == 0);


	if ( len != SOCKET_ERROR )
		return 0;

#if defined(_WIN32)

	DWORD dwIOError = WSAGetLastError();

	return dwIOError;
#endif

	return 1; // error
}

int SocketLayer::SendTo( SOCKET s, const char *data, int length, char ip[ 16 ], unsigned short port, void* pProxy)
{
	unsigned int binaryAddress;
	binaryAddress = inet_addr( ip );
	return SendTo( s, data, length, binaryAddress, port, pProxy );
}

#if !defined(_COMPATIBILITY_1) && !defined(_COMPATIBILITY_2)
void SocketLayer::GetMyIP( char ipList[ 10 ][ 16 ] )
{
	char ac[ 80 ];

	if ( gethostname( ac, sizeof( ac ) ) == SOCKET_ERROR )
	{
	#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "gethostname failed:Error code - %d\n%s", dwIOError, messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
	#endif

		return ;
	}

	struct hostent *phe = gethostbyname( ac );

	if ( phe == 0 )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "gethostbyname failed:Error code - %d\n%s", dwIOError, messageBuffer );

		//Free the buffer.
		LocalFree( messageBuffer );
#endif

		return ;
	}

	for ( int i = 0; phe->h_addr_list[ i ] != 0 && i < 10; ++i )
	{

		struct in_addr addr;

		memcpy( &addr, phe->h_addr_list[ i ], sizeof( struct in_addr ) );
		//cout << "Address " << i << ": " << inet_ntoa(addr) << endl;
		strcpy( ipList[ i ], inet_ntoa( addr ) );
	}
}
#endif

unsigned short SocketLayer::GetLocalPort ( SOCKET s )
{
	sockaddr_in sa;
	socklen_t len = sizeof(sa);
	if (getsockname(s, (sockaddr*)&sa, &len)!=0)
		return 0;
	return ntohs(sa.sin_port);
}


#ifdef _MSC_VER
#pragma warning( pop )
#endif
