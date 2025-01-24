# clang_chat_system

## System Requirements
1. **Operating System**: Linux
2. **Communication Protocol**: TCP
3. **Server Capabilities**:
   - Handle multiple client connections (e.g., max 5 connections)
   - Support group messaging and private chatting
4. **Process Description**:
   - **Client**: 
     - Connect to the server and receive messages from the server.
     - Display group or private messages, sender information, and the message content.
     - Update the client list when a client joins or exits.
     - Allow the user to choose between group messaging or private chatting.
     - Send input messages to the server for group or private chats.
     - Exit the program by terminating the process or thread.
   - **Server**:
     - Create a process or thread for each client connection.
     - Notify all clients when a new client joins or an existing client exits.
     - Forward group messages to all clients.
     - Forward private messages to the intended recipient.
     - Close the corresponding process or thread when a client exits.

## Design Report Requirements
1. **Literature Review**: Research communication system design literature.
2. **Requirement Analysis**: Analyze the system requirements.
3. **Functional Design**: Design the system functionalities.
4. **Detailed Design**: Provide a detailed design plan.
5. **Code Development**: Develop and debug the program.
6. **Functional Verification**: Verify the functionality of each feature.

## Submission Requirements
1. Upload the design report to the learning platform.
2. Record and upload a defense video.
3. Upload the source code for both the client and server.

---

# Design and Implementation of a Chatroom System on Linux

## Requirement Analysis
1. **Development Environment**: Linux
2. **Communication Protocol**: TCP
3. **Programs**:
   - **Server**: 
     - Implemented using multi-threading.
     - Capable of handling multiple client requests simultaneously.
     - Forwards private and group messages.
   - **Client**:
     - Connects to the server.
     - Logs in and logs out of the server.
     - Sends messages to the server based on user input (group or private).
     - Exits the program, and the server releases resources (socket, thread).

## Functional Design
1. **Client**:
   - **Input Parameters**: Username and password for login.
   - **Connection**: Connect to the server using a fixed IP address. Login with username and password. Exit if login fails.
   - **Options**:
     - **Group Chat**: Send a message to all clients.
     - **Private Chat**: Send a message to a specific client.
       - If the recipient is offline or does not exist, display a failure message.
       - If the recipient is online and receives the message, display a success message.
     - **Exit**: Terminate the process. The server will mark the user as offline.
     - **Query**: Display the status (online/offline) of all users.
   - **Logging**: Print logs to the console for debugging purposes.

2. **Server**:
   - **User List**: Read a list of usernames and passwords.
   - **Login Request**: Validate the username and password. Update the user status to online if successful.
   - **Private Chat Request**: Check the recipient's status. If offline, return a failure message. If online, forward the message and return a success message.
   - **Group Chat Request**: Broadcast the message to all clients and return a success message.
   - **Exit Request**: Update the user status to offline.
   - **Status Query**: Return the status (online/offline) of all users.
   - **Thread Management**: Create a separate thread for each user. Release resources when the user exits.
   - **Logging**: Print logs to the console for debugging purposes.
