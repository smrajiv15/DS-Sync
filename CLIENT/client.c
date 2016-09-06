#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include "message.h"

char ip_address[LEN];

/* scanf_check - This function checks whether valid number is given when asked.
 * it does the clean up and error handling when user inputs string value instead
 * of numbers.
 */ 

int scanf_check(int value) {

	char ch;
	if(value == 0) {
		while( ( ch = getchar() ) != '\n' && ch != EOF );
		return 1;
	}

	return 0;
}

/* show_search_result - This function prints the search result to the user */

void show_search_result(union server_response *resp) {
			
	int i = 0;
	unsigned item_number;

	printf("\n-------Search Results----\n\n");
	printf("Total books found for the topic : %d\n\n", resp->search.total_books);
	
	for(i = 0; i < 2; i++) {
		item_number = resp->search.info[i].item_number;
		printf("Book Name       : %s\n", resp->search.info[i].book_name);
		printf("Book Item Number: %d\n\n\n", item_number);
	}
}

/* contact_server - This method is repsonsible for contacting to the sever
 * for each of the client request. client_request structure is framed in the
 * show_menu function is passed to this function.
 */

void contact_server(struct client_request *req, union server_response *resp) {

	int n = 0, socket_c;
	struct sockaddr_in server;

	bzero(&server, sizeof(struct sockaddr_in));

	server.sin_family = AF_INET;
	server.sin_port  = htons(9901);	
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

/* list_item_no - Helper Menu to display the list of Books with item number */

void list_item_no() {
	printf("\nItem Numbers:\n");
	printf("How to be good at CS5523          : %d\n", ITEM_1);
	printf("RMI's & RPC's                     : %d\n", ITEM_2);
	printf("Why go to the Graduate School     : %d\n", ITEM_3);
	printf("How to Survive the Graduate School: %d\n\n", ITEM_4);
}

/* check_valid_item - checks whether the user entered the valid item */

int check_valid_item(int item) {
	if(item >= ITEM_4 && item <= ITEM_1)
		return 1;
	else
		return 0;
}

/* show_look_up_result - this method prints the result for lookup menu */

void show_look_up_result(union server_response *resp) {

	printf("\n\n--------Look Up Results ------------\n");
	printf("Topic of the Item : %s\n", resp->look.topic);
	printf("Item Book Name    : %s\n", resp->look.info.book_name);
	printf("Book Cost ($)     : %d\n", resp->look.info.cost);
	printf("Books in Stock	  : %d\n", resp->look.info.avail_items);
	printf("-----------------------------------------\n\n");
}

/* list_perf_opt - Menu to display performance options available */

void list_perf_opt() {
	printf("\n1. Total Service Requestes to the Server\n");
	printf("2. Report Good Orders\n");
	printf("3. Report Failed Orders\n");
	printf("4. Report Service Performance\n");
}

/* list_available_service - Helper menu to print the available service */

void list_available_service() {

	printf("\nAvailable Services\n");
	printf("--------------------\n");
	printf("\n1. Search\n");
	printf("2. lookup\n");
	printf("3. order\n");

}

/* print_service - Helper menu to print the available service to the User */

void print_service(int id) {

	switch(id) {
	case SEARCH: 
		printf("\nSEARCH:\n");
		break;
	case LOOK_UP:
		printf("\nLOOK UP:\n");   
		break;	
	case ORDER:
		printf("\nORDER:\n");
	}	

}

/* input - It is to verify whether the valid method is selected by the user */

void input(struct client_request *req) {

	int service_id, n;

opt:	list_available_service();
	printf("\nEnter your option:");
	n = scanf("%d", &service_id);

	if(scanf_check(n)) {
		printf("\nInvalid Option Entered\n");
		goto opt;
	}
	
	if(service_id >= 1 && service_id <= 3) {
		req->in.item_number = service_id;
	} else {
		printf("Invalid Option Entered\n");
		goto opt;
	}		 

}

/* assign_method_id - Each Method has corresponding ID to contact to the server
 * Depending on the option selected from the menu provided corresponding method
 * ID and input parameters are assigned to the client_request structure.
 */

void assign_method_id(int id, struct client_request *req) {

	int service_id;
	union server_response resp;
	double avg_time = 0;

	if(id == 1) { 
		req->method_id = RQN;
		input(req);
		contact_server(req, &resp);
		print_service(req->in.item_number);
		printf("\nResult:\n");
		printf("Total Requests Registered: %d\n", resp.search.status); 	
	} else if(id == 2) {
		req->method_id = RGO;
		contact_server(req, &resp);
		printf("\nResult:\n");
		printf("Number of Good Orders : %d\n\n", resp.order.status);
	} else if(id == 3) {
		req->method_id = RFO;
		contact_server(req, &resp);
		printf("\nResult:\n");
		printf("Number of Failed Orders : %d\n", resp.order.status);
	} else {
		req->method_id = RSP;
		input(req);
		contact_server(req, &resp);
		print_service(req->in.item_number);
		printf("\nResult:\n");
		printf("Overall Time : %ld\n", resp.perf.time);
		printf("Total request : %d\n", resp.perf.request_count);
		avg_time =  (double)resp.perf.time / (double)resp.perf.request_count; 	
		printf("Average Processing Time %.08lf (Microseconds)\n", avg_time); 
	}
}

/* show_menu -lists the available options in the book stores. This menu 
 * frames the client_request structure used to contact to the server and 
 * retreive information. The important parameters are method ID and their
 * corresponding inputs given by the user. Available options are Search,
 * Lookup, Order and performance parameters
 */

void show_menu() {

	int n = 0, opt;
	struct client_request req;
	union server_response resp;

main_menu:
	printf("\n========================");
	printf("\nMy Book Store : \n\n");
	printf("Available Options:\n");
	printf("1. Search\n");
	printf("2. lookup\n");
	printf("3. order\n");
	printf("4. Performance Queries\n");
	printf("5. Exit\n\n");
	printf("Enter the Required Option:");
	opt = scanf("%d", &n);

	if(scanf_check(opt)) {
		printf("\nError: Invalid Option Entered\n");
		goto main_menu;
	}

	switch(n) {
	case SEARCH:
search:
		printf("\n\nSEARCH @ MyBookStore.com\n");
		printf("----------------------------\n"); 
		printf("\nAvailable Topics\n\n");
		printf("1. Distributed Systems\n");
		printf("2. Graduate School\n\n");
		printf("----------------------\n");	
		printf("3. Go to Main Menu\n\n");
		printf("Enter the topic to search:");
		n = scanf("%d", &opt);
		
		if(scanf_check(n)) {
			printf("\nError: Invalid Topic Entered\n"); 
			goto search;
		}

		if(opt == 1 || opt == 2) {
			req.method_id   = SEARCH;
			req.in.topic_id = opt;
			contact_server(&req, &resp);
			show_search_result(&resp);
			goto search;
		}
		else if(opt == 3) { 
			goto main_menu;
		} else {
			printf("\nError: Invalid Topic Entered\n");
			goto search;		
		}	
		break;
		
	case LOOK_UP:
		printf("\nLOOKUP @ MyBookStore.com\n");
		printf("---------------------------\n\n");
enter_item:	list_item_no();
		printf("Enter the Item Number:");
		n = scanf("%d", &opt);

		if(scanf_check(n)) {
			printf("\nError: Enter Valid Item Number\n");
			goto enter_item;
		}
		
		if(!check_valid_item(opt)) {
			printf("\nError: Enter Valid Item Number\n");
			goto enter_item; 
		}

		req.method_id = LOOK_UP;
		req.in.item_number = opt;
		contact_server(&req, &resp);
		show_look_up_result(&resp);	
		goto main_menu;
	case ORDER:
		printf("\nORDER @ MyBookStore.com\n");
		printf("--------------------------\n\n");
		printf("Available Item Numbers to Order\n");
enter_item1:	list_item_no();		
		printf("Enter the Item Number:");
		n = scanf("%d", &opt);
		
		if(scanf_check(n)) {
			printf("Error: Enter Valid Item Number\n");
			goto enter_item1;
		}
	
		if(!check_valid_item(opt)) {
			printf("Error: Enter Valid Item Number\n");
			goto enter_item1; 
		}

		req.method_id = ORDER;
		req.in.item_number = opt;
		contact_server(&req, &resp);

		printf("\nORDER INFORMATION:\n");
		printf("---------------------\n");
		if(resp.order.status == SUCESS) {
			printf("\nBook With Item Number : %d is Ordered\n", opt);
			if(resp.order.discount) {
				printf("\nSURPRISE!! You will get 10%% discount for this order\n");
			}
			printf("Thank You for Purchasing @ MyBooks.com\n");
		} else {
			printf("\nBook with Item Number : %d is UN-AVAILABLE\n", opt);
			printf("For more Info run Look up option for the Item Number\n");
		}	
		goto main_menu;
	case PERF:
		printf("\nPerformance @ MyBookStore.com\n");
		printf("--------------------------------\n");
		printf("\nAvailable Performance Options\n");
perf_opt:	list_perf_opt();
		printf("\nEnter your option:");
		n = scanf("%d", &opt);

		if(scanf_check(n)) {
			printf("\nInvalid Request Entered: Enter Again\n");
			goto perf_opt;
		}

		if(n >= 1 && n <= 4) {
			assign_method_id(opt, &req);
		} else {
			printf("\nInvalid Request Entered: Enter Again\n");
			goto perf_opt;	
		}
			
		goto main_menu;
	case EXIT:
	default:
		printf("\nExiting the Book Store\n");
		printf("Thank you for Visiting us\n\n");
		exit(0);
	}
}

int main(int argc, char *argv[]) {
	if(argv[1] == NULL) {
		printf("No IP Address entered\n");
		exit(0);	
	}

	strcpy(ip_address, argv[1]);
	show_menu();	
	return 0;
}

