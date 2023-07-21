#pragma once

#include <vector>
#include <string>
#include <functional>
#include <JSON.h>
#include <typeinfo> // for types in templates
#include <memory>
#include <DataBuffer.h>
#include <set>
#include "libraries/WSServer/WSServer.h"

// forward declarations

class Gui
{
	private:

		std::vector<DataBuffer> _buffers;
		std::unique_ptr<WSServer> ws_server;
		std::set<const WSServerDetails*> wsConnections;

		void ws_connect(const std::string& address, const WSServerDetails* id);
		void ws_disconnect(const std::string& address, const WSServerDetails* id);
		void ws_onControlData(const std::string& address, const WSServerDetails* id, const unsigned char* data, size_t size);
		void ws_onData(const std::string& address, const WSServerDetails* id, const unsigned char* data, size_t size);
		int doSendBuffer(const char* type, unsigned int bufferId, const void* data, size_t size);

		unsigned int _port;
		std::string _addressControl;
		std::string _addressData;
		std::wstring _projectName;

		// User defined functions
		std::function<bool(JSONObject&, void*)> customOnControlData;
		std::function<bool(const std::string&, const WSServerDetails*, const unsigned char*, size_t, void*)> customOnData;

		void* controlCallbackArg = nullptr;
		void* binaryCallbackArg = nullptr;

	public:
		Gui();
		Gui(unsigned int port, std::string address);
		~Gui();

		int setup(unsigned int port = 5555, std::string address = "gui");
		/**
		 * Sets the web socket communication between server and client.
		 * Two different web socket connections will be configured, one for control data and the other one for raw binary data. 
		 * @param port Port on which to stablish the the web socket communication.
		 * @param address Base address used to stalish the web socket communication.
		 * @param projectName Project name to be sent to via the web-socket to the client.
		 * @returns 0 if web sockets have been configured.
		 **/
		int setup(std::string projectName, unsigned int port = 5555, std::string address = "gui");
		void cleanup();

		size_t numConnections(){ return wsConnections.size(); };

		// BUFFERS
		/**
		 * Sets the buffer type and size of the container.
		 * @param bufferType type of the buffer, can be float (f), int (i) or char (c)
		 * @param size Maximum number of elements that the buffer can hold.
		 * @returns Buffer ID, generated automatically based on the number of buffers and
		 * 	the order on which they have been created.
		 **/
		unsigned int setBuffer(char bufferType, unsigned int size);
		/**
		 * Get the DataBuffer object corresponding to the identifier that was assigned to it during creation
		 * @param bufferId: buffer ID
		 **/
		DataBuffer& getDataBuffer(unsigned int bufferId);

		/**
		 * Set callback to parse control data received from the client.
		 *
		 * @param callback Callback to be called whenever new control
		 * data is received.
		 * The callback takes a JSONObject, and an opaque pointer, which is
		 * passed at the moment of registering the callback.
		 * The callback should return `true` if the default callback should
		 * be called afterward or `false` otherwise.
		 *
		 * @param callback the function to be called upon receiving data on the
		 * control WebSocket
		 * @param callbackArg an opaque pointer that will be passed to the
		 * callback
		 **/
		void setControlDataCallback(std::function<bool(JSONObject&, void*)> callback, void* callbackArg=nullptr){
			customOnControlData = callback;
			controlCallbackArg = callbackArg;
		};

		/**
		 * Set callback to parse binary data received from the client.
		 *
		 * @param callback Callback to be called whenever new binary
		 * data is received.
		 * It takes a byte buffer, the size of the buffer and a pointer
		 * as parameters, returns `true `if the default callback should
		 * be called afterward or `false` otherwise. The first two
		 * parameters to the callback are a pointer to and the size of the
		 * data received on the web-socket. The third parameter is a
		 * user-defined opaque pointer.
		 *
		 * @param callbackArg: Pointer to be passed to the
		 * callback.
		 **/
		void setBinaryDataCallback(std::function<bool(const std::string&, const WSServerDetails* id, const unsigned char*, size_t, void*)> callback, void* callbackArg=nullptr){
			customOnData = callback;
			binaryCallbackArg = callbackArg;
		};
		/** Sends a JSON value to the control websocket.
		 * @returns 0 on success, or an error code otherwise.
		 * */
		int sendControl(JSONValue* root, WSServer::CallingThread callingThread);

		/**
		 * Sends a buffer (a vector) through the web-socket to the client with a given ID.
		 * @param bufferId Given buffer ID
		 * @param buffer Vector of arbitrary length and type
		 **/
		template<typename T, typename A>
		int sendBuffer(unsigned int bufferId, std::vector<T,A> & buffer);
		/**
		 * Sends a buffer (an array) through the web-socket to the client with a given ID.
		 * @param bufferId Given buffer ID
		 * @param buffer Array of arbitrary size and type
		 **/
		template <typename T, size_t N>
		int sendBuffer(unsigned int bufferId, T (&buffer)[N]);
		/**
		 * Sends a buffer (pointer) of specified length through the
		 * websocket to the client with a given ID.
		 * @param bufferId Buffer ID
		 * @param buffer Pointer to the location of memory to send
		 * @param count number of elements to send
		 */
		template <typename T>
		int sendBuffer(unsigned int bufferId, T* ptr, size_t count);
		/**
		 * Sends a single value through the web-socket to the client with a given ID.
		 * @param bufferId Given buffer ID
		 * @param value of arbitrary type
		 **/
		template <typename T>
		int sendBuffer(unsigned int bufferId, T value);
};

template<typename T, typename A>
int Gui::sendBuffer(unsigned int bufferId, std::vector<T,A> & buffer)
{
	const char* type = typeid(T).name();
	return doSendBuffer(type, bufferId, (const void*)buffer.data(), (buffer.size()*sizeof(T)));
}

template <typename T, size_t N>
int Gui::sendBuffer(unsigned int bufferId, T (&buffer)[N])
{
	const char* type = typeid(T).name();
	return doSendBuffer(type, bufferId, (const void*)buffer, N*sizeof(T));
}

template <typename T>
int Gui::sendBuffer(unsigned int bufferId, T* ptr, size_t count){
	const char* type = typeid(T).name();
	return doSendBuffer(type, bufferId, ptr, count * sizeof(T));
}

template <typename T>
int Gui::sendBuffer(unsigned int bufferId, T value)
{
	const char* type = typeid(T).name();
	return doSendBuffer(type, bufferId, (const void*)&value, sizeof(T));
}
