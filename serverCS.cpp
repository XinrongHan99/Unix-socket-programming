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
int cs_skt;
struct addrinfo cs_addr, *cs_res;
struct sockaddr_storage ms_addr;
socklen_t addr_len = sizeof(ms_addr);
string input_course, input_category;

void create_udp_cs()
{
    memset(&cs_addr, 0, sizeof cs_addr);
    cs_addr.ai_family = AF_UNSPEC;
    cs_addr.ai_socktype = SOCK_DGRAM;
    cs_addr.ai_flags = AI_PASSIVE;
    int cs_addr_res = getaddrinfo(NULL, CS_UDP_PORT, &cs_addr, &cs_res);
    if (cs_addr_res < 0)
    {
        perror("serverCS fails to get address info");
        exit(1);
    }
    cs_skt = socket(cs_res->ai_family, cs_res->ai_socktype, cs_res->ai_protocol);
    if (cs_skt < 0)
    {
        perror("failed to create socket for c");
        exit(1);
    }
    int bind_cs = bind(cs_skt, cs_res->ai_addr, cs_res->ai_addrlen);
    if (bind_cs < 0)
    {
        perror("failed to bind");
        close(cs_skt);
        exit(1);
    }
    freeaddrinfo(cs_res);
    cout << "The Server CS is up and running using UDP on port " << CS_UDP_PORT << " ." << endl;
}

string courseinfo(char *buf)
{
    string s(buf);
    int blank = s.find(" ");
    input_course = s.substr(0, blank);
    input_category = s.substr(blank + 1);

    // read from cred.txt
    ifstream fp("cs.txt");
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
    create_udp_cs();
    while (true)
    {
        // PHASE 3B-2: serverEE receive data from serverM and find the info from query
        int mtocs = recvfrom(cs_skt, buf, MAXBUF - 1, 0, (struct sockaddr *)&ms_addr, &addr_len);
        if (mtocs < 0)
        {
            perror("ServerCS fails to receive from serverM");
            exit(1);
        }
        // cout << buf << endl;
        string info = courseinfo(buf);
        cout << "The Server CS received an authentication request from the Main Server about the " << input_category << " of " << input_course << endl;

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
        int cstom = sendto(cs_skt, res_buf, MAXBUF - 1, 0, (const struct sockaddr *)&ms_addr, addr_len);
        if (cstom < 0)
        {
            perror("ServerCS fails to send result to serverM");
            exit(1);
        }
        cout << "The Server CS finished sending the response to the Main Server." << endl;
    }
}