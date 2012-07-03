package com.axiosys.atomix.tools.logconsole;

import java.util.Date;
import java.util.Map;

/**
 * @author Gilles
 */
public class LogEvent {
    private String  logger;
    private String  level;
    private String  message;
    private String  sourceFile;
    private String  sourceLine;
    private Date    timeStamp;
    
    public LogEvent(Map headers, char[] message) {
        this.message = new String(message);
        this.logger = (String)headers.get("Logger");
        this.level  = (String)headers.get("Level");
        this.sourceFile = (String)headers.get("Source-File");
        this.sourceLine = (String)headers.get("Source-Line");
        
        // parse the timestamp
        String ts = (String)headers.get("TimeStamp");
        if (ts != null) {
            String[] parts = ts.split(":");
            if (parts != null && parts.length == 2) {
                long seconds = Long.parseLong(parts[0]);
                long milliseconds = Long.parseLong(parts[1]);
                this.timeStamp = new Date(seconds*1000L+milliseconds);
            }            
        }
    }

    public String toString() {
        return "["+logger+"] " + sourceFile + ":" + sourceLine + ", " + timeStamp + ", " + level + ": " + message;  
    }
}
