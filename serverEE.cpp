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
#include <sstream>
#include <map>

using namespace std;
#define M_TCP_PORT "25167"
#define M_UDP_PORT "24167"
#define EE_UDP_PORT "23167"
#define CS_UDP_PORT "22167"
#define C_UDP_PORT "21167"
#define BACKLOG 10
#define MAXBUF 1024

char buf[MAXBUF];
char res_buf[MAXBUF];
int ee_skt;
struct addrinfo ee_addr, *ee_res;
struct sockaddr_storage ms_addr;
socklen_t addr_len = sizeof(ms_addr);
string input_course, input_category;

void create_udp_ee()
{
    memset(&ee_addr, 0, sizeof ee_addr);
    ee_addr.ai_family = AF_UNSPEC;
    ee_addr.ai_socktype = SOCK_DGRAM;
    ee_addr.ai_flags = AI_PASSIVE;
    int ee_addr_res = getaddrinfo(NULL, EE_UDP_PORT, &ee_addr, &ee_res);
    if (ee_addr_res < 0)
    {
        perror("serverEE fails to get address info");
        exit(1);
    }
    ee_skt = socket(ee_res->ai_family, ee_res->ai_socktype, ee_res->ai_protocol);
    if (ee_skt < 0)
    {
        perror("failed to create socket for c");
        exit(1);
    }
    int bind_ee = bind(ee_skt, ee_res->ai_addr, ee_res->ai_addrlen);
    if (bind_ee < 0)
    {
        perror("failed to bind");
        close(ee_skt);
        exit(1);
    }
    freeaddrinfo(ee_res);
    cout << "The Server EE is up and running using UDP on port " << EE_UDP_PORT << " ." << endl;
}

string courseinfo(char *buf)
{
    string s(buf);
    int blank = s.find(" ");
    input_course = s.substr(0, blank);
    input_category = s.substr(blank + 1);

    // read from cred.txt
    ifstream fp("ee.txt");
    string line;
    string title[] = {"CourseCode",
                      "Credit",
                      "Professor",
                      "Days",
                      "CourseName"};
    int index = 0;
    map<string, string> m;
    while (getline(fp, line))
    {
        int divide = line.find(",");
        string coursecode = line.substr(0, divide);
        if (coursecode == input_course)
        {
            stringstream s_stream(line);
            while (s_stream.good())
            {
                string c;
                getline(s_stream, c, ',');
                // cout << c << endl;
                m.insert(pair<string, string>(title[index], c));
                index++;
            }
            string info = m[input_category];
            return info;
        }
    }
    return "NOT FOUND";
}

int main()
{
    create_udp_ee();
    while (true)
    {
        // PHASE 3B-2: serverEE receive data from serverM and find the info from query
        int mtoee = recvfrom(ee_skt, buf, MAXBUF - 1, 0, (struct sockaddr *)&ms_addr, &addr_len);
        if (mtoee < 0)
        {
            perror("ServerEE fails to receive from serverM");
            exit(1);
        }
        // cout << buf << endl;
        string info = courseinfo(buf);
        cout << "The Server EE received an authentication request from the Main Server about the " << input_category << " of " << input_course << endl;

        // PHASE 3B-3: serverEE process the data and send the information to main server
        // ------- store the information in buf
        if (info == "NOT FOUND")
        {
            cout << "Didn't find the course: " << input_course << endl;
        }
        else
        {
            cout << "The course information has been founded: The " << input_course << " of " << input_category << " is " << info << endl;
        }

        strncpy(res_buf, info.c_str(), MAXBUF);
        int eetom = sendto(ee_skt, res_buf, MAXBUF - 1, 0, (const struct sockaddr *)&ms_addr, addr_len);
        if (eetom < 0)
        {
            perror("ServerEE fails to send result to serverM");
            exit(1);
        }
        cout << "The Server EE finished sending the response to the Main Server." << endl;
    }
}