//#define DEBUGGING
#include "B4RDefines.h"
#include "global.h"
#include "WebSocketClient.h"

bool WebSocketClient::handshake(Client &client) {

    socket_client = &client;

    // If there is a connected client->
    if (socket_client->connected()) {
        // Check request and look for websocket handshake
#ifdef DEBUGGING
            Serial.println(F("Client connected"));
#endif
        if (analyzeRequest()) {
#ifdef DEBUGGING
                Serial.println(F("Websocket established"));
#endif

                return true;

        } else {
            // Might just need to break until out of socket_client loop.
#ifdef DEBUGGING
            Serial.println(F("Invalid handshake"));
#endif
            disconnectStream();

            return false;
        }
    } else {
        return false;
    }
}

bool WebSocketClient::analyzeRequest() {

    int bite;
    randomSeed(analogRead(0));

#ifdef DEBUGGING
    Serial.println(F("Sending websocket upgrade headers"));
#endif    
    socket_client->print(F("GET "));
    socket_client->print(path);
    socket_client->print(F(" HTTP/1.1\r\n"));
    socket_client->print(F("Upgrade: websocket\r\n"));
    socket_client->print(F("Connection: Upgrade\r\n"));
    socket_client->print(F("Host: "));
    socket_client->print(host);
    socket_client->print(CRLF); 
    socket_client->print(F("Sec-WebSocket-Key: "));
    socket_client->print(F("GDFkRWErJpefcUw6Q2O1/w=="));
    socket_client->print(CRLF);
    socket_client->print(F("Sec-WebSocket-Protocol: "));
    // socket_client->print(protocol);
    socket_client->print(CRLF);
    socket_client->print(F("Sec-WebSocket-Version: 13\r\n"));
    socket_client->print(CRLF);
	socket_client->flush();
#ifdef DEBUGGING
    Serial.println(F("Analyzing response headers"));
#endif    

    while (socket_client->connected() && !socket_client->available()) {
        delay(100);
        //Serial.println("Waiting...");
    }

    // TODO: More robust string extraction
    while ((bite = socket_client->read()) != -1) {

        // temp += (char)bite;

        // if ((char)bite == '\n') {
// #ifdef DEBUGGING
            // Serial.print("Got Header: " + temp);
// #endif
            // if (!foundupgrade && temp.startsWith("Upgrade: websocket")) {
                // foundupgrade = true;
            // } else if (temp.startsWith("Sec-WebSocket-Accept: ")) {
                // serverKey = temp.substring(22,temp.length() - 2); // Don't save last CR+LF
            // }
            // temp = "";		
        // }

        if (!socket_client->available()) {
          delay(20);
        }
    }
	return true;
    // key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    // uint8_t *hash;
    // char result[21];
    // char b64Result[30];

    // Sha1.init();
    // Sha1.print(key);
    // hash = Sha1.result();

    // for (int i=0; i<20; ++i) {
        // result[i] = (char)hash[i];
    // }
    // result[20] = '\0';

    // base64_encode(b64Result, result, 20);

    // // if the keys match, good to go
    // return serverKey.equals(String(b64Result));
}


byte WebSocketClient::handleStream(B4R::B4RString strings[]) {
    unsigned int length;
    uint8_t mask[4];
    unsigned int i;
    bool hasMask = false;

    /*msgtype = */timedRead();
    if (!socket_client->connected()) {
        return 0;
    }

    length = timedRead();

    if (length & WS_MASK) {
        hasMask = true;
        length = length & ~WS_MASK;
    }


    if (!socket_client->connected()) {
        return 0;
    }


    if (length == WS_SIZE16) {
        length = timedRead() << 8;
        if (!socket_client->connected()) {
            return 0;
        }
            
        length |= timedRead();
        if (!socket_client->connected()) {
            return 0;
        }   

    } else if (length == WS_SIZE64) {
#ifdef DEBUGGING
        Serial.println(F("No support for over 16 bit sized messages"));
#endif
        return 0;
    }

    if (hasMask) {
        // get the mask
        mask[0] = timedRead();
        if (!socket_client->connected()) {
            return 0;
        }

        mask[1] = timedRead();
        if (!socket_client->connected()) {

            return 0;
        }

        mask[2] = timedRead();
        if (!socket_client->connected()) {
            return 0;
        }

        mask[3] = timedRead();
        if (!socket_client->connected()) {
            return 0;
        }
    }
	bool inQuote = false;
	byte elementsInArray = 0;
	UInt numberOfElements = 0;
	
	bool interestingElement = false;
    for (i=0; i<length; ++i) {
		char c;
		if (hasMask)
			c = (char) (timedRead() ^ mask[i % 4]);
		else
			c = (char) timedRead();
		if (c == '\"') {
			inQuote = !inQuote;
			if (inQuote) {
				numberOfElements++;
				if (numberOfElements == 2 || numberOfElements > 5) {
					{
						using namespace B4R;
						AlignCP;
					}
					strings[elementsInArray].data = (char*)(B4R::StackMemory::buffer + B4R::StackMemory::cp);
					interestingElement = true;
				} else
					interestingElement = false;
			} else {
				if (interestingElement) {
					*(char*)(B4R::StackMemory::buffer + B4R::StackMemory::cp++) = 0;
					elementsInArray = Common_Min(11 - 1, elementsInArray + 1);
				}
			}
			continue;
		}
		if (inQuote && (numberOfElements == 2 || numberOfElements > 5)) {
			*(char*)(B4R::StackMemory::buffer + B4R::StackMemory::cp++) = c;
		}
		
		
		if (!socket_client->connected()) {
			return 0;
		}
	}   
    return elementsInArray;
}

void WebSocketClient::disconnectStream() {
#ifdef DEBUGGING
    Serial.println(F("Terminating socket"));
#endif
    // Should send 0x8700 to server to tell it I'm quitting here.
    socket_client->write((uint8_t) 0x87);
    socket_client->write((uint8_t) 0x00);
    
    socket_client->flush();
    delay(10);
    socket_client->stop();
}


void WebSocketClient::sendData(B4R::B4RString* Event, B4R::ArrayString* DataMap) {
	uint8_t mask[4];
    UInt size = 32 + Event->getLength() + DataMap->length * 3;
	for (UInt i = 0;i < DataMap->length;i++) {
		B4R::B4RString* s = ((B4R::B4RString**)DataMap->data)[i];
		size += s->getLength();
	}
    // Opcode; final fragment
    socket_client->write(WS_OPCODE_TEXT | WS_FIN);
    // NOTE: no support for > 16-bit sized messages
    if (size > 125) {
        socket_client->write(WS_SIZE16 | WS_MASK);
        socket_client->write((uint8_t) (size >> 8));
        socket_client->write((uint8_t) (size & 0xFF));
    } else {
        socket_client->write((uint8_t) size | WS_MASK);
    }

    mask[0] = random(0, 256);
    mask[1] = random(0, 256);
    mask[2] = random(0, 256);
    mask[3] = random(0, 256);
    
    socket_client->write(mask[0]);
    socket_client->write(mask[1]);
    socket_client->write(mask[2]);
    socket_client->write(mask[3]);
    UInt counter = 0;
	const char* prefix1 = "{type:\"event\",event:\"";
    for (UInt i=0; i < 21; ++i) {
        socket_client->write(prefix1[i] ^ mask[counter++ % 4]);
    }
	UInt len = Event->getLength();
	for (UInt i=0; i<len; ++i) {
        socket_client->write(Event->data[i] ^ mask[counter++ % 4]);
    }
	prefix1 = "\",params:{";
	for (UInt i = 0; i < 10; ++i) {
        socket_client->write(prefix1[i] ^ mask[counter++ % 4]);
    }
	for (UInt i = 0;i <= DataMap->length - 1;i += 2) {
		if (i > 0)
			socket_client->write(',' ^ mask[counter++ % 4]);
		B4R::B4RString* a = ((B4R::B4RString**)DataMap->data)[i];
		B4R::B4RString* b = ((B4R::B4RString**)DataMap->data)[i + 1];
		UInt alen = a->getLength();
		socket_client->write('\"' ^ mask[counter++ % 4]);
		for (UInt ii=0; ii<alen; ++ii) {
			socket_client->write(a->data[ii] ^ mask[counter++ % 4]);
		}
		socket_client->write('\"' ^ mask[counter++ % 4]);
		socket_client->write(':' ^ mask[counter++ % 4]);
		socket_client->write('\"' ^ mask[counter++ % 4]);
		UInt blen = b->getLength();
		for (UInt ii=0; ii<blen; ++ii) {
			socket_client->write(b->data[ii] ^ mask[counter++ % 4]);
		}
		socket_client->write('\"' ^ mask[counter++ % 4]);
	}
	socket_client->write('}' ^ mask[counter++ % 4]);
	socket_client->write('}' ^ mask[counter++ % 4]);
	socket_client->flush();
}


int WebSocketClient::timedRead() {
	Byte attempts = 5;
	while (!socket_client->available() && attempts-- > 0) {
		delay(20);  
	}

	return socket_client->read();
}

