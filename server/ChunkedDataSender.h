#ifndef CHUNKEDDATASENDER_H
#define CHUNKEDDATASENDER_H

#include <cstddef>
#include <fstream>
#include <iostream>


const size_t CHUNK_SIZE = 4096;

/**
 * An interface for sending data in fixed-sized chunks over a network socket.
 * This interface contains one function, send_next_chunk, which should send
 * the next chunk of data.
 */
class ChunkedDataSender {
  public:
	virtual ~ChunkedDataSender() {}

	virtual ssize_t send_next_chunk(int sock_fd) = 0;
};

/**
 * Class that allows sending an array of over a network socket.
 */
class ArraySender : public virtual ChunkedDataSender {
  private:
	char *array; // the array of data to send
	size_t array_length; // length of the array to send (in bytes)
	size_t curr_loc; // index in array where next send will start

  public:
	/**
	 * Constructor for ArraySender class.
	 */
	ArraySender(const char *array_to_send, size_t length);

	/**
	 * Destructor for ArraySender class.
	 */
	~ArraySender() {
		delete[] array;
	}

	/**
	 * Sends the next chunk of data, starting at the spot in the array right
	 * after the last chunk we sent.
	 *
	 * @param sock_fd Socket which to send the data over.
	 * @return -1 if we couldn't send because of a full socket buffer,
	 * 	otherwise the number of bytes actually sent over the socket.
	 */
	virtual ssize_t send_next_chunk(int sock_fd);
};

// TODO: Create a new class named FileSender that implements the
// ChunkedDataSender interface. This class should allow the user to send a big
// file over a socket in chunks.
/**
 * Class that allows sending a file over a network socket.
 */
class FileSender : public virtual ChunkedDataSender {
  private:
	std::string filename; // the file of data to send
	std::ifstream file;
	size_t file_size; // length of the file to send (in bytes)
	ssize_t curr_loc = 0;


  public:
	/**
	 * Constructor for FileSender class.
	 */
	FileSender(std::string filename, size_t file_size);

	/**
	 * Destructor for ArraySender class.
	 */
	~FileSender() {
		this->file.close();
	}

	/**
	 * Sends the next chunk of data, starting at the spot in the array right
	 * after the last chunk we sent.
	 *
	 * @param sock_fd Socket which to send the data over.
	 * @return -1 if we couldn't send because of a full socket buffer,
	 * 	otherwise the number of bytes actually sent over the socket.
	 */
	virtual ssize_t send_next_chunk(int sock_fd);
};

#endif // CHUNKEDDATASENDER_H
