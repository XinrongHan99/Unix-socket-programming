#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>

using namespace std;
#define M_TCP_PORT "25167"
#define M_UDP_PORT "24167"
#define EE_UDP_PORT "23167"
#define CS_UDP_PORT "22167"
#define C_UDP_PORT "21167"
#define BACKLOG 10
#define MAXBUF 1024
char buf[MAXBUF];
char encrypted_buf[MAXBUF];
char backup_buf[MAXBUF];
char authres_buf[MAXBUF];
char query_buf[MAXBUF];
char courseinfo_buf[MAXBUF];
char receiveinfo_buf[MAXBUF];

char *username;

int m_skt, c_skt, cs_skt, ee_skt;
int recv_result;
int tcp_child_skt;
struct addrinfo m_addr, *m_res;
struct addrinfo c_addr, *c_res;
struct addrinfo cs_addr, *cs_res;
struct addrinfo ee_addr, *ee_res;
struct sockaddr_storage client_addr;
socklen_t client_addr_size;

// create socket for serverM tcp connection
void create_tcp_m()
{
    // initialize the enviornment for sockaddr structure
    memset(&m_addr, 0, sizeof m_addr);
    m_addr.ai_family = AF_UNSPEC;
    m_addr.ai_socktype = SOCK_STREAM;
    m_addr.ai_flags = AI_PASSIVE;
    int m_addr_res = getaddrinfo(NULL, M_TCP_PORT, &m_addr, &m_res);

    // initilize the socket
    m_skt = socket(m_res->ai_family, m_res->ai_socktype, m_res->ai_flags);
    if (m_skt < 0)
    {
        cout << "The socket not opened" << endl;
        exit(EXIT_FAILURE);
    }

    // bind the socket to the local port
    int m_bind_res = bind(m_skt, m_res->ai_addr, m_res->ai_addrlen);
    if (m_bind_res < 0)
    {
        cout << "Fail to bind to local port" << endl;
        exit(EXIT_FAILURE);
    }
    // listen the request from client (queues the requests)
    freeaddrinfo(m_res);

    int m_listen_res = listen(m_skt, BACKLOG);
    if (m_listen_res < 0)
    {
        cout << "Fail to start listen to local port" << m_listen_res << endl;
        exit(EXIT_FAILURE);
    }
}

// create socket for serverC udp connection
void create_udp_c()
{
    memset(&c_addr, 0, sizeof c_addr);
    c_addr.ai_family = AF_UNSPEC;
    c_addr.ai_socktype = SOCK_DGRAM;
    int c_addr_res = getaddrinfo("127.0.0.1", C_UDP_PORT, &c_addr, &c_res);
    c_skt = socket(c_res->ai_family, c_res->ai_socktype, c_res->ai_protocol);
}

// create socket for serverEE udp connection
void create_udp_ee()
{
    memset(&ee_addr, 0, sizeof ee_addr);
    ee_addr.ai_family = AF_UNSPEC;
    ee_addr.ai_socktype = SOCK_DGRAM;
    int ee_addr_info = getaddrinfo("127.0.0.1", EE_UDP_PORT, &ee_addr, &ee_res);
    ee_skt = socket(ee_res->ai_family, ee_res->ai_socktype, ee_res->ai_protocol);
}

// create socket for serverCS udp connection
void create_udp_cs()
{
    memset(&cs_addr, 0, sizeof cs_addr);
    cs_addr.ai_family = AF_UNSPEC;
    cs_addr.ai_socktype = SOCK_DGRAM;
    int cs_addr_info = getaddrinfo("127.0.0.1", CS_UDP_PORT, &cs_addr, &cs_res);
    cs_skt = socket(cs_res->ai_family, cs_res->ai_socktype, cs_res->ai_protocol);
}

void encrypt_user_info(char input[])
{
    for (int i = 0; i < strlen(input); i++)
    {
        char &c = input[i];
        if (c >= '0' && c <= '9')
        {
            c += 4;
            if (c > '9')
            {
                c -= 10;
            }
        }
        else if (c >= 'A' && c <= 'Z')
        {
            c += 4;
            if (c > 'Z')
            {
                c -= 26;
            }
        }
        else if (c >= 'a' && c <= 'z')
        {
            c += 4;
            if (c > 'z')
            {
                c -= 26;
            }
        }
    }
}

int main()
{
    cout << "The main server is up and running." << endl;
    create_tcp_m(); // create tcp socket for main server
    create_udp_c(); // create udp socket to connect to server C
    create_udp_cs();
    create_udp_ee();

    // PHASE 1A-2: serverM accept tcp child socekt from client
    tcp_child_skt = accept(m_skt, (struct sockaddr *)&client_addr, &client_addr_size);
    if (tcp_child_skt < 0)
    {
        perror("Main server fails to accept tcp child socket");
        exit(1);
    }

    int count = 0;
    while (true)
    {
        if (count == 3)
        {
            count = 0;
            tcp_child_skt = accept(m_skt, (struct sockaddr *)&client_addr, &client_addr_size);
            if (tcp_child_skt < 0)
            {
                perror("Main server fails to accept tcp child socket");
                exit(1);
            }
        }
        // PHASE 1A-4:main server receive aucentication request from client
        count++;
        int clientom_res = recv(tcp_child_skt, buf, MAXBUF, 0);
        if (clientom_res < 0)
        {
            perror("Main server fails to receive");
            exit(1);
        }

        // PHASE 1A-5:encryption
        strncpy(encrypted_buf, buf, sizeof(buf));
        encrypt_user_info(encrypted_buf);
        username = strtok(buf, " ");
        cout << "The main server received the authentication for " << username << " using TCP over port " << M_TCP_PORT << endl;

        // PHASE 1B: Main server forwards the authentication request to the credentials server over UDP
        int mtoc_res = sendto(c_skt, encrypted_buf, MAXBUF, 0, c_res->ai_addr, c_res->ai_addrlen);
        if (mtoc_res < 0)
        {
            perror("main server failed to sent the authentication request");
            exit(1);
        }
        cout << "The main server sent an authentication request to serverC" << endl;

        // PHASE 2A-2: serverM receive the authentication result from sever C
        int ctom_auth = recvfrom(c_skt, authres_buf, MAXBUF, 0, c_res->ai_addr, &(c_res->ai_addrlen));
        cout << "The main server received the result of the authentication request from ServerC using UDP over port " << C_UDP_PORT << endl;
        // cout << authres_buf[0] << endl;

        // PHASE 2B-1: serverM sends the result of the authentication request to the client over TCP connection
        int mtoclient_auth = send(tcp_child_skt, authres_buf, sizeof(authres_buf), 0);
        if (mtoclient_auth < 0)
        {
            perror("main server failed to send authentication result to client");
            exit(1);
        }
        cout << "The main server sent the authentication result to the client" << endl;

        // ------ repeat verification if information not correct
        if (authres_buf[0] == '2')
        {
            break;
        }
        else
        {
            continue;
        }
    }

    while (true)
    {
        // PHASE 3A-2 serverM receive query from client about course
        int clientom_query = recv(tcp_child_skt, query_buf, MAXBUF, 0);
        strncpy(courseinfo_buf, query_buf, sizeof(query_buf));
        if (clientom_query < 0)
        {
            perror("failed to receive query from the client");
            exit(1);
        }
        string coursecode = strtok(query_buf, " ");
        string category = strtok(NULL, " ");
        cout << "The main server received from " << username << " to query course " << coursecode << " about " << category << " using TCP over port " << M_TCP_PORT << endl;

        string dep = coursecode.substr(0, 2);

        if (dep == "EE")
        {
            // EE PHASE 3B-1 serverM sent query information to corresponding department backend using UDP connection
            int mtoee = sendto(ee_skt, courseinfo_buf, MAXBUF, 0, ee_res->ai_addr, ee_res->ai_addrlen);
            if (mtoee < 0)
            {
                perror("error sending to ee server");
                exit(1);
            }
            cout << "The main server sent a request to server EE" << endl;

            // EE PHASE 3B-4 Mainserver receive the information from server EE
            int eetom = recvfrom(ee_skt, receiveinfo_buf, MAXBUF, 0, ee_res->ai_addr, &(ee_res->ai_addrlen));
            if (eetom < 0)
            {
                perror("error receive from ee server");
                exit(1);
            }
            cout << "The main server received the response from serverEE using UDP over port " << EE_UDP_PORT << endl;
        }
        else if (dep == "CS")
        {
            // CS PHASE 3B-1 serverM sent query information to corresponding department backend using UDP connection
            int mtocs = sendto(cs_skt, courseinfo_buf, MAXBUF, 0, cs_res->ai_addr, cs_res->ai_addrlen);
            if (mtocs < 0)
            {
                perror("error sending to cs server");
                exit(1);
            }
            cout << "The main server sent a request to server CS" << endl;

            // CS PHASE 3B-4 Mainserver receive the information from server CS
            int cstom = recvfrom(cs_skt, receiveinfo_buf, MAXBUF, 0, cs_res->ai_addr, &(cs_res->ai_addrlen));
            if (cstom < 0)
            {
                perror("error receive from cs server");
                exit(1);
            }
            cout << "The main server received the response from serverCS using UDP over port " << CS_UDP_PORT << endl;
        }

        // PHASE 4B-1 send the result to client
        int mtoclient_info = send(tcp_child_skt, receiveinfo_buf, sizeof(receiveinfo_buf), 0);
        if (mtoclient_info < 0)
        {
            perror("error sending course info to client");
            exit(1);
        }
        cout << "The main server sent the query information to the client." << endl;
    }

    // return 0;
}