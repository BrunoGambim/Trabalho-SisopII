final: sleep_server manager member
sleep_server: sleep_server.c
				gcc -o sleep_server sleep_server.c
manager: network.o members_table.o custom_mutex.o member_data.o manager.c
	gcc -o manager manager.c members_table.o member_data.o network.o custom_mutex.o -lpthread
member: network.o member_data.o custom_mutex.o member.c
	gcc -o member member.c member_data.o network.o custom_mutex.o -lpthread
members_table.o: members_table.c members_table.h
	gcc -o members_table.o -c members_table.c
network.o: network.c network.h
	gcc -o network.o -c network.c
member_data.o: member_data.c member_data.h
	gcc -o member_data.o -c member_data.c
custom_mutex.o: custom_mutex.c custom_mutex.h
	gcc -o custom_mutex.o -c custom_mutex.c
clean: 
	rm *.o manager member sleep_server




