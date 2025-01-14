To start execution:
g++ -o track1 tracker/tracker.cpp
g++ -o clit1 client/client.cpp

Then to execute
# Tracker commands:
./track1 tracker_info.txt 1

# client commands:
./clit1 127.0.0.1:Randomport tracker_info.txt

tracker_info.txt - Contains ip, port details of all the trackers

Completed commands and ways to run in client terminal:

Create User Account: create_user <user_id> <passwd>
Login: login <user_id> <passwd>
Create Group: create_group <group_id>
Join Group: join_group <group_id>
Leave Group: leave_group <group_id>
List pending join: list_requests <group_id>
Accept Group Joining Request: accept_request <group_id> <user_id>
List All Group In Network: list_groups
List All sharable Files In Group: list_files <group_id>
Upload File: upload_file <file_path> <group_id>