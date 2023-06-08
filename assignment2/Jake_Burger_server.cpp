//Jake Burger
//COSC 3360
//Assignment #2
//Due: 31 March 2023

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
using namespace std;

int main(){
  int server_port_number, server_socket, nBytes;
  string stolen_car_db_name, read_text; //stole car text file database name and string for getline
  char buffer[1024]; //where messages to and from the client will be processes
  struct sockaddr_in server_address, client_address; //to store transport addresses
  struct sockaddr_storage server_storage; //to store socket addresses info
  socklen_t address_size; //socket address size
  vector<string> stolen_cars; //where stolen_car data will be stored

  cout << "Enter today’s stolen car DB namee: "; 
  cin >> stolen_car_db_name;
  cout << "Enter the server port number: ";
  cin >> server_port_number;

  //Creates the server socket
  server_socket = socket(PF_INET, SOCK_DGRAM, 0);

  //Configure settings in address struct
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(server_port_number);
  server_address.sin_addr.s_addr = inet_addr("127.0.0.1"); //server to be locally hosted
  memset(server_address.sin_zero, '\0', sizeof server_address.sin_zero);

  //Binds socket with the socket address
  bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));

  //Initialize size variable that'll be used in the loop
  address_size = sizeof server_storage;

  ifstream in_stream(stolen_car_db_name); //adds contents from text file to vector of stolen_cars
  while(getline(in_stream, read_text)) {
    stolen_cars.push_back(read_text);
  }
  
  while(1){

    //receives incoming data from client
    nBytes = recvfrom(server_socket,buffer,1024,0,(struct sockaddr *)&server_storage, &address_size);

    //exits program if the buffer equals killsvc
    if(strcmp(buffer, "killsvc")==0) {
      cout << "Received request to terminate the service." << "\n";
      exit(0);
    }

    string temp_buffer = buffer; //temporarily holds buffer's value
    if (find(stolen_cars.begin(), stolen_cars.end(), buffer) != stolen_cars.end()) //if stolen_cars contains client input (buffer)
    {
      temp_buffer = temp_buffer + ": Reported as stolen."; //modifies value of temp buffer to send back to client
      memset(buffer, 0, sizeof buffer); //clears buffer
      for(int i = 0; i < temp_buffer.size(); i++) {
        buffer[i] = temp_buffer[i]; //sets buffer's elements to message that'll be sent to client
      }
    }
    else //if not in the stolen_cars vector
    {
      temp_buffer = temp_buffer + ": Not in the database.";
      memset(buffer, 0, sizeof buffer); //clears buffer
      for(int i = 0; i < temp_buffer.size(); i++) {
        buffer[i] = temp_buffer[i]; //sets buffer's elements to output message
      }
    }
    
    //prints buffer in console before sending back to client
    cout << buffer << "\n";
    //sends message back to the client using the server_storage address
    sendto(server_socket,buffer,1024,0,(struct sockaddr *)&server_storage,address_size);
  }

  return 0;
}