package com.axiosys.atomix.tools.logconsole;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;

public class TcpLogEventSourceFactory extends Thread {
    ServerSocket server;
    ArrayList    listeners = new ArrayList();
    
    public TcpLogEventSourceFactory(int port) throws IOException {
        this.server = new ServerSocket(port);
    }

    public void addListener(LogEventSourceFactoryListener listener) {
        this.listeners.add(listener);
    }
    
    public void run() {
        for (;;) {
            // wait for a client to connect
            Socket client;
            try {
                client = server.accept();
            } catch (IOException e) {
                e.printStackTrace();
                return;
            }
            
            try {
                // create the source
                TcpLogEventSource source = new TcpLogEventSource(client);
                
                // notify all listeners
                for(int i=0; i<listeners.size(); i++) {
                    LogEventSourceFactoryListener listener = (LogEventSourceFactoryListener)listeners.get(i);
                    listener.onEventSourceCreated(source);
                }
            } catch (IOException e) {}
        }
    }    
}
