#include "Engine/Net/NetSystem.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/core/EventSystem.hpp"
#include "Engine/core/DevConsole.hpp"
#include <algorithm>


// #define _WINSOCKAPI_           // Prevents windows.h from including winsock.h
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

extern NetSystem* g_theNetSystem;

NetSystem::NetSystem(NetSystemConfig config)
		: m_config(config)
{
	// based on the config, initialize the send and receive buffers
	m_sendBuffer = new char[m_config.m_sendBufferSize];  // Allocate send buffer
	m_recvBuffer = new char[m_config.m_recvBufferSize];  // Allocate receive buffer

	// Initialize buffers with \0
	std::memset(m_sendBuffer, 0, m_config.m_sendBufferSize);
	std::memset(m_recvBuffer, 0, m_config.m_recvBufferSize);
}

void NetSystem::Startup()
{
	// for server and client, we both need to set this up
	SetHostAddressAndPortFromConfig();

	SubscribeEventCallbackFunction("RemoteCommand",  NetSystem::RemoteCommand);
	SubscribeEventCallbackFunction("BurstTest", NetSystem::BurstTest);

	// Initialize Winsock, no matter it is client or server
	// todo: ??? what actually it starts?
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (result == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		ERROR_RECOVERABLE(Stringf("WSAStartup failed with error: %i ", errorCode));
	}
	else
	{
		DebuggerPrintf("The net system is started \n");
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (IsServer())
	{
		if (!m_listenSocket && m_serverState == ServeState::INVALID)
		{
			// create a listen socket
			m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			// set the listen socket to non-blocking
			unsigned long blockingMode = 1;
			ioctlsocket(m_listenSocket, FIONBIO, &blockingMode);

			// Bind the listen socket to a port
			sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_addr.S_un.S_addr = htonl(m_hostAddress);
			addr.sin_port = htons(m_hostPort);
			result = bind(m_listenSocket, (sockaddr*)&addr, (int)sizeof(addr));

			if (result == SOCKET_ERROR)
			{
				int errorCode = WSAGetLastError();
				ERROR_RECOVERABLE(Stringf("listen socket binding failed with error:  %i \n", errorCode));
			}
			else
			{
				DebuggerPrintf("listen Socket successfully bound");
			}

			// Tell the listen socket to start listening for connections to accept
			result = listen(m_listenSocket, SOMAXCONN);

			if (result == SOCKET_ERROR)
			{
				int errorCode = WSAGetLastError();
				ERROR_RECOVERABLE(Stringf("listen socket failed for listening:  %i ", errorCode));
			}
			else
			{
				DebuggerPrintf("listen Socket is set to listen mode, now the server can accept connections\n");
			}

			m_serverState = ServeState::DISCONNECTED;
		}
	}
}

void NetSystem::SetHostAddressAndPortFromConfig()
{
	// Get host address and port from string, using hard coded string literals, which you should not do.
	Strings hostAddrAndPort = SplitStringOnDelimiter(m_config.m_hostAddressString, ':', false);
	if ((int)hostAddrAndPort.size() != 2)
	{
		ERROR_AND_DIE("Missing host address or port information");
	}
	else
	{
		std::string hostAddr = hostAddrAndPort[0];
		std::string hostPort = hostAddrAndPort[1];

		IN_ADDR addr;
		int result = inet_pton(AF_INET, hostAddr.c_str(), &addr);
		if (result == 0)
		{
			ERROR_AND_DIE("Invalid IP address format");
		}
		else
		{
			m_hostAddress = ntohl(addr.S_un.S_addr);
			m_hostPort = (unsigned short)(atoi(hostPort.c_str()));
		}
	}
}

void NetSystem::Shutdown()
{
	// todo: continue to send the the stuff in the send queue
	g_theNetSystem->BeginFrame();
	g_theNetSystem->EndFrame();

	if (IsClient())
	{
		closesocket(m_clientSocket);
	}

	if (IsServer())
	{
		closesocket(m_clientSocket);
		closesocket(m_listenSocket);
	}

	// shut down windows socket
	WSACleanup();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Used for arrays allocated with new[].
	// It ensures that the destructors of all elements in the array(if needed) are called and the entire memory block is freed correctly.
	delete[] m_sendBuffer;
	delete[] m_recvBuffer;
}

void NetSystem::BeginFrame()
{
	if (IsClient())
	{
		if (m_clientSocket == INVALID_SOCKET)
		{
			// Create client socket.
			m_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			// Set the client socket to non - blocking.
			unsigned long blockingMode = 1;
			ioctlsocket(m_clientSocket, FIONBIO, &blockingMode);

			m_clientState = ClientState::DISCONNECTED; // update client status
		}
		else
		{
			switch (m_clientState)
			{
				case ClientState::DISCONNECTED :
				{
					// start a connection attempt
					int result = ConnectClientSocket();

					if (result == SOCKET_ERROR)
					{
						int errorCode = WSAGetLastError();
						if (errorCode == WSAEWOULDBLOCK)
						{
							DebuggerPrintf("the client socket is establishing a connection \n");
						}
						else
						{
							ERROR_RECOVERABLE(Stringf("Connect failed with error: %i \n", errorCode));
						}					
					}

					m_clientState = ClientState::CONNECTING; // update client status
				} break;
				//----------------------------------------------------------------------------------------------------------------------------------------------------
				case ClientState::CONNECTING :
				{
					fd_set writeSockets;
					fd_set exceptSockets;
					FD_ZERO(&writeSockets);
					FD_ZERO(&exceptSockets);
					FD_SET(m_clientSocket, &writeSockets);
					FD_SET(m_clientSocket, &exceptSockets);
					timeval waitTime = { };
					int result = select(0, NULL, &writeSockets, &exceptSockets, &waitTime); // checks the status of the sockets in the sets passed to it

					if (result == SOCKET_ERROR)
					{
						// connection attempt failed
						DebuggerPrintf("socket connection attempt failed, try connect again \n");
						// ConnectClientSocket(); // todo:??? should reset the status or call connect again to check the result of the connect?
						m_clientState = ClientState::DISCONNECTED; // try to connect next frame
					}

					// No sockets are ready within the timeout period (non-blocking in this case, so it would return immediately
					if (result == 0)
					{
						// do nothing and keep checking the socket status each frame.
						DebuggerPrintf("client socket is connecting \n");
						return;
					}

					if (result > 0)
					{
						if (FD_ISSET(m_clientSocket, &exceptSockets))
						{
							// connection attempt failed
							DebuggerPrintf("socket connection attempt failed, try connect again \n");
							m_clientState = ClientState::DISCONNECTED; // try to connect next frame
							return;
						}

						// todo:????? this is true when there is no server for the client
						if (FD_ISSET(m_clientSocket, &writeSockets))
						{
							// connecting successfully, able to start send and receive now
							DebuggerPrintf("socket connection succeeded \n");
							m_clientState = ClientState::CONNECTED; // update client status
							return;
						}
					}
				} break;
				//----------------------------------------------------------------------------------------------------------------------------------------------------
				case ClientState::CONNECTED : 
				{				
					// if we don't need to send out any information, we just send 1 byte '\0'
					// because we need the result to check connection
					int numSentRequested = 0;
					if (m_bytesNeedToSend == 0)
					{
						numSentRequested = 1;
					}
					else
					{
						numSentRequested = m_bytesNeedToSend;
					}
					int sendResult = send(m_clientSocket, m_sendBuffer, numSentRequested, 0); 
					// we might send multiple command strings a single frame
					// so we can't use this
					// int sendResult = send(m_clientSocket, m_sendBuffer, (int)strlen(m_sendBuffer) + 1, 0);

					if (sendResult == 0)
					{
						// 0 means the connection has been closed.
						// shutdown socket and go back to trying to establish
						CloseClientSocketAndSetInvalid();
						DebuggerPrintf("Sending process failed. Shut down client socket \n");
						m_clientState = ClientState::DISCONNECTED;
					}
					else if (sendResult < 0)
					{
						int errorCode = WSAGetLastError();
						if (errorCode == WSAEWOULDBLOCK)
						{
							// WSAEWOULDBLOCK: The socket is non-blocking, and the send operation would block (try again later).
							DebuggerPrintf("No data to send out, client socket is waiting to send \n");
						}
						else if (errorCode == WSAECONNABORTED || errorCode == WSAECONNRESET) // means the other end has closed the connection
						{
							// This is not a fatal error but should stop using the client sockets.
							// WSAECONNRESET : The connection was reset by the remote host.
							CloseClientSocketAndSetInvalid();
							DebuggerPrintf("Sending process failed. Shut down client socket \n");
							m_clientState = ClientState::DISCONNECTED;
						}
						else if (errorCode == WSAETIMEDOUT)
						{
							DebuggerPrintf("A timeout occurred during client socket's sending operation \n");
						}
					}
					else
					{
						DebuggerPrintf(Stringf("Client Socket successfully sending out %i Bytes data \n", sendResult).c_str());

						ClearSendBufferBySendResult(sendResult);
						// we cant do this because the sendResult is not guarantee to be exact as the argument we pass in
						// and also, the butter cannot be null
						// delete[] m_sendBuffer;
						// m_sendBuffer = nullptr;
					}						

					//----------------------------------------------------------------------------------------------------------------------------------------------------
					// int recvResult = recv(m_clientSocket, m_recvBuffer, sizeof(m_recvBuffer), 0);
					int recvResult = recv(m_clientSocket, m_recvBuffer, m_config.m_recvBufferSize, 0);
					if (recvResult == 0)
					{
						// 0 means the connection has been closed.
						// shutdown socket and go back to trying to establish
						CloseClientSocketAndSetInvalid();
						DebuggerPrintf("Receive process failed. Shut down client socket \n");
						m_clientState = ClientState::DISCONNECTED;
					}
					else if (recvResult < 0)
					{
						int errorCode = WSAGetLastError();
						if (errorCode == WSAEWOULDBLOCK)
						{
							// WSAEWOULDBLOCK : The socket is non - blocking, and no data is available to read.
							DebuggerPrintf("No data receiving, client socket is waiting to receive \n");
						}
						else if (errorCode == WSAECONNABORTED || errorCode == WSAECONNRESET) // means the other end has closed the connection
						{
							// This is not a fatal error but should stop using the client sockets.
							// WSAECONNRESET: The connection was forcibly closed by the remote host.
							CloseClientSocketAndSetInvalid();
							DebuggerPrintf("Receiving process failed. Shut down client socket \n");
							m_clientState = ClientState::DISCONNECTED;
						}
						else if (errorCode == WSAETIMEDOUT)
						{
							DebuggerPrintf("A timeout occurred during client socket's receive operation \n");
						}
					}
					else
					{
						DebuggerPrintf(Stringf("Client Socket successfully receive %i Bytes data \n", recvResult).c_str());

						// Append received data to m_recvQueue
						Receive(recvResult);
					}
				} break;
			}
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (IsServer())
	{
		switch (m_serverState)
		{
			case ServeState::DISCONNECTED :
			{
				if (m_clientSocket == INVALID_SOCKET)
				{
					m_clientSocket = accept(m_listenSocket, NULL, NULL);

					// should not use the client socket
					if (m_clientSocket == INVALID_SOCKET)
					{
						int errorCode = WSAGetLastError();
						DebuggerPrintf(Stringf("Connect failed with error: %i \n", errorCode).c_str());
						// todo: ??? close the failed client socket?
						CloseClientSocketAndSetInvalid();
						m_serverState = ServeState::DISCONNECTED; // go back to listening for connections next frame
					}
					else
					{
						// If accept returned a valid client socket, set it to non-blocking
						unsigned long blockingMode = 1;
						ioctlsocket(m_clientSocket, FIONBIO, &blockingMode);
						m_serverState = ServeState::CONNECTED;
						DebuggerPrintf("Server successfully connected to the client\n");
					}
				}
			} break;
			case ServeState::CONNECTED :
			{
				// If we have a connection, send and receive on our client socket
				int numSentRequested = 0;
				if (m_bytesNeedToSend == 0)
				{
					numSentRequested = 1;
				}
				else
				{
					numSentRequested = m_bytesNeedToSend;
				}
				int sendResult = send(m_clientSocket, m_sendBuffer, numSentRequested, 0);
				if (sendResult == 0)
				{
					// 0 means the connection has been closed.
					// shutdown socket and go back to trying to establish
					CloseClientSocketAndSetInvalid();
					DebuggerPrintf("Sending process failed. Shut down client socket\n");
					m_serverState = ServeState::DISCONNECTED;
				}
				else if (sendResult < 0)
				{
					int errorCode = WSAGetLastError();
					if (errorCode == WSAEWOULDBLOCK)
					{
						// WSAEWOULDBLOCK: The socket is non-blocking, and the send operation would block (try again later).
						DebuggerPrintf("No data to send out, client socket is waiting to send");
					}
					else if (errorCode == WSAECONNABORTED || errorCode == WSAECONNRESET) // means the other end has closed the connection
					{
						// This is not a fatal error but should stop using the client sockets.
						// WSAECONNRESET : The connection was reset by the remote host.
						CloseClientSocketAndSetInvalid();
						DebuggerPrintf("Sending process failed. Shut down client socket\n");
						m_serverState = ServeState::DISCONNECTED;
					}
					else if (errorCode == WSAETIMEDOUT)
					{
						DebuggerPrintf("A timeout occurred during client socket's sending operation");
					}
				}
				else
				{
					DebuggerPrintf(Stringf("Client Socket successfully sending out %i Bytes data \n", sendResult).c_str());
					ClearSendBufferBySendResult(sendResult);
				}

				//----------------------------------------------------------------------------------------------------------------------------------------------------
				// int recvResult = recv(m_clientSocket, m_recvBuffer, sizeof(m_recvBuffer), 0);
				int recvResult = recv(m_clientSocket, m_recvBuffer, m_config.m_recvBufferSize, 0);
				if (recvResult == 0)
				{
					// 0 means the connection has been closed.
					// shutdown socket and go back to trying to establish
					CloseClientSocketAndSetInvalid();
					DebuggerPrintf("Receive process failed. Shut down client socket");
					m_serverState = ServeState::DISCONNECTED;
				}
				else if (recvResult < 0)
				{
					int errorCode = WSAGetLastError();
					if (errorCode == WSAEWOULDBLOCK)
					{
						// WSAEWOULDBLOCK : The socket is non - blocking, and no data is available to read.
						DebuggerPrintf("No data receiving, client socket is waiting to receive\n");
					}
					else if (errorCode == WSAECONNABORTED || errorCode == WSAECONNRESET) // means the other end has closed the connection
					{
						// This is not a fatal error but should stop using the client sockets.
						// WSAECONNRESET: The connection was forcibly closed by the remote host.
						DebuggerPrintf("Receiving process failed. Shut down client socket");
						CloseClientSocketAndSetInvalid();
						m_serverState = ServeState::DISCONNECTED;
					}
					else if (errorCode == WSAETIMEDOUT)
					{
						DebuggerPrintf("A timeout occurred during client socket's receive operation\n");
					}
				}
				else
				{
					DebuggerPrintf(Stringf("Client Socket successfully receive %i Bytes data\n", recvResult).c_str());
					Receive(recvResult);
				}
			} break;
		}
	}
}

void NetSystem::EndFrame()
{
	PackSendQueueWithCommands();
	PrepareSendBuffer();
	ReadCommandFromReceiveQueue();
}

void NetSystem::CloseClientSocketAndSetInvalid()
{
	closesocket(m_clientSocket);
	m_clientSocket = INVALID_SOCKET;
	DebuggerPrintf("Sending process failed. Shut down client socket\n");
}

bool NetSystem::RemoteCommand(EventArgs& args)
{
	std::string command = args.GetValue("command", "Command not undefined");
	std::string info = "Remote command sent: ";

	g_theDevConsole->m_linesMutex.lock();
	g_theDevConsole->m_lines.push_back(DevConsoleLine(info + command, DevConsole::INFO_MINOR));
	g_theDevConsole->m_linesMutex.unlock();

	g_theNetSystem->Send(command);

	return true;
}

bool NetSystem::BurstTest(EventArgs& args)
{
	UNUSED(args);

	for (int i = 0; i < 20; ++i)
	{
		std::string echoMessage = Stringf("echo message = %i", i + 1);
		g_theNetSystem->Send(echoMessage);
	}

	return true;
}

void NetSystem::Send(std::string& data)
{
	if (data.length() <= m_config.m_sendBufferSize)
	{
		g_theNetSystem->m_commandQueue.push_back(data);
	}
	else // Error if the message will not fit in the send buffer
	{
		g_theNetSystem->m_commandQueue.push_back(data);
		DebuggerPrintf(Stringf("The command is larger than the send buffer size : %i", m_config.m_sendBufferSize).c_str());
	}
}

void NetSystem::Receive(int bytesReceived)
{
	// take out the \0 in the data 
	for (int dataIndex = 0; dataIndex < bytesReceived; ++dataIndex)
	{
		if (m_recvBuffer[dataIndex] != '\0') 
		{
			char singleChar = m_recvBuffer[dataIndex];
			m_recvQueue += singleChar;
		}
	}
}

void NetSystem::ClearSendBufferBySendResult(int bytesSent)
{
	if (m_bytesNeedToSend == 0) 
	{
		return;  // we don't want to send any data
	}
	else
	{
		if (bytesSent == (int)m_bytesNeedToSend)
		{
			// we need to reset the send buffer
			std::memset(m_sendBuffer, 0, m_config.m_sendBufferSize);

			m_bytesNeedToSend = 0;
		} 
		else if (bytesSent < (int)m_bytesNeedToSend)
		{
			int unfinishedData = (int)m_bytesNeedToSend - bytesSent;
			ERROR_AND_DIE(Stringf("Bytes sent are not matched with requested: %i, check your net connection", unfinishedData));
			// // Calculate remaining data size in the buffer
			// size_t remainingDataSize = m_bytesNeedToSend - bytesSent;
			// 
			// // Shift the remaining data to the front of the buffer
			// std::memmove(m_sendBuffer, m_sendBuffer + bytesSent, remainingDataSize);
			// 
			// // Null out the remaining part at the end of the buffer
			// std::memset(m_sendBuffer + remainingDataSize, 0, bytesSent);	
			// 
			// m_bytesNeedToSend = m_bytesNeedToSend - bytesSent;
		}
	}
}

void NetSystem::PrepareSendBuffer()
{
	if (m_totalBytesNeedToSend == 0)
	{
		return; // no further quest to send
	}
	else
	{
		// Determine how many bytes to copy (limit to buffer size)
		m_bytesNeedToSend = min(m_totalBytesNeedToSend, (unsigned int)m_config.m_sendBufferSize);
		if (m_bytesNeedToSend ==  (unsigned int)m_config.m_sendBufferSize)
		{
			m_totalBytesNeedToSend -= m_bytesNeedToSend;
		}
		else
		{
			m_totalBytesNeedToSend = 0;
		}

		if (m_totalBytesNeedToSend < 0)
		{
			ERROR_AND_DIE("Total bytes need to send is below zero");
		}

		// Copy the data from the queue to the send buffer
		std::memcpy(m_sendBuffer, m_sendQueue.data(), m_bytesNeedToSend);

		// Shift the remaining data to the front of the buffer
		m_sendQueue.erase(0, m_bytesNeedToSend);
	}
}

//void NetSystem::Send()
//{
//	// Keep sending while there is data to send or if there is unsent data in the buffer
//	while (!m_sendQueue.empty() || m_bytesInBuffer > 0) {
//
//		// Pack the send buffer if there's room
//		if (m_bytesInBuffer < BUFFER_SIZE) {
//			PackSendBuffer();
//		}
//
//		// If there's nothing packed to send, break the loop
//		if (m_bytesInBuffer == 0) {
//			break;
//		}
//
//		// Send the data from the send buffer
//		ssize_t bytesSent = send(m_clientSocket, m_sendBuffer, m_bytesInBuffer, 0);
//		if (bytesSent == -1) {
//			perror("send failed");
//			return;
//		}
//
//		// Handle the case where some or all of the buffer was sent
//		OnDataSent(bytesSent);
//
//		// Continue to check if there's more data to send
//	}
//}

void NetSystem::PackSendQueueWithCommands()
{
	if (!m_commandQueue.empty())
	{
		for (std::string const& command : m_commandQueue)
		{
			m_sendQueue.append(command);
			m_sendQueue.push_back('\r'); // the strings will not automatically include null terminators (\0) between them

			m_totalBytesNeedToSend += ((unsigned int)command.size() + 1);
		}
	}

	m_commandQueue.clear();
	//// Continue packing until the buffer is full or the queue is empty
	//while (!m_sendQueue.empty() && m_bytesInBuffer < BUFFER_SIZE) 
	//{
	//	std::string& front = m_sendQueue.front();

	//	// Calculate how much of this string can be packed into the buffer
	//	size_t bytesAvailable = BUFFER_SIZE - m_bytesInBuffer;
	//	size_t bytesToPack = std::min(front.size() - m_currentPos, bytesAvailable);

	//	// Pack the available portion into the send buffer
	//	memcpy(m_sendBuffer + m_bytesInBuffer, front.data() + m_currentPos, bytesToPack);
	//	m_bytesInBuffer += bytesToPack;
	//	m_currentPos += bytesToPack;

	//	// If the entire current string has been packed, move to the next string
	//	if (m_currentPos == front.size()) {
	//		m_sendQueue.pop();
	//		m_currentPos = 0;  // Reset position for the next string
	//	}

	//	// If the buffer is full, stop packing
	//	if (m_bytesInBuffer == BUFFER_SIZE) {
	//		break;
	//	}
	//}
}

void NetSystem::ReadCommandFromReceiveQueue()
{
	// looking for null terminators '\0'
	size_t nullPos = m_recvQueue.find('\r');
	while (nullPos != std::string::npos) 
	{
		// Extract the command (substring up to null terminator)
		std::string command = m_recvQueue.substr(0, nullPos);
		g_theDevConsole->Execute(command);

		// Remove the executed command and its null terminator
		m_recvQueue.erase(0, nullPos + 1);

		// continuing searching for the next null terminator
		nullPos = m_recvQueue.find('\r');
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
bool NetSystem::IsServer()
{
	return (m_config.m_mode == NetSystemMode::SERVER);
}

bool NetSystem::IsClient()
{
	return (m_config.m_mode == NetSystemMode::CLIENT);
}

bool NetSystem::IsConnected()
{
	if (IsServer())
	{
		return m_serverState == ServeState::CONNECTED;
	}
	else if (IsClient())
	{
		return m_clientState == ClientState::CONNECTED;
	}
	return false;
}

int NetSystem::ConnectClientSocket()
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = htonl(m_hostAddress);
	addr.sin_port = htons(m_hostPort);
	int result = connect(m_clientSocket, (sockaddr*)(&addr), (int)sizeof(addr));
	return result;
}

