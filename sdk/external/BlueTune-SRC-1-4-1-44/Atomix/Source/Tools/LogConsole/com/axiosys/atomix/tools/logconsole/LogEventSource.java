package com.axiosys.atomix.tools.logconsole;

public interface LogEventSource {
    public void addListener(LogEventSourceListener listener);
    public void start();
}
