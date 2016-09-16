//============================================================================
// Name        : UDPEchoClient.cpp
// Author      : Bruce
//============================================================================

#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include "Packet.h"
#define SERVER_PORT 5908
#define BUFFER_LEN 255

using namespace std;

void identify(char *exeName) {
	cout << "---------------------------------------------------" << endl;
	cout << "Executable name:  " << exeName << endl;
	cout << "Source file name: " << __FILE__ << endl;
	cout << "---------------------------------------------------" << endl;
}
static int global_seq_num = 0;

int send_reliable(int socket, char* buffer, int buffer_len, struct sockaddr_in remoteServAddr){
	int rc, n;
	unsigned int len = sizeof (remoteServAddr);
	struct sockaddr_in clientAddress;
	char ack_buffer[BUFFER_LEN];

	start_again:

	rc = sendto(socket, buffer, buffer_len, 0, (struct sockaddr *) &remoteServAddr, len);
	// wait for ACK
	n = recvfrom(socket, &ack_buffer[0], BUFFER_LEN, 0, (struct sockaddr *) &remoteServAddr, &len );
	if (n > 0) {
		cout << "got ack for data : " << n << ack_buffer << endl;
	} else {
		cout << "timeout" << n << endl;
		goto start_again;
	}
	return 10;
}

void check_args(int argc, char* argv[]) {
	// check command line
	if (argc < 2) {
		cout << "Usage: " << argv[0] << " <server> " << endl;
		exit(1);
	}
}

int establish_connection(char* hostname, struct sockaddr_in* remoteServAddr) {
	struct sockaddr_in cliAddr;
	struct hostent *h;
	int s;
	// get IP address from host name
	h = gethostbyname(hostname);
	if (h == NULL) {
		cout << "unknown host '" << hostname << "'" << endl;
		exit(1);
	}

	// set port and address
	remoteServAddr->sin_family = h->h_addrtype;
	memcpy((char *) &remoteServAddr->sin_addr.s_addr, h->h_addr_list[0], h->h_length);
	remoteServAddr->sin_port = htons(SERVER_PORT);

	// create socket
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		cout << "cannot open socket (" << strerror(errno) << ")" << endl;
		exit(1);
	}
	//socket opt
	struct timeval read_timeout;
	read_timeout.tv_sec = 5;
	read_timeout.tv_usec = 10;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

	// bind any port
	cliAddr.sin_family = AF_INET;
	cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	cliAddr.sin_port = htons(0);
	int rc = bind(s, (struct sockaddr *) &cliAddr, sizeof(cliAddr));
	if (rc < 0) {
		cout << "cannot bind port (" << strerror(errno) << ")" << endl;
		exit(1);
	}
	return s;
}

int connect(int argc, char* argv[]) {
	struct sockaddr_in remoteServAddr;
	check_args(argc, argv);
	int s = establish_connection(argv[1], &remoteServAddr);
	// send data
	string input;
	while (1) {
		cin >> input;
        Packet packet;
        packet.length = input.length() + 1;
        packet.seq_num = ++global_seq_num;
        strcpy(packet.data, input.c_str());
        int rc = send_reliable(s, (char *) &packet, sizeof(int) * 2 + sizeof(packet.data), remoteServAddr);
		if (rc < 0) {
			cout << argv[0] << ": cannot send data " << input << endl;
			close(s);
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
	// output name
	identify(argv[0]);

	// send strings from command line
	int ret = connect(argc, argv);

	return ret;
}
