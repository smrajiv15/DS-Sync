#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include "message.h"

#define PORT		 9901

/* Search - This function will take care of the search request received from
 * the user and sends the corresponding result to the user */

void search(int topic_id, int socket_c) {
	int i, books, id;
	union server_response resp;
	char topic_name[LEN];
	struct timeval start, end;

	gettimeofday(&start, NULL);

	pthread_mutex_lock(&search_count);
	search_request_count++;
	pthread_mutex_unlock(&search_count);

	for(i = 0; i < store.total_topics; i++) {
		id = store.topic_list[i].topic_id;
		if(id == topic_id) {
			resp.search.total_books = store.topic_list[i].diff_books;
			books = resp.search.total_books;
			memcpy(&resp.search.info, &store.topic_list[i].info, \
						books * sizeof(struct book_info));
			resp.search.status = SUCESS; 			
			break;
		}
	}

	write(socket_c, &resp, sizeof(union server_response));
	
	gettimeofday(&end, NULL);

	pthread_mutex_lock(&time_search);
	search_time += (((end.tv_sec * 1000000 + end.tv_usec) \
		  - (start.tv_sec * 1000000 + start.tv_usec)));	
	pthread_mutex_unlock(&time_search);
	
	close(socket_c);   							
}

/* look_up - This function helps the user to lookup the details of the book
 * with item number provided by them */

void look_up(unsigned item_no, int socket_c) {
	int i, j, found = 0;
	unsigned item;
	union server_response resp;
	struct timeval start, end;

	gettimeofday(&start, NULL);

	pthread_mutex_lock(&lookup_count);
	lookup_request_count++;
	pthread_mutex_unlock(&lookup_count);

	for(i = 0; i < 2; i++) {
		for(j = 0; j< 2; j++) {
			item = store.topic_list[i].info[j].item_number;
			if(item == item_no) {
				memcpy(&resp.look.info, &store.topic_list[i].info[j],\
								sizeof(struct book_info)); 	
				memcpy(resp.look.topic, store.topic_list[i].topic_name, LEN);
				resp.look.status = SUCESS;
				found = 1;
				break;	
			}
		}
		if(found)
			break;
	}

	write(socket_c, &resp, sizeof(union server_response));

	gettimeofday(&end, NULL);
	
	pthread_mutex_lock(&time_lookup);
	lookup_time += (((end.tv_sec * 1000000 + end.tv_usec) \
                  - (start.tv_sec * 1000000 + start.tv_usec)));
	pthread_mutex_unlock(&time_lookup);

	close(socket_c);   			
}

/* order - This function does the reservation for the particular user to do
 * order the books of their intrest and gives the order confirmation to
 * the user. */

void order(unsigned item_no, int socket_c) {
	int i, j, found = 0;
	unsigned item;
	union server_response resp;
	struct timeval start, end;
	
	gettimeofday(&start, NULL);

	pthread_mutex_lock(&order_count);
	buy_request_count++;
	
	if((buy_request_count % DISC_REQ) == 0)
		resp.order.discount = 1;
	pthread_mutex_unlock(&order_count);

	for(i = 0; i < 2; i++) {
		for(j = 0; j< 2; j++) {
			item = store.topic_list[i].info[j].item_number;
			if(item == item_no) {
				if(store.topic_list[i].info[j].avail_items > 0) {
					pthread_mutex_lock(&book_order);	
					store.topic_list[i].info[j].avail_items--;
					pthread_mutex_unlock(&book_order);	
					resp.order.status = SUCESS;
					good_oreders++;	
				} else {
					resp.order.status = FAILURE;
					failed_orders++;
				} 	
				found = 1;
				break;	
			}
		}
		if(found)
			break;
	}

	write(socket_c, &resp, sizeof(union server_response));

	gettimeofday(&end, NULL);

	pthread_mutex_lock(&time_order);	
	order_time += (((end.tv_sec * 1000000 + end.tv_usec) \
                  - (start.tv_sec * 1000000 + start.tv_usec)));
	pthread_mutex_unlock(&time_order);
	
	
	close(socket_c);   	
}

/* report_request - This functions gives the user the number of request
 * registered for each of the services. */

void report_request(int service, int socket_c) {
	union server_response resp;

	switch(service) {
	case SEARCH:
		pthread_mutex_lock(&search_count);
		resp.search.status = search_request_count;
		pthread_mutex_unlock(&search_count);	
		break;
	case LOOK_UP:
		pthread_mutex_lock(&lookup_count);
		resp.search.status = lookup_request_count;
		pthread_mutex_unlock(&lookup_count);
		break;
	case ORDER:
		pthread_mutex_lock(&order_count);
		resp.search.status = buy_request_count;
		pthread_mutex_unlock(&order_count);
	}	

	write(socket_c, &resp, sizeof(union server_response));
	close(socket_c);   	
}

/* report_good_orders - This function gives the users the number of good orders
 * registered in the server. */

void report_good_orders(int socket_c) {

	union server_response resp;

	resp.order.status = good_oreders;

	write(socket_c, &resp, sizeof(union server_response));
	close(socket_c);   	
}

/* report_failed_orders - This function gives the users the number of failed
 * orders registered in the server */

void report_failed_orders(int socket_c) {
	union server_response resp;
	
	resp.order.status = failed_orders;

	write(socket_c, &resp, sizeof(union server_response));
	close(socket_c);  
}

/* report_service_perf - this function takes care of any queries related to the
 * performance asked by the user. It frames the server_response structure and 
 * writes to the corresponding socket for the user to read the data. */

void report_service_perf(int item_no, int socket_c) {

	union server_response resp;

	switch(item_no) {
	case SEARCH:
		pthread_mutex_lock(&time_search);
		resp.perf.time = search_time;
		pthread_mutex_unlock(&time_search);

		pthread_mutex_lock(&search_count);
		resp.perf.request_count = search_request_count;
		pthread_mutex_unlock(&search_count);
		break;
	case LOOK_UP:
		pthread_mutex_lock(&time_lookup);
		resp.perf.time = lookup_time;
		pthread_mutex_unlock(&time_lookup);

		pthread_mutex_lock(&lookup_count);
		resp.perf.request_count = lookup_request_count;
		pthread_mutex_unlock(&lookup_count);
		break;
	case ORDER:
		pthread_mutex_lock(&time_order);
		resp.perf.time = order_time;
		pthread_mutex_unlock(&time_order); 
	
		pthread_mutex_lock(&order_count);
		resp.perf.request_count = buy_request_count;
		pthread_mutex_unlock(&order_count);
		break;
	}

	write(socket_c, &resp, sizeof(union server_response));
	close(socket_c);  

}

/* process_request - this is a threaded function which processes the user request
 * from the remote. It gets invoked when ever a user request arrives */ 

void * process_request(void *arg) {

	unsigned topic_id, item_number;	
	struct client_request req;
	int method, n = 0;
	int socket_c = *(int *)arg;


	while((n = recv(socket_c, &req, sizeof(struct client_request), 0) == 0)); 
	method = req.method_id;

	switch(method) {
	case SEARCH:
		topic_id = req.in.topic_id;
		search(topic_id, socket_c);
		break;
	case LOOK_UP:
		item_number = req.in.item_number;
		look_up(item_number, socket_c);
		break;
	case ORDER:
		item_number = req.in.item_number;
		order(item_number, socket_c);
		break;
	case RQN:
		item_number = req.in.item_number;
		report_request(item_number, socket_c);
		break;
	case RGO:
		report_good_orders(socket_c);
		break;
	case RFO:
		report_failed_orders(socket_c);
		break;
	case RSP:
		item_number = req.in.item_number;
		report_service_perf(item_number, socket_c);
		break;		
	}
	
	pthread_exit((void*)1);	

}


int main() {

	int socket_s, socket_c, n;
	struct sockaddr_in server, client;
	socklen_t client_len;
	pthread_attr_t attr;
	pthread_t thread_id;

	/* Each thread is created on the runtime when needed. All threads are
	 * detachable threads. So, no need of joining it. It also automatically
	 * deletes all of its resourse independed of any joining */

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	/*Statical Assigning of Book details */
	store.total_topics = 2;
	strcpy(store.topic_list[0].topic_name, "Distributed Systems\n");
	strcpy(store.topic_list[1].topic_name, "Graduate School");	
	store.topic_list[0].topic_id = 1;
	store.topic_list[1].topic_id = 2;
	
	/* Assigning Total Books in the topic */
	store.topic_list[0].diff_books = 2;
	store.topic_list[1].diff_books = 2;

	/*Info On Topic 1 Book 1*/
	store.topic_list[0].info[0].cost = 250;
	store.topic_list[0].info[0].avail_items = ITEM_1_C;
	store.topic_list[0].info[0].item_number = ITEM_1;
	strcpy(store.topic_list[0].info[0].book_name, "How to be good at CS5523");

	/* Info on Topic 1 Book 2 */
	store.topic_list[0].info[1].cost = 100;
	store.topic_list[0].info[1].avail_items = ITEM_2_C;
	store.topic_list[0].info[1].item_number = ITEM_2;
	strcpy(store.topic_list[0].info[1].book_name, "RMI's & RPC's");

	/* Info on Topic 2 Book 1 */
	store.topic_list[1].info[0].cost = 175;
	store.topic_list[1].info[0].avail_items = ITEM_3_C;
	store.topic_list[1].info[0].item_number = ITEM_3;
	strcpy(store.topic_list[1].info[0].book_name, "Why go to the Graduate School");

	/* Info on Topic 2 Book 2 */
	store.topic_list[1].info[1].cost = 50;
	store.topic_list[1].info[1].avail_items = ITEM_4_C;
	store.topic_list[1].info[1].item_number = ITEM_4;
	strcpy(store.topic_list[1].info[1].book_name, "How to Survive the Graduate School");

	/* creating the socket to be communicated by the client */
	socket_s = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_s < 0) {
		printf("Error: Unable to create Socket\n");
		return -errno;
	}

	bzero(&server, sizeof(struct sockaddr_in));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port  = htons(PORT);	

	if(bind(socket_s, (struct sockaddr*)&server, sizeof(server)) < 0) {
		printf("Error: Unable to bind the port to Socket\n");
		return -errno;
	}

	listen(socket_s, 5);

	for(;;) {

		socket_c = accept(socket_s, (struct sockaddr *)&client, &client_len); 
		if(socket_c < 0) {
			printf("Error: In accepting the client : %d\n", socket_c);
			return -errno;
		}

		if((pthread_create(&thread_id, &attr, process_request,(void *)&socket_c)) < 0) {
			printf("Error : Unable to create Request Thread\n");
			return -errno;
		}			
	}
	
	close(socket_s);

	return 0;
}

