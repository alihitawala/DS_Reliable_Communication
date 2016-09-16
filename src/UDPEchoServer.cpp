//============================================================================
// Name        : UDPEchoServer.cpp
// Author      : Bruce
//============================================================================

#include <cstdlib>
#include <iostream>
#include <string>
#include <ctime>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "Packet.h"

using namespace std;
static int global_seq_num = 0;

void identify(char *exeName)
{
	cout << "---------------------------------------------------" << endl;
	cout << "Executable name:  " << exeName << endl;
	cout << "Source file name: " << __FILE__ << endl;
	cout << "---------------------------------------------------" << endl;
}

class UDPServer {
private:
	static const short unsigned int BufferSize = 255;
	uint16_t m_port;
	char m_buffer[BufferSize];
	int m_socket;
	int m_drop_probability;
	bool m_alreadyBound;

	// clear the data buffer
	void clearBuffer(){
		memset(m_buffer, 0, BufferSize);
	}
public:
	// a class for error-exceptions
	class Error {
	private:
		string m_errstr;
		int m_errno;
	public:
		Error(string errstr, int e) : m_errstr(errstr), m_errno(e) {}
		void print(){
			cout << m_errstr << " (" << m_errno << ")" << endl;
		}
	};

	// specify a port to bind in constructor
	UDPServer(int drop_probability=0, uint16_t port = 1234) : m_port(port), m_socket(-1),
											m_alreadyBound(false), m_drop_probability(drop_probability){}

	// bind server to port for any address
	void bind();
	// receive data and provided it in internal buffer
	const char *receive(sockaddr_in* clientAddress, unsigned int len);
	const Packet *receive_reliable();
	void do_processing(const Packet*);
	bool should_drop();
	void send_ack(sockaddr_in clientAddress, unsigned int len);
};

void UDPServer::bind()
{
	struct sockaddr_in serverAddress;
	const int y = 1;

	if(m_alreadyBound){
		return;
	}

	// create socket
	m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(m_socket < 0){
		throw Error(string("ERROR: cannot open socket"), errno);
	}

	// bind correct server port
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl (INADDR_ANY);
	serverAddress.sin_port = htons(m_port);
	setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
	int rc = ::bind(m_socket, (struct sockaddr *) &serverAddress, sizeof (serverAddress));
	if(rc != 0){
		throw Error(string("ERROR: cannot bind port number"), errno);
	}

	// no error => server is bound
	m_alreadyBound = true;
}

const char *UDPServer::receive(sockaddr_in* clientAddress, unsigned int len)
{
	// throw exception, if server not yet bound
	if(!m_alreadyBound){
		throw Error(string("ERROR: server not yet bound"), errno);
	}

	// clear buffer and receive messages
	clearBuffer();
	int n = recvfrom(m_socket, m_buffer, BufferSize, 0, (struct sockaddr *) clientAddress, &len );
	if (n < 0) {
		// received dirty data
		return NULL;
	}
	return m_buffer;
}

bool UDPServer::should_drop() {
	srand((int)time(0));
	int r = (rand() % 100) + 1;
	return r <= m_drop_probability;
}

void UDPServer::send_ack(sockaddr_in clientAddress, unsigned int len) {
	char *ack = (char*) "ACK";
	sendto(m_socket, ack, 4, 0, (struct sockaddr *) &clientAddress, len);
}

const Packet *UDPServer::receive_reliable()
{
	Packet* result;
	struct sockaddr_in clientAddress;
	while (1) {
		this->receive(&clientAddress, sizeof (clientAddress));
		if (!should_drop()) {
			result = (Packet*) m_buffer;
			if (result->seq_num > global_seq_num) {
				global_seq_num = result->seq_num;
				this->send_ack(clientAddress, sizeof (clientAddress));
				return result;
			} else {
				cout << "Retransmission received!" << endl;
				this->send_ack(clientAddress, sizeof (clientAddress));
			}
		} else {
			return NULL;
		}
	}

}

void UDPServer::do_processing(const Packet* pkt) {
	//print only if new seq number
	cout << pkt->seq_num << pkt->data << pkt->length <<endl;
	cout << "Received data :|" << pkt->data << "|" << endl;
}

int main(int argc, char* argv[]) {
	// output name
	identify(argv[0]);

	// parse command line
	unsigned short int port = 5908;
	unsigned short int drop_p = 50;
	if(argc > 1){
		drop_p = atoi(argv[1]);
	}
	if(argc > 2){
		port = atoi(argv[2]);
	}

	UDPServer server(drop_p, port);
	try{
		server.bind();
		while(1){
			const Packet *packet = server.receive_reliable();
			if (packet){
				server.do_processing(packet);
			} else {
				cout << "Packet dropped!" << endl;
			}
		}
	}catch(UDPServer::Error &err){
		err.print();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
