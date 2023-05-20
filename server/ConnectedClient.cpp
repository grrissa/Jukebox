#include <iostream>

#include <cstring>

#include <unistd.h>
#include <sys/epoll.h>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "ChunkedDataSender.h"
#include "ConnectedClient.h"

using std::cout;
using std::cerr;
using std::string;

namespace fs = std::filesystem;

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
        epoll_out.events = EPOLLOUT  | EPOLLRDHUP;

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

void ConnectedClient::play_response(int epoll_fd, int song_num, string dir) {

	// send the song length
	char *segment = new char[sizeof(Header)];
	memset(segment, 0, sizeof(Header));
	Header* hdr = (Header*)segment;
	hdr->type = SONG_LEN;

	std::vector<std::string> song_vector = this->get_songs(dir);
	// if this song is not valid then just send a -1 and client tries again
	if (song_num < 0 && song_num >= (int)song_vector.size()){
		hdr->song_num = -1;
		ArraySender *array_sender = new ArraySender(segment, sizeof(Header));
		this->sender = array_sender;
		delete[] segment; // The ArraySender creates its own copy of the data so let's delete this copy
		this->send_message(epoll_fd, array_sender);
		return;
	}

	std::uintmax_t song_num_bytes = 0;
	// finding the song in the directory
	string filename = song_vector[song_num] + ".mp3";
    for(auto& entry: fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().filename() == filename){			
			song_num_bytes = fs::file_size(entry);
		}   
		break;         
    }

	hdr->song_num = song_num_bytes;
	ArraySender *array_sender = new ArraySender(segment, sizeof(Header));
	this->sender = array_sender;
	delete[] segment; // The ArraySender creates its own copy of the data so let's delete this copy
	this->send_message(epoll_fd, array_sender);

	// this should be sending the actualy song file in chunks...
	FileSender *file_sender = new FileSender(dir + filename);
	this->sender = file_sender;
	this->send_message(epoll_fd, file_sender);
}




void ConnectedClient::info_response(int epoll_fd, int song_num, string dir) {
	cout << "in info response";
	string info = this->get_info(dir, song_num);

	// now actually making a INFO_DATA message
	char *segment = new char[sizeof(Header) + info.size()];
	memset(segment, 0, sizeof(Header) + info.size());
	Header* hdr = (Header*)segment;

	hdr->type = INFO_DATA;
	hdr->song_num = info.size();

	if (info.size() == 0){
		hdr->song_num = htonl(-1); // this means that there was no info about the song or song was invalid
	}

	cout << info;
	memcpy(hdr+1, info.c_str(), info.size());
	ArraySender *array_sender = new ArraySender(segment, sizeof(Header) + info.size());
	this->sender = array_sender;
	delete[] segment; // The ArraySender creates its own copy of the data so let's delete this copy
	this->send_message(epoll_fd, array_sender);
}

std::vector<std::string> ConnectedClient::get_songs(string dir){
	// Turn the char array into a C++ string for easier processing.
	std::string str(dir);
    
	std::vector<std::string> song_vector;
    for(fs::directory_iterator entry(dir); entry != fs::directory_iterator(); ++entry) {
        if (entry->is_regular_file()){
			if (entry->path().extension() == ".mp3"){
				string song = entry->path().filename().stem().string(); // this will get you the file name
				song_vector.push_back(song);
			}	
		}  
    }

	std::sort(song_vector.begin(), song_vector.end());

	return song_vector;
}

string ConnectedClient::get_info(string dir, int song_num){

	// Turn the char array into a C++ string for easier processing.
	std::string str(dir);

	std::vector<std::string> song_vector = this->get_songs(dir);
	if (song_num < 0 && song_num >= (int)song_vector.size()){ // checking if this is a valid song_num
		return "";
	}

	// finding the song in the directory
	string filename = song_vector[song_num] + ".mp3.info";
	cout << filename << "\n";
    for(auto& entry: fs::directory_iterator(dir)) {
		cout << entry.path().filename() << "\n";
        if (entry.is_regular_file() && entry.path().filename() == filename){

			filename = dir + filename;

			// reading the file into string info
			std::ifstream file(filename);
			std::stringstream buffer;
			buffer << file.rdbuf();
			std::string info = buffer.str();

			file.close();

			cout << info << "\n";
			return info;
		}            
    }

	// this is if we couln't find info for the requested song_num
	return "";

}

void ConnectedClient::list_response(int epoll_fd, string dir) {

	string list_data = "";
	std::vector<std::string> song_vector = this->get_songs(dir);

	// coping the list of songs into list_data
	for (int i = 0; i < (int)song_vector.size(); i++){
		list_data += std::to_string(i) + " - ";
		list_data += song_vector[i];
		list_data += "\n";
	}

	// now actually making a LIST_DATA message
	char *segment = new char[sizeof(Header) + list_data.size()];
	memset(segment, 0, sizeof(Header) + list_data.size());
	Header* hdr = (Header*)segment;
	hdr->type = LIST_DATA;
	hdr->song_num = list_data.size();

	memcpy(hdr + 1, list_data.c_str(), list_data.size()); // this is copying data into the messsage HELP


	ArraySender *array_sender = new ArraySender(segment, sizeof(Header) + list_data.size());
	this->sender = array_sender;
	delete[] segment; // The ArraySender creates its own copy of the data so let's delete this copy
	this->send_message(epoll_fd, array_sender);


}

void ConnectedClient::handle_input(int epoll_fd, string dir) {
	// QUESTION: so this is the driver... we are doing all of the receiving in here and
	// then calling send dummy to send the actual data that theyre req?????? how do we pass that data onto send dummy 
	// should we create a function for each request type from the client

	cout << "Ready to read from client " << this->client_fd << "\n";
	char data[1024];
	Header* hdr = (Header*)data;    
	
	this->state = RECEIVING;
	ssize_t bytes_received = recv(this->client_fd, data, 1024, 0);
	if (bytes_received < 0) {
		perror("client_read recv");
		exit(EXIT_FAILURE);
	}

	cout << "Received data: ";
	for (int i = 0; i < bytes_received; i++)
		cout << data[i];
	cout << hdr->type;
	cout << "\n";


	if (hdr->type == PLAY){
        this->play_response(epoll_fd, hdr->song_num, dir);

    } else if (hdr->type == INFO) {
		cout << "in info else if";
		this->info_response(epoll_fd, hdr->song_num, dir);
	} else if (hdr->type == LIST) {
		this->list_response(epoll_fd, dir);
	}else if (hdr->type == DISCONNECT) {
		this->handle_close(epoll_fd);
	}
}

void ConnectedClient::send_message(int epoll_fd, ChunkedDataSender *sender){

	ssize_t num_bytes_sent;
	ssize_t total_bytes_sent = 0;

	// keep sending the next chunk until it says we either didn't send
	// anything (0 return indicates nothing left to send) or until we can't
	// send anymore because of a full socket buffer (-1 return value)
	while((num_bytes_sent = sender->send_next_chunk(this->client_fd)) > 0) {
		total_bytes_sent += num_bytes_sent;
	}
	cout << "sent " << total_bytes_sent << " bytes to client\n";

	if (num_bytes_sent < 0) {

		this->state = SENDING;
		this->sender = sender;

		// QUESTION
		struct epoll_event epoll_out;
        epoll_out.data.fd = this->client_fd;
        epoll_out.events = EPOLLOUT;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, this->client_fd, &epoll_out) == -1) {
            perror("sending message");
            exit(EXIT_FAILURE);
        }
    }
	else {
		// Sent everything with no problem so we are done with our ArraySender
		// object.
		delete sender;
	}
}

void ConnectedClient::continue_sending(int epoll_fd){

	ssize_t num_bytes_sent;
	ssize_t total_bytes_sent = 0;

	// keep sending the next chunk until it says we either didn't send
	// anything (0 return indicates nothing left to send) or until we can't
	// send anymore because of a full socket buffer (-1 return value)
	while((num_bytes_sent = this->sender->send_next_chunk(this->client_fd)) > 0) {
		total_bytes_sent += num_bytes_sent;
	}
	cout << "sent " << total_bytes_sent << " bytes to client\n";

	if (num_bytes_sent < 0) {

		this->state = SENDING;

		// QUESTION
		struct epoll_event epoll_out;
        epoll_out.data.fd = this->client_fd;
        epoll_out.events = EPOLLOUT;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, this->client_fd, &epoll_out) == -1) {
            perror("sending message");
            exit(EXIT_FAILURE);
        }
    }
	else {
		// Sent everything with no problem so we are done with our ArraySender
		// object.
		delete this->sender;
	}
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

