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
#include <fstream>

using namespace std;
#define M_TCP_PORT "25167"
#define M_UDP_PORT "24167"
#define EE_UDP_PORT "23167"
#define CS_UDP_PORT "22167"
#define C_UDP_PORT "21167"
#define BACKLOG 10
#define MAXBUF 1024

char buf[MAXBUF];
int c_skt;
struct addrinfo c_addr, *c_res;
struct sockaddr_storage ms_addr;
socklen_t addr_len = sizeof(ms_addr);

void create_udp_c()
{
    memset(&c_addr, 0, sizeof c_addr);
    c_addr.ai_family = AF_UNSPEC;
    c_addr.ai_socktype = SOCK_DGRAM;
    c_addr.ai_flags = AI_PASSIVE;
    int c_addr_res = getaddrinfo(NULL, C_UDP_PORT, &c_addr, &c_res);
    if (c_addr_res < 0)
    {
        perror("serverC fails to get address info");
        exit(1);
    }
    c_skt = socket(c_res->ai_family, c_res->ai_socktype, c_res->ai_protocol);
    if (c_skt < 0)
    {
        perror("failed to create socket for c");
        exit(1);
    }
    int bind_c = bind(c_skt, c_res->ai_addr, c_res->ai_addrlen);
    if (bind_c < 0)
    {
        perror("failed to bind");
        close(c_skt);
        exit(1);
    }
    freeaddrinfo(c_res);
    cout << "The ServerC is up and running using UDP on port " << C_UDP_PORT << " ." << endl;
}

char validation(char *buf)
{
    string s(buf);
    int blank = s.find(" ");
    string input_un = s.substr(0, blank);
    string input_pw = s.substr(blank + 1);

    // read from cred.txt
    ifstream fp("cred.txt");
    string line;
    while (getline(fp, line))
    {
        int divide = line.find(",");
        string un = line.substr(0, divide);
        string pw = line.substr(divide + 1);
        if (fp.peek() != EOF)
        {
            pw.erase(pw.size() - 1);
        }
        if (input_un == un)
        {
            if (input_pw == pw)
            {
                return '2'; // correct
            }
            else
            {
                return '1'; // wrong password
            }
        }
    }
    return '0'; // no username
}

int main()
{
    create_udp_c();
    while (true)
    {
        // serverC receive data from serverM
        int mtoc = recvfrom(c_skt, buf, MAXBUF - 1, 0, (struct sockaddr *)&ms_addr, &addr_len);
        if (mtoc < 0)
        {
            perror("ServerC fails to receive from serverM");
            exit(1);
        }
        // cout << buf << endl;
        cout << "The ServerC received an authentication request from the Main Server." << endl;

        // get the sent result and verify the result
        char validation_resarray[1];
        // char msg = validation(buf);
        // cout << msg << endl;
        // string validation_str = validation(buf);
        validation_resarray[0] = validation(buf);

        // PHASE 2A-1: serverC sends the result of the authentication request to serverM over a UDP connection
        int ctom = sendto(c_skt, validation_resarray, MAXBUF - 1, 0, (const struct sockaddr *)&ms_addr, addr_len);
        if (ctom < 0)
        {
            perror("ServerC fails to send the results to main server");
            exit(1);
        }
        cout << "The ServerC finished sending the response to the main server" << endl;
    }
    close(c_skt);
    return 0;
}