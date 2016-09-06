#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include "message.h"

struct time_reply slave1_offset;
char ip_address[256];
int cached = 0;
union server_response csearch_ds;
union server_response csearch_gs;

#define PORT 9901

pthread_mutex_t off_lock = PTHREAD_MUTEX_INITIALIZER;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/* time_sync_req - This function gives its time to the master. The master 
 * compute the corresponding offset and gives it to the slaves it manages */

void time_sync_req(int socket_c) {
	struct timeval time;
	struct tm *tm_p;
	union server_response resp;
	
	gettimeofday(&time, NULL);
	tm_p = localtime(&time.tv_sec);
	
	
        resp.time.secs = tm_p->tm_sec;
        resp.time.msecs = (time.tv_usec / 1000);

	write(socket_c, &resp, sizeof(union server_response));
        close(socket_c);
}

/* time_sync_req - this function sets the offset value for itself which
 * is given by the master node */

void time_sync_set(int socket_c, struct client_request *req) {
	
	union server_response resp;
	
	pthread_mutex_lock(&off_lock);
	memcpy(&slave1_offset, &(req->off), sizeof(struct time_reply));
	pthread_mutex_unlock(&off_lock);
	
	resp.order.status = SUCESS;
	write(socket_c, &resp, sizeof(union server_response));
        close(socket_c);
}

/* contact_server - This method is repsonsible for contacting to the sever
 * for each of the client request.
 */

void contact_server(struct client_request *req, union server_response *resp) {

        int n = 0, socket_c;
        struct sockaddr_in server;

        bzero(&server, sizeof(struct sockaddr_in));

        server.sin_family = AF_INET;
        server.sin_port  = htons(PORT);
        server.sin_addr.s_addr = inet_addr(ip_address);

        socket_c = socket(AF_INET, SOCK_STREAM, 0);
        if(socket_c < 0) {
                printf("Error: Unable to create Socket\n");
                exit(0);
        }

        if(connect(socket_c,(struct sockaddr *)&server,sizeof(struct sockaddr_in)) < 0) {
                error("Error: Connecting Server Failed\n");
        }

        n = write(socket_c, req, sizeof(struct client_request));
        if(n < 0) {
                printf("Error: Unable to Contact the server\n");
                exit(0);
        }

        while((n = recv(socket_c, resp, sizeof(union server_response), 0) == 0));

        close(socket_c);
}

/* send_default_req - It is a by-pass function which get request from the 
 * client and send it to the backend server */

void send_default_req(int socket_c, struct client_request *req) {
	
	union server_response resp;
	int found = 0;

	if((req->method_id == SEARCH) && cached) {
		if((cached & 0x1) && (req->in.topic_id == 1)) {
			write(socket_c, &csearch_ds, sizeof(union server_response));
			found = 1;
		} else if((cached & 0x2) && (req->in.topic_id == 2)) {	
			write(socket_c, &csearch_gs, sizeof(union server_response));
			found = 1;
		}
	
		if(found) {
        		close(socket_c);
			return;
		}
	}

	contact_server(req, &resp);	

	if(req->method_id == SEARCH) {
		if(req->in.topic_id == 1) {
			memcpy(&csearch_ds, &resp, sizeof(union server_response));
			cached = (cached | 1);
		} else {	
			memcpy(&csearch_gs, &resp, sizeof(union server_response));
			cached = (cached | 2);
		} 
	}

 	write(socket_c, &resp, sizeof(union server_response));
        close(socket_c);
}

/* set_time - this function will set the time stamp for each of the request
 * recieved by this server */

void set_time(struct client_request *req) {
        struct timeval tv;
        struct tm *tm_p;

	gettimeofday(&tv, NULL);
        tm_p = localtime(&tv.tv_sec);
        req->off.hours = tm_p->tm_hour;
        req->off.mins = tm_p->tm_min;

	pthread_mutex_lock(&off_lock);
        if(slave1_offset.s_sign == '+') {
                req->off.secs = tm_p->tm_sec + slave1_offset.secs;
        } else {
                req->off.secs = tm_p->tm_sec - slave1_offset.secs;
        }

        if(slave1_offset.m_sign == '+') {
                req->off.msecs = (tv.tv_usec / 1000) + slave1_offset.msecs;
        } else {
		if((tv.tv_usec / 1000) < slave1_offset.msecs) {
                        req->off.msecs = ((1000 + (tv.tv_usec / 1000)) - slave1_offset.msecs);
                        req->off.secs--;
         	} else { 
                	req->off.msecs = (tv.tv_usec / 1000) - slave1_offset.msecs;
		}
        }

	pthread_mutex_unlock(&off_lock);
}

/* process_request - This is an thread function called for each request 
 * recived by this server */

void *process_request(void *arg) {
	struct client_request req;
	int n;
	int socket_c = *(int*)arg;
	
	while((n = recv(socket_c, &req, sizeof(struct client_request), 0) == 0));

	switch(req.method_id) {
	case TIME_SYNC:	
		time_sync_req(socket_c);
		break;
	case TIME_SET:
		time_sync_set(socket_c, &req);
		break;
	default:
		if(req.method_id == ORDER) {
			set_time(&req);
		}
		send_default_req(socket_c, &req);
		break;
	}

	pthread_exit((void*)1);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd;
	socklen_t clilen;
	pthread_t thread_id;
	struct client_request req;
	union server_response resp;
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	pthread_attr_t attr;

	if (argc <  2) {
		printf("Usage: %s data_base_ip\n", argv[0]);
		exit(1);
	}

	strcpy(ip_address, argv[1]);

	pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) 
		error("ERROR opening socket");

     	bzero((char *) &serv_addr, sizeof(serv_addr));
    	serv_addr.sin_family = AF_INET;
     	serv_addr.sin_addr.s_addr = INADDR_ANY;
     	serv_addr.sin_port = htons(PORT);
	
     	if(bind(sockfd, (struct sockaddr *) &serv_addr,
              				sizeof(serv_addr)) < 0) 
  		error("ERROR on binding");

     	listen(sockfd,5);
     	clilen = sizeof(cli_addr);

	for(;;) {

     		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
     		if(newsockfd < 0) 
			error("ERROR on accept");
		
		if((pthread_create(&thread_id, &attr, process_request, (void*)&newsockfd)) < 0) {
                        error("Thread Creation Failed");
		}
	}

     	close(sockfd);
     	return 0; 
}
