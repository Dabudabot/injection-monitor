# Injector monitor library

### imlib.lib

In order to simplify collection of records this library is created. It connects to communication port using FilterConnectCommunicationPort. Create separate thread and in infinite loop start to call FilterSendMessage to driver. Only errors and deinitialisation of library may stop this loop. Library provides communication with driver to collect logs. Library requires callback which is triggered every time when record is recieved from driver.

---------------------------------------
Tested on Windows 7 x64, builds for Windows 7 x86 x64, Windows 10 x86 x64