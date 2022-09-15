#include "member_ip_list.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h> 

void createMemberIpList(member_ip_list ** list){
    *list = (member_ip_list*) malloc(sizeof(member_ip_list));
    (*list)->rootNode = NULL;
}

void addMemberIpToList(member_ip_list * list, char *ipAddress){
    member_ip_node *lastNode;
    if(list->rootNode == NULL){
        list->rootNode = (member_ip_node*) malloc(sizeof(member_ip_node));
        list->rootNode->ipAddress = strdup(ipAddress);
        list->rootNode->nextNode = NULL;
    }else{
        lastNode = list->rootNode;
        while(lastNode->nextNode != NULL){
            lastNode = lastNode->nextNode;
        }
        lastNode->nextNode = (member_ip_node*) malloc(sizeof(member_ip_node));
        lastNode->nextNode->ipAddress = strdup(ipAddress);
        lastNode->nextNode->nextNode = NULL;
    }
}

void freeMemberIpList(member_ip_list * list){
    member_ip_node *auxNode, *auxNode2;
    if(list->rootNode != NULL){
        auxNode = list->rootNode;
        while(auxNode->nextNode != NULL){
            auxNode2 = auxNode->nextNode;
            free(auxNode->ipAddress);
            free(auxNode);
            auxNode = auxNode2;
        }
    }
    free(list);
}