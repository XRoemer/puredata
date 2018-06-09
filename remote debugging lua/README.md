# Remote Debugging of Lua for pd

This description and files are working in Purr Data only. pd vanilla uses another version of pdlua, where the paths and the lua version seemes to be different


For debugging ZeroBrane Studio is used: https://studio.zerobrane.com/

Copy mobdebug (version 0.628) and socket from the ZeroBrane folder ZeroBraneStudioEduPack-1.70-win32\bin\clibs53 into your project path. 

The wrapper is used for reloading the file test.lua, so changes can be reflected on the fly. For loading and reloading of test.lua use the message dofile (complete/path/to/test.lua)

On executing the other messages, classes and functions are called and remote debugging should be started.

The watch window of ZeroBrane doesn't show any entries in my case

Any comments of improving this method are welcome
