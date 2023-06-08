/*
Server Will:
1. Read in the day's stolen car database
2. Create a socket
3. Bind an address to that socket
4. Wait for incoming messages
5. Receive and print out a message from the client
6. Consult its DB and send a Yes/No reply to the client
7. Wait for next client message request
7A. Loop ends when the server gets a killsvc message from one of the clients
*/


//prompt user for file db name
//prompt user for port number

/* 
 *           server.c: Server program for storing and providing 
 *                     temperatures of cities around the world.
 *                     (Uses UDP for communication with clients)
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>

#define SERVER_PORT                "4357"
#define STORE_TEMPERATURE          1
#define READ_TEMPERATURE           2
#define READ_TEMPERATURE_RESULT    3
#define CITY_NOT_FOUND             4
#define ERROR_IN_INPUT             9

void error (char *msg);

struct message {
    long message_id;
    char city [100];
    char temperature [16];   // degrees Celcius 
};

struct tnode {
    char *city;
    double temperature;      // degrees Celcius
    struct tnode *left;
    struct tnode *right;
};

struct message recv_message, send_message;
int sock_fd;
socklen_t client_addrlen;
struct sockaddr_storage client_addr;

struct tnode *add_to_tree (struct tnode *p, char *city, double temperature);
struct tnode *find_city_rec (struct tnode *p, char *city);
void print_tree (struct tnode *p);
void error (char *msg);

int main (int argc, char **argv)
{
    const char * const ident = "temperature-server";

    openlog (ident, LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);
    syslog (LOG_USER | LOG_INFO, "%s", "Hello world!");
    
    struct addrinfo hints;
    memset (&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* for wildcard IP address */

    struct addrinfo *result;
    int s; 
    if ((s = getaddrinfo (NULL, SERVER_PORT, &hints, &result)) != 0) {
        fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
        exit (EXIT_FAILURE);
    }

    /* Scan through the list of address structures returned by 
       getaddrinfo. Stop when the the socket and bind calls are successful. */

    struct addrinfo *rptr;
    for (rptr = result; rptr != NULL; rptr = rptr -> ai_next) {
        sock_fd = socket (rptr -> ai_family, rptr -> ai_socktype,
                       rptr -> ai_protocol);
        if (sock_fd == -1)
            continue;

        if (bind (sock_fd, rptr -> ai_addr, rptr -> ai_addrlen) == 0)  // Success
            break;

        if (close (sock_fd) == -1)
            error ("close");
    }

    if (rptr == NULL) {               // Not successful with any address
        fprintf(stderr, "Not able to bind\n");
        exit (EXIT_FAILURE);
    }

    freeaddrinfo (result);

    ssize_t numbytes;
    struct tnode *root = NULL;

    while (1) {
         client_addrlen = sizeof (struct sockaddr_storage);
         if ((numbytes = recvfrom (sock_fd, &recv_message, sizeof (struct message), 0,
                        (struct sockaddr *) &client_addr, &client_addrlen)) == -1)
             error ("recvfrom");

         double temperature;

         switch (ntohl (recv_message.message_id)) {
             case STORE_TEMPERATURE : sscanf (recv_message.temperature, "%lf", &temperature);
                                      if (strlen (recv_message.city) > 99 || temperature > 99) {
                                          send_message.message_id = htonl (ERROR_IN_INPUT);
                                          send_message.city [0] = '\0';
                                          send_message.temperature [0] = '\0';
                                          if (sendto (sock_fd, &send_message.message_id, sizeof (struct message), 0,
                                              (struct sockaddr *) &client_addr, client_addrlen) == -1)
                                              error ("sendto");
                                      }
                                      root = add_to_tree (root, recv_message.city, temperature);
                                      break;

             case READ_TEMPERATURE  : if (strlen (recv_message.city) > 99) {
                                          send_message.message_id = htonl (ERROR_IN_INPUT);
                                          send_message.city [0] = '\0';
                                          send_message.temperature [0] = '\0';
                                          if (sendto (sock_fd, &send_message.message_id, sizeof (struct message), 0,
                                              (struct sockaddr *) &client_addr, client_addrlen) == -1)
                                              error ("sendto");
                                      }

                                      struct tnode *tmp = find_city_rec (root, recv_message.city);
                                      break;

             default                : fprintf (stderr, "Unknown message\n");
         }
    }
    syslog (LOG_USER | LOG_INFO, "%s", "Bye.");
    closelog ();

    exit (EXIT_SUCCESS);
}

// record temperature of a city in data structure
struct tnode *add_to_tree (struct tnode *p, char *city, double temperature)
{
    int res;

    if (p == NULL) {  // new entry
        if ((p = (struct tnode *) malloc (sizeof (struct tnode))) == NULL)
            error ("malloc");
        p -> city = strdup (city);
        p -> temperature = temperature;
        p -> left = p -> right = NULL;
    }
    else if ((res = strcmp (city, p -> city)) == 0) // entry exists
        p -> temperature = temperature;
    else if (res < 0) // less than city for this node, put in left subtree
        p -> left = add_to_tree (p -> left, city, temperature);
    else   // greater than city for this node, put in right subtree
        p -> right = add_to_tree (p -> right, city, temperature);
    return p;
}

// find node for the city whose temperature is queried
struct tnode *find_city_rec (struct tnode *p, char *city)
{
    int res;

    if (p == NULL) {
        send_message.message_id = htonl (CITY_NOT_FOUND);
        strcpy (send_message.city, city);
        send_message.temperature [0] = '\0';
        if (sendto (sock_fd, &send_message.message_id, sizeof (struct message), 0,
            (struct sockaddr *) &client_addr, client_addrlen) == -1)
            error ("sendto");
        return NULL;
    }
    else if ((res = strcmp (city, p -> city)) == 0) { // entry exists
        send_message.message_id = htonl (READ_TEMPERATURE_RESULT);
        strcpy (send_message.city, p -> city);
        sprintf (send_message.temperature, "%4.1lf", p -> temperature);
        if (sendto (sock_fd, &send_message, sizeof (struct message), 0,
            (struct sockaddr *) &client_addr, client_addrlen) == -1)
            error ("sendto");
        return p;
    }
    else if (res < 0) // less than city for this node, search left subtree
        p -> left = find_city_rec (p -> left, city);
    else   // greater than city for this node, search right subtree
        p -> right = find_city_rec (p -> right, city);
}

// print_tree: print the tree (in-order traversal)
void print_tree (struct tnode *p)
{
    if (p != NULL) {
        print_tree (p -> left);
        printf ("%s: %4.1lf\n\n", p -> city, p -> temperature);
        print_tree (p -> right);
    }
}

void error (char *msg)
{
    perror (msg);
    exit (1);
}