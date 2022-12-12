#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#define PORT "25167"    // the port client will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once
#define BUF 1024
using namespace std;

int client_tcp_port;
int skt_client;
struct addrinfo c_addr, *c_res;
string username;
string password;
string course;
string category;
char c;
char send_user_buf[BUF];
char receive_user_buf[BUF];
char send_query_buf[BUF];
char receive_query_buf[BUF];
char auth_res = -1;

void clinet_tcp_connect()
{
    memset(&c_addr, 0, sizeof c_addr);
    c_addr.ai_family = AF_UNSPEC;
    c_addr.ai_socktype = SOCK_STREAM;
    c_addr.ai_flags = AI_PASSIVE;
    int server_addr = getaddrinfo("127.0.0.1", PORT, &c_addr, &c_res);
    if (server_addr < 0)
    {
        cout << "fails to get address info";
        exit(1);
    }
    skt_client = socket(c_res->ai_family, c_res->ai_socktype, c_res->ai_protocol);
    if (skt_client < 0)
    {
        perror("Main server fails to connet to the socket");
        exit(1);
    }
    int connect_client = connect(skt_client, c_res->ai_addr, c_res->ai_addrlen);
    if (connect_client < 0)
    {
        perror("client: fails to connect");
        exit(1);
    }
    client_tcp_port = ((struct sockaddr_in *)(void *)c_res->ai_addr)->sin_port;
}

int main(int argc, char *argv[])
{
    // PHASE 1A-1:clients sends the tcp connection request to serverM
    clinet_tcp_connect();
    cout << "The client is up and running." << endl;

    int attemp = 3;
    while (attemp > 0)
    {
        if (auth_res == '2')
        {
            break;
        }
        cout << "Please enter the username: ";
        username = "";
        while ((c = cin.get()) != '\n')
        {
            username = username + c;
        }
        // cin >> username;
        cout << "Please enter the password: ";
        // cin >> password;
        password = "";
        while ((c = cin.get()) != '\n')
        {
            password = password + c;
        }

        string input = username + " " + password;

        strncpy(send_user_buf, input.c_str(), BUF);

        // PHASE 1A-3:clients sends the authentication request to the main server over TCP connection
        int clientom_auth = send(skt_client, send_user_buf, BUF, 0);
        if (clientom_auth < 0)
        {
            perror("client fails to send");
        }
        // PHASE 2B-2:clients received the results of authentication from serverM.
        int mtoclient_auth = recv(skt_client, receive_user_buf, BUF, 0);
        cout << username << " received the result of authentication using TCP over port " << PORT << ": ";

        if (mtoclient_auth < 0)
        {
            perror("client fails to receive");
        }
        auth_res = receive_user_buf[0];
        if (auth_res == '1')
        {
            cout << "Password does not match" << endl;
            attemp--;
            cout << "Attempts remaining: " << attemp << endl;
        }
        else if (auth_res == '0')
        {
            cout << "Username does not exist" << endl;
            attemp--;
            cout << "Attempts remaining: " << attemp << endl;
        }
        memset(&receive_user_buf, 0, sizeof(receive_user_buf));
    }

    if (auth_res == '2')
    {
        cout << "Authentication is successful" << endl;
        while (true)
        {
            // PHASE 3A-1: client sending query to the central registration server
            cout << "Please enter the course code to query: ";
            cin >> course;
            cout << "Please enter the category(Credit/Professor/Days/CourseName): ";
            cin >> category;
            // ------ construct query string
            string query = course + " " + category;
            strncpy(send_query_buf, query.c_str(), BUF);
            // ------ send query to main server
            int clientom_query = send(skt_client, send_query_buf, BUF, 0);
            if (clientom_query < 0)
            {
                cout << "error sending query to main server" << endl;
                exit(1);
            }
            cout << username << " sent a request to the main server." << endl;

            // PHASE 4B-2: receive query result from main server
            int mtoclient_queryres = recv(skt_client, receive_query_buf, BUF, 0);
            if (mtoclient_queryres < 0)
            {
                cout << "error receiving from the main server" << endl;
            }
            cout << "The client received the response from the Main server using TCP over port " << PORT << endl;

            // ------ received info
            string s(receive_query_buf);
            if (s == "NOT FOUND")
            {
                cout << "Didn't find the course: " << course << endl;
            }
            else
            {
                cout << "The " << category << " of " << course << " is " << receive_query_buf << endl;
            }

            // ------ start new request
            cout << "----- Start a new request ------" << endl;
        }
    }
    else
    {
        cout << "Authentication Failed for 3 attempts. Client will shut down." << endl;
        exit(1);
    }
}