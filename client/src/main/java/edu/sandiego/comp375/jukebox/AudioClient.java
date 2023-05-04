package edu.sandiego.comp375.jukebox;

import java.io.BufferedInputStream;
import java.net.Socket;
import java.util.Scanner;

enum MESSAGE {
  PLAY,
  INFO,
  LIST,
  STOP,
  BAD_REQ,
  SONG_LEN,
  INFO_DATA,
  LIST_DATA
}

public class Header {
	int song_num;
	MESSAGE type;
}

/**
 * Class representing a client to our Jukebox.
 */
public class AudioClient {
	public static void main(String[] args) throws Exception {
		Scanner s = new Scanner(System.in);
		BufferedInputStream in = null;
		Thread player = null;

		System.out.println("Client: Connecting to localhost (127.0.0.1) port 6666");

		while (true) {
			System.out.print(">> ");
			String command = s.nextLine();
			String[] command = command.split(" ");
			Socket socket = new Socket("127.0.0.1", 6666); // moved this outside the if (command) statements
			if (command[0].equals("play")) {
				try {
					if (socket.isConnected()) {
						in = new BufferedInputStream(socket.getInputStream(), 2048); // QUESTION: what is in

						// checking that the song number is a number
						try {
							int song_num = (int)command[0];

							Header hdr = new Header();
							hdr.song_num = song_num;
							hdr.type = PLAY;

							// put the header in a buffer and send it
							// QUESTION: how do we send a message?

							// we wait to receive a message from server with the byte size of the song
							// continue receiving until the recv_count is equal to ^^
							// utilize 'in'?

						} catch (Exception e) {
							System.out.println(e);
						}

						player = new Thread(new AudioPlayerThread(in));
						player.start();
					}
				}
				catch (Exception e) {
					System.out.println(e);
				}
			}
			else if (command[0].equals("exit")) {
				// Currently this doesn't actually stop the music from
				// playing.
				// Your final solution should make sure that the exit command
				// causes music to stop playing immediately.
				System.out.println("Goodbye!");
				break;
			}
			else if (command[0].equals("list")) {
				
				break;
			}
			else if (command[0].equals("info")) {
				
				break;
			}
			else if (command[0].equals("stop")) {
				
				break;
			}
			else {
				System.err.println("ERROR: unknown command");
			}
		}

		System.out.println("Client: Exiting");
	}
}
