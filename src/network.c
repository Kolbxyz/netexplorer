#include "network.h"
#include "storage.h"
#include "protocol.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

//bolt - green - OK! - normal color - normal text
#define MSG_OK "\e[1m\e[32mOK\e[39m\e[0m"
#define MSG_FAIL "\e[1m\e[31mFAIL\e[39m\e[0m"

uint16_t search_ports[] = { 27,
    20,     //FTP
    21,     //FTP
    22,     //SSH
    23,     //telnet
    25,     //smtp
    53,     //dns
    80,     //HTTP
    110,    //POP3 (email)
    119,    //NNTP
    123,    //net time
    143,    //IMAP (email)
    161,    //SNMP
    162,    //SNMP
    194,    //IRC
    179,    //bgp
    443,    //HTTPS
    445,    //PME (microsoft)
    500,    //ISAKMP
    554,    //RTSP (audio/video)
    1080,   //SOCKS (proxy)
    3306,   //mySQL
    3389,   //RDP (windows)
    5000,   //flask
    5432,   //PostgreSQL
    8080,   //other http
    25565,  //minecraft java
};

port_t network_addr_req(const char* addr, uint16_t port, struct network_task_info info){
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    struct timeval timeout;      
    timeout.tv_sec = 0;
    timeout.tv_usec = info.timeout;
    char* msg = malloc(1500);
    strcpy(msg,"");
    
    setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in server;
    memset(&server,0,sizeof(struct sockaddr_in));
    server.sin_addr.s_addr = inet_addr(addr);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    port_t port_result = port_new(addr,port,0);

    if(connect(sockfd, (struct sockaddr*)&server, sizeof(server)) != -1){
        port_result.type = PORT_TYPE_UNKNOW;
        if(port == 25565){
            port_result.type = PORT_TYPE_MINECRAFT;
            minecraft_server_info_t res = protocol_get_minecraft_server_info(sockfd, port_result, info, msg);
        }

        storage_save_port(port_result);
    }
    else{
        
    }
    close(sockfd);
    if(port_result.type == 0){
        if(info.log_mode == LOG_ALL){printf("try connect to %s:%d -> %s\n",addr,port,MSG_FAIL);}
        return port_result;
    }

    printf("try connect to %s:%d -> %s %s\n",addr,port,MSG_OK, msg);

    free(msg);
    return port_result;
    
}

void* network_search_random(void* _info){
    struct network_task_info* info = _info;
    srand(time(NULL));
    char* tmpip = malloc(50);
    for(int i=0;i<info->requests;i++){
        sprintf(tmpip,"%d.%d.%d.%d",rand()%255,rand()%255,rand()%255,rand()%255);
        network_addr_search(tmpip, *info);
    }

    free(tmpip);
    return 0;
}

int network_explore(const char* addr, struct network_task_info info){
    if(strcmp(addr,"random")==0){
        pthread_t* threads = malloc(sizeof(pthread_t)*info.threads);
        for(int t=0;t<info.threads;t++){
            //network_search_random(info);
            pthread_create(&threads[t], NULL, &network_search_random, (void*)&info);
        }
        for(int t=0;t<info.threads;t++){
            pthread_join(threads[t],NULL);
        }
        
        return 0;
    }
    network_search_task(addr, info);
}

int network_search_task(const char* addr, struct network_task_info info){
    char* ip = malloc(strlen(addr));
    strcpy(ip,addr);
    uint8_t task_result[4][2];

    int iplen = strlen(ip);
    char* iplist = malloc(strlen(addr));
    strcpy(iplist,addr);
    for(int i=0;i<iplen;i++){
        if(iplist[i] == '.'){
            iplist[i] = '\0';
        }
    }

    char* sip_ptr = iplist;
    for(int i=0;i<4;i++){
        if(strcmp(sip_ptr,"*")==0){
            task_result[i][0] = 0;
            task_result[i][1] = 255;
            printf("ip%d 0-255\n",i);
            sip_ptr += strlen(sip_ptr)+1;
            continue;
        }
        task_result[i][0] = atoi(sip_ptr);
        task_result[i][1] = atoi(sip_ptr);
        sip_ptr += strlen(sip_ptr)+1;
    }

    char* tmpip = malloc(50);
    for(int ip0=task_result[0][0];ip0<=task_result[0][1];ip0++){
        for(int ip1=task_result[1][0];ip1<=task_result[1][1];ip1++){
            for(int ip2=task_result[2][0];ip2<=task_result[2][1];ip2++){
                for(int ip3=task_result[3][0];ip3<=task_result[3][1];ip3++){
                    sprintf(tmpip,"%d.%d.%d.%d",ip0,ip1,ip2,ip3);
                    network_addr_search(tmpip, info);
                }
            }
        }
    }
    free(tmpip);
    free(ip);
    free(iplist);
    return 0;
}

int network_addr_search(const char* addr, struct network_task_info info){
    if(info.port == 0){
        for(int i=1;i<search_ports[0];i++){
            network_addr_req(addr, search_ports[i],info);
        }
    }
    else{
        network_addr_req(addr, info.port,info);
    }
}

int network_search_local(uint8_t subnet, struct network_task_info info){
    char addr[50] = {0}; 
    for(int i=0;i<256;i++){
        sprintf(addr,"192.168.%d.%d",subnet,i);
        network_addr_search(addr, info);
    }
}
