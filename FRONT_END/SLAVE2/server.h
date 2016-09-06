#include <pthread.h>
/* Pthread Locks */ 

//SEARCH 
pthread_mutex_t search_count = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t time_search  = PTHREAD_MUTEX_INITIALIZER; 
 
//LOOKUP 
pthread_mutex_t lookup_count = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t time_lookup  = PTHREAD_MUTEX_INITIALIZER; 

//ORDER 
pthread_mutex_t order_count  = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t time_order   = PTHREAD_MUTEX_INITIALIZER; 

//Book Decrement
pthread_mutex_t book_order   = PTHREAD_MUTEX_INITIALIZER; 

#define LEN 	75

#define SUCESS		100
#define FAILURE		200

#define SEARCH		1
#define LOOK_UP		2
#define ORDER		3
#define PERF		4
#define RQN		5
#define RGO		6
#define RFO		7
#define RSP		8
#define TIME_SET	9
#define TIME_SYNC	10
#define EXIT		100

/* Item Number */
#define ITEM_1		111100078
#define ITEM_2		111100077
#define ITEM_3 		111100076
#define ITEM_4		111100075

/* Books Count */
#define ITEM_1_C	100
#define ITEM_2_C	200
#define ITEM_3_C	300
#define ITEM_4_C	400

/* Book information on each one */
struct book_info {
        char book_name[LEN];
        unsigned cost;
        unsigned avail_items;
        unsigned item_number;
};

/* Each book topic details */
struct topic{
        char topic_name[LEN];
	int topic_id;
        int diff_books;
        struct book_info info[2];
};

/* Book Store Model for the given topics */
struct book_store {
        int total_topics;
        struct topic topic_list[2];
};

struct book_store store;

/* Request Counts */
unsigned search_request_count = 0;
unsigned lookup_request_count = 0;
unsigned buy_request_count    = 0;

/*Good Orders */
unsigned good_oreders  = 0;
unsigned failed_orders = 0;

/* time counters */
long int search_time = 0; 
long int lookup_time = 0;
long int order_time  = 0;
