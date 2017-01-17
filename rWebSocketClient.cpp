#include "B4RDefines.h"
namespace B4R {
	void B4RWebSocketClient::Initialize (SubVoidStringArray NewMessageSub, SubVoidVoid DisconnectedSub) {
		this->NewMessageSub = NewMessageSub;
		this->DisconnectedSub = DisconnectedSub;
	}
	bool B4RWebSocketClient::connect(B4RStream* Stream, const char* host, B4RString* Path) {
		webClient.path = Path->data;
		webClient.host = host;
		FunctionUnion fu;
		fu.PollerFunction = looper;
		pnode.functionUnion = fu;
		pnode.tag = this;
		bool b = webClient.handshake(*Stream->wrappedClient);
		if (b) {
			if (pnode.next == NULL)
				pollers.add(&pnode);
		}
		return b;
	}
	bool B4RWebSocketClient::ConnectHost (B4RStream* Stream, B4RString* Host, UInt Port, B4RString* Path) {
		if (!Stream->wrappedClient->connect(Host->data, Port))
			return false;
		return connect(Stream, Host->data, Path);
	}
	bool B4RWebSocketClient::ConnectIP (B4RStream* Stream, ArrayByte* IP, UInt Port, B4RString* Path) {
		IPAddress ip((Byte*)IP->data);
		if (!Stream->wrappedClient->connect(ip, Port))
			return false;
		return connect(Stream, "host", Path);
	}
	void B4RWebSocketClient::SendEventToServer (B4RString* Event, ArrayString* DataMap) {
		webClient.sendData(Event, DataMap);
		
	}
	void B4RWebSocketClient::looper(void* b) {
		B4RWebSocketClient* me = (B4RWebSocketClient*)b;
		if (me->webClient.socket_client->connected() == false) {
			pollers.remove(&me->pnode);
			me->webClient.socket_client->stop();
			if (me->DisconnectedSub != NULL)
				me->DisconnectedSub();
		} else {
			if (me->webClient.socket_client->available()) {
				B4RString strings[11];
				const UInt cp = B4R::StackMemory::cp;
				AlignCP;
				byte numberOfElements = me->webClient.handleStream(strings);
				if (numberOfElements > 0) {
					B4RString* pstrings[numberOfElements];
					Array arrString;
					Array* arr = arrString.wrapObjects((void**)pstrings, &strings[1], 
						numberOfElements - 1, sizeof(B4RString));
					me->NewMessageSub(&strings[0], arr);

				}
				StackMemory::cp = cp;
			}
			
		}
	}
	
}