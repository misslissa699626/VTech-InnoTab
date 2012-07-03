package com.axiosys.atomix.tools.logconsole;

import java.io.IOException;

public class LogConsole implements LogEventSourceFactoryListener, LogEventSourceListener {

    public void onEventSourceCreated(LogEventSource source) {
        System.out.println("++++++++++ new source " + source + "++++++++++");
        source.addListener(this);
        source.start();
    }
    
    public void onLogEventReceived(LogEventSource source, LogEvent event) {
        System.out.println(event);
    }

    public void onEndOfSource(LogEventSource source) {
        System.out.println("---------- end of source " + source + "----------\n");
    }

    public static void main(String[] args) throws IOException {
        TcpLogEventSourceFactory factory = new TcpLogEventSourceFactory(7723);
        LogConsole console = new LogConsole();
        factory.addListener(console);
        new Thread(factory).start();
    }
}
