#define MAX_PEERS 15
#define PORT 6346

#include <stdio.h>
#include <stdlib.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h> //inet_addr
#include "p2p.h"


void peer_handler (int peer_sock);
int connect_peer (char *ip_address, int port);
void handle_ping(int peer_sock, char *buffer);
void handle_join(int peer_sock, char *buffer);
char * get_my_ip ();
void get_sock_ip (int sock, char *ip);
uint32_t get_message_id();

char *own_ip;
int peer_socks[MAX_PEERS] = {0};

FILE *log_file;
 
int main(int argc , char *argv[])
{
	struct in_addr addr;
	
	char user_input_buff[1024];
	
/*--------------------------------------------------*/
	int listener_sock, max_sock, sock, peer_sock1;
	
	struct sockaddr_in listener_addr, peer_addr;
	
	int i, max_peers = MAX_PEERS;	
	int activity;

	unsigned int addrlen;
	
	/* Socket set to handle multiple sockets*/
	fd_set readfds;
/*--------------------------------------------------*/

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
	
	
	//now connect to one peer
	
	peer_socks[0] = connect_peer ("130.233.195.30", 6346);
	
	//loop indifinitely thru all sockets
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
		
		activity = select (max_sock + 1, &readfds, NULL, NULL, NULL);
		
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
		
		//check other FDs. if set it means activity has happened
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
			
			char *cmd, *arg1, *arg2;
			
			if ((len = read(0, user_input_buff, 1024)) > 0) 
			{
				user_input_buff[len-1] = '\0';
				strncpy (command, user_input_buff, strlen (user_input_buff) + 1);
				char ip_str[16];
				
				cmd = strtok (command, " ");
				
				if (strcmp (cmd, "exit") == 0)
				{
					exit (0);
				}
				else if (strcmp (cmd, "peers") == 0) 
				{
					printf ("Listing all connected peers\n");
					for (i = 0; i < MAX_PEERS; i++)
					{
						if (peer_socks[i] > 0)
						{
							get_sock_ip (peer_socks[i], ip_str);
							printf ("%s\n", ip_str);
						}
					}
				}
				else if (strcmp (cmd, "query") == 0)
				{
					arg1 = strtok(NULL, " "); //ip address
					printf ("You want to query %s", arg1);
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
		
		fclose(log_file);
	}
	
	return 0;
}


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
	sock_peer = socket(AF_INET , SOCK_STREAM , 0);
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
	
	//writing header to buffer
	memcpy(&msg_join_recv_head, buffer, HLEN);
	
	//writing body to buffer
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
	}
	else 
	{
		//msg_join_recv_body.status = JOIN_ACC;
		printf ("Connected to %s, but couldn't join P2P network\n", ip_address);
	}
	
	return sock_peer;
}

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
				
			case MSG_QUERY:
				//handle_query(peer_sock, buffer);
			
			default:
				puts ("Unknown");
		}
		
	}
	else 
	{
		getpeername(peer_sock, (struct sockaddr*)&peer_addr , (socklen_t*)&addrlen);
                printf("%s disconnected, port %d \n" , inet_ntoa(peer_addr.sin_addr) , ntohs(peer_addr.sin_port));
	}
}


void get_sock_ip (int sock, char *ip)
{
	//char *ip;
	
	struct sockaddr_in peer_addr;
	int addrlen = sizeof (peer_addr);
	
	getpeername(sock, (struct sockaddr*)&peer_addr , (socklen_t*)&addrlen);
	
        sprintf(ip, "%s", inet_ntoa(peer_addr.sin_addr));
        
        //return ip;
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

void handle_ping(int peer_sock, char *buffer)
{
	struct in_addr addr;
	
	//structure for message header
	struct P2P_h msg_head;
	
	//structure for pong message (type a)
	struct P2P_h msg_pong_send_head;
	
	
	msg_head = *((struct P2P_h *) buffer);
	
	//log that ping received from a peer
	addr.s_addr = (uint32_t) msg_head.org_ip;
	
	if (msg_head.ttl == PING_TTL_HB) 
	{
		//Type A ping, respond with Type A pong
		fprintf (log_file, "<-- PING (A) (msg id: %u) from %s\n", ntohl(msg_head.msg_id), inet_ntoa(addr));
		
		//Build the type B pong message
		msg_pong_send_head.version = 1;
		msg_pong_send_head.ttl = PING_TTL_HB;	//heart beat pong
		msg_pong_send_head.msg_type = MSG_PONG;
		msg_pong_send_head.reserved = 0;
		msg_pong_send_head.org_port = htons(6346);
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
		
		fprintf (log_file, "--> PONG (B) (msg id: %u) to %s \n", ntohl(msg_head.msg_id), inet_ntoa(addr));
	}
}


void handle_join(int peer_sock, char *buffer)
{
	struct in_addr addr;
	
	char buff_send[1024];
	
	//structure for message header
	struct P2P_h msg_req_head;
	
	//structure for join response
	struct P2P_h msg_resp_head;
	struct P2P_join msg_resp_body;
	
	printf("<-- JOIN request (msg id: %u) from from %s\n", msg_req_head.msg_id, inet_ntoa(addr));
	
	//getting the request data from buffer
	msg_req_head = *((struct P2P_h *) buffer);
	
	//log that ping received from a peer
	addr.s_addr = (uint32_t) msg_req_head.org_ip;
	
	//Build the Join header response the network
	msg_resp_head.version = P_VERSION;
	msg_resp_head.ttl = 1;
	msg_resp_head.msg_type = MSG_JOIN;
	msg_resp_head.reserved = 0;
	msg_resp_head.org_port = htons(6346);
	msg_resp_head.length = htons(2);
	msg_resp_head.org_ip = inet_addr(own_ip);
	msg_resp_head.msg_id = msg_req_head.msg_id;
	
	//build body for response
	msg_resp_body.status = htons(JOIN_ACC); //join accepted
	
	//writing header to buffer
	memcpy(buff_send, &msg_resp_head, HLEN);
	
	//writing body to buffer
	memcpy(buff_send+HLEN, &msg_resp_body, JOINLEN);
	
	if(send(peer_sock , (void *) &buff_send , HLEN+JOINLEN, 0) < 0)
	{
		perror("Failed to send: ");
		exit(-1);
	}
	else
		printf("--> JOIN request (msg id: %u) ACCEPTED for %s\n", msg_req_head.msg_id, inet_ntoa(addr));
}








