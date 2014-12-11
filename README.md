        
        ██╗     ██╗██╗         ████████╗ ██████╗ ██████╗ ██████╗ ███████╗███╗   ██╗████████╗
        ██║     ██║██║         ╚══██╔══╝██╔═══██╗██╔══██╗██╔══██╗██╔════╝████╗  ██║╚══██╔══╝
        ██║     ██║██║            ██║   ██║   ██║██████╔╝██████╔╝█████╗  ██╔██╗ ██║   ██║   
        ██║     ██║██║            ██║   ██║   ██║██╔══██╗██╔══██╗██╔══╝  ██║╚██╗██║   ██║   
        ███████╗██║███████╗       ██║   ╚██████╔╝██║  ██║██║  ██║███████╗██║ ╚████║   ██║   
        ╚══════╝╚═╝╚══════╝       ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝   ╚═╝  




Lil' Torrent is just a p2p app that allows you to download an play some music shared and owned by other users.

The app was developed in 
- C++ as a main language 
and this libraries:
  - ZMQ
  - CZMQ
  - SFML 2.1

The main componets are: 
1. Tracker (aka. Broker)
  Is the central component of the architecture. Its function is just know who has
  what music.
 to launch it 
  ./Broker [Port to listen to the peers]
2. Peer
  Is something that works as a client and a server at the same time. The peers have the music and they are in charge 
  of sending them to the requesters.
  To launch it
  ./CLient [Ip Broker] [Port where the broker is listen to the peers] [Client ip] [Puerto para la escucha] Temp         [an int]

  Temp = is the directory where the music is stored by default
