package com.axiosys.atomix.tools.logconsole;

public interface LogEventSourceListener {
    public void onLogEventReceived(LogEventSource source, LogEvent event);
    public void onEndOfSource(LogEventSource source);
}
