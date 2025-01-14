#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>  // For open()
#include <unistd.h> // For read() and close()
#include <cstring>  // For strtok()
#include <algorithm>

using namespace std;

struct FileInfo {
    string file_path;
};

map<string, map<string, FileInfo>> group_files; 

// Database to store user credentials
map<string, string> credentials;

// Database to store group information
struct Group {
    string owner;
    vector<string> members;
    vector<string> pendingRequests;
};

map<string, Group> groups;

void handle_client(int client_socket) {
    
    string response;

    while (true) {
        char buffer[1024] = {0};
        int bytes_read = read(client_socket, buffer, 1024);
        if (bytes_read <= 0) {
            cout << "Client disconnected or read error. Closing connection." << endl;
            close(client_socket);
            return;
        }
    
        vector<string> tokens;
        char* token = strtok(buffer, " ");
        while (token != nullptr) {
            tokens.push_back(string(token));
            token = strtok(nullptr, " ");
        }

        string response;
        if (tokens[0] == "create_user" && tokens.size() == 3) {
            string user_id = tokens[1];
            string password = tokens[2];

            cout<<"Received request to create user...\n";
            if (credentials.find(user_id) != credentials.end()) {
                response = "User already exists\n";
            } else {
                credentials[user_id] = password;
                response = "User created successfully\n";
            }
        } else if (tokens[0] == "login" && tokens.size() == 3) {
            string user_id = tokens[1];
            string password = tokens[2];

            cout<<"Received request to login...\n";
            if (credentials.find(user_id) != credentials.end() && credentials[user_id] == password) {
                response = "Login successful\n";
            } else {
                response = "Invalid credentials\n";
            }
        } else if (tokens[0] == "create_group" && tokens.size() == 3) {
            string user_id = tokens[1];
            string group_id = tokens[2];

            cout<<"Received request to create group...\n";
            if (groups.find(group_id) != groups.end()) {
                response = "Group already exists\n";
            } else {
                groups[group_id] = {user_id, {user_id}, {}};
                response = "Group created successfully\n";
            }
        } else if (tokens[0] == "join_group" && tokens.size() == 3) {
            string user_id = tokens[1];
            string group_id = tokens[2];

            cout<<"Received request to join group...\n";
            if (groups.find(group_id) == groups.end()) {
                response = "Group does not exist\n";
            } else if (find(groups[group_id].members.begin(), groups[group_id].members.end(), user_id) != groups[group_id].members.end()) {
                response = "Already a member of the group\n";
            } else if (find(groups[group_id].pendingRequests.begin(), groups[group_id].pendingRequests.end(), user_id) != groups[group_id].pendingRequests.end()) {
                response = "Join request already pending\n";
            } else {
                groups[group_id].pendingRequests.push_back(user_id);
                response = "Join request sent\n";
            }
        } else if (tokens[0] == "leave_group" && tokens.size() == 3) {
            string user_id = tokens[1];
            string group_id = tokens[2];

            cout<<"Received request to leave group...\n";
            if (groups.find(group_id) == groups.end()) {
                response = "Group does not exist\n";
            } else {
                auto& group = groups[group_id];
                auto it = find(group.members.begin(), group.members.end(), user_id);
                if (it == group.members.end()) {
                    response = "Not a member of the group\n";
                } else {
                    group.members.erase(it);

                    if (group.members.empty()) {
                        groups.erase(group_id);
                        group_files.erase(group_id);
                        response = "Left the group successfully. Group deleted as no members are left.\n";
                    } else {
                        if (group.owner == user_id) {
                            group.owner = group.members[0];
                            response = "Left the group successfully. New owner is " + group.owner + "\n";
                        } else {
                            response = "Left the group successfully\n";
                        }
                    }
                }
            }
        } else if (tokens[0] == "list_requests" && tokens.size() == 3) {
            string user_id = tokens[1];
            string group_id = tokens[2];
            
            cout<<"Received request to list requests...\n";
            if (groups.find(group_id) == groups.end()) {
                response = "Group does not exist\n";
            } else if (groups[group_id].owner != user_id) {
                response = "Only the owner can list requests\n";
            } else {
                response = "Pending requests:\n";
                auto& group = groups[group_id];
                if(group.pendingRequests.empty()) {
                    response += "NONE\n";
                }
                else { 
                    for (const auto& req : group.pendingRequests) {
                        response += req + "\n";
                    }
                }
            }
        } else if (tokens[0] == "accept_request" && tokens.size() == 4) {
            string owner_id = tokens[1];
            string group_id = tokens[2];
            string user_id = tokens[3];

            cout<<"Received request to accept request...\n";
            if (groups.find(group_id) == groups.end()) {
                response = "Group does not exist\n";
            } else if (groups[group_id].owner != owner_id) {
                response = "Only the owner can accept requests\n";
            } else {
                auto& group = groups[group_id];
                auto it = find(group.pendingRequests.begin(), group.pendingRequests.end(), user_id);
                if (it == group.pendingRequests.end()) {
                    response = "No pending request from this user\n";
                } else {
                    group.pendingRequests.erase(it);
                    group.members.push_back(user_id);
                    response = "Request accepted\n";
                }
            }
        } else if (tokens[0] == "reject_request" && tokens.size() == 4) {
            string owner_id = tokens[1];
            string group_id = tokens[2];
            string user_id = tokens[3];

            cout<<"Received request to reject request...\n";
            if (groups.find(group_id) == groups.end()) {
                response = "Group does not exist\n";
            } else if (groups[group_id].owner != owner_id) {
                response = "Only the owner can reject requests\n";
            } else {
                auto& group = groups[group_id];
                auto it = find(group.pendingRequests.begin(), group.pendingRequests.end(), user_id);
                if (it == group.pendingRequests.end()) {
                    response = "No pending request from this user\n";
                } else {
                    group.pendingRequests.erase(it);
                    response = "Request rejected\n";
                }
            }
        } else if (tokens[0] == "list_groups") {

            cout<<"Received request to list groups...\n";
            if (groups.empty()) {
                response = "No groups available\n";
            } else {
                response = "Available groups:\n";
                for (const auto& group : groups) {
                    response += group.first + " (Owner: " + group.second.owner + ")\n";
                }
            }
        } else if (tokens[0] == "upload_file" && tokens.size() == 3) {
            string file_path = tokens[1];
            string group_id = tokens[2];

            cout << "Received request to upload file...\n";
            if (groups.find(group_id) == groups.end()) {
                response = "Group does not exist\n";
            } else {
                FileInfo file_info;
                file_info.file_path = file_path;

                group_files[group_id][file_path] = file_info;
                response = "File uploaded successfully\n";
            }
        } else if (tokens[0] == "list_files" && tokens.size() == 2) {
            string group_id = tokens[1];

            cout << "Received request to list files...\n";
            if (groups.find(group_id) == groups.end()) {
                response = "Group does not exist\n";
            } else if (group_files.find(group_id) == group_files.end() || group_files[group_id].empty()) {
                response = "No files uploaded in this group\n";
            } else {
                response = "Files in group " + group_id + ":\n";
                for (const auto& file : group_files[group_id]) {
                    response += file.first + "\n";
                }
            }
        } else {
            response = "Invalid command\n";
        }

        send(client_socket, response.c_str(), response.size(), 0);
    }
    close(client_socket);
}

// Function to run the tracker
void run_tracker(const string &ip, int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        cout << "Socket creation failed\n";
        return;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip.c_str());
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cout << "Bind failed\n";
        return;
    }

    if (listen(server_fd, 3) < 0) {
        cout << "Listen failed\n";
        return;
    }

    cout << "Tracker running on " << ip << ":" << port << endl;

    while (true) {
        int new_socket = accept(server_fd, nullptr, nullptr);
        if (new_socket < 0) {
            cout << "Accept failed\n";
            continue;
        }

        // Handle client requests in a separate thread
        thread(handle_client, new_socket).detach();
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "Use format: ./tracker tracker_info.txt tracker_no\n";
        return -1;
    }

    string tracker_info_f = argv[1];
    int tracker_no = stoi(argv[2]);

    cout << "Starting tracker with info file: " << tracker_info_f << " and tracker number: " << tracker_no << endl;

    const char* tracker_info_file = "tracker_info.txt";
    
    int fd = open(tracker_info_file, O_RDONLY);
    if (fd < 0) {
        cout << "Failed to open tracker info file\n";
        return -1;
    }

    char buffer[256];  
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read < 0) {
        cout << "Failed to read tracker info file\n";
        close(fd);
        return -1;
    }
    
    buffer[bytes_read] = '\0';
    
    close(fd);

    char* token = strtok(buffer, " \n");
    string ip1 = token;
    
    token = strtok(nullptr, " \n");
    int port1 = stoi(token);
    
    token = strtok(nullptr, " \n");
    string ip2 = token;
    
    token = strtok(nullptr, " \n");
    int port2 = stoi(token);

    if (tracker_no == 1) {
        cout << "Tracker 1 info: " << ip1 << ":" << port1 << endl;
        cout << "Running as Tracker 1" << endl;
        run_tracker(ip1, port1); // Tracker 1
    } else if (tracker_no == 2) {
        cout << "Tracker 2 info: " << ip2 << ":" << port2 << endl;
        cout << "Running as Tracker 2" << endl;
        run_tracker(ip2, port2); // Tracker 2
    } else {
        cout << "Invalid tracker number\n";
        return -1;
    }

    return 0;
}
