#include "http_preprocessor.h"
#include <string.h>
#include <stdio.h>
#include "os_log.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
os_log_create_module(http_preprocessor,OS_LOG_LEVEL_INFO);

struct HTTP_RES_HEADER
{
    int status_code;//HTTP/1.1 '200' OK
    char content_type[128];//Content-Type: application/gzip
    long content_length;//Content-Length: 11683079
};

static int *check_native_audio_type(char *target, char **type) {
    if(!strcmp(target,"audio/mpeg")) {
        *type = "mp3";
    } else {
        return -1;
    }
    return 0;
}

void get_ip_addr(char *host_name, char *ip_addr)
{
    int i;
    struct hostent *host = gethostbyname(host_name);
    if (!host)
    {
        ip_addr = NULL;
        OS_LOG_D(http_preprocessor,"host is NULL\n");
        return;
    }

    for (i = 0; host->h_addr_list[i]; i++)
    {

        strcpy(ip_addr, inet_ntoa( * (struct in_addr*) host->h_addr_list[i]));
        break;
    }
}

void parse_url(const char *url, char *host, int *port, char *file_name)
{
    int j = 0,i;
    int start = 0;
    *port = 80;
    char *patterns[] = {"http://", "https://", NULL};

    for (i = 0; patterns[i]; i++)
        if (strncmp(url, patterns[i], strlen(patterns[i])) == 0)
            start = strlen(patterns[i]);

    for (i = start; url[i] != '/' && url[i] != '\0'; i++, j++)
        host[j] = url[i];
    host[j] = '\0';

    char *pos = strstr(host, ":");
    if (pos)
        sscanf(pos, ":%d", port);

    for (i = 0; i < (int)strlen(host); i++)
    {
        if (host[i] == ':')
        {
            host[i] = '\0';
            break;
        }
    }

    j = 0;
    for (i = start; url[i] != '\0'; i++)
    {
        if (url[i] == '/')
        {
            if (i !=  strlen(url) - 1)
                j = 0;
            continue;
        }
        else
            file_name[j++] = url[i];
    }
    file_name[j] = '\0';
}

int http_read_line(int sock, char * buf, int size)
{
	int i = 0;

	while (i < size - 1)
	{
		if (read(sock, buf + i, 1) <= 0)
		{
			if (i == 0)
				return -1;
			else
				break;
		}
		if (buf[i] == '\n')
			break;
		if (buf[i] != '\r')
			i++;
	}
	buf[i] = '\0';
	return i;
}

struct HTTP_RES_HEADER parse_header(const char *response)
{
    struct HTTP_RES_HEADER resp;

    char *pos = strstr(response, "HTTP/");
    if (pos)
        sscanf(pos, "%*s %d", &resp.status_code);

    pos = strstr(response, "Content-Type:");
    if (pos)
        sscanf(pos, "%*s %s", resp.content_type);

    pos = strstr(response, "Content-Length:");
    if (pos)
        sscanf(pos, "%*s %ld", &resp.content_length);

    return resp;
}


int http_read_first_line(int sock, char * buf, int size)
{
	/* Skips the HTTP-header, if there is one, and reads the first line into buf.
	   Returns number of bytes read. */
	
	int i;
	/* Skip the HTTP-header */
	if ((i = http_read_line(sock, buf, size)) < 0)
		return -1;
	if (!strncmp(buf, "HTTP", 4)) /* Check to make sure its not HTTP/0.9 */
	{
		while (http_read_line(sock, buf, size) > 0)
			/* nothing */;
		if ((i = http_read_line(sock, buf, size)) < 0)
			return -1;
	}
	
	return i;
}

int http_preprocessor_init_impl(struct play_preprocessor *self,
        play_preprocessor_cfg_t *cfg) {
    char host_addr[64] = {0};
    int port = 80;
    int i;
    char file_name[2048];
	struct sockaddr_in address;
	struct hostent *host;
    int * socket_ptr = (int*)malloc(sizeof(int)); 
    parse_url(cfg->target,host_addr,&port,file_name);

    *socket_ptr = socket(AF_INET, SOCK_STREAM, 0);
	address.sin_family = AF_INET;

    OS_LOG_D(http_preprocessor,"http_preprocessor_init_imp gethostbyname\n");
	if (!(host = gethostbyname(host_addr)))
		return 0;
    OS_LOG_D(http_preprocessor,"http_preprocessor_init_imp gethostbyname success\n");

	memcpy(&address.sin_addr.s_addr, *(host->h_addr_list), sizeof(address.sin_addr.s_addr));
	address.sin_port = htons(port);

	if (connect(*socket_ptr, (struct sockaddr *) &address, sizeof (struct sockaddr_in)) == -1) {
		OS_LOG_D(http_preprocessor,"connect sock failed\n");
        return 0;
    }
    OS_LOG_D(http_preprocessor,"http_preprocessor_init_imp connect socket success\n");
   
    OS_LOG_D(http_preprocessor,"cfg->target:%s,host_addr:%s,port:%d,file_name:%s\n",
            cfg->target,host_addr,port,file_name); 
    //设置http请求头信息
    char header[2048] = {0};
    sprintf(header, \
            "GET %s HTTP/1.1\r\n"\
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537(KHTML, like Gecko) Chrome/47.0.2526Safari/537.36\r\n"\
            "Host: %s\r\n"\
            "Connection: keep-alive\r\n"\
            "\r\n"\
            ,cfg->target , host_addr);
    if(write(*socket_ptr, header, strlen(header)) == -1) {
        OS_LOG_D(http_preprocessor,"write socket failed\n");
        return -1;    
    }

    int mem_size = 4096;
    int length = 0;
    int len;
    char *buf = (char *) malloc(mem_size * sizeof(char));
    char *response = (char *) malloc(mem_size * sizeof(char));

    while ((len = read(*socket_ptr, buf, 1)) != 0)
    {
        if (length + len > mem_size)
        {
            mem_size *= 2;
            char * temp = (char *) realloc(response, sizeof(char) * mem_size);
            if (temp == NULL)
            {
                OS_LOG_D(http_preprocessor,"malloc failed\n");
                exit(-1);
            }
            response = temp;
        }

        buf[len] = '\0';
        strcat(response, buf);

        int flag = 0;
        for (i = strlen(response) - 1; response[i] == '\n' || response[i] == '\r'; i--, flag++);
        if (flag == 4)
            break;

        length += len;
    }

    struct HTTP_RES_HEADER resp = parse_header(response);

    OS_LOG_D(http_preprocessor,"\n>>>>http header parse success:<<<<\n");

    OS_LOG_D(http_preprocessor,"\tHTTP reponse: %d\n", resp.status_code);
    if (resp.status_code != 200)
    {
        OS_LOG_D(http_preprocessor,"file can't be download,status code: %d\n", resp.status_code);
        return 0;
    }
    OS_LOG_D(http_preprocessor,"\tHTTP file type: %s\n", resp.content_type);
    OS_LOG_D(http_preprocessor,"\tHTTP file length: %ld byte\n\n", resp.content_length);

    int ret = check_native_audio_type(resp.content_type,&cfg->type);
    if(ret != 0) {
        OS_LOG_D(http_preprocessor,"http_preprocessor can't find decode type\n");
        return -1;
    } 
    free(buf);
    free(response);
    self->userdata = (void*)socket_ptr;
    cfg->frame_size = 1024;
    OS_LOG_D(http_preprocessor,"[%s]open native http ok, http: %s, audio type:%s\n", cfg->tag, cfg->target, cfg->type);
    return 0;
}
int http_preprocessor_read_impl(struct play_preprocessor *self, char *data,
        size_t data_len) {
    OS_LOG_D(http_preprocessor,"http_preprocessor_read_impl read data_len:%d\n",data_len);
    if(!self->userdata) {
        return -1;
    }
    int client_socket = *(int*)self->userdata;
    size_t read_len = read(client_socket,data,data_len);
    return read_len;
}
void http_preprocessor_destroy_impl(struct play_preprocessor *self) {
    if (!self) return;
    if(!self->userdata) {
        return -1;
    }
    int client_socket = *(int*)self->userdata;
    close(client_socket);
}
