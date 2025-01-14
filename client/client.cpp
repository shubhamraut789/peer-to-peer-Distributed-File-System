#include <iostream>
#include <string>
#include <vector>
#include <syscall.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

vector<string> tokenize_command(const string& ip){
    vector<string> tokens;
    char inputcpy[ip.length()+1];

    for(long long i=0; i<ip.size(); i++){
        inputcpy[i] = ip[i];
    }
    
    inputcpy[ip.length()] ='\0';

    char delimator[] = " ";
    char* token = strtok(inputcpy,delimator);

    while(token != nullptr){
        tokens.push_back(string(token));
        token = strtok(nullptr,delimator);
    }

    return tokens;
}

// Function to connect to the tracker
int connect_to_tracker(const string &ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cout << "Socket creation error\n";
        return -1;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(1234);  // Tracker's port

    // Convert IP address from text to binary form
    int IP_status = inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr);
    if (IP_status <= 0) {
        cout << "Invalid IP address\n";
        return -1;
    }

    // Connect to the tracker
    int connect_status = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (connect_status < 0) {
        cout << "Connection to tracker failed\n";
        return -1;
    }

    return sock;
}

string send_command_to_tracker( int sock, const string& command) {
    send(sock, command.c_str(), command.size(), 0);

    char buffer[1024] = {0};
    read(sock, buffer, 1024);

    string response (buffer);
    return response;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Use format: ./client <IP>:<PORT> tracker_info.txt\n";
        return -1;
    }

    // Parse the IP and port from the command-line arguments
    string ip_port = argv[1];
    int i=0;
    string ip="",ports="";
    while(ip_port[i] != ':'){
        ip += ip_port[i];
        i++;
    }

    for(int j=i+1; j<ip_port.length(); j++){
        ports += ip_port[j];
    }

    int port = stoi(ports);
    
    string file_name = argv[2];
    int ti_fd = open(file_name.c_str(),O_RDONLY);
    
    if(ti_fd < 0){
        cout<<"Failed to open tracker_info.txt\n";
        return -1;
    }
    cout <<ip<<" "<<port<<'\n';
    int sock = connect_to_tracker(ip, 1234);
    if(sock >=0){
        cout<<"Connected to tracker....\n";
    }
    else cout<<"Not able to connect to tracker!!\n";
    
    cout << "Completed Commands:\n"
         << "create_user <user_id> <passwd>\n"
         << "login <user_id> <passwd>\n"
         << "create_group <group_id>\n"
         << "join_group <group_id>\n"
         << "leave_group <group_id>\n"
         << "list_requests <group_id>\n"
         << "accept_request <group_id> <user_id>\n"
         << "list_groups\n"
         << "upload_file <file_path> <group_id>\n"
         << "list_files <group_id>\n"
         << "logout\n";

    string current_user;

    // Main loop for handling user commands
    while (true) {
        string user_command;
        
        cout << "Enter command: ";
        getline(cin, user_command);

        // Tokenize the command
        vector<string> tokens = tokenize_command(user_command);

        if (tokens.empty()) {
            cout << "Invalid command!" << endl;
            continue;
        }

        if (tokens[0] == "create_user" && tokens.size() == 3) {
            
                string command = "create_user " + tokens[1] + " " + tokens[2];
                string response = send_command_to_tracker(sock, command);
                cout << "Response from tracker: " << response;
           
        } else if (tokens[0] == "login" && tokens.size() == 3) {
                string command = "login " + tokens[1] + " " + tokens[2];
                string response = send_command_to_tracker(sock, command);
                cout << "Response from tracker: " << response;
                if (response.find("Login successful") != string::npos) {
                    current_user = tokens[1];
                }
        } else if (tokens[0] == "create_group" && tokens.size() == 2) {
            if (current_user.empty()) {
                cout << "User has not logged in...\n";
                continue;
            }
                string command = "create_group " + current_user + " " + tokens[1];
                string response = send_command_to_tracker(sock, command);
                cout << "Response from tracker: " << response;
        } else if (tokens[0] == "join_group" && tokens.size() == 2) {
            if (current_user.empty()) {
                cout << "User has not logged in...\n";
                continue;
            }
            // Send a join group command to the tracker
            string command = "join_group " + current_user + " " + tokens[1];
            string response = send_command_to_tracker(sock, command);
            
            if (response.find("Group does not exist") != string::npos) {
                cout << "Error: Group " << tokens[1] << " does not exist.\n";
            } else {
                cout << "Response from tracker: " << response;
            }
        } else if (tokens[0] == "leave_group" && tokens.size() == 2) {
            if (current_user.empty()) {
                cout << "User has not logged in...\n";
                continue;
            }
            // Send a leave group command to the tracker
            string command = "leave_group " + current_user + " " + tokens[1];
            string response = send_command_to_tracker(sock, command);
            cout << "Response from tracker: " << response;

            if (response == "Group deleted") {
                cout << "Group " << tokens[1] << " has been deleted as no members are left.\n";
            }
        } else if (tokens[0] == "list_requests" && tokens.size() == 2) {
            if (current_user.empty()) {
                cout << "User has not logged in...\n";
                continue;
            }
                string command = "list_requests " + current_user + " " + tokens[1];
                string response = send_command_to_tracker(sock, command);
                cout << "Response from tracker: " << response;
        } else if (tokens[0] == "accept_request" && tokens.size() == 3) {
            if (current_user.empty()) {
                cout << "User has not logged in...\n";
                continue;
            }
                string command = "accept_request " + current_user + " " + tokens[1] + " " + tokens[2];
                string response = send_command_to_tracker(sock, command);
                cout << "Response from tracker: " << response;
        } else if (tokens[0] == "list_groups") {
                string command = "list_groups";
                string response = send_command_to_tracker(sock, command);
                cout << "Response from tracker: " << response;
        } 
        else if (tokens[0] == "upload_file" && tokens.size() == 3) {
            string file_path = tokens[1];
            string group_id = tokens[2];

            // Open the file to upload
            int file_fd = open(file_path.c_str(), O_RDONLY);
            if (file_fd < 0) {
                cout << "File could not be opened\n";
                continue;
            }
                string command = "upload_file " + file_path + " " + group_id;
                string response = send_command_to_tracker(sock, command);
                cout << "Response from tracker: " << response;
        }
        else if(tokens[0] == "list_files" && tokens.size() == 2){
            string group_id = tokens[1];

                string command = "list_files " + group_id;
                string response = send_command_to_tracker(sock, command);
                cout << "Response from tracker: " << response;

        }
        else if (tokens[0] == "logout") {
            if (current_user.empty()){
                cout << "No user is currently logged in...\n";
            } else{
                cout << current_user << " logged out successfully.\n";
                current_user = "";  
            }
        } else{
            cout << "Unknown command! Please refer to the Completed commands list.\n";
        }
    }

    return 0;
}
