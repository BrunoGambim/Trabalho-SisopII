#ifndef MEMBER_IP_LIST_H
#define MEMBER_IP_LIST_H

typedef struct _member_ip_node{
    char *ipAddress;
    struct _member_ip_node *nextNode;
} member_ip_node;

typedef struct _member_ip_list{
    member_ip_node *rootNode;
} member_ip_list;

void createMemberIpList(member_ip_list ** list);
void addMemberIpToList(member_ip_list * list, char *ipAddress);
void freeMemberIpList(member_ip_list * list);

#endif