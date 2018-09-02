#ifndef HTTPS_HTTPCONNECTION_HPP_
#define HTTPS_HTTPCONNECTION_HPP_

#include <Arduino.h>

#include <string>

// Required for sockets
#include "lwip/netdb.h"
#undef read
#include "lwip/sockets.h"

#include "HTTPSServerConstants.hpp"
#include "HTTPHeaders.hpp"
#include "HTTPHeader.hpp"
#include "ResourceResolver.hpp"
#include "ResolvedResource.hpp"
#include "ResourceNode.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

namespace httpsserver {

class HTTPConnection : private ConnectionContext {
public:
	HTTPConnection(ResourceResolver * resResolver);
	virtual ~HTTPConnection();

	virtual int initialize(int serverSocketID, HTTPHeaders *defaultHeaders);
	virtual void closeConnection();
	virtual bool isSecure();

	void loop();
	bool isClosed();
	bool isError();

protected:
	friend class HTTPRequest;
	friend class HTTPResponse;

	virtual size_t writeBuffer(byte* buffer, size_t length);
	virtual size_t readBytesToBuffer(byte* buffer, size_t length);
	virtual size_t pendingByteCount();

	// Timestamp of the last transmission action
	unsigned long _lastTransmissionTS;

	// Timestamp of when the shutdown was started
	unsigned long _shutdownTS;

	// Internal state machine of the connection:
	//
	// O --- > STATE_UNDEFINED -- initialize() --> STATE_INITIAL -- get / http/1.1 --> STATE_REQUEST_FINISHED --.
	//                     |                          |                                       |                 |
	//                     |                          |                                       |                 | Host: ...\r\n
	// STATE_ERROR <- on error-----------------------<---------------------------------------<                  | Foo: bar\r\n
	// ^                                              |                                       |                 | \r\n
	// | shutdown   .--> STATE_CLOSED                 |                                       |                 | \r\n
	// | fails     |                                  |                                       |                 |
	// |           | close()                          |                                       |                 |
	// STATE_CLOSING <---- STATE_WEBSOCKET <-.        |                                       |                 |
	//  ^                                    |        |                                       |                 |
	//  `---------- close() ---------- STATE_BODY_FINISHED <-- Body received or GET -- STATE_HEADERS_FINISHED <-´
	//
	enum {
		// The order is important, to be able to use state <= STATE_HEADERS_FINISHED etc.

		// The connection has not been established yet
		STATE_UNDEFINED,
		// The connection has just been created
		STATE_INITIAL,
		// The request line has been parsed
		STATE_REQUEST_FINISHED,
		// The headers have been parsed
		STATE_HEADERS_FINISHED,
		// The body has been parsed/the complete request has been processed (GET has body of length 0)
		STATE_BODY_FINISHED,
		// The connection is in websocket mode
		STATE_WEBSOCKET,
		// The connection is about to close (and waiting for the client to send close notify)
		STATE_CLOSING,
		// The connection has been closed
		STATE_CLOSED,
		// An error has occured
		STATE_ERROR
	} _connectionState;

	enum {
		CSTATE_UNDEFINED,
		CSTATE_ACTIVE,
		CSTATE_CLOSED
	} _clientState;

private:
	void serverError();
	void clientError();
	void readLine(int lengthLimit);

	bool isTimeoutExceeded();
	void refreshTimeout();

	int updateBuffer();
	size_t pendingBufferSize();

	void signalClientClose();
	void signalRequestError();
	size_t readBuffer(byte* buffer, size_t length);
	size_t getCacheSize();


	// The receive buffer
	char _receiveBuffer[HTTPS_CONNECTION_DATA_CHUNK_SIZE];

	// First index on _receive_buffer that has not been processed yet (anything before may be discarded)
	int _bufferProcessed;
	// The index on the receive_buffer that is the first one which is empty at the end.
	int _bufferUnusedIdx;

	// Socket address, length etc for the connection
	struct sockaddr _sockAddr;
	socklen_t _addrLen;
	int _socket;

	// Resource resolver used to resolve resources
	ResourceResolver * _resResolver;

	// The parser line. The struct is used to read the next line up to the \r\n in readLine()
	struct {
		std::string text = "";
		bool parsingFinished = false;
	} _parserLine;

	// HTTP properties: Method, Request, Headers
	std::string _httpMethod;
	std::string _httpResource;
	HTTPHeaders * _httpHeaders;

	// Default headers that are applied to every response
	HTTPHeaders * _defaultHeaders;

	// Should we use keep alive
	bool _isKeepAlive;


};

} /* namespace httpsserver */

#endif /* HTTPS_HTTPCONNECTION_HPP_ */