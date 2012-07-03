//
//  main.m
//  CocoaPlayer
//
//  Created by Gilles on 9/6/08.
//  Copyright Gilles Boccon-Gibod 2008. All rights reserved.
//

#import <UIKit/UIKit.h>
#include "Neptune.h"
#include "BtPlayerServer.h"

class PlayerServerThread : public NPT_Thread
{
public:
    PlayerServerThread(unsigned int port) : NPT_Thread(true), m_Port(port) {}
    
    // NPT_Runnable methods
    virtual void Run() {
        NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
        // create the server
        NPT_String bundle_root = [[[NSBundle mainBundle] bundlePath] UTF8String];
        BtPlayerServer* server = new BtPlayerServer(bundle_root+"/WebRoot");

        // loop until a termination request arrives
        server->Loop();
        
        // delete the controller
        delete server;
        
        [pool release];
    }
    
    unsigned int m_Port;
};

int main(int argc, char *argv[])
{
    (new PlayerServerThread(9080))->Start();
    
    return UIApplicationMain(argc, argv, nil, nil);
}
