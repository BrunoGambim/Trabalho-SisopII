#include "replication_buffer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h> 

void freeAckNode(ack_node *node){
    ack_node *nextNode;
    while(nextNode != NULL){
        nextNode = node->nextNode;
        free(node->hostname);
        free(node);
        node = nextNode;
    }
}

void freeMemberNode(member_node *node){
    member_node *nextNode;
    while(nextNode != NULL){
        nextNode = node->nextNode;
        freeAckNode(node->acks);
        free(node->ipAddress);
        free(node);
        node = nextNode;
    }
}

void freeDataNode(data_node *node){
    data_node *nextNode;
    while(nextNode != NULL){
        nextNode = node->nextNode;
        free(node->hostname);
        free(node);
        node = nextNode;
    }
}

void freeBuffer(replication_buffer *buffer){
    freeDataNode(buffer->dataList);
    free(buffer);
}


void createReplicationBuffer(replication_buffer **buffer){
    *buffer = (replication_buffer*) malloc(sizeof(replication_buffer));
    (*buffer)->dataNumber = 0;
    (*buffer)->dataList = NULL;
    (*buffer)->memberList = NULL;
}

void addDataNodeToBuffer(replication_buffer *buffer, char *hostname, char *macAddress, char *ipAddress, char *status){
    data_node *data;
    buffer->dataNumber += 1;
    if(buffer->dataList == NULL){
        buffer->dataList = (data_node*) malloc(sizeof(data_node));
        buffer->dataList->hostname = strdup(hostname);
        buffer->dataList->macAddress = strdup(macAddress);
        buffer->dataList->ipAddress = strdup(ipAddress);
        buffer->dataList->status = strdup(status);
        buffer->dataList->nextNode = NULL;
    }else{
        data = buffer->dataList;
        while(data->nextNode != NULL){
            data = data->nextNode;
        }
        data->nextNode = (data_node*) malloc(sizeof(data_node));
        data->nextNode->hostname = strdup(hostname);
        data->nextNode->macAddress = strdup(macAddress);
        data->nextNode->ipAddress = strdup(ipAddress);
        data->nextNode->status = strdup(status);
        data->nextNode->nextNode = NULL;
    }
}

void ackNode(member_node *member, char *hostname){
    ack_node *ack;
    if(member->acks == NULL){
        member->ackCounter += 1;
        member->acks = (ack_node*) malloc(sizeof(ack_node));
        member->acks->hostname = strdup(hostname);
        member->acks->nextNode = NULL;
    }else{
        ack = member->acks;
        while(ack->nextNode != NULL){
            if(strcmp(ack->hostname,hostname) == 0){
                break;
            }
            ack = ack->nextNode;
        }
        if(strcmp(ack->hostname,hostname) != 0){
            member->ackCounter += 1;
            ack->nextNode = (ack_node*) malloc(sizeof(ack_node));
            ack->nextNode->hostname = strdup(hostname);
            ack->nextNode->nextNode = NULL;
        }
    }
}

void ackBuffer(replication_buffer *buffer, char *hostname, char *ipAddress){
    member_node *member;
    if(buffer->memberList != NULL){
        member = buffer->memberList;
        while(member->nextNode != NULL){
            if(strcmp(member->ipAddress,ipAddress) == 0){
                break;
            }
            member = member->nextNode;
        }
        if(member != NULL){
            ackNode(member, hostname);           
        }
    }
}

int isBufferAcked(replication_buffer *buffer){
    member_node *member;
    member = buffer->memberList;
    while(member != NULL){
        if(member->ackCounter < buffer->dataNumber){
            return 0;
        }
        member = member->nextNode;
    }
    return 1;
}

int hasMember(replication_buffer *buffer, char *ipAddress){
    member_node *node;
    node = buffer->memberList;
    while(node != NULL){
        if(strcmp(node->ipAddress,ipAddress) == 0){
            return 1;
        }
        node = node->nextNode;
    }
    return 0;
}

void addMember(replication_buffer *buffer, char *ipAddress){
    member_node *node;
    if(!hasMember(buffer, ipAddress)){
        if(buffer->memberList == NULL){
            buffer->memberList = (member_node*) malloc(sizeof(member_node));
            buffer->memberList->ipAddress = strdup(ipAddress);
            buffer->memberList->acks = NULL;
            buffer->memberList->nextNode = NULL;
            buffer->memberList->ackCounter = 0;
        }else{
            node = buffer->memberList;
            while(node->nextNode != NULL){
                node = node->nextNode;
            }
            node->nextNode = (member_node*) malloc(sizeof(member_node));
            node->nextNode->ipAddress = strdup(ipAddress);
            node->nextNode->nextNode = NULL;
            node->nextNode->acks = NULL;
            node->nextNode->ackCounter = 0;
        }
    }
}

void removeMember(replication_buffer *buffer, char *ipAddress){
    member_node *node, *node2;
    if(hasMember(buffer, ipAddress)){
        node = buffer->memberList;
        if(strcmp(buffer->memberList->ipAddress,ipAddress) == 0){
            buffer->memberList = buffer->memberList->nextNode;
        }else{
            while(strcmp(node->nextNode->ipAddress,ipAddress) != 0){
                node = node->nextNode;
            }
            node2 = node;
            node = node->nextNode;
            node2->nextNode = node2->nextNode->nextNode;
        }
        free(node->ipAddress);
        free(node);
        node = NULL;
    }
}

int hasData(replication_buffer *buffer, char *hostname){
    data_node *node;
    node = buffer->dataList;
    while(node != NULL){
        if(strcmp(node->hostname,hostname) == 0){
            return 1;
        }
        node = node->nextNode;
    }
    return 0;
}

void removeDataNode(replication_buffer *buffer, char *hostname){
    data_node *node, *node2;
    if(hasData(buffer, hostname)){
        node = buffer->dataList;
        if(strcmp(buffer->dataList->hostname,hostname) == 0){
            buffer->dataList = buffer->dataList->nextNode;
        }else{
            while(strcmp(node->nextNode->hostname,hostname) != 0){
                node = node->nextNode;
            }
            node2 = node;
            node = node->nextNode;
            node2->nextNode = node2->nextNode->nextNode;
        }
        freeDataNode(node);
        node = NULL;
    }
}
