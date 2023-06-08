//Jake Burger
//COSC 3360
//Assignment #1
//Due February 27

#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include <fstream>

using namespace std;

struct instruction { //struct for each process instruction
    string name;
    int time;

    instruction(string n, int t) {
        name = n;
        time = t;
    }
};
int GetNcores(string input) { //returns number of Ncores
    int r = 0;
    string ncore_line = "";
    string ncore_number_string = "";
    while(input[r] != '\n') {
        ncore_line = ncore_line + input[r];
        r++;
    }
    for(int i = ncore_line.find(' '); i < ncore_line.size(); i++) {
        if(ncore_line[i] != ' ') {
            ncore_number_string = ncore_number_string + ncore_line[i]; //sets ncores to all values after the space and before \n
        }
    }
    if(stoi(ncore_number_string) > 0) {
        return stoi(ncore_number_string); //converts ncores to an int
    }
    else {
        return 1; //default of 1 NCORE if there is no NCORE parameter in input
    }
}
vector<vector<instruction>> LoadProcesses(string input) { //returns a vector of processes containing sets of instructions from input.txt
    vector<vector<instruction>> processes;
    for(int i = 0; i < input.size(); i++) 
    {
        if(input.substr(i, 5)=="START") 
        {
            string temp_process = "";
            vector<instruction> temp_process_vector;
            int j = i;
            while(input.substr(j, 3) != "END") {
                temp_process = temp_process + input[j];
                j++;
            }
            for(int k = 0; k < temp_process.size(); k++) {
                int m = k;
                if(temp_process[m-1] == '\n' || temp_process.substr(m,5)=="START") 
                {
                    string temp_line ="";
                    string temp_instruction = "";
                    string time_string = "";
                    while(temp_process[m] != '\n') {
                        temp_line = temp_line + temp_process[m];
                        m++;
                    }
                    for(int q = 0; q < temp_line.find(' '); q++) {
                        temp_instruction = temp_instruction + temp_line[q];
                    }
                    for(int q = temp_line.find(' '); q < temp_line.size(); q++) {
                        if(temp_line[q] != ' ') {
                            time_string = time_string + temp_line[q];
                        }
                    }
                    temp_process_vector.push_back(instruction(temp_instruction, stoi(time_string)));
                }
            }
            processes.push_back(temp_process_vector);
        }
    }
    return processes;
} //end of LoadProcesses

int main() {
    priority_queue<int> proccesses_occupying_core;
    vector<vector<instruction>> processes;
    queue<int> ssd_queue; //ssd queue for processes released by the core
    queue<int> ready_queue; //to be of ncores size
    vector<queue<bool>> locks(64); //queue of 64 locks
    int clock_time = 0; //initialized at size 0
    int ncores = 1; //initalized at 1, but will change based on input

    string line; //line for getline
    string input = ""; //input for getline
    while(getline(cin, line)) { //
        input = input + line + '\n';
    }
    ncores = GetNcores(input);
    processes = LoadProcesses(input);
    int cores_array [ncores] = {-1};
    int cores_available = ncores;

    for(int i = 0; i < processes.size(); i++) {
        queue<string> process_states;
        for(int j = 0; j < processes.at(i).size(); j++) {

            int process_number = i;
            string instruction = processes.at(i).at(j).name;
            int instruction_time = processes.at(i).at(j).time;
            int start_time = 0;
            
            if(instruction =="START") { //when the process starts
                start_time = instruction_time;
                clock_time = clock_time + start_time;
                cout << "Process " << process_number << " starts at time t = " << start_time << " ms\n"; 
                cout << "Current number of busy cores " << proccesses_occupying_core.size() << "\n";
                cout << "Process Table:\n";

                if(process_states.size() == 0) {
                    cout << "Process " << process_number << " is RUNNING.\n"; //initially begins in running state if no cores are taken
                }
                while(!process_states.empty()) {
                    cout << "Process " << process_number << " is " << process_states.front() << ".\n";
                    process_states.pop();
                }
                if(ready_queue.empty()) {
                    cout << "Ready queue is empty.\n";
                }
                else {
                    while(!ready_queue.empty()) { //loops through ready queues contents
                        cout <<"Ready queue contains \n Process " << ready_queue.front() << ".\n";
                        ready_queue.pop();
                    }
                }
            }
            if(instruction == "CPU") { //occupy a core of the CPU
                if(ready_queue.size() > 0 && proccesses_occupying_core.size() < ncores) { //check if there are elements in the ready queue to load to core
                        proccesses_occupying_core.push(ready_queue.front()); //removes front element of the ready queue
                        ready_queue.pop();
                        process_states.push("RUNNING"); //make process running when taking up core space since it is moved out of ready queue
                        cores_available--;
                }
                else if(ready_queue.size() == 0 && proccesses_occupying_core.size() < ncores) { //if no elements waiting in ready queue
                    for(int k = 0; k < ncores; k++) {
                        if(cores_array[k] == -1) { //if cores are available
                            cores_array[k] = process_number; //occupy the available core with process
                            proccesses_occupying_core.push(i);
                            cores_available--;
                            process_states.push("RUNNING"); //move to running state because process is moved to core
                        }
                        else if(proccesses_occupying_core.size() == ncores) { //if we reach the end of the cores array and none are available
                            ready_queue.push(i);
                            process_states.push("READY"); //move to ready state since cores are full
                        }
                    }
                }
                clock_time = clock_time + instruction_time; //add CPU time
            }
            if(instruction == "SSD") { //free up core space
                if(cores_available == 0) {
                    ssd_queue.push(proccesses_occupying_core.top()); //add process to ssd queue
                    proccesses_occupying_core.pop(); //remove a process from the core
                    process_states.push("BLOCKED"); //process is in blocked state since it is in ssd
                }
                clock_time = clock_time + instruction_time; //add SSD time
                cores_available++; //new core is available because of ssd freeing up space
            }
            if(instruction == "LOCK") {
                locks.at(process_number).push(false); //locking one of the locks
                process_states.push("BLOCKED"); //locks cause process to go into locked state
            }
            if(instruction == "UNLOCK") {
                for(int i = 0; i < process_number; i++) {
                    if(!locks.at(i).empty()) {
                        locks.at(i).pop();
                        locks.at(i).push(true);
                    }
                }
                process_states.pop(); //remove blocked state
            }
            if(instruction == "OUTPUT") {
                if(!process_states.empty()) {
                    process_states.pop();
                }
                clock_time = clock_time + instruction_time; //add OUTPUT time
            }
            if(j==processes.at(i).size()-1) { //PROCESS END (because j is the last element in the list of instructions)
                cout << "\n";
                cout << "Process " << i << " terminated at time t = " << clock_time << " ms \n";
                int busy_cores = 0;
                for(int i = 0; i < ncores; i++) {
                    if(cores_array[i] > 0) {
                        busy_cores++;
                    }
                }
                cout << "Current number of busy cores " << busy_cores << "\n";
                cout << "Process Table:\n";

                if(process_states.size()==0) {
                    cout << "Process " << process_number << " is TERMINATED" << ".\n";
                }
                else {
                    while(!process_states.empty()) {
                        cout << "Process " << process_number << " is " << process_states.front() << ".\n";
                        process_states.pop();
                    }
                }
                if(ready_queue.empty()) {
                    cout << "Ready queue is empty.\n";
                }
                else {
                    while(!ready_queue.empty()) { //loops through ready queues contents
                        cout <<"Ready queue contains \n Process " << ready_queue.front() << ".\n";
                        ready_queue.pop();
                    }
                }
            }
        }
        cout << endl;
    } 
    return 0;
}
