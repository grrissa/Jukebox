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

/**
 * Class representing a client to our Jukebox.
 */
public class AudioClient {
	public static void main(String[] args) throws Exception {
		Scanner s = new Scanner(System.in);
		BufferedInputStream in = null;

		System.out.println("Client: Connecting to "+args[0]+" port "+args[1]);
		int port_num = Integer.parseInt(args[1]);
		Socket socket = new Socket(args[0],port_num);
		in = new BufferedInputStream(socket.getInputStream(), 2048); 
		Thread player = new Thread();

		while (true) {
			System.out.print(">> ");
			String c = s.nextLine();
			String[] command = c.split(" ");
			if (player.isAlive()){ // stop music when another command is entered
				player.stop(); // stop music
				// reset socket and input stream
				socket.close();
				socket = new Socket(args[0], port_num);
				in = new BufferedInputStream(socket.getInputStream(), 2048);
			}
			if (command[0].equals("play")){
				try {
					if (command.length == 2){
						if (socket.isConnected()) {
							try {
								DataInputStream i = new DataInputStream(socket.getInputStream());
								int song_num = Integer.parseInt(command[1]);

								if (song_num < 0){
									System.err.println("ERROR: Song number needs to be positive.");
									continue;
								}
								sendHeader(socket, MessageType.PLAY, song_num);

								//read header
								MessageType response_type = MessageType.get(i.readByte());
								int data_len = i.readInt();
								if(data_len == 255){
									System.out.println("Song does not exist.");
									byte[] res = socket.getInputStream().readNBytes(3); // clear out rest of header
								}
								else{				
									byte[] res = socket.getInputStream().readNBytes(3); // clear out rest of header			
									player = new Thread(new AudioPlayerThread(in));
									player.start();
								}
							} catch(NumberFormatException e){
								System.err.println("ERROR: Please enter a whole number for the song number");
							} catch (Exception e) {
								System.out.println(e);
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
				if(player != null){player.stop(); }
				// player.join();
				socket.close();
				s.close();
				in.close();
				break;
			}
			else if (command[0].equals("list")) {
				if (command.length == 1){
					// System.out.println("sending header");
					sendHeader(socket, MessageType.LIST, 0);
					// System.out.println("sent header");
					getMessage(socket, in);
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
								if (song_num < 0){
									System.err.println("ERROR: Song number needs to be positive.");
									continue;
								}
								sendHeader(socket, MessageType.INFO, song_num);
								getMessage(socket, in);
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
					if (command.length == 1){
						if(player != null){player.stop(); }
						// player.join();
						// reset socket and input stream
						// socket = new Socket(args[0], port_num);
						// in = new BufferedInputStream(socket.getInputStream(), 2048);
					}else{
						System.err.println("ERROR: If you would like the song to stop playing please just type 'stop'.");
					}

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
		ByteBuffer header = ByteBuffer.allocate(8);
		header.order(ByteOrder.BIG_ENDIAN);

		// send the header
		header.put((byte)messageType.ordinal()); // ordinal gets the number associated with the MessageType
		header.put((byte)0);
		header.put((byte)0);
		header.put((byte)0);
		header.putInt(songNumber);
		

		// use basic output stream to write header 
		OutputStream o = s.getOutputStream();
		o.write(header.array());
	}

	/**
	 * Reads a message from the socket.
	 *
	 * @param s The Socket to read data from
	 * @param in The BufferedInputStream to read data from
	 * @return False if there are no more messages to receive. True otherwise.
	 */
	public static void getMessage(Socket socket, BufferedInputStream in) throws IOException {		
		DataInputStream i = new DataInputStream(socket.getInputStream());
		MessageType response_type = MessageType.get(i.readByte());
		// System.out.println(response_type);
		int data_len = i.readInt();
		// System.out.println(data_len);
		byte[] res = socket.getInputStream().readNBytes(3);
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
}
