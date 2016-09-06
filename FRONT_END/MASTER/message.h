#include "server.h"

struct time_reply {
	unsigned hours;
	unsigned mins;
	unsigned secs;
	unsigned msecs;
	char s_sign;
	char m_sign;
};

/* Request From Client */
struct inputs{
	unsigned item_number;
	int topic_id;
};

/* Client Request format */
struct client_request {
	int method_id;	
	struct inputs in;
	struct time_reply off;
};

/* Response structure for the search query */
struct search_response {
	char message[LEN];
	int status;
	unsigned total_books;
	struct book_info info[2];
};

/* Response structure for look up */
struct look_up_response {
	char message[LEN];
	int status;
	struct book_info info;
	char topic[LEN];
};

/* Response structure for order queries */
struct order_response {
	int status;
	int discount;
};

/* Repsonse structure for performance related queries from the user */
struct perf_response {
	long int time;
	unsigned request_count; 
};


/* This is the union of server reponses depending on request from the client */
union server_response {
	struct search_response search;
	struct look_up_response look; 
	struct order_response order;
	struct perf_response perf;
	struct time_reply time;
};
