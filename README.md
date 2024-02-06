## Minimal LocalHost / LoopBack example.

The ESP creates a server socket and listens for incoming connections.  
The ESP itself connects to this server via localhost / 127.0.0.1 using lwip loop back.


### Log output:
```
START SERVER TASK
START CLIENT TASK
Server socket created. (id: 54)
Server IP: 0.0.0.0
Server PORT: 3333
(at 321) Client: Message sent: ping
TCP connection accepted!
TCP connection Incoming ip: 127.0.0.1
TCP connection Incoming port: 64417
TCP connection server socket: 54
TCP connection client socket: 56
Server: Connection accepted from 127.0.0.1
(at 341) Server: Received message from client: ping
(at 346) Client: Received reply from server: ping
(at 10349) Client: Message sent: ping
(at 10350) Server: Received message from client: ping
(at 10352) Client: Received reply from server: ping
(at 20350) Client: Message sent: ping
(at 20350) Server: Received message from client: ping
(at 20351) Client: Received reply from server: ping
(at 30349) Client: Message sent: ping
(at 30350) Server: Received message from client: ping
(at 30351) Client: Received reply from server: ping
```
