#ifndef REPLICATION_BUFFER_H
#define REPLICATION_BUFFER_H

typedef struct _ack_node{
    char *hostname;
    struct _ack_node* nextNode;
} ack_node;

typedef struct _data_node{
    char *hostname;
    char *macAddress;
    char *ipAddress;
    char *status;
    struct _data_node* nextNode;
} data_node;

typedef struct _member_node{
    char *ipAddress;
    int ackCounter;
    ack_node *acks;
    struct _member_node* nextNode;
} member_node;

typedef struct _replication_buffer{
    int dataNumber;
    member_node *memberList;
    data_node* dataList;
} replication_buffer;


void createReplicationBuffer(replication_buffer **buffer);
void ackBuffer(replication_buffer *buffer, char *hostname, char *ipAddress);
void freeBuffer(replication_buffer *buffer);
void addDataNodeToBuffer(replication_buffer *buffer, char *hostname, char *macAddress, char *ipAddress, char *status);
int isBufferAcked(replication_buffer *buffer);
void removeMember(replication_buffer *buffer, char *ipAddress);
void addMember(replication_buffer *buffer, char *ipAddress);
int hasMember(replication_buffer *buffer, char *ipAddress);

#endif