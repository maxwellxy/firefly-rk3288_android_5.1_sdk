#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include<stdlib.h>


#ifdef ANDROID
#include <cutils/properties.h>
#endif

#define USE_UNIX_SOCKET

#ifdef USE_UNIX_SOCKET
#include <sys/un.h>
const char * SOCKET_PATH = "/data/gps/brcm_gps_unix_socket";

#else

unsigned short get_gps_serv_port()
{
#ifndef ANDROID
	return 10001;
#else
	char port_buf[20];
	memset(port_buf, 0, sizeof(20));
	property_get("brcm.gps.serv.port", port_buf, "10001");	
	return atoi(port_buf);
#endif
}

#endif

int main(int argc, char **argv)
{
	int sockfd;
	int c, i;
	char buf[100] = {0xFF, 0x00, 0xFD, 0x40, 0x00, 0x00, 0xE6, 0xFC}; // HCI command(0xFC28) that will get lpm params
								  // there is not params for this command.
	/*
		01 1C FC 05 00 00 00 00 00
		01 00 FC 05 F3 88 01 01 00
	*/
#ifdef USE_UNIX_SOCKET
	int t, len;
	struct sockaddr_un remote;
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
                perror("socket");
                exit(1);
    }

    printf("Trying to connect...\n");

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCKET_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(sockfd, (struct sockaddr *)&remote, len) == -1) {
                perror("connect");
                exit(1);
    }else
    {
      	printf("Connected to unix_socket succefully.\n");
    }
#else
	struct sockaddr_in servaddr;

	sockfd=socket(AF_INET,SOCK_STREAM,0);

	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(get_gps_serv_port());

	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
 
     	if(connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))<0)
	{
		printf("Error connecting");
		return 0;
	}
	else
		printf("Connected\n");
#endif

	if((c = send(sockfd, buf, 8, 0))<0)
    {
     	printf("Client: Error Sending Data %d\n", c);
	}
	
#if 1     
	if((c = recv(sockfd,buf,100,0))>0)
	{	
		for(i = 0; i < c; i++)
		printf("0x%2x \n", buf[i]); 
	}
#endif
	close(sockfd);
	return 0;
}
