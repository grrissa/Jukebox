# Internet Jukebox with Custom Client-Server Communication Protocol

This project implements a custom client-server application for an Internet Jukebox system. The server streams MP3 audio files to clients, who can send commands such as `play`, `list`, `info`, and `stop`. The project uses a custom binary protocol for communication over TCP sockets, with a focus on efficient data transfer and real-time streaming.

## Features

### Server
- Built in **C++** with non-blocking sockets and epoll for handling multiple clients simultaneously.
- Streams audio files in chunks to ensure smooth playback and handle network delays gracefully.
- Processes client commands such as:
  - `PLAY`: Streams a specific song to the client.
  - `LIST`: Sends a list of available songs.
  - `INFO`: Provides metadata about a specific song.
  - `STOP` and `DISCONNECT`: Manage playback and connection states.

### Client
- Developed in **Java**, the client allows users to:
  - Play songs by their index number.
  - View the list of available songs.
  - Get metadata about a specific song.
  - Stop playback or disconnect from the server.
- Includes a custom audio playback system using the `javax.sound.sampled` library.

### Protocol Design
The server and client communicate using a custom binary protocol:
- **Header structure:** Includes message type and optional payload (e.g., song index).
- **Message types:** Enumerated as `PLAY`, `INFO`, `LIST`, `STOP`, `DISCONNECT`, and responses like `SONG_LEN` or `LIST_DATA`.

## Architecture

### Server
- Uses **ChunkedDataSender** classes for streaming data efficiently in fixed-sized chunks.
- Implements client connection management with the `ConnectedClient` class.
- Tracks client states and manages communication using an event loop with epoll.

### Client
- Reads and parses responses from the server based on the binary protocol.
- Provides a command-line interface for sending commands and interacting with the server.
- Plays streamed audio in real-time using a dedicated playback thread.

### Prerequisites
- **C++17** or later for compiling the server.
- **Java 11** or later for running the client.
- A directory containing `.mp3` audio files to serve.
