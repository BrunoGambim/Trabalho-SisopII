final: sleep_server
sleep_server: sleep_server.c discovery_subservice.o election_subservice.o exit_subservice.o interface_reader_subservice.o print_subservice.o replication_subservice.o update_status_subservice.o members_table.o replication_buffer.o network.o member_ip_list.o custom_mutex.o
				gcc -o sleep_server sleep_server.c discovery_subservice.o election_subservice.o exit_subservice.o interface_reader_subservice.o print_subservice.o replication_subservice.o members_table.o update_status_subservice.o replication_buffer.o network.o member_ip_list.o custom_mutex.o -lpthread
discovery_subservice.o: discovery_subservice.c discovery_subservice.h
	gcc -o discovery_subservice.o -c discovery_subservice.c
election_subservice.o: election_subservice.c election_subservice.h
	gcc -o election_subservice.o -c election_subservice.c
exit_subservice.o: exit_subservice.c exit_subservice.h
	gcc -o exit_subservice.o -c exit_subservice.c
interface_reader_subservice.o: interface_reader_subservice.c interface_reader_subservice.h
	gcc -o interface_reader_subservice.o -c interface_reader_subservice.c
print_subservice.o: print_subservice.c print_subservice.h
	gcc -o print_subservice.o -c print_subservice.c
replication_subservice.o: replication_subservice.c replication_subservice.h
	gcc -o replication_subservice.o -c replication_subservice.c
update_status_subservice.o: update_status_subservice.c update_status_subservice.h
	gcc -o update_status_subservice.o -c update_status_subservice.c
members_table.o: members_table.c members_table.h
	gcc -o members_table.o -c members_table.c
replication_buffer.o: replication_buffer.c replication_buffer.h
	gcc -o replication_buffer.o -c replication_buffer.c
network.o: network.c network.h
	gcc -o network.o -c network.c
member_ip_list.o: member_ip_list.c member_ip_list.h
	gcc -o member_ip_list.o -c member_ip_list.c
custom_mutex.o: custom_mutex.c custom_mutex.h
	gcc -o custom_mutex.o -c custom_mutex.c
clean: 
	rm *.o sleep_server
