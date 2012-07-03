/*****************************************************************
|
|   BlueTune - Player Web Service
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "BtPlayerServer.h"

/*----------------------------------------------------------------------
|    main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: btplayerserver <port> <web-root> [<media-root>]\n");
        return 1;
    }
    const char* web_root = argv[2];
    
    // parse the port
    int port = 0;
    if (NPT_FAILED(NPT_ParseInteger(argv[1], port, true))) {
        fprintf(stderr, "ERROR: invalid port\n");
        return 1;
    }
    
    // create the server
    BtPlayerServer* server = new BtPlayerServer(web_root, port);

    // loop until a termination request arrives
    server->Loop();
    
    // delete the controller
    delete server;

    return 0;
}
