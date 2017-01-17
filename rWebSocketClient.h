#pragma once
#include "B4RDefines.h"
#include "WebSocketClient.h"
//~version: 1.20
namespace B4R {
	/**
	*WebSocket client implementation. It is intended to work with B4J WebSocket servers.
	*/
	//~shortname: WebSocketClient
	//~event: Disconnected
	//~event: NewMessage (FunctionName As String, Params() As String)
	class B4RWebSocketClient {
		private:
			WebSocketClient webClient;
			PollerNode pnode;
			static void looper(void* b);
			bool connect(B4RStream* Stream, const char* host, B4RString* Path);
			typedef void (*SubVoidStringArray)(B4RString* func, Array* barray) ;
			SubVoidStringArray NewMessageSub;
			SubVoidVoid DisconnectedSub;
		public:
			//Initializes the object and set the NewMessage and Disconnected subs.
			void Initialize (SubVoidStringArray NewMessageSub, SubVoidVoid DisconnectedSub);
			/**
			*Tries to connect to the server. Returns true if connection was successful.
			*Stream - Stream of an unconnected client.
			*Host - Host name.
			*Port - Port number.
			*Path - WebSocket handler path.
			*/
			bool ConnectHost (B4RStream* Stream, B4RString* Host, UInt Port, B4RString* Path);
			/**
			*Tries to connect to the server. Returns true if connection was successful.
			*Stream - Stream of an unconnected client.
			*IP - Server ip address as an array of bytes.
			*Port - Port number.
			*Path - WebSocket handler path.
			*/
			bool ConnectIP (B4RStream* Stream, ArrayByte* IP, UInt Port, B4RString* Path);
			/**
			*Raises an event on the server.
			*Event - Event name. Must include an underscore.
			*DataMap - An array of strings with the parameters. The parameters will be converted to a 
			*	key / value map on the server side. Number of parameters must be even.
			*	Quotes are not allowed.
			*/
			void SendEventToServer (B4RString* Event, ArrayString* DataMap);
	};
}