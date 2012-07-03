/*****************************************************************
|
|      Atomix Apps - NetPump
|
| Copyright (c) 2002-2010, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Atomix.h"

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
typedef enum {
    ENDPOINT_TYPE_UDP_CLIENT,
    ENDPOINT_TYPE_UDP_SERVER,
    ENDPOINT_TYPE_TCP_CLIENT,
    ENDPOINT_TYPE_TCP_SERVER,
    ENDPOINT_TYPE_FILE
} EndPointType;

typedef enum {
    ENDPOINT_DIRECTION_IN,
    ENDPOINT_DIRECTION_OUT
} EndPointDirection;

typedef struct {
    EndPointType      type;
    EndPointDirection direction;
    union {
        struct {
            int port;
        } udp_server;
        struct {
            char* hostname;
            int   port;
        } udp_client;
        struct {
            int port;
        } tcp_server;
        struct {
            char* hostname;
            int   port;
        } tcp_client;
        struct {
            char* name;
        }         file;
    }            info;
} EndPoint;

/*----------------------------------------------------------------------
|       constants
+---------------------------------------------------------------------*/
#define PUMP_DEFAULT_PACKET_SIZE 16384
#define PUMP_MATX_UNDERFLOW      5000
#define PUMP_MIN_SLEEP           10

/*----------------------------------------------------------------------
|       PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit(void)
{
    fprintf(stderr, 
            "usage: pump [options] <input endpoint> <output endpoint>, \n"
            "where each endpoint is:\n"
            "    udp [client <hostname> <port>]|[server <port>]\n"
            "  or\n"
            "    tcp [client <hostname> <port>]|[server <port>]\n"
            "  or\n"
            "    file [<filename>|" ATX_FILE_STANDARD_INPUT "|" ATX_FILE_STANDARD_OUTPUT "|" ATX_FILE_STANDARD_ERROR "\n"
            "\n"
            "options are:\n"
            "  -p <packet_size>: send by burst of <packet_size> (default: %d)"
            "\n"
            " and\n"
            "  -b <bitrate>: send at the specified bitrate (default: as "
            "fast as possible)\n"
            "\n", PUMP_DEFAULT_PACKET_SIZE);
    exit(1);
}

/*----------------------------------------------------------------------
|       ConnectClient
+---------------------------------------------------------------------*/
static ATX_Result
ConnectClient(ATX_Socket*        client, 
              char*              hostname, 
              ATX_IpPort         port,
              ATX_InputStream**  input_stream,
              ATX_OutputStream** output_stream)
{
    ATX_Result result = ATX_SUCCESS;
    
    fprintf(stderr, ":: connecting to %s on port %d\n", hostname, port);

    /* connect client */
    result = ATX_Socket_ConnectToHost(client, hostname, port, 10000);
    if (result != ATX_SUCCESS) {
        fprintf(stderr, 
                "ERROR: connection failed (%d)\n", 
                result);
        goto done;
    }
    fprintf(stderr, ":: connected\n");

    /* get socket input stream */
    if (input_stream != NULL) {
        result = ATX_Socket_GetInputStream(client, input_stream);
        if (result != ATX_SUCCESS) {
            fprintf(stderr, "ERROR: cannot get socket input stream\n");
            goto done;
        }
    }

    /* get socket output stream */
    if (output_stream != NULL) {
        result = ATX_Socket_GetOutputStream(client, output_stream);
        if (result != ATX_SUCCESS) {
            fprintf(stderr, "ERROR: cannot get socket output stream\n");
            goto done;
        }
    }

done:
    /* destroy the client */
    ATX_DESTROY_OBJECT(client);

    return result;
}

/*----------------------------------------------------------------------
|       GetEndPoinStreams
+---------------------------------------------------------------------*/
static ATX_Result
GetEndPointStreams(EndPoint*          endpoint, 
                   ATX_InputStream**  input_stream,
                   ATX_OutputStream** output_stream)
{
    ATX_Result result;

    switch (endpoint->type) {
      case ENDPOINT_TYPE_UDP_CLIENT:
        {
            ATX_DatagramSocket* client;
            ATX_Socket*         socket;

            /* create socket */
            result = ATX_UdpSocket_Create(&client);
            if (result) return result;

            /* cast to ATX_Socket interface */
            socket = ATX_CAST(client, ATX_Socket);
            if (result != ATX_SUCCESS) return result;

            /* connect socket */
            return ConnectClient(socket, 
                                 endpoint->info.udp_client.hostname,
                                 endpoint->info.udp_client.port,
                                 input_stream,
                                 output_stream);
        }
        break;

      case ENDPOINT_TYPE_TCP_CLIENT:
        {
            ATX_Socket* client;

            /* create socket */
            result = ATX_TcpClientSocket_Create(&client);
            if (result) return result;

            /* connect socket */
            return ConnectClient(client, 
                                 endpoint->info.udp_client.hostname,
                                 endpoint->info.udp_client.port,
                                 input_stream,
                                 output_stream);
        }
        break;

      case ENDPOINT_TYPE_UDP_SERVER:
        {
            ATX_DatagramSocket* server;
            ATX_Socket*         socket;
            ATX_SocketAddress   address;

            /* create socket */
            result = ATX_UdpSocket_Create(&server);
            if (result) return result;

            /* cast to ATX_Socket interface */
            socket = ATX_CAST(server, ATX_Socket);
            if (result != ATX_SUCCESS) return result;

            /* listen on port */
            fprintf(stderr, ":: listening on port %d\n", 
                    endpoint->info.udp_server.port);
            ATX_SocketAddress_Set(&address,
                                  NULL,
                                  endpoint->info.udp_server.port);
            ATX_Socket_Bind(socket, &address);

            /* get the input stream */
            if (input_stream) {
                ATX_Socket_GetInputStream(socket, input_stream);
            }
        }
        break;

      case ENDPOINT_TYPE_TCP_SERVER:
        {
            ATX_ServerSocket*  server;
            ATX_Socket*        socket;
            ATX_Socket*        client;
            ATX_SocketAddress  address;

            /* create socket */
            result = ATX_TcpServerSocket_Create(&server);
            if (result) return result;

            /* cast to ATX_Socket interface */
            socket = ATX_CAST(server, ATX_Socket);
            if (result != ATX_SUCCESS) return result;

            /* bind to local address */
            ATX_SocketAddress_Set(&address,
                                  NULL,
                                  endpoint->info.tcp_server.port);
            ATX_Socket_Bind(socket, &address);

            /* wait for client */
            fprintf(stderr, ":: waiting for client on port %d...\n", 
                    endpoint->info.tcp_server.port);
            result = ATX_ServerSocket_WaitForNewClient(server, &client);
            if (result != ATX_SUCCESS) {
                fprintf(stderr, 
                        "ERROR: cannot wait for client (%d)\n", 
                        result);
                return result;
            }
            fprintf(stderr, ":: client connected\n");

            /* get the streams */
            if (input_stream) {
                ATX_Socket_GetInputStream(client, input_stream);
            }
            if (output_stream) {
                ATX_Socket_GetOutputStream(client, output_stream);
            }
        }
        break;

      case ENDPOINT_TYPE_FILE:
        {
            /* create a file object */
            ATX_File* file;
            result = ATX_File_Create(endpoint->info.file.name, &file);
            if (ATX_FAILED(result)) {
                fprintf(stderr, "ERROR: cannot create file object (%d)\n", result);
                return result;
            }

            /* open the file */
            if (endpoint->direction == ENDPOINT_DIRECTION_IN) {
                result = ATX_File_Open(
                    file,
                    ATX_FILE_OPEN_MODE_READ |
                    ATX_FILE_OPEN_MODE_UNBUFFERED);
            } else {
                result = ATX_File_Open(
                    file,
                    ATX_FILE_OPEN_MODE_WRITE     |
                    ATX_FILE_OPEN_MODE_CREATE    |
                    ATX_FILE_OPEN_MODE_TRUNCATE  |
                    ATX_FILE_OPEN_MODE_UNBUFFERED);
            }
            if (result != ATX_SUCCESS) {
                fprintf(stderr, "ERROR: cannot open file (%d)\n", 
                        result);
                return result;
            }

            /* get the streams */
            if (input_stream) {
                result = ATX_File_GetInputStream(file, input_stream);
                if (result != ATX_SUCCESS) {
                    fprintf(stderr, "ERROR: cannot get file input stream\n");
                    ATX_DESTROY_OBJECT(file);
                    return result;
                }
            }
            if (output_stream) {
                result = ATX_File_GetOutputStream(file, output_stream);
                if (result != ATX_SUCCESS) {
                    fprintf(stderr, "ERROR: cannot get file output stream\n");
                    ATX_DESTROY_OBJECT(file);
                    return result;
                }
            }
            ATX_DESTROY_OBJECT(file);
        }
        break;
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    char*        arg;
    EndPoint     in_endpoint;
    EndPoint     out_endpoint;
    EndPoint*    current_endpoint = &in_endpoint;
    unsigned int packet_size = PUMP_DEFAULT_PACKET_SIZE;
    unsigned int bitrate = 0;

    if (argc < 2) {
        PrintUsageAndExit();
    }

    /* parse command line */
    in_endpoint.direction = ENDPOINT_DIRECTION_IN;
    out_endpoint.direction = ENDPOINT_DIRECTION_OUT;
    argv++;

    while ((arg = *argv++)) {
        if (current_endpoint == NULL) {
            fprintf(stderr, "ERROR: unexpected argument (%s)\n", arg);
            exit(1);    
        }
                 
        if (!strcmp(arg, "-p")) {
            packet_size = strtoul(*argv++, NULL, 10);
            continue;
        } else if (!strcmp(arg, "-b")) {
            bitrate = strtoul(*argv++, NULL, 10);
            continue;
        } else if (!strcmp(arg, "udp")) {
            if (argv[0] && argv[1]) {
                if (!strcmp(argv[0], "server")) {
                    if (current_endpoint->direction == ENDPOINT_DIRECTION_OUT){
                        fprintf(stderr, 
                                "ERROR: cannot use 'udp server' as output\n");
                        exit(1);
                    }
                    current_endpoint->type = ENDPOINT_TYPE_UDP_SERVER;
                    current_endpoint->info.udp_server.port = 
                        strtoul(argv[1], NULL, 10);
                    argv += 2;
                } else if (!strcmp(argv[0], "client")) {
                    if (current_endpoint->direction == ENDPOINT_DIRECTION_IN) {
                        fprintf(stderr, 
                                "ERROR: cannot use 'udp client' as input\n");
                        exit(1);
                    }
                    if (argv[2]) {
                        current_endpoint->type = ENDPOINT_TYPE_UDP_CLIENT;
                        current_endpoint->info.udp_client.hostname = argv[1];
                        current_endpoint->info.udp_client.port = 
                            strtoul(argv[2], NULL, 10);
                        argv += 3;                        
                    } else {
                        fprintf(stderr, 
                                "ERROR: missing argument for 'udp client'\n");
                        exit(1);
                    }
                }
            } else {
                fprintf(stderr, 
                        "ERROR: missing argument for 'udp' endpoint\n");
                exit(1);
            }
        } else if (!strcmp(arg, "tcp")) {
            if (argv[0] && argv[1]) {
                if (!strcmp(argv[0], "server")) {
                    current_endpoint->type = ENDPOINT_TYPE_TCP_SERVER;
                    current_endpoint->info.tcp_server.port = 
                        strtoul(argv[1], NULL, 10);
                    argv += 2;
                } else if (!strcmp(argv[0], "client")) {
                    if (argv[2]) {
                        current_endpoint->type = ENDPOINT_TYPE_TCP_CLIENT;
                        current_endpoint->info.tcp_client.hostname = argv[1];
                        current_endpoint->info.tcp_client.port = 
                            strtoul(argv[2], NULL, 10);
                        argv += 3;                        
                    } else {
                        fprintf(stderr, 
                                "ERROR: missing argument for 'tcp client'\n");
                        exit(1);
                    }
                }
            } else {
                fprintf(stderr, 
                        "ERROR: missing argument for 'tcp' endpoint\n");
                exit(1);
            }
        } else if (!strcmp(arg, "file")) {
            if (argv[0]) {
                current_endpoint->type = ENDPOINT_TYPE_FILE;
                current_endpoint->info.file.name = *argv++;
            } else {
                fprintf(stderr, 
                        "ERROR: missing argument for 'file' endpoint\n");
                exit(1);
            }
        } else {
            fprintf(stderr, "ERROR: invalid argument (%s)\n", arg);
            exit(1);
        }

        if (current_endpoint == &in_endpoint) {
            current_endpoint = &out_endpoint;
        } else {
            current_endpoint = NULL;
        }
    }

    if (current_endpoint) {
        fprintf(stderr, "ERROR: missing endpoint specification\n");
        exit(1);
    }

    /* data pump */
    {
        ATX_InputStream*  in;
        ATX_OutputStream* out;
        unsigned char*    buffer;
        ATX_Result        result;
        unsigned long     offset = 0;

        /* allocate buffer */
        buffer = (unsigned char*)malloc(packet_size);
        if (buffer == NULL) {
            fprintf(stderr, "ERROR: out of memory\n");
            exit(1);
        }

        /* get input stream */
        result = GetEndPointStreams(&in_endpoint, &in, NULL);
        if (result) {
            fprintf(stderr, "ERROR: failed to get stream for input (%d)\n",
                    result);
            exit(1);
        }

        /* get output stream */
        result = GetEndPointStreams(&out_endpoint, NULL, &out);
        if (result) {
            fprintf(stderr, "ERROR: failed to get stream for output (%d)\n",
                    result);
            exit(1);
        }

        /* measure the current time */
        /*if (bitrate) gettimeofday(&start, NULL);*/

        /* loop */
        do {
            ATX_Size bytes_read;
            ATX_Size bytes_written;

#if 0
            if (bitrate) {
                float when_to_send;
                float now_ms;
                float diff;

                /* measure the current time */
                gettimeofday(&now, NULL);
                now.tv_sec -= start.tv_sec;
                if (now.tv_usec > start.tv_usec) {
                    now.tv_usec -= start.tv_usec;
                } else {
                    now.tv_sec--;
                    now.tv_usec = now.tv_usec+1000000-start.tv_usec;
                }

                /* wait until it's time to send */
                when_to_send = (8000.0f*(float)offset)/(float)bitrate;
                now_ms = 1000.0f*(float)now.tv_sec + (float)(now.tv_usec/1000);
                diff = when_to_send - now_ms;
                printf("now=%f, when=%f, diff=%f\n", 
                       now_ms, when_to_send, diff);

                /* check the time */
                if (diff < 0.0f) {
                    if (-diff > PUMP_MAX_UNDERFLOW) {
                        /* restart from here */
                        fprintf(stderr, 
                                "WARNING: sending too slow, "
                                "reseting reference\n");
                        gettimeofday(&start, NULL);
                        offset = 0;
                    }
                } else if (diff > PUMP_MIN_SLEEP) {
                    struct timespec delay;
                    delay.tv_sec = (diff/1000.0f);
                    delay.tv_nsec = 1000000 * 
                        (diff - (1000.0f*(float)delay.tv_sec));
                    printf("sleeping for %ld.%09ld\n", 
                           delay.tv_sec, delay.tv_nsec);
                    nanosleep(&delay, NULL);
                }
            }
#endif

            /* send */
            result = ATX_InputStream_Read(in, buffer, packet_size, &bytes_read);
            fprintf(stderr, "[%d] read %d bytes\n", result, bytes_read);
            if (result == ATX_SUCCESS && bytes_read) {
                result = ATX_OutputStream_Write(out, buffer, bytes_read,
                                                &bytes_written);
                fprintf(stderr, "[%d] wrote %d bytes\n", 
                        result, bytes_written);
                offset += bytes_written;
            } else {
                printf("*******************\n");
                exit(1);
            }
        } while (result == ATX_SUCCESS);
    }
    
    return 0;
}




