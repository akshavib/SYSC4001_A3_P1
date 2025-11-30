/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_student1_student2.hpp>

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;
    int quantum_counter = 0;  //time quantum for RR scheduling
    const int TIME_QUANTUM = 100; // define the time quantum

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);

                process.state = READY;  //Set the process state to READY
                ready_queue.push_back(process); //Add the process to the ready queue
                job_list.push_back(process); //Add it to the list of processes

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue
        for (auto itr = wait_queue.begin(); itr != wait_queue.end();){ // same method approach as in EP 
            itr->remaining_io_time--;

            if (itr->remaining_io_time <= 0){
                PCB process = *itr; // process for the completed IO
                
                process.state = READY; 
                ready_queue.push_back(process); // add process to the ready queue

                sync_queue(job_list, process); // syncing copy of the process state to the real process state
                execution_status += print_exec_status(current_time + 1, process.PID, WAITING, READY); // process went from waiting to ready
                itr = wait_queue.erase(itr); // remove from wait queue
            }
            else{
                ++itr;
            }
        }
        ///////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        //Round Robin: pick the next process from the front of the ready queue
        if (running.PID == -1 && !ready_queue.empty()) { // handling empty CPU
            running = ready_queue.front();
            ready_queue.erase(ready_queue.begin());
            running.state = RUNNING;
            running.start_time = current_time;
            quantum_counter = 0; // reset quantum count
            sync_queue(job_list, running);
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
        }

        if(running.PID != -1){ // handling if there is a running process
            running.remaining_time--; // decrement remaining time of running process
            if(running.io_freq > 0) {
                running.time_until_next_io--; // decrement IO timer
            }
            quantum_counter++; // increment quantum timer 

            sync_queue(job_list, running);

            if(running.remaining_time <= 0){ // handling when the process is finished
                execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, TERMINATED);
                terminate_process(running, job_list);
                idle_CPU(running);
                quantum_counter = 0; // reset quantum count
            }

            else if (running.io_freq > 0 && running.time_until_next_io <= 0){ // handling IO interrupt
                running.state = WAITING;
                running.remaining_io_time = running.io_duration; // set remaining IO time for waiting
                running.time_until_next_io = running.io_freq; // reset time until next IO interrupt

                wait_queue.push_back(running);
                execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, WAITING);

                idle_CPU(running);
                quantum_counter = 0;
            }

            else if (quantum_counter >= TIME_QUANTUM){ // handling if quantum time is over
                running.state = READY;
                ready_queue.push_back(running); // add running process to back of ready queue
                execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, READY);

                idle_CPU(running);
                quantum_counter = 0; 
            }
        }
        current_time++;
        ///////////////////////////////////////////////////////////////

    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}