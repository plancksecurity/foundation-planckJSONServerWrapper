// encoder/decoder for WebSocket frames according to RFC 6455

#ifndef JSON_ADAPTER_WEBSOCKET_HH
#define JSON_ADAPTER_WEBSOCKET_HH
#include <string>
#include <istream>

namespace ws
{
	enum class Opcode
	{
		Cont   = 0x0,
		Text   = 0x1,
		Binary = 0x2,
		// 2 ... 7: reserved
		Close  = 0x8,
		Ping   = 0x9,
		Pong   = 0xA,
		// B ... F: reserved
		Illegal = 0xFFFF
	};
	
	struct Frame
	{
		Opcode opcode;
		bool final; // if false the next frame has to be added
		std::string data;
	};

	// no XOR masking of the payload, because we are the "server"
	// creates only one "binary" frame (Opcode 2)
	std::string encode_frame(const std::string& payload);
	
	// accepts only XOR-masked payload, because we are the "server"
	Frame decode_frame(std::istream& frame);
}

#endif // JSON_ADAPTER_WEBSOCKET_HH
