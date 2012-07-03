package com.axiosys.atomix.tools.logconsole;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.Socket;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class TcpLogEventSource implements Runnable, LogEventSource {
    private ArrayList      listeners = new ArrayList();
    private BufferedReader input;
    private String         name;
    
    public TcpLogEventSource(Socket client) throws IOException {
        input = new BufferedReader(new InputStreamReader(client.getInputStream(), "UTF-8"));
        name = client.getRemoteSocketAddress().toString();
    }

    public void start() {
        new Thread(this).start();
    }
    
    public void run() {
        // read headers
        Map headers = new HashMap();
        try {
            for (;;) {
                char[] message;
                headers.clear();
                int contentLength = 0;
                for(;;) {
                    String line;
                    line = input.readLine();
                    if (line.length() == 0) break;
                    int separator = line.indexOf(':');
                    if (separator > 0) {
                        String key = line.substring(0, separator);
                        String value = line.substring(separator+1).trim();
                        headers.put(key, value);
                        if (key.equals("Content-Length")) {
                            contentLength = Integer.parseInt(value);
                        }
                    }
                }
                if (contentLength == 0) continue;

                // read the message
                message = new char[contentLength];
                input.read(message);
                
                // we have a complete event
                LogEvent event = new LogEvent(headers, message);
                
                // notify all the listeners
                for (int i=0; i<listeners.size(); i++) {
                    LogEventSourceListener listener = (LogEventSourceListener)listeners.get(i);
                    listener.onLogEventReceived(this, event);
                }
            }            
        } catch (Exception e) {
            // notify all the listeners that the source is dead
            for (int i=0; i<listeners.size(); i++) {
                LogEventSourceListener listener = (LogEventSourceListener)listeners.get(i);
                listener.onEndOfSource(this);
            }
            
            return;
        }
    }

    public void addListener(LogEventSourceListener listener) {
        listeners.add(listener);
    }

    public String toString() {
        return name;
    }
}
