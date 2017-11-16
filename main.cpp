/**
    this is the simple http client
    @author mphilip
*/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include <fstream>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define MAXDATASIZE 1000 // max number of bytes we can get at once

using namespace std;
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/**
    this function to parse the line
    @param line input line that will parse
*/
vector<string> parse_line(string temp)
{
    char buffer[1024];
    strcpy(buffer, temp.c_str());
    int length = strlen(buffer);

    string line="";

    vector<string> data;

    for(int i = 0 ; i < length ; i++)
    {

        if(buffer[i] == ' ')
        {
            data.push_back(line);
            line = "";
        }
        else
        {
            line+=buffer[i];
        }

    }
    return data;
}

/*void save_get_file(char buffer[])
{

    int length = strlen(buffer);
    string line = "";
    int count_n = 0;
    std::string s (buffer);
    std::string delimiter = "\r\n";

    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos)
    {
        token = s.substr(0, pos);
        std::cout <<"token:"<< token << std::endl;
        s.erase(0, pos + delimiter.length());
    }
    std::cout << s << std::endl;
}*/

bool check_response(char buffer[])
{

    int length = strlen(buffer);

    string line = "";

    for(int i = 0 ; i < length ; i++)
    {
        if(buffer[i] == ' ')
        {
            if( (line.compare("404")) == 0)
            {
                return false;
            }
            else if( (line.compare("200")) == 0)
            {
                return true;
            }
            line = "";
        }else
            line+=buffer[i];

    }
    return false;
}

char* read_post_file(string name)
{
    std::ifstream file(name.c_str());
    std::string str;
    string data = "";
    while (std::getline(file, str))
    {
        data+= str;
    }

    char *cstr = new char[data.length() + 1];
    strcpy(cstr, data.c_str());
    file.close();
    return cstr;

}
/**
    this function read the input file and parse it

*/
void read_input_file(int sockfd)
{
    std::ifstream file("input.txt");
    std::string str;
    while (std::getline(file, str))
    {
        vector<string> data = parse_line(str);

        cout<<"data[1]:"<<data[1]<<"\n";

        if((data[0].compare("POST")) == 0)
        {
            // create a reply post request
            string reply = "POST /"+data[1]+" http/1.1\r\n";

            cout<< reply<<"\n";

            char *cstr = new char[reply.length() + 1];
            strcpy(cstr, reply.c_str());

            // send the post request
            if (send(sockfd, cstr, strlen(cstr), 0) == -1)
                perror("send");

            // receive ok
            int numbytes;
            char buf[MAXDATASIZE];
            if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
            {
                perror("recv");
                exit(1);
            }

            buf[numbytes] = '\0';

            printf("client: received '%s'\n",buf);

            char * cstr_post = read_post_file(data[1]);
            cout << "\n"<<cstr_post;

            if (send(sockfd,cstr_post, strlen(cstr_post), 0) == -1)
                perror("send");

            cout<<"post"<<"\n";
        }
        else if((data[0].compare("GET")) == 0)
        {
            cout<<"get"<<"\n";

            string reply = "GET /"+data[1]+" http/1.1\r\n";

            cout<< reply<<"\n";

            char *cstr = new char[reply.length() + 1];
            strcpy(cstr, reply.c_str());

            // send the post request
            if (send(sockfd, cstr, strlen(cstr), 0) == -1)
                perror("send");

            // receive the get
            int numbytes;
            char buf[MAXDATASIZE];
            if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
            {
                perror("recv");
                exit(1);
            }

            buf[numbytes] = '\0';

            printf("client: received '%s'\n",buf);

            if(check_response(buf))
            {
                cout<<"****************************************************";
                save_get_file(buf);
            }else{
                cout<<"////////////////////////////////////////////////////";
            }

        }
        else
        {
            cout<<"error"<<"\n";
        }
    }

    file.close();
}
/**
    the main method
*/
int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    //string port_number = argv[2];

   /* if (argc != 3)
    {
        fprintf(stderr,"usage: client server_iP port_number\n");
        exit(1);
    }*/

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("192.168.1.50", "3490", &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);

    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    /*
        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';

        printf("client: received '%s'\n",buf);
    */

    read_input_file(sockfd);

    close(sockfd);

    return 0;
}
