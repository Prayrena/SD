#pragma once
#include "Engine/core/EventSystem.hpp"
#include "Engine/Core/StringUtils.hpp"
#include <queue>

enum class NetSystemMode
{
	NONE = 0,
	CLIENT,
	SERVER,
};

enum class ClientState
{
	INVALID,
	DISCONNECTED,
	CONNECTING,
	CONNECTED,
};

enum class ServeState
{
	INVALID,
	DISCONNECTED,
	CONNECTED,
};

struct NetSystemConfig
{
	NetSystemMode	m_mode = NetSystemMode::NONE;
	std::string		m_hostAddressString;
	int				m_sendBufferSize = 2048;
	int				m_recvBufferSize = 2048;
};

class NetSystem
{
public:
	NetSystem(NetSystemConfig config);

	void Startup();
	void Shutdown();

	void BeginFrame();
	void EndFrame();

	bool IsServer();
	bool IsClient();
	bool IsConnected();

	void SetHostAddressAndPortFromConfig();

	// socket operations
	int	 ConnectClientSocket();
	void CloseClientSocketAndSetInvalid();

	static bool RemoteCommand(EventArgs& args);
	static bool BurstTest(EventArgs& args);

	void Send(std::string& data);
	void Receive(int bytesReceived);
	void ClearSendBufferBySendResult(int bytesSent);
	void PrepareSendBuffer();
	void PackSendQueueWithCommands();
	unsigned int m_bytesNeedToSend = 0; // calculate the length of the message we need to send
	unsigned int m_totalBytesNeedToSend = 0; // calculate the total length of all the message we need to send

	void ReadCommandFromReceiveQueue();


	NetSystemConfig m_config;

	uintptr_t		m_clientSocket = ~0ull;	// equivalent to INVALID_SOCKET
	uintptr_t		m_listenSocket = 0;

	ClientState		m_clientState = ClientState::INVALID;
	ServeState		m_serverState = ServeState::INVALID;

	unsigned long	m_hostAddress = 0;
	unsigned short	m_hostPort = 0;

	char*			m_sendBuffer;
	char*			m_recvBuffer;

	Strings			m_commandQueue;
	std::string		m_sendQueue;
	std::string		m_recvQueue;
};
