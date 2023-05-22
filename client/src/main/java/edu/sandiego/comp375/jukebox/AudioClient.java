package edu.sandiego.comp375.jukebox;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Scanner;
import java.lang.Exception;
import java.util.stream.*;

enum MessageType {
  PLAY, INFO, LIST, STOP, DISCONNECT, BAD_REQ, SONG_LEN, INFO_DATA, LIST_DATA;

  private static final MessageType values[] = values();

	/**
	 * Returns a MessageType based on the given integer. Returns ILLEGAL type
	 * if the integer isn't valid.
	 */
	public static MessageType get(int ordinal) { 
		if (ordinal >= values.length) {
			return MessageType.BAD_REQ;
		}
		return values[ordinal];
	}
}

// public class Header {
// 	int song_num;
// 	MessageType type;
// }

/**
 * Class representing a client to our Jukebox.
 */
public class AudioClient {
	public static void main(String[] args) throws Exception {
		Scanner s = new Scanner(System.in);
		BufferedInputStream in = null;
		Thread player = new Thread();
		// Socket socket;

		System.out.println("Client: Connecting to "+args[0]+" port "+args[1]);
		int port_num = Integer.parseInt(args[1]);
		Socket socket = new Socket(args[0],port_num);//"127.0.0.1", 7102); // moved this outside the if (command) statements
		in = new BufferedInputStream(socket.getInputStream(), 2048); // QUESTION: what is in
		while (true) {
			System.out.print(">> ");
			String c = s.nextLine();
			String[] command = c.split(" ");

			if (command[0].equals("play")){
				try {
					if (command.length == 2){
						if (socket.isConnected()) {
							//checking that the song number is a number
							if (player.isAlive()){
								player.stop(); // stop music
								// reset socket and input stream
								socket = new Socket(args[0], port_num);
								in = new BufferedInputStream(socket.getInputStream(), 2048);
							}
							try {
								
								Socket temp = new Socket(args[0],port_num);
								DataInputStream i = new DataInputStream(temp.getInputStream());
								BufferedInputStream temp_in = new BufferedInputStream(temp.getInputStream(), 2048);
								int song_num = Integer.parseInt(command[1]);
								if (song_num < 0){
									System.err.println("ERROR: Song number needs to be positive.");
									continue;
								}
								sendHeader(temp, MessageType.PLAY, song_num);

								System.out.println(song_num);
								//read header
								MessageType response_type = MessageType.get(i.readByte());
								System.out.println(response_type);
								int data_len = i.readInt();
								if(data_len == 255){
									System.out.println("Song does not exist.");
									byte[] res = temp.getInputStream().readNBytes(3); // clear out rest of header
								}
								else{							
									player = new Thread(new AudioPlayerThread(temp_in));
									player.start();
								}
							} catch (Exception e) {
								System.out.println(e);
								break;
							}
						}
					}
					else{
						System.err.println("ERROR: play format required: play <song_number>");
					}
				}
				catch (Exception e) {
					System.out.println(e);
				}
			}
			else if (command[0].equals("exit")) {
				System.out.println("Goodbye!");
				if(player != null){player.stop(); }// stop music hopefully?
				player.join();
				socket.close();
				break;
			}
			else if (command[0].equals("list")) {
				if (command.length == 1){
					sendHeader(socket, MessageType.LIST, 0);
					// keep calling getMessage until 
					//while (getMessage(socket, MessageType.LIST_DATA));
					DataInputStream i = new DataInputStream(socket.getInputStream());
					MessageType response_type = MessageType.get(i.readByte()); // changed from Byte
					int data_len = i.readInt();
					if(data_len == 255 || data_len == -1){
						System.out.println("Invalid song number.");
						// byte[] res = s.getInputStream().readNBytes(data_len);
						//return false;
					}else{
						byte[] buffer = new byte[1024];
						int read;
						while ((read = in.read(buffer)) != -1) {
							String output = new String(buffer, 0, read);
							System.out.print(output);
							System.out.flush();
							if (output.charAt(output.length() - 1) == '\n') {
								break;
							}
						}
						
					}
					// byte[] res = socket.getInputStream().readNBytes(data_len+3);
					// String response_str = new String(res);
					// System.out.println(response_str);
				}
				else{
					System.err.println("ERROR: If you would like the songs to be listed, please just type 'list'.");
				}
			}
			else if (command[0].equals("info")) {
				try {
					if (socket.isConnected()) {
						// checking that the song number is a number
						if (command.length == 2){
							try {
								int song_num = Integer.parseInt(command[1]);
								System.out.println(song_num);
								if (song_num < 0){
									System.err.println("ERROR: Song number needs to be positive.");
									continue;
								}
								sendHeader(socket, MessageType.INFO, song_num);

								// keep calling getMessage until 
								//while (getMessage(socket, MessageType.INFO_DATA));
								//read header
								DataInputStream i = new DataInputStream(socket.getInputStream());
								MessageType response_type = MessageType.get(i.readByte());
								int data_len = i.readInt();
								System.out.println(data_len);
								if(data_len == 255){
									System.out.println("Song does not have an info file.");
									byte[] res = socket.getInputStream().readNBytes(3);
								}
								else{
									byte[] buffer = new byte[1024];
									int read;
									while ((read = in.read(buffer)) != -1) {
										String output = new String(buffer, 0, read);
										System.out.print(output);
										System.out.flush();
										if (output.charAt(output.length() - 1) == '\n') {
											break;
										}
									}
									
								}
							} catch (Exception e) {
								System.out.println(e);
							}
						}else{
							System.err.println("ERROR: info format required: info <song_number>");
						}
					}
				}
				catch (Exception e) {
					System.out.println(e);
				}
			}
			else if (command[0].equals("stop")){
				try{
					player.stop(); // stop music
					// reset socket and input stream
					socket = new Socket(args[0], port_num);
					in = new BufferedInputStream(socket.getInputStream(), 2048);
				} catch (Exception e){
					System.out.println("No music is playing!");
				}
			}
			else {
				System.err.println("ERROR: unknown command");
			}

		}

		System.out.println("Client: Exiting");
		socket.close();
		
	}
	/**
	 * Creates and sends a message header.
	 *
	 * @param s The socket to send the data over
	 * @param messageType The type of message
	 * @param songNumber The size (in bytes) of the message (not including the header)
	 */

	public static void sendHeader(Socket s, MessageType messageType, int songNumber) throws IOException {
		// Creates a ByteBuffer, which is a convenient class for turning
		// different types of values into their byte representations.
		// The byte order is set to BIG_ENDIAN, because that is the standard
		// format for data sent of a network.
		ByteBuffer header = ByteBuffer.allocate(5);
		header.order(ByteOrder.BIG_ENDIAN);

		// send the header
		header.put((byte)messageType.ordinal()); // ordinal gets the number associated with the MessageType
		header.putInt(songNumber);

		// use basic output stream to write header 
		OutputStream o = s.getOutputStream();
		o.write(header.array());
	}

	/**
	 * Reads a message from the socket.
	 *
	 * @param s The Socket to read data from
	 * @return False if there are no more messages to receive. True otherwise.
	 */
	public static boolean getMessage(Socket s, MessageType looking_for) throws IOException {		// throws IOException 
		DataInputStream in = new DataInputStream(s.getInputStream());
		
		MessageType response_type = MessageType.get(in.readByte()); // changed from Byte
		System.out.println(response_type);
		if (response_type == MessageType.BAD_REQ) {
			System.out.println("Server said the request was bad!");
			in.readInt();
			return false;
		}
		int data_len = in.readInt();
		if(data_len == 255 || data_len == -1){
			System.out.println("invalid song number");
			// byte[] res = s.getInputStream().readNBytes(data_len);
			return false;
		}
		if (response_type == looking_for) {
			// System.out.println("response==lookingfor");
			if (response_type == MessageType.INFO_DATA) {
				System.out.println("Server replied with info!");
				System.out.print(data_len);
				byte[] res = s.getInputStream().readNBytes(data_len+3);
				String response_str = new String(res);
				System.out.println(response_str);
				return false;
			}
			else if (response_type == MessageType.LIST_DATA) {
				System.out.println("Server replied with list!");
				byte[] res = s.getInputStream().readNBytes(data_len+3);
				String response_str = new String(res);
				System.out.println(response_str);
				return false;
			}
			else { // may delete this
				// Something weird happened here...
				return false;
			}
		}
		else{
			System.out.println("else");
			return false;
		}
	}
}
