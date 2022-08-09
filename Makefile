final: sleep_server manager member
sleep_server: sleep_server.c
				gcc -o sleep_server sleep_server.c
manager: network.o members_table.o custom_mutex.o manager.c
	gcc -o manager manager.c members_table.o network.o custom_mutex.o -lpthread
member: network.o manager_data.o custom_mutex.o member.c
	gcc -o member member.c manager_data.o network.o custom_mutex.o -lpthread
members_table.o: members_table.c members_table.h
	gcc -o members_table.o -c members_table.c
network.o: network.c network.h
	gcc -o network.o -c network.c
manager_data.o: manager_data.c manager_data.h
	gcc -o manager_data.o -c manager_data.c
custom_mutex.o: custom_mutex.c custom_mutex.h
	gcc -o custom_mutex.o -c custom_mutex.c
clean: 
	rm *.o manager member sleep_server




