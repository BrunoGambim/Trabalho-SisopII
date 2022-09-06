final: sleep_server manager member
sleep_server: sleep_server.c
				gcc -o sleep_server sleep_server.c
manager: network.o members_table.o custom_mutex.o manager.c replication_buffer.o
	gcc -o manager manager.c members_table.o network.o custom_mutex.o replication_buffer.o -lpthread
member: network.o members_table.o custom_mutex.o member.c replication_buffer.o
	gcc -o member member.c members_table.o network.o custom_mutex.o replication_buffer.o -lpthread
members_table.o: members_table.c members_table.h
	gcc -o members_table.o -c members_table.c
replication_buffer.o: replication_buffer.c replication_buffer.h
	gcc -o replication_buffer.o -c replication_buffer.c
network.o: network.c network.h
	gcc -o network.o -c network.c
manager_data.o: manager_data.c manager_data.h
	gcc -o manager_data.o -c manager_data.c
custom_mutex.o: custom_mutex.c custom_mutex.h
	gcc -o custom_mutex.o -c custom_mutex.c
clean: 
	rm *.o manager member sleep_server




