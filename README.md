# PathHider overview
#The project is a filesystem minifilter driver I wrote to learn\refresh kernel mode development for Windows. The main feature - it can "hide" files\folder from a user (they will not be visible in Windows Explorer or similar applications). 
#The mechanism is simple - the minifilter adds callbacks for IRP_MJ_DIRECTORY_CONTROL and modifies the result of this call to "hide" the file objects (remove the data about them results of the lower level call).

#Build
#You need VS 2019 (I use community edition) and WDK 10. 
# - PathHider is the actual driver file system minifilter
# - PathHiderConfig is a command line tool to send commands to hide\unhide files

#Run and debug
#The driver supposed to be used on virtual machine. The machine should be configured for usage of test drivers(this allows to load\use unsigned drivers). 
#I am using Hyper-V as virtualisation tool. So, just copy the output folders from both projects to the machine, install the driver using PathHider.inf. 
#Use "fltmc load pathHider" to load the driver. Now you are ready to use pathHiderConfig to actually "hide" a file, like this:
# pathHiderConfig.exe -hide C:\test\1.txt
#
