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

void ConnectedClient::play_response(int epoll_fd, int song_num, const char *dir) {
	// REMEMBER: first send the amnt of bytes that the song is going to be

	// HELLO: figure out how to retrieve song (this is dummy)
	int song_num_bytes = 0;

	// send the song length
	char segment[sizeof(Header)];
	memset(segment, 0, sizeof(Header));
	Header* hdr = (RDTHeader*)segment;

	hdr1->type = SONG_LEN;
	hdr->song_num = htonl(song_num_bytes);

	// QUESTION: is this sock_fd.. or epoll_fd?
	if (send(this->sock_fd, segment, sizeof(Header), 0) < 0) {
		perror("sending song length");
		exit(EXIT_FAILURE);
	}


	// send the song in chunks
	char *song_data = new char[song_num_bytes];
	memset(song_data, 0, song_num_bytes);

	// HELLO: REMEMBER THIS WILL HCNAGE TO FILE SENDER (where do we define this?)
	ArraySender *array_sender = new ArraySender(song_data, song_num_bytes);
	delete[] song_data; // The ArraySender creates its own copy of the data so let's delete this copy

	ssize_t num_bytes_sent;
	ssize_t total_bytes_sent = 0;

	// keep sending the next chunk until it says we either didn't send
	// anything (0 return indicates nothing left to send) or until we can't
	// send anymore because of a full socket buffer (-1 return value)
	while((num_bytes_sent = array_sender->send_next_chunk(this->client_fd)) > 0) {
		total_bytes_sent += num_bytes_sent;
	}
	cout << "sent " << total_bytes_sent << " bytes to client\n";

	// if theres a full socket buffer
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
    } else {
		// Sent everything with no problem so we are done with our ArraySender
		// object.
		delete array_sender;
	}

}

void ConnectedClient::info_response(int epoll_fd, int song_num, const char *dir) {

	// so supposedly the info gets written into info data
	char *info_data = new char[1400];
	int valid = get_info(dir, info_data, song_num);

	// now actually making a INFO_DATA message
	char segment[sizeof(Header) + 1400];
	memset(segment, 0, sizeof(Header) + 1400);
	Header* hdr = (RDTHeader*)segment;

	hdr->type = INFO_DATA;

	memcpy(segment, info_data, 1400); // this is copying data into the messsage //HELP

	// if the song does not exist send back a 0
	if ( valid == 0 ){
		hdr->song_num = 0;
		if (send(this->sock_fd, segment, sizeof(Header), 0) < 0) {
			perror("sending song info");
			exit(EXIT_FAILURE);
		}
		return;
	}

	
	// QUESTION: is this sock_fd.. or epoll_fd?
	if (send(this->sock_fd, segment, sizeof(Header) + 1400, 0) < 0) {
		perror("sending song info");
		exit(EXIT_FAILURE);
	}
}

std::vector<std::string> ConnectedClient::get_songs(const char *dir){
	// Turn the char array into a C++ string for easier processing.
	std::string str(dir);
    
	std::vector<std::string> song_vector;
    for(auto& entry: fs::directory_iterator(dir)) {
        if (entry.is_regular_file()){
			if (entry.path.filename().extension().c_str() == ".mp3"){
				string song = entry.path.filename().stem().c_str(); // this will get you the file name
				song_vector.push_back(song);
			}	
		}  
    }

	std::sort(song_vector.begin(), song_vector.end());

	return song_vector;
}

int ConnectedClient::get_info(const char *dir, char *info_data, int song_num){

	// Turn the char array into a C++ string for easier processing.
	std::string str(dir);

	std::vector<std::string> song_vector = this->get_songs(dir);
	if (song_num < 0 && song_num >= song_vector.size()){ // checking if this is a valid song_num
		return 0;
	}
    
	string info = "";

	// finding the song in the directory
	string filename = vector[song_num-1] + ".mp3.info"
    for(auto& entry: fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().filename() == filename){

			std::ifstream file(entry.path().filename(), std::ios::binary);
			file.read(info_data, 1400); // read up to buffer_size bytes into file_data buffer
			file.close();

			return 1;
		}            
    }


	// this is if we couln't find info for the requested song_num
	return 0;

}

void ConnectedClient::list_response(int epoll_fd, const char *dir) {

	char *list_data = new char[1400];

	// now actually making a LIST_DATA message
	char segment[sizeof(Header) + 1400];
	memset(segment, 0, sizeof(Header) + 1400);
	Header* hdr = (RDTHeader*)segment;

	hdr->type = LIST_DATA;

	std::vector<std::string> song_vector = this->get_songs(dir);


	// coping the list of songs into list_data
	char *pointer = list_data
	for (int i = 0; i < song_vector.size(); i++){
		strcpy(pointer, song_vector[i]);
		pointer += song_vector[i].size();
	}


	memcpy(list_data, hdr, 1400); // this is copying data into the messsage HELP

}

void ConnectedClient::handle_input(int epoll_fd, const char *dir) {
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

		// QUESTION: will we have to ntohl/htonl?
        this->play_response(epoll_fd, hdr->song_num, dir)

    } else if (hdr->type == INFO) {
		this->info_response(epoll_fd, dir);
	} else if (hdr->type == LIST) {
		this->list_response(epoll_fd, dir);
	}else if (hdr->type == DISCONNECT) {
		this->handle_close(epoll_fd);
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

