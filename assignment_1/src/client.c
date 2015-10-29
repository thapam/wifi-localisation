#include <stdio.h>
#include <stdlib.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include "p2p.h"
 
int main(int argc , char *argv[])
{
	struct in_addr addr;
	
	/* Peer (server) declarations */
	
	int sock_peer;				//socket descriptor
	struct sockaddr_in peer;		//Server (peer)
	char buffer[1024];			//buffer to store reply
	int num_bytes_recv;

	/* Protocol related  declarations */
	
	//Structure for peer response for JOIN message
	struct P2P_h msg_join_recv_head;
	struct P2P_join msg_join_recv_body;
	
	//structure for ping message (type a)
	struct P2P_h msg_ping_recv_head;
	
	//structure for pong message (type a)
	struct P2P_h msg_pong_send_head;
	
		
	//Variable for sending message
	struct P2P_h msg_join_send;
	

	//Build the message to JOIN the network
	msg_join_send.version = 1;
	msg_join_send.ttl = 5;
	msg_join_send.msg_type = MSG_JOIN;
	msg_join_send.reserved = 0;
	msg_join_send.org_port = htons(6346);
	msg_join_send.length = htons(0);
	msg_join_send.org_ip = inet_addr("10.0.2.15");
	msg_join_send.msg_id = htonl(65537);


	//Create socket
	sock_peer = socket(AF_INET , SOCK_STREAM , 0);
	if (sock_peer == -1)
	{
		perror("Could not create socket: ");
		exit (-1);
	}

	puts("Socket Created");
	
	//build the server object. For connecting to bootstrap peer.
	peer.sin_addr.s_addr = inet_addr("130.233.195.31");
	peer.sin_family = AF_INET;
	peer.sin_port = htons(6346);

	//Connect to remote server
	if (connect(sock_peer, (struct sockaddr *)&peer , sizeof(peer)) < 0)
	{
		perror("Cannot connect to peer: ");
		exit(-1);
	}

	puts("Connected to peer");

	//Send some data
	if(send(sock_peer , (void *) &msg_join_send , sizeof(struct P2P_h), 0) < 0)
	{
		perror("Failed to send: ");
		exit(-1);
	}

	puts("Join request sent");

	//Receive a reply from the server (store it in buffer)
	num_bytes_recv = recv(sock_peer, buffer, 1024, 0);
	
	if (num_bytes_recv < 0)
	{
		perror("recv failed");
		exit(-1);
	}
	
	printf("Number of bytes received: %d\n", num_bytes_recv);
	
	//Store the buffer into proper structures
	msg_join_recv_head = *((struct P2P_h *) buffer);
	msg_join_recv_body = *((struct P2P_join *) (buffer + sizeof(struct P2P_h)));
	
	puts("--header--");
	printf("%u\t%u\t%u\t%u\n", (uint8_t)msg_join_recv_head.version, (uint8_t)msg_join_recv_head.ttl, (uint8_t)msg_join_recv_head.msg_type, (uint8_t)msg_join_recv_head.reserved);
	printf("%u\t%u\n", ntohs((uint16_t)msg_join_recv_head.org_port), (uint16_t)ntohs(msg_join_recv_head.length));
	addr.s_addr = (uint32_t) msg_join_recv_head.org_ip;
	printf("%s\n", inet_ntoa(addr));
	printf("%u\n", ntohl((uint32_t)msg_join_recv_head.msg_id));
	
	puts("--body--");
	printf("%X\n", ntohl((uint16_t)msg_join_recv_body.status));
	
	while(1)
	{
		//sleep for 5 seconds
		sleep(5);
		
		num_bytes_recv = recv(sock_peer, buffer, 1024, 0);
	
		if (num_bytes_recv < 0)
		{
			perror("recv failed");
			exit(-1);
		}
		else
		{
			msg_ping_recv_head = *((struct P2P_h *) buffer);
			printf("Message Received Type: %u PING. ID: %u\n", (uint8_t)msg_ping_recv_head.msg_type, ntohl((uint32_t)msg_ping_recv_head.msg_id));
			
			//Build the pong message
			msg_pong_send_head.version = 1;
			msg_pong_send_head.ttl = 1;
			msg_pong_send_head.msg_type = MSG_PONG;
			msg_pong_send_head.reserved = 0;
			msg_pong_send_head.org_port = htons(6346);
			msg_pong_send_head.length = htons(0);
			msg_pong_send_head.org_ip = inet_addr("10.0.2.15");
			msg_pong_send_head.msg_id = msg_ping_recv_head.msg_id;
			
			if(send(sock_peer , (void *) &msg_pong_send_head , sizeof(struct P2P_h), 0) < 0)
			{
				perror("Failed to send: ");
				exit(-1);
			}
			else
				puts ("Sent PONG");
		}
	}
	
	return 0;
}











