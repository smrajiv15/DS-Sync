#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "message.h"

struct time_reply master_off;
char ip_address[LEN];
int cached = 0;
union server_response csearch_ds;
union server_response csearch_gs;

#define PORT 9901

pthread_mutex_t off_lock = PTHREAD_MUTEX_INITIALIZER;

struct sync_time_in {
	char *slave1_ip;
	char *slave2_ip;
};

/* contact_server - This method is repsonsible for contacting to the sever
 * for each of the client request.
 */
 
void contact_server(struct client_request *req, union server_response *resp,char *ip_address) {

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
                printf("Error: Connecting Server Failed\n");
                exit(0);
        }

        n = write(socket_c, req, sizeof(struct client_request));
        if(n < 0) {
                printf("Error: Unable to Contact the server\n");
                exit(0);
        }

        while((n = recv(socket_c, resp, sizeof(union server_response), 0) == 0));

        close(socket_c);
}

/* get_self_time - this function gets it own time to sync it with the slaves */

void get_self_time(unsigned *secs, unsigned *msecs) {

        struct tm *tm_p;
	struct timeval timer;
	
	gettimeofday(&timer, NULL);
        tm_p = localtime(&timer.tv_sec);
        *secs = tm_p->tm_sec;
        *msecs = (timer.tv_usec / 1000);
}

/* set_offset - this function sets the offset to itself and slaves it manages */

void set_offset(unsigned secs, unsigned msecs, unsigned avg_secs, unsigned avg_msecs, struct time_reply *ent) {

	pthread_mutex_lock(&off_lock);
	if(avg_secs > secs) {
		ent->s_sign = '+';	
		ent->secs = avg_secs - secs;
	} else {
		ent->s_sign = '-';
		ent->secs = secs - avg_secs; 
	}

	if(avg_msecs > msecs) { 
		ent->msecs = avg_msecs - msecs;
		ent->m_sign = '+';
	} else {
		ent->msecs = msecs - avg_msecs;
		ent->m_sign = '-';
	}
	pthread_mutex_unlock(&off_lock);
}

/* synchronize_time - this is a master front end server which will sync
 * the time of all slaves for every minute */

void *synchronize_time(void *arg){

	struct client_request req;
	union server_response resp;
	struct time_reply slave_off;
	unsigned total_msecs, self_secs, self_msecs, avg_secs, avg_msecs;
	unsigned slave1_secs, slave1_msecs;
	unsigned slave2_secs, slave2_msecs;
	struct sync_time_in *in = (struct sync_time_in*)arg; 
	

	while(1) {
		//getting the self time to adjust synchronization
		get_self_time(&self_secs, &self_msecs);
		
		//geting the slave1 timings to adjust the clock
		req.method_id = TIME_SYNC;
		contact_server(&req, &resp, in->slave1_ip);
		slave1_secs  = resp.time.secs;	
		slave1_msecs = resp.time.msecs;
			
		//getting the slave2 timings to adjust the clock
		req.method_id = TIME_SYNC;
		contact_server(&req, &resp, in->slave2_ip);
		slave2_secs  = resp.time.secs;	
		slave2_msecs = resp.time.msecs;
	
		//Average Tollerance Calculation 	
		total_msecs = slave1_msecs + slave2_msecs + self_msecs;
		avg_secs = (self_secs + slave1_secs + slave2_secs + (total_msecs / 1000)) / 3;
		avg_msecs = (total_msecs % 1000)/ 3;

		//set time offset - self
		set_offset(self_secs, self_msecs, avg_secs, avg_msecs, &master_off);
		
		//set time offset - slave-1
		set_offset(slave1_secs, slave1_msecs, avg_secs, avg_msecs, &slave_off);	

		//sending the offset to the slave 1
		req.method_id = TIME_SET;
		memcpy(&req.off, &slave_off, sizeof(struct time_reply));	
		contact_server(&req, &resp, in->slave1_ip);

		//set time offset - slave-2
		set_offset(slave2_secs, slave2_msecs, avg_secs, avg_msecs, &slave_off);

		//Sending the time offset - Slave 2	
		req.method_id = TIME_SET;
		memcpy(&req.off, &slave_off, sizeof(struct time_reply));	
		contact_server(&req, &resp, in->slave2_ip);
	
		sleep(60);
	}
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

        contact_server(req, &resp, ip_address);

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
	if(master_off.s_sign == '+') {
		req->off.secs = tm_p->tm_sec + master_off.secs;
	} else {		
		req->off.secs = tm_p->tm_sec - master_off.secs;
	}

	if(master_off.m_sign == '+') {
		req->off.msecs = (tv.tv_usec / 1000) + master_off.msecs;
	} else {
		if((tv.tv_usec / 1000) < master_off.msecs) {
			req->off.msecs = ((1000 + (tv.tv_usec / 1000)) - master_off.msecs);
			req->off.secs--;
		} else {		
			req->off.msecs = (tv.tv_usec / 1000) - master_off.msecs;
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
	struct sync_time_in in;
	int sockfd, newsockfd;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	pthread_t thread_id, req_thread;
	pthread_attr_t attr;

	if(argc < 4) {
		printf("Usage:%s slave1_ip slave2_ip data_base_ip\n", argv[0]);
		exit(0);
	}
	
	//get the time of the slaves to know the time to synchronize
	in.slave1_ip = argv[1];
	in.slave2_ip = argv[2];
	strcpy(ip_address, argv[3]);

	if((pthread_create(&thread_id, NULL, synchronize_time,(void *)&in)) < 0) {
		printf("Error : Unable to create Request Thread\n");
		exit(0);
	} 

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

                if((pthread_create(&req_thread, &attr, process_request, (void*)&newsockfd)) < 0) {
                        error("Thread Creation Failed");
                }
        }

        close(sockfd);
        return 0;
	
}
