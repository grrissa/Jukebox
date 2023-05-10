#include <iostream>

#include <cstring>

#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ChunkedDataSender.h"
#include "ConnectedClient.h"

using std::cout;
using std::cerr;

ConnectedClient::ConnectedClient(int fd, ClientState initial_state) :
	client_fd(fd), sender(NULL), state(initial_state) {}

void ConnectedClient::send_dummy_response(int epoll_fd) {
	// Create a large array, just to make sure we can send a lot of data in
	// smaller chunks.
	char *data_to_send = new char[CHUNK_SIZE*2000];
	memset(data_to_send, 117, CHUNK_SIZE*2000); // 117 is ascii 'u'

	ArraySender *array_sender = new ArraySender(data_to_send, CHUNK_SIZE*2000);
	delete[] data_to_send; // The ArraySender creates its own copy of the data so let's delete this copy

	ssize_t num_bytes_sent;
	ssize_t total_bytes_sent = 0;

	// keep sending the next chunk until it says we either didn't send
	// anything (0 return indicates nothing left to send) or until we can't
	// send anymore because of a full socket buffer (-1 return value)
	while((num_bytes_sent = array_sender->send_next_chunk(this->client_fd)) > 0) {
		total_bytes_sent += num_bytes_sent;
	}
	cout << "sent " << total_bytes_sent << " bytes to client\n";

	/*
	 * TODO: if the last call to send_next_chunk indicated we couldn't send
	 * anything because of a full socket buffer, we should do the following:
	 *
	 * 1. update our state field to be sending
	 * 2. set our sender field to be the ArraySender object we created
	 * 3. update epoll so that it also watches for EPOLLOUT for this client
	 *    socket (use epoll_ctl with EPOLL_CTL_MOD).
	 *
	 * WARNING: These steps are to be done inside of the following if statement,
	 * not before it.
	 */
	if (num_bytes_sent < 0) {
		// Fill this in with the three steps listed in the comment above.
		// WARNING: Do NOT delete array_sender here (you'll need it to continue
		// sending later).
		this->state = SENDING;
		this->sender = array_sender;

		// QUESTION
		struct epoll_event epoll_out;
        epoll_out.data.fd = this->client_fd;
        epoll_out.events = EPOLLOUT;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, this->client_fd, &epoll_out) == -1) {
            perror("send_dummy_response epoll_ctl");
            exit(EXIT_FAILURE);
        }
    }


	else {
		// Sent everything with no problem so we are done with our ArraySender
		// object.
		delete array_sender;
	}
}

void ConnectedClient::handle_input(int epoll_fd) {
	// QUESTION: so this is the driver... we are doing all of the receiving in here and
	// then calling send dummy to send the actual data that theyre req?????? how do we pass that data onto send dummy 
	// should we create a function for each request type from the client

	cout << "Ready to read from client " << this->client_fd << "\n";
	char data[1024];
	Header* hdr = (Header*)data;    
	// added

	// until here
	ssize_t bytes_received = recv(this->client_fd, data, 1024, 0);
	if (bytes_received < 0) {
		perror("client_read recv");
		exit(EXIT_FAILURE);
	}

	cout << "Received data: ";
	for (int i = 0; i < bytes_received; i++)
		cout << data[i];

	cout << "\n";

	// TODO: Eventually you need to actually look at the response and send a
	// response based on what you got from the client (e.g. did they ask for a
	// list of songs or for you to send them a song?)
	// For now, the following function call just demonstrates how you might
	// send data.

	if (hdr->type == PLAY){
        // this->play_response(epoll_fd)
		// from the CLIENT SIDE... is it just going to play the song even if it hasnt received eerything?
		// how do we implement the STOP... inside play reponse going to wait for a stop to stop sending chunks? how does this work
    } else if (hdr->type == INFO) {
		// this->info_response(epoll_fd)
	} else if (hdr->type == LIST) {
		// this->list_response(epoll_fd)

	}else if (hdr->type == STOP) {
		// this->stop_response(epoll_fd)

	}else if (hdr->type == DISCONNECT) {
		// this->disconnect

	}

	// QUESTION: just want to make sure we odn't have to send eveything from this. this is j a placeholder
	this->send_dummy_response(epoll_fd);
}


// You likely should not need to modify this function.
void ConnectedClient::handle_close(int epoll_fd) {
	cout << "Closing connection to client " << this->client_fd << "\n";

	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, this->client_fd, NULL) == -1) {
		perror("handle_close epoll_ctl");
		exit(EXIT_FAILURE);
	}

	close(this->client_fd);
}

