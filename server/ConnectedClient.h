#ifndef CONNECTEDCLIENT_H
#define CONNECTEDCLIENT_H

#include <vector>


/**
 * Represents the state of a connected client.
 */
enum ClientState { RECEIVING, SENDING };
/**
 * Represents the message types sent and received in connected client.
 */
enum MessageType  {PLAY, INFO, LIST, STOP, DISCONNECT, BAD_REQ, SONG_LEN, INFO_DATA, LIST_DATA};

struct Header {
	MessageType type;
	int song_num;	
};
/**
 * Class that models a connected client.
 * 
 * One object of this class will be created for every client that you accept a
 * connection from.
 */
class ConnectedClient {
  public:
	// Member Variablesa (i.e. fields)
	int client_fd;
	ChunkedDataSender *sender;
	ClientState state;

	// Constructors
	/**
	 * Constructor that takes the client's socket file descriptor and the
	 * initial state of the client.
	 */
	ConnectedClient(int fd, ClientState initial_state);

	/**
	 * No argument constructor.
	 */
	ConnectedClient() : client_fd(-1), sender(NULL), state(RECEIVING) {}


	// Member Functions (i.e. Methods)
	
	/**
	 * Sends a response to the client.
	 * Note that this is just to demonstrate sending to the client: it doesn't
	 * send anything intelligent.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	void send_dummy_response(int epoll_fd);

	/**
	 * Sends a response to the client.
	 * Note that this is just to demonstrate sending to the client: it doesn't
	 * send anything intelligent.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	void play_response(int epoll_fd, int song_num, std::string dir);

	/**
	 * Sends a response to the client.
	 * Note that this is just to demonstrate sending to the client: it doesn't
	 * send anything intelligent.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	void info_response(int epoll_fd, int song_num, std::string dir);

	/**
	 * Sends a response to the client.
	 * Note that this is just to demonstrate sending to the client: it doesn't
	 * send anything intelligent.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	std::string get_info(std::string dir, int song_num);

	/**
	 * Sends a response to the client.
	 * Note that this is just to demonstrate sending to the client: it doesn't
	 * send anything intelligent.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	std::vector<std::string> get_songs(std::string dir);

	/**
	 * Sends a response to the client.
	 * Note that this is just to demonstrate sending to the client: it doesn't
	 * send anything intelligent.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	void list_response(int epoll_fd, std::string dir);


	/**
	 * Sends a response to the client.
	 * Note that this is just to demonstrate sending to the client: it doesn't
	 * send anything intelligent.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	void send_message(int epoll_fd, char *message, uint32_t size);



	/**
	 * Handles new input from the client.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	void handle_input(int epoll_fd, std::string dir);

	/**
	 * Handles a close request from the client.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	void handle_close(int epoll_fd);
};

#endif
