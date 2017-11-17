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
#include <sstream>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#define MAXDATASIZE 1024 // max number of bytes we can get at once

#define FILE_NOT_FOUND_DESC -1

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

void save_get_file(char buffer[])
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
}


/**
    @param file name of found file & size
    @return the OK -200- HTTP reply
*/

int get_file_size_from_header(char buffer[])
{
    int length = strlen(buffer);
    int file_size=0;
    string line = "";

    for(int i = 0 ; i < length ; i++)
    {
        if(buffer[i] == ' ' || buffer[i] == '\r' || buffer[i] == '\n')
        {

            if( (line.compare("Content-Length:")) == 0)
            {
                line = "";
                i++;

                while(buffer[i] != '\r')
                {
                    line+= buffer[i];
                    i++;
                }

                file_size = stoi(line);

                return file_size;
            }

            line = "";
        }
        else
        {
            line+=buffer[i];
        }

    }

    return -1;

}

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
        }
        else
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

long get_file_size(int fd)
{
    struct stat stat_buf;
    int rc = fstat(fd, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

/**
    @param file path
    @return file descriptor
*/
int get_file_descriptor(string file_name)
{
    char *cstr = new char[file_name.length() + 1];
    strcpy(cstr, file_name.c_str());
    return open(cstr, O_RDONLY);
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

        cout<<"data[1]: "<<data[1]<<"\n";



        if((data[0].compare("POST")) == 0)
        {
    ///////////////////////////////////////////////////////////////////////////////////////
            string file_name = data[1];

            int fd = get_file_descriptor (file_name);

            long file_size = get_file_size(fd);

            cout << "file size : "<<file_size<<endl;



            // create a reply post request
            stringstream  ss;
            ss.str("");
            ss<<"POST /";
            ss<<data[1];
            ss<<" http/1.1\r\n";
            ss<<"Content-Length: " ;
            ss<<file_size;
            ss<<"\r\n";
            string reply = ss.str();
            cout<< "reply: "<<reply<<"\n";

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

            /////////////////////////////////////////////////////////////////////////////

           //  file_name = data[1];
            //file_name.erase(0,1);
           // cout << "requested file : "<<file_name<<endl;

            //fd = get_file_descriptor (file_name);
            //cout << "file descriptor : "<<fd<<endl;
            if(fd!=FILE_NOT_FOUND_DESC)
            {
                // file is found

                //long file_size = get_file_size(fd);
                //cout << "file size : "<<file_size<<endl;

                off_t offset = 0;
                int remain_data = file_size;
                size_t sent_bytes = 0;
                /* Sending file data */
                while (((sent_bytes = sendfile(sockfd, fd, &offset, MAXDATASIZE)) > 0) && (remain_data > 0))
                {
                    remain_data -= sent_bytes;
                    fprintf(stdout, " sent  = %d bytes, offset : %d, remaining data = %d\n",
                            sent_bytes, offset, remain_data);
                    usleep(500);
                    if(remain_data<MAXDATASIZE){
                        sendfile(sockfd, fd, &offset, remain_data);
                    }
                }

                close(fd);



            }

            usleep(1000000);
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
            if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1)
            {
                perror("recv");
                exit(1);
            }

            buf[numbytes] = '\0';

            printf("client: received '%s'\n",buf);

            if(check_response(buf))
            {
                cout<<"****************************************************";
                FILE * recieved_file ;
                string path = data[1];
                char *file_name = new char[path.length() + 1];
                strcpy(file_name, path.c_str());

                recieved_file = fopen(file_name, "w");

                if (recieved_file == NULL)
                {
                    fprintf(stderr, "Failed to open file foo --> %s\n", strerror(errno));

                    exit(EXIT_FAILURE);
                }
                int remain_data = get_file_size_from_header(buf);
                cout<<"remain: "<<remain_data<<endl;
                int numbytes ;
              /*  while (remain>0 &&(numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) > 0)
                {
                    fwrite(buf, sizeof(char), numbytes, recieved_file);
                    remain -= numbytes;
                    fprintf(stdout, "Receive %d bytes\n", numbytes);


                }*/
                 //timeval timeout = { 3, 0 };
                //fd_set in_set;

                while ( remain_data > 0)
                {
                    timeval timeout = { 3, 0 };
                    fd_set in_set;

                    FD_ZERO(&in_set);
                    FD_SET(sockfd, &in_set);

                    // select the set
                    int cnt = select(sockfd + 1, &in_set, NULL, NULL, &timeout);

                    /////////////////////////////////////////////////////////////////////////
                    if (FD_ISSET(sockfd, &in_set))
                    {
                        numbytes = recv(sockfd, buf, MAXDATASIZE, 0);
                        if (numbytes <= 0)
                        {
                            // nothing received from client
                            cout<< "nothing received from client \n";
                            break;
                        }
                        fwrite(buf, sizeof(char), numbytes, recieved_file);
                        remain_data -= numbytes;
                        fprintf(stdout, "Remain %d bytes\n", remain_data);

                    }
                    else
                    {
                        // nothing received from client in last 5 seconds
                        cout << "nothing received from client in last 3 seconds\n";
                        break;
                    }

                }

                fclose(recieved_file);
                cout<<"client after while\n";
            }
            else
            {
                cout<<"////////////////////////////////////////////////////";
            }
            /***/
        usleep(1000000);
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

    if ((rv = getaddrinfo("192.168.1.133","3490", &hints, &servinfo)) != 0)
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


    read_input_file(sockfd);

    close(sockfd);

    return 0;
}
