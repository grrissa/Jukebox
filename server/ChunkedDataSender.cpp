#include <algorithm>

#include <cstring>
#include <cerrno>
#include <cstdio>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>

#include "ChunkedDataSender.h"

ArraySender::ArraySender(const char *array_to_send, size_t length) {
	this->array = new char[length];
	std::copy(array_to_send, array_to_send+length, this->array);
	this->array_length = length;
	this->curr_loc = 0;
}

ssize_t ArraySender::send_next_chunk(int sock_fd) {
	// Determine how many bytes we need to put in the next chunk.
	// This will be either the CHUNK_SIZE constant or the number of bytes left
	// to send in the array, whichever is smaller.
	size_t num_bytes_remaining = array_length - curr_loc;
	size_t bytes_in_chunk = std::min(num_bytes_remaining, CHUNK_SIZE);

	if (bytes_in_chunk > 0) {
		// Create the chunk and copy the data over from the appropriate
		// location in the array
		char chunk[CHUNK_SIZE];
		memcpy(chunk, array+curr_loc, bytes_in_chunk);

		ssize_t num_bytes_sent = send(sock_fd, chunk, bytes_in_chunk, 0);

		if (num_bytes_sent > 0) {
			// We successfully send some of the data so update our location in
			// the array so we know where to start sending the next time we
			// call this function.
			curr_loc += num_bytes_sent;
			return num_bytes_sent;
		}
		else if (num_bytes_sent < 0 && errno == EAGAIN) {
			// We couldn't send anything because the buffer was full
			return -1;
		}
		else {
			// Send had an error which we didn't expect, so exit the program.
			perror("send_next_chunk send in array sender");
			exit(EXIT_FAILURE);
		}
	}
	else {
		return 0;
	}
}



FileSender::FileSender(std::string filename) //file(filename, std::ios::binary)
{
	//f(filename, std::ios::binary);
	this->filename = filename;
	//this->file = new std::ifstream(filename.c_str(), std::ifstream::in);
	this->file.open((this->filename), std::ios::binary );
}

ssize_t FileSender::send_next_chunk(int sock_fd) {
	//std::cout << "file to be opened "<< this->filename << "\n";
	//std::cout << "opened file\n";
	char chunk[CHUNK_SIZE];
	memset(chunk, 0, CHUNK_SIZE);

	this->file.seekg(this->curr_loc);
	this->file.read((char *)&chunk, CHUNK_SIZE); // read up to buffer_size bytes into file_data buffer
	int bytes_in_chunk = this->file.gcount();
	//std::cout << bytes_in_chunk <<"\n";
	ssize_t num_bytes_sent = send(sock_fd, chunk, bytes_in_chunk, 0);

	if (num_bytes_sent > 0){
		this->curr_loc += num_bytes_sent;
		return num_bytes_sent;
	}
	else if (num_bytes_sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)){
		return -1;
	}
	else {
		perror("send_next_chunk send in file sender");
		//exit(EXIT_FAILURE);
	}

	return 0;


}
