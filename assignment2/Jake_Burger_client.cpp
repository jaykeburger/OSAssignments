//Jake Burger
//COSC 3360
//Assignment #2
//Due: 31 March 2023

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#define h_addr h_addr_list[0]
using namespace std;

int main(){
  int client_socket, server_port_number, nBytes;
  string server_host_name, license_plate;
  char buffer[1024]; //buffer where messages to and from server will be processed
  struct sockaddr_in server_address; //to store socket addresses info
  socklen_t address_size; //socket address size
  struct hostent *host_structure; //host structure for processing custom hostname

  cout << "Enter the server host name: ";
  cin >> server_host_name;
  cout << "Enter the server port number: ";
  cin >> server_port_number;

  //creates client socket
  client_socket = socket(PF_INET, SOCK_DGRAM, 0);

  //initalizes host structure with user input (server_host_name)
  host_structure = gethostbyname(server_host_name.c_str());
  
  //Set settings in address struct
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(server_port_number);
  server_address.sin_addr = *(struct in_addr *)host_structure->h_addr;
  memset(server_address.sin_zero, '\0', sizeof server_address.sin_zero);  

  //Initialize size variable to be used in the loop to send to server
  address_size = sizeof server_address;

  while(1){
    cout << "Enter a license plate number: ";
    cin >> buffer;
    
    nBytes = strlen(buffer) + 1;
    
    //Send message to server
    sendto(client_socket,buffer,nBytes,0,(struct sockaddr *)&server_address,address_size);

    if(strcmp(buffer, "killsvc")==0) { //if buffer is killsvc
      cout << "Client terminates. Bye! \n";
      exit(0);
    }

    //Receive message from server
    nBytes = recvfrom(client_socket,buffer,1024,0,NULL, NULL);
    cout << buffer << "\n";
  }
  return 0;
}