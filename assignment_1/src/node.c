/* 

Applications and Services in the Internet
Assignment 1

A client for the peer to peer application based on Gnutella 0.6

Programmers:
Gaurav Bhorkar (545691)
Manish Thapa (534398)

*/


#define MAX_PEERS 15

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h> //inet_addr
#include "p2p.h"


/* MESSAGE HANDLERS */
void peer_handler (int peer_sock);
void handle_ping(int peer_sock, char *buffer);
void handle_join(int peer_sock, char *buffer);
void handle_qhit(int peer_sock, char *buffer);
void handle_query(int peer_sock, char *buffer);
void handle_pong(int peer_sock, char *buffer);


/* MESSAGE SENDERS */
void send_query(char *query);
void send_ping (int peer_sock, uint8_t ttl);

/* CONNECT PEER */
int connect_peer (char *ip_address, int port);

/* Misc Functions */
char * get_my_ip ();
int get_sock_ip (int sock, char *ip);
uint32_t get_message_id();
void display_menu ();

/* Global Section Start */
char *own_ip;
int peer_socks[MAX_PEERS] = {0};

FILE *log_file;
int PORT;

/* Global section end */



int main(int argc , char *argv[])
{
	//Time variables
	struct timeval tv;
	time_t old_time, curr_time;
	
	char user_input_buff[1024]; //buffer for storing user input commands
	
 	//Listener and peer sockets
	int listener_sock, max_sock, sock, peer_sock1;
	
	struct sockaddr_in listener_addr, peer_addr;
	struct in_addr addr;
	
	int i, max_peers = MAX_PEERS;
		
	int activity;

	unsigned int addrlen;
	
	/* Socket set to handle multiple sockets*/
	fd_set readfds;
 
	//initialize port variable to first argument
	if (argc == 2)
	{
		PORT = atoi(argv[1]);
	}
	else 
	{
		printf ("Please specify port \n");
		exit (-1);
	}

	remove ("log.txt");
	
	own_ip = get_my_ip ();
	
	/* Create listening socket*/
	listener_sock = socket (AF_INET, SOCK_STREAM, 0);
	if (listener_sock == 0)
	{
		perror("Couldn't create socket listener: ");
		exit (-1);
	}
	
	/* Set up listener properties */
	listener_addr.sin_family = AF_INET;
	listener_addr.sin_addr.s_addr = INADDR_ANY;
	listener_addr.sin_port = htons(PORT);

	/* Bind listener socket to port */
	if (bind (listener_sock, (struct sockaddr *)&listener_addr, sizeof(listener_addr)) < 0)
	{
		perror ("Failed to bind: ");
		exit (-1);
	}
	
	/* Start listening to the port, max 3 pending connectios */
	if (listen (listener_sock, 5) < 0)
	{
		perror ("listener failed: ");
		exit (-1);
	}
	
	printf ("Node is listening on port %d\n", PORT);
	
	display_menu ();
	
	old_time = time (NULL);
	
	//Looping indifinitely thru all file descriptors
	//take appropriate action upon set
	
	while (1)
	{
		log_file = fopen ("log.txt", "a");
		
		//clear socket set
		FD_ZERO (&readfds);
		
		//adding listener to socket set
		FD_SET (listener_sock, &readfds);
		
		//add stdin (fd 0) as well to check for user inputs
		FD_SET (0, &readfds);
		
		max_sock = listener_sock;
		
		for (i = 0; i < max_peers; i++)
		{
			//add to FD list of socket is set
			if (peer_socks[i] > 0)
				FD_SET(peer_socks[i], &readfds);
			
			//update the max socket number
			if (peer_socks[i] > max_sock)
				max_sock = peer_socks[i];
		}
		
		tv.tv_sec = 0.5;
		tv.tv_usec = 0;
		activity = select (max_sock + 1, &readfds, NULL, NULL, &tv);
		
		if (activity < 0)
		{
			perror ("Could not select FD: ");
			exit (-1);
		}
		
		//check if listener socket is set, then accept new connection
		if (FD_ISSET(listener_sock, &readfds))
		{
			addrlen = sizeof(listener_addr);
			sock = accept(listener_sock, (struct sockaddr *)&listener_addr, &addrlen);
			
			if (sock < 0)
			{
				perror("Accept new connection failed: ");
				exit (-1);
			} 
			
			puts("New peer connected");
				
			//adding to peer list
			for (i = 0; i < max_peers; i++)
			{
				if (peer_socks[i] == 0)
				{
					peer_socks[i] = sock;
					break;
				}
			}
			
		}
		
		//check other peer sockets connected. if set it means activity has happened
		for (i = 0; i < max_peers; i++)
		{
			if (FD_ISSET(peer_socks[i], &readfds) && peer_socks[i] != 0)
			{
				peer_handler(peer_socks[i]);
			}	
		}	
		
		//check for user input
		if (FD_ISSET (0, &readfds))
		{
			int len;
			char command[80];
			char delim[2] = " ";
			
			char *cmd, *arg1, *arg2;
			
			if ((len = read(0, user_input_buff, 1024)) > 0) 
			{
				user_input_buff[len-1] = '\0';
				strncpy (command, user_input_buff, strlen (user_input_buff) + 1);
				char ip_str[16];
				
				if (strlen(command) != 0)
				{
				
					cmd = strtok (command, delim);
/* COMMAND LINE implementation below */			
					if (strcmp (cmd, "bye") == 0)
					{
						printf ("Moi Moi! Disconnected from all sockets\n\n");
						exit (0);
					}
	
					// peers
					else if (strcmp (cmd, "peers") == 0) 
					{
						int is_present = 0;
						uint16_t port;
						for (i = 0; i < MAX_PEERS; i++)
						{
							if (peer_socks[i] > 0)
							{
								port = get_sock_ip (peer_socks[i], ip_str);
								printf ("%s\t%u\n", ip_str, port);
								is_present = 1;
							}
						}
						
						if (is_present == 0)
						{
							printf ("No peers connected\n");
						}
					}

					//query <query string>
					else if (strcmp (cmd, "query") == 0)
					{
						arg1 = strtok(NULL, delim); //query string
						if (arg1 != NULL)
						{
							send_query(arg1);
						}
						else
						{
							printf ("Specify query parameter. Ex. query <key>\n");
						}
					}
				
					//connect ip_addr port
					else if (strcmp (cmd, "connect") == 0)
					{
						arg1 = strtok (NULL, " "); //IP_ADDRESS is first argument
						if (arg1 != NULL) 
						{						
							arg2 = strtok (NULL, " ");
							if (arg2 != NULL) 
							{
									int sock;
									sock = connect_peer(arg1, atoi(arg2));
									
									if (sock > 0)
									{
										//add to socket list
										for (i = 0; i < MAX_PEERS; i++)
										{
											if (peer_socks[i] == 0)
											{
												peer_socks[i] = sock;
												break;	
											}
										}
									}							
							}
							else
							{
								printf ("Specify port number. Format - connect ip_address port\n");
							}
						}
						else
						{
							printf ("Specify ip address. Format - connect ip_address port\n");
						}
					}
					
					//pingb ip_addr port
					else if (strcmp (cmd, "pingb") == 0)
					{
						char ip[16];
						uint16_t port;
						
						int found = 0;
						
						arg1 = strtok(NULL, delim); //Ip address string
						if (arg1 != NULL)
						{
							arg2 = strtok (NULL, " ");
							if (arg2 != NULL) 
							{
								for (i = 0; i < MAX_PEERS; i++)
								{
									port = get_sock_ip (peer_socks[i], ip);
									if (strcmp (arg1, ip) == 0) //if ip is found
									{
										if (port == atoi(arg2))
										{
											send_ping (peer_socks[i], 5); //send type B ping (ttl 5)
											found = 1;
											break;
										}
									}
								}
								if (found == 0)
								{
									printf ("IP Port combination not found in connected peers. Use command 'peers' to verify\n");
								}
							}
							else
							{
								printf ("Please provide port. Format - pingb <ip> <port>\n");
							}
						}
						else
						{
							printf ("Please provide ip address. Format - pingb <ip> <port>\n");
						}
					}
					
					//Unknown command
					else 
					{
						printf ("Unknown command\n");
						display_menu ();
					}
				

					/*
					 while( token != NULL ) 
					   {
					      printf( "%s\n", token );
					    
					      token = strtok(NULL, " ");
					   }
					   
					   */
				   }
				
			}
		}
		
		/* Every 5 second logic */
		curr_time = time (NULL);
		
		if (curr_time >= old_time+5)
		{
			/* This code will every 5 seconds approximately */
			
			for (i = 0; i < MAX_PEERS; i++)
			{
				if (peer_socks[i] != 0)
					send_ping (peer_socks[i], 1);
			}
			
			old_time = curr_time;
		}
		
		fclose(log_file);
		
	}
	
	return 0;
}

/* Display the menu */
void display_menu () 
{
	puts ("\n ** The ultimate Gnutella p2p node - by group20 **");
	puts ("Commands supported");
	puts (" - connect <ip> <port>");
	puts (" - peers");
	puts (" - query <key>");
	puts (" - pingb <ip> <port>");
	puts (" - bye\n");
}



/* 
Function name: send_ping
Sends ping A or ping B message to the specified socket
Accepts: peer_socket, time to live
Returns: nothing
*/
void send_ping (int peer_sock, uint8_t ttl) 
{
	int i;
	uint32_t hash = get_message_id();
	
	char ip[16];
	
	struct P2P_h msg_ping_send_head;
	
	//Build the ping msg
	msg_ping_send_head.version = 1;
	msg_ping_send_head.ttl = ttl;
	msg_ping_send_head.msg_type = MSG_PING;
	msg_ping_send_head.reserved = 0;
	msg_ping_send_head.org_port = htons(PORT);
	msg_ping_send_head.length = htons(0);
	msg_ping_send_head.org_ip = inet_addr(own_ip);
	msg_ping_send_head.msg_id = htonl (hash);

	if(send (peer_sock, (void *) &msg_ping_send_head , sizeof(struct P2P_h), 0) < 0)
	{
		perror("Failed to send: ");
		exit(-1);
	}
	else
	{
		//send successfull log to file
		get_sock_ip (peer_sock, ip);
		if (ttl == 1) {
			fprintf (log_file, "--> PING (A) (msg id: %u) to %s\n", hash, ip);
		}
		else 
			fprintf (log_file, "--> PING (B) (msg id: %u) to %s\n", hash, ip);
	}
}


/*
Function: connect_peer
Connect to the specified peer. If successful return socket number.
Accepts: ip address, port
Returns: socket number of connected peer
*/
int connect_peer (char *ip_address, int port)
{
	struct in_addr addr;
	
	unsigned int hash; 
	char data_to_hash[500];
	
	/* Peer (server) declarations */
	
	struct sockaddr_in peer;		//Server (peer)
	char buffer[1024];			//buffer to store reply
	int num_bytes_recv;


	/* Protocol related  declarations */
	
	//Structure for peer response for JOIN message
	struct P2P_h msg_join_recv_head;
	struct P2P_join msg_join_recv_body;
	
	//Variable for sending message
	struct P2P_h msg_join_send;
	
	int sock_peer;				//socket descriptor
	
	printf ("Connecting to %s on port %d\n", ip_address, port);
	fprintf (log_file, "Connecting to %s on port %d\n", ip_address, port);
	
	hash = get_message_id(); //will give me a unique message id
	
	//Build the message to JOIN the network
	msg_join_send.version = 1;
	msg_join_send.ttl = 5;
	msg_join_send.msg_type = MSG_JOIN;
	msg_join_send.reserved = 0;
	msg_join_send.org_port = htons(PORT);
	msg_join_send.length = htons(0);
	msg_join_send.org_ip = inet_addr(own_ip);
	msg_join_send.msg_id = htonl(hash);

	//Create socket
	sock_peer = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_peer == -1)
	{
		perror("Could not create socket: ");
		exit (-1);
	}
	
	//build the server object. For connecting to bootstrap peer.
	//peer.sin_addr.s_addr = inet_addr("130.233.195.31");
	peer.sin_addr.s_addr = inet_addr(ip_address);
	peer.sin_family = AF_INET;
	peer.sin_port = htons(port);

	//Connect to remote server
	if (connect(sock_peer, (struct sockaddr *)&peer , sizeof(peer)) < 0)
	{
		perror("Cannot connect to peer: ");
		return (-1);
	}

	//Send the JOIN message
	if(send(sock_peer , (void *) &msg_join_send , sizeof(struct P2P_h), 0) < 0)
	{
		perror("Failed to send: ");
		return (-1);
	}
	
	//Receive a reply from the server (store it in buffer)
	num_bytes_recv = recv(sock_peer, buffer, 1024, 0);
	
	if (num_bytes_recv < 0)
	{
		perror ("recv failed");
		return (-1);
	}
	
	//Store the buffer into proper structures
	//msg_join_recv_head = *((struct P2P_h *) buffer);
	//msg_join_recv_body = *((struct P2P_join *) (buffer + sizeof(struct P2P_h)));
	
	//writing buffer to header
	memcpy(&msg_join_recv_head, buffer, HLEN);
	
	//writing buffer to body
	memcpy(&msg_join_recv_body, buffer+HLEN, JOINLEN);
	
	/*
	
	printf("Number of bytes received: %d\n", num_bytes_recv);
	
	puts("--header--");
	printf("%u\t%u\t%u\t%u\n", (uint8_t)msg_join_recv_head.version, (uint8_t)msg_join_recv_head.ttl, (uint8_t)msg_join_recv_head.msg_type, (uint8_t)msg_join_recv_head.reserved);
	printf("%u\t%u\n", ntohs((uint16_t)msg_join_recv_head.org_port), (uint16_t)ntohs(msg_join_recv_head.length));
	addr.s_addr = (uint32_t) msg_join_recv_head.org_ip;
	printf("%s\n", inet_ntoa(addr));
	printf("%u\n", ntohl((uint32_t)msg_join_recv_head.msg_id));
	
	puts("--body--");
	printf("%x\n", ntohl((uint16_t)msg_join_recv_body.status));
	
	*/
	
	if (ntohl((uint16_t)msg_join_recv_body.status) == 0x02000000)
	{
		printf("Connected %s and joined P2P network\n", ip_address);
		fprintf(log_file, "Connected %s and joined P2P network\n", ip_address);
	}
	else 
	{
		//msg_join_recv_body.status = JOIN_ACC;
		printf ("Connected to %s, but couldn't join P2P network\n", ip_address);
	}
	
	return sock_peer;
}


/*
Function: peer_handler
Handle the peer socket by checking the message type received and calling appropriate message handler
Accepts: Socket to the connected peer
Returns: Nothing
*/
void peer_handler (int peer_sock)
{
	struct in_addr addr;
	/** MESSAGE DECLARATIONS **/
	
	//structure for message header
	struct P2P_h msg_head;
	
	//structure for pong message (type a)
	struct P2P_h msg_pong_send_head;
	
	char buffer[1024];			//buffer to store reply
	int num_bytes_recv;
	
	struct sockaddr_in peer_addr;
	int addrlen = sizeof (peer_addr);
	
	num_bytes_recv = recv(peer_sock, buffer, 1024, 0);

	if (num_bytes_recv < 0)
	{
		perror("Peer recv failed");
		exit(-1);
	}
	else if (num_bytes_recv != 0)
	{
		//getting the header of the message
		msg_head = *((struct P2P_h *) buffer);

		//Check the Message type and handle accordingly
		
		switch (msg_head.msg_type)
		{
			case MSG_PING:
				handle_ping (peer_sock, buffer);	
				break;
				
			case MSG_JOIN:
				handle_join (peer_sock, buffer);
				break;
				
			case MSG_QHIT:
				handle_qhit(peer_sock, buffer);
				break;
				
			case MSG_QUERY:
				handle_query(peer_sock, buffer);				
				break;
			
			case MSG_PONG:
				handle_pong(peer_sock, buffer);
				break;
			default:
				puts ("Unknown Message Received");
		}
		
	}
	else 
	{
		getpeername(peer_sock, (struct sockaddr*)&peer_addr , (socklen_t*)&addrlen);
                printf("Peer disconnected! ip - %s\n" , inet_ntoa(peer_addr.sin_addr));
                fprintf(log_file, "Peer disconnected! ip - %s\n" , inet_ntoa(peer_addr.sin_addr));
                
                int i;
                for (i = 0; i < MAX_PEERS; i++) 
                {
                	if (peer_socks[i] == peer_sock)
                	{
                		peer_socks[i] = 0;
                	}
                }
	}
}


/*
Function: handle_pong
Handle PONG message type. Log the PONG message and display the list of neighbours if PONG B received.
Accepts: peer socket, buffer which contains the message
Returns: nothing
*/
void handle_pong(int peer_sock, char *buffer) 
{
	int i;
	
	struct in_addr addr;
	
	char ip[16];
	
	uint16_t payload_length;
	
	struct P2P_pong_front pong_front;
	struct P2P_pong_entry pong_entry;
	
	char *ptr_entry;
	
	//structure for message header
	struct P2P_h msg_head;
	
	msg_head = *((struct P2P_h *) buffer);
	
	payload_length = ntohs (msg_head.length);
	
	if (payload_length == 0) //pong message for TYPE A ping carries no payload
	{
		get_sock_ip (peer_sock, ip);
		fprintf (log_file, "<-- PONG (A) (msg id: %u) from %s\n", ntohl(msg_head.msg_id), ip);
	}
	else
	{
		fprintf (log_file, "<-- PONG (B) (msg id: %u) from %s\n", ntohl(msg_head.msg_id), ip);
		
		//copy pong_front from buffer. First 4 bytes after header
		memcpy (&pong_front, buffer+HLEN, 4);
		
		pong_front.entry_size = ntohs(pong_front.entry_size); //convert to host short
		
		ptr_entry = buffer + HLEN + 4;
		
		for (i = 0; i < pong_front.entry_size; i++)
		{
			memcpy (&pong_entry, ptr_entry, 8);	//8 is the size fo entry
			
			//build the ip to string
			addr.s_addr = (uint32_t) pong_entry.ip;
			
			printf ("IP - %s\tPort - %u\n", inet_ntoa(addr), ntohs(pong_entry.port));
			
			ptr_entry += 8; //point to the next entry
		}
		
	}

}


/*
Function: get_sock_ip
Stores the IP address of the socket specified in the character variable and returns the port.
Accepts: Socket number, Pointer to the IP address variable
Returns: Port of the socket
*/
int get_sock_ip (int sock, char *ip)
{
	//char *ip;
	
	struct sockaddr_in peer_addr;
	int addrlen = sizeof (peer_addr);
	
	getpeername(sock, (struct sockaddr*)&peer_addr , (socklen_t*)&addrlen);
	
        sprintf(ip, "%s", inet_ntoa(peer_addr.sin_addr));
        
        return ntohs(peer_addr.sin_port);
}


/* Function to calculate the adler32 hash adapted from wikipedia adler32 hash description */
uint32_t get_message_id() 
{
	const int MOD_ADLER = 65521;
	uint32_t a = 1, b = 0;
	size_t index;
	
	int hash;
	
	char data[100];
	int len;
	
	sprintf (data, "%s%u%u", own_ip, PORT, (unsigned)time(NULL));
	len = strlen (data);

	/* Process each byte of the data in order */
	for (index = 0; index < len; ++index)
	{
		a = (a + data[index]) % MOD_ADLER;
		b = (b + a) % MOD_ADLER;
	}

	hash = (b << 16) | a;
	return hash;
}


//To get my oun IP address
char * get_my_ip ()
{
	/*
	This code has been adapted from the following page
	http://www.binarytides.com/c-program-to-get-ip-address-from-interface-name-on-linux/ 
	*/
	int sock;
	
	char iface[] = "eth0";
	struct ifreq ifr;
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	
	//Get IPv4 IP address
    	ifr.ifr_addr.sa_family = AF_INET;
    	strcpy(ifr.ifr_name , iface);
    	ioctl(sock, SIOCGIFADDR, &ifr);
    	
    	return (inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr) );

}

/*  
Function: handle_ping
Handle the ping message. If PING A received, send PONG A. For PING B send PONG B specifying the list of neighbors.
Accepts: Peer socket, buffer which contains the message
Returns: nothing
*/
void handle_ping(int peer_sock, char *buffer)
{
	int i;
	
	struct in_addr addr;
	
	//structure for message header
	struct P2P_h msg_head;
	
	//structure for pong message (type a)
	struct P2P_h msg_pong_send_head;
	
	
	msg_head = *((struct P2P_h *) buffer);
	
	//store that ping received from a peer
	addr.s_addr = (uint32_t) msg_head.org_ip;
	
	if (msg_head.ttl == PING_TTL_HB) 
	{
		//Type A ping, respond with Type A pong
		fprintf (log_file, "<-- PING (A) (msg id: %u) from %s\n", ntohl(msg_head.msg_id), inet_ntoa(addr));
		
		//Build the type A pong message
		msg_pong_send_head.version = 1;
		msg_pong_send_head.ttl = PING_TTL_HB;	//pong message will have ttl 1
		msg_pong_send_head.msg_type = MSG_PONG;
		msg_pong_send_head.reserved = 0;
		msg_pong_send_head.org_port = htons(PORT);
		msg_pong_send_head.length = htons(0);
		msg_pong_send_head.org_ip = inet_addr(own_ip);
		msg_pong_send_head.msg_id = msg_head.msg_id;
	
		if(send(peer_sock , (void *) &msg_pong_send_head , sizeof(struct P2P_h), 0) < 0)
		{
			perror("Failed to send: ");
			exit(-1);
		}
		else
			fprintf (log_file, "--> PONG (A) (msg id: %u) to %s\n", ntohl(msg_pong_send_head.msg_id), inet_ntoa(addr));
	}
	else
	{
		//Type B ping, respond with Type B pong
		fprintf (log_file, "<-- PING (B) (msg id: %u) from %s\n", ntohl(msg_head.msg_id), inet_ntoa(addr));
		
		//To-DO
		//Send PONG B reply here
		
		struct in_addr addr_peer;
			
		uint16_t payloadlen;
		uint16_t num_entries;
		
		uint16_t port;
		
		struct P2P_pong_front pong_front;
		struct P2P_pong_entry pong_entry;
		
		char sender_ip[16];
		char sock_ip[16];
		
		char buffer_out[1024];
		char *ptr_buff;
		
		//Build the type B pong message
		msg_pong_send_head.version = 1;
		msg_pong_send_head.ttl = PING_TTL_HB;	//pong message will have ttl 1
		msg_pong_send_head.msg_type = MSG_PONG;
		msg_pong_send_head.reserved = 0;
		msg_pong_send_head.org_port = htons(PORT);
		
		msg_pong_send_head.org_ip = inet_addr(own_ip);
		msg_pong_send_head.msg_id = msg_head.msg_id;
		
		//initialize ptr to point after HEADER and BODY Front (4 bytes)
		ptr_buff = buffer_out + HLEN + sizeof (struct P2P_pong_front);
		
		num_entries = 0;
		//find number of neighbours excluding the sender
		for (i = 0; i < MAX_PEERS; i++)
		{
			if (peer_socks[i] != 0)
			{
				port = get_sock_ip (peer_socks[i], sock_ip); //get ip of neighbor
				get_sock_ip (peer_sock, sender_ip); //get ip of sender
				
				if (strcmp (sock_ip, sender_ip) != 0) //exclude the sender as a neighbour
				{
					//build the entry for pong body
					pong_entry.ip = inet_addr(sock_ip);
					pong_entry.port  = htons (port);
					pong_entry.sbz = 0;
					
					//copy the entry into the buffer
					memcpy (ptr_buff, &pong_entry, sizeof (struct P2P_pong_entry));
					
					//point the ptr for start of next entry
					ptr_buff = ptr_buff + sizeof (struct P2P_pong_entry);
					
					//increment the number of neighbours counter //To be used later for calculating payloadlen
					num_entries++;
				}
			}
		}
		
		//build the pong front
		pong_front.entry_size = htons (num_entries);
		pong_front.sbz = 0;
		
		//write the pong into buffer after header
		memcpy (buffer_out + HLEN, &pong_front, sizeof (struct P2P_pong_front));
		
		
		//entries have been now written into the buffer
		
		//calculate the payload length now
		//length of payload = length of pong front + (number of entries) * length of each entry
		payloadlen = sizeof (struct P2P_pong_front) + (num_entries * sizeof (struct P2P_pong_entry));
		
		//write payload length to message header
		msg_pong_send_head.length = htons(payloadlen);
		
		//copy message header into the buffer start
		memcpy (buffer_out, &msg_pong_send_head, HLEN);
		
		//send the buffer
		if(send(peer_sock, buffer_out, HLEN + payloadlen, 0) < 0)
		{
			perror("Failed to send: ");
			exit(-1);
		}
		else
			fprintf (log_file, "--> PONG (B) (msg id: %u) to %s \n", ntohl(msg_head.msg_id), inet_ntoa(addr));
	}
}


/*
Function: handle_join
Handles JOIN message. Accepts the connection and adds to the list of connected peers
Accepts: Peer socket, buffer containing message
Returns: nothing
*/
void handle_join(int peer_sock, char *buffer)
{
	struct in_addr addr;
	
	char buff_send[1024];
	
	//structure for message header
	struct P2P_h msg_req_head;
	
	//structure for join response
	struct P2P_h msg_resp_head;
	struct P2P_join msg_resp_body;
	
	//getting the request data from buffer
	msg_req_head = *((struct P2P_h *) buffer);
	
	addr.s_addr = (uint32_t) msg_req_head.org_ip;
	
	fprintf(log_file, "<-- JOIN request (msg id: %u) from %s\n", msg_req_head.msg_id, inet_ntoa(addr));
	
	//log that ping received from a peer
	addr.s_addr = (uint32_t) msg_req_head.org_ip;
	
	//Build the Join header response the network
	msg_resp_head.version = P_VERSION;
	msg_resp_head.ttl = 1;
	msg_resp_head.msg_type = MSG_JOIN;
	msg_resp_head.reserved = 0;
	msg_resp_head.org_port = htons(PORT);
	msg_resp_head.length = htons(2);
	msg_resp_head.org_ip = inet_addr(own_ip);
	msg_resp_head.msg_id = msg_req_head.msg_id;
	
	//build body for response
	msg_resp_body.status = htons(JOIN_ACC); //join accepted
	
	//writing header to buffer
	memcpy(buff_send, &msg_resp_head, HLEN);
	
	//writing body to buffer
	memcpy(buff_send+HLEN, &msg_resp_body, JOINLEN);
	
	//recently changed
	if(send(peer_sock , (void *) buff_send , HLEN+JOINLEN, 0) < 0)
	{
		perror("Failed to send: ");
		exit(-1);
	}
	else
	{
		fprintf(log_file, "--> JOIN request (msg id: %u) ACCEPTED for %s\n", msg_req_head.msg_id, inet_ntoa(addr));
		printf ("Peer connected! ip - %s \n", inet_ntoa(addr));
	}
}


/*
Function: send_query
Send the QUERY message specifying the query string to all connected peers
Accepts: Query string
Returns: nothing
*/
void send_query(char *query) 
{
	int i;
	char ip[16];
	
	char buffer_out[1024];
	
	struct P2P_h msg_head;	
	
	int querylen = strlen(query) + 1;  //plus 1 for null termiate char
	
	int flag_sent = 0;
	
	msg_head.version = P_VERSION;
	msg_head.ttl = 5; //hop for 5 nodes (to search for query). Traverse highest possible nodes
	msg_head.msg_type = MSG_QUERY;
	msg_head.reserved = 0;
	msg_head.org_port = htons(PORT);
	msg_head.length = htons((uint16_t) querylen);
	msg_head.org_ip = inet_addr(own_ip);
	
	msg_head.msg_id = htonl (get_message_id());
	
	//writing header to buffer
	memcpy(buffer_out, &msg_head, HLEN);
	
	//writing body to buffer
	memcpy(buffer_out+HLEN, query, querylen);
	
	for (i= 0; i < MAX_PEERS; i++)
	{
		if (peer_socks[i] != 0)
		{
			if (send(peer_socks[i], (void *) buffer_out, HLEN+querylen, 0) < 0)
			{
				perror ("Failed to send");
			}
			else
			{
				//query has been sent.. set the flag
				flag_sent = 1;
				
				//log
				get_sock_ip (peer_socks[i], ip);			
				fprintf (log_file, "--> QUERY %s to ip - %s\n", query, ip);
			}
		}
	}
	
	if (flag_sent == 0)
	{
		printf ("Query not sent. You are not connected to any node\n");
	}

}


/*
Function: handle_qhit
Handle QHIT message and output the list of key values in the payload.
Accepts: Peer socket, buffer containing message
Returns: nothing
*/
void handle_qhit(int peer_sock, char *buffer)
{
	int i;
	char ip[16];
	
	struct in_addr addr;
	
	//structure for message header
	struct P2P_h msg_head;
	
	//structure for resourse entries in the body
	struct P2P_qhit_resource msg_body_res;
	
	char *ptr_res_entry;
	
	//defining the entry size (first two bytes in the body) 
	uint16_t entry_size;
	
	//copy the header from buffer
	memcpy (&msg_head, buffer, HLEN);
	
	//copy the first two bytes of body into entry_size
	memcpy (&entry_size, buffer+HLEN, 2);
	
	//converting entry size to host byte order
	entry_size = ntohs (entry_size);
		
	//skip first 4 bytes of the body (because we already got entry_size)
	//copy first block of 8 bytes into resource structure
	ptr_res_entry = buffer+HLEN+4;
	memcpy (&msg_body_res, ptr_res_entry, 8);
	
	addr.s_addr = (uint32_t) msg_head.org_ip;
	
	printf ("\nQuery Hit from ip - %s, Number of entries: %u\n", inet_ntoa (addr), entry_size);
	
	//log to file
	fprintf (log_file, "<-- QHIT from ip - %s, Number of entries: %u\n", inet_ntoa (addr), entry_size);
	
	for (i = 1; i <= entry_size; i++) //loop thru other resources in the list
	{
		printf ("Resource ID: %u, Value: 0x%X\n", ntohs ((uint16_t)msg_body_res.id), ntohl (msg_body_res.value));
		
		//now copy the next entry
		ptr_res_entry = ptr_res_entry + sizeof (struct P2P_qhit_resource);
		memcpy (&msg_body_res, ptr_res_entry, 8);
	}
	printf ("\n"); 
	
	
}


/*
Function: handle_query
Handle the QUERY message. If query is hit, send back QHIT message with matched key value as a payload. Key values are stored in p2p.h header.
Accepts: Peer socket, buffer containing message
Returns: nothing
*/
void handle_query(int peer_sock, char *buffer) 
{
	int i;
	struct in_addr addr;
	
	int flag_found = 0;
	
	uint16_t sbz = 0; //should be zero
	
	char key_input[1024];
	char ip[16]; 
	
	uint32_t keyvalue;
	
	int payloadlength;
	uint16_t resource_id = htons (1); //hard coded res id as 1
	
	char buffer_out[1024]; //output buffer for QHIT HEADER+BODY
  	
	//structure for query message header
	struct P2P_h msg_query_head;
	
	//structure for qhit message header	
	struct P2P_h msg_qhit_head;
	
	//structure for resourse entries in the body
	struct P2P_qhit_resource msg_body_res;
	
	uint16_t entry_size;
	
	msg_query_head = *((struct P2P_h *) buffer);
	
	//get address field of message
	addr.s_addr = (uint32_t) msg_query_head.org_ip;
		
	//decrement the TTL - TO-DO
	
	//copy the body (query key) into key_input
	strcpy(key_input, buffer+HLEN);
	
	//log to file that Query received
	get_sock_ip (peer_sock, ip);
	fprintf (log_file, "<-- QUERY %s from %s\n", key_input, ip);
	
	for (i = 0; i < NUM_KEYS; i++)
	{
		if (strcmp (key_input, keys[i]) == 0) 
		{
			msg_qhit_head.version = P_VERSION;
			msg_qhit_head.ttl = 5; //ttl should be 5 for qhit
			msg_qhit_head.msg_type = MSG_QHIT;
			msg_qhit_head.reserved = 0;
			msg_qhit_head.org_port = htons(PORT);
			msg_qhit_head.org_ip = inet_addr(own_ip);
			msg_qhit_head.msg_id = msg_query_head.msg_id;	
			
			payloadlength = 12; //for now 12 since we are only considering single resource for a key
			msg_qhit_head.length = htons((uint16_t) payloadlength);
			
			entry_size = htons (1); //only 1 entry (no multiple resources supported
			
			keyvalue = htonl (keyvalues[i]);
			
			//copy header into buffer
			memcpy (buffer_out, &msg_qhit_head, HLEN);
			
			//copy entry size into first two bytes after header
			memcpy (buffer_out+HLEN, &entry_size, 2);
			
			//append zeros for next two bytes
			memcpy (buffer_out+HLEN+2, &sbz, 2);
			
			//appdned resource ID for next two bytes
			memcpy (buffer_out+HLEN+2+2, &resource_id, 2);
			
			//append zeros for next two bytes
			memcpy (buffer_out+HLEN+2+2+2, &sbz, 2);
			
			//append value for next 4 bytes
			memcpy (buffer_out+HLEN+2+2+2+2, &keyvalue, 4); 
			
			if (send(peer_sock, (void *) buffer_out, HLEN + payloadlength, 0) < 0)
			{
				perror ("Failed to send QHIT");
			}
			else
			{	
				flag_found = 1;
				fprintf (log_file, "--> QHIT %x to %s\n", keyvalues[i], ip);
			}
			break;
		}
	}
	
	if (flag_found == 0)
	{
		fprintf (log_file, "No key-value pair found for query key %s\n", key_input);
	}
	
}




