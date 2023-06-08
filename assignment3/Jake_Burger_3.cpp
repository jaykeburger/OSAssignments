#include <cstdlib>
#include <ctime>
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <vector>
using namespace std; 

int timeToRaiseDrawbridge, timeToLowerDrawbridge; //time to raise and lower drawbridge
int cars_num, ships_num = 0; //access w mutex, counts how many cars and ships cross bridge
bool carCanGo = true; //if car can cross bridge
bool shipCanGo = true; //if ship can go
bool shipInChannel = false; //if ship is in the channel
bool carOnBridge = false; //if there is a car on the bridge
bool bridgeCanRise = true; //if the bridge can rise
vector<pthread_t> threads; //parent thread
static pthread_mutex_t drawbridge_mutex = PTHREAD_MUTEX_INITIALIZER; //passive drawbridge mutex
pthread_cond_t bridge_con = PTHREAD_COND_INITIALIZER; //to notify bridge
pthread_cond_t car_con = PTHREAD_COND_INITIALIZER; //to notify car thread
pthread_cond_t ship_con = PTHREAD_COND_INITIALIZER; //to notify ship thread

struct carData { //struct for car data
  string car_name;
  int arrival_time;
  int crossing_time;
};
struct shipData { //struct for ship data
  string ship_name;
  int arrival_time;
  int crossing_time;
};

void *cars_thread(void *arg) { //car pthread
  struct carData *car = (struct carData *) arg;
  string car_name = car->car_name;
  int arrival_time = car->arrival_time;
  int crossing_time = car->crossing_time;

  sleep(arrival_time); //sleeps until car arrives
  cout << "Car "<< car_name <<" arrives at the bridge." << endl;
  pthread_mutex_lock(&drawbridge_mutex); //requests lock so car can cross drawbridge
  while(!carCanGo)
  {
    if(carOnBridge || shipInChannel) {
      pthread_cond_wait(&car_con, &drawbridge_mutex); //waits until car can access the drawbridge
    }
  }
  if(carCanGo) {
    cout << "Car " << car_name << " goes on the bridge." << endl;
    carCanGo = false;
    carOnBridge = true;
    sleep(crossing_time); //sleeps during crossing time
  }
  
  cout << "Car " << car_name << " leaves the bridge." << endl;
  carCanGo = true;
  carOnBridge = false;
  
  pthread_cond_broadcast(&ship_con); //lets ship know car has passed
  pthread_cond_broadcast(&bridge_con); //lets bridge know car has passed
  pthread_mutex_unlock(&drawbridge_mutex); //releases drawbridge
  cars_num++; //increments number of cars that pass
  return NULL;
}

void *ships_thread(void *arg) { //ship thread
  struct shipData *ship = (struct shipData *) arg;
  string ship_name = ship->ship_name;
  int arrival_time = ship->arrival_time;
  int crossing_time = ship->crossing_time;

  sleep(arrival_time); //sleeps for arrival time
  cout << "Ship "<< ship_name <<" arrives at the bridge." << "\n" <<"Bridge is closed to car traffic" << endl;
  pthread_mutex_lock(&drawbridge_mutex); //requests lock for access to the drawbridge so ship can cross
  
  while (!shipCanGo)
  {
    if(carOnBridge || shipInChannel) {
      pthread_cond_wait(&ship_con, &drawbridge_mutex);  //waits until ship can cross
    }
  }
  shipInChannel = true;
  shipCanGo = false; //ship cant go yet since bridge hasnt risen

  if(shipCanGo || bridgeCanRise) {
    cout << "Bridge can now be raised." << endl;
    carCanGo = false;
    sleep(timeToRaiseDrawbridge); //sleep so drawbridge can be raised
  }
  
  cout << "Ship " << ship_name << " goes under the raised bridge." << endl;
  sleep(crossing_time);
  cout << "Ship " << ship_name << " is now leaving." << endl;
  cout << "Bridge can now accomodate car traffic." << endl;
  shipCanGo = true;
  shipInChannel = false;
  carCanGo = true;
  pthread_cond_broadcast(&car_con); //lets cars know that ship has passed
  pthread_mutex_unlock(&drawbridge_mutex); //unlocks access to bridge
  ships_num++; //increments amount of ships that pass
  return NULL;
}

int main() {
  string bridge, input;
  
  cin >> bridge >> timeToRaiseDrawbridge >> timeToLowerDrawbridge; //reads first line of input
  
  while(cin >> input) {
    if(input == "Car") { //if line starts with Car
      carData * car = new carData;
      cin >> car->car_name >> car->arrival_time >> car->crossing_time; //get car data
      sleep(car->arrival_time); //Sleep for the duration of the inter-arrival delay
      pthread_t tid;
      pthread_create(&tid, NULL, cars_thread, (void *) car); //creates new car child thread
      threads.push_back(tid); //adds thread id to list
    }
    else if(input == "Ship") { //if line starts with ship
      string name;
      shipData * ship = new shipData;
      cin >> ship->ship_name >> ship->arrival_time >> ship->crossing_time; //get ship data
      sleep(ship->arrival_time); //Sleep for the duration of the inter-arrival delay
      pthread_t thread_id; //declares thread id
      pthread_create(&thread_id, NULL, ships_thread, (void *) ship); //creates new ship child thread
      threads.push_back(thread_id); //adds thread id to list
    }
  }

  for(int i = 0; i < threads.size(); i++) {
    pthread_join(threads[i], NULL); //waits until car and ship threads end
  }
  cout << cars_num << " car(s) crossed the bridge." << endl; //car crossing summary
  cout << ships_num << " ship(s) crossed the bridge." << endl; //ship crossing summary

  return 0;
}