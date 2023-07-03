#include "hidePath.h"

void showUsage()
{
    std::cout << "PathHiderConfig.exe -hide [path] or PathHiderConfig.exe -unhide" << "/n";
    std::cout << "ex PathHiderConfig.exe -hide C:\\Test\\test.txt" << "/n";
}


int wmain(int argc, wchar_t* argv[])
{
    try
    {
        if (argc == 2)
        {
            if (wcscmp(argv[1], L"-unhide") != 0)
            {
                showUsage();
            }
            else
            {
                UnhideAll();
            }
        }
        else if(argc == 3)
        {
            if (wcscmp(argv[1], L"-hide") != 0)
            {
                showUsage();
            }
            else
            {
                HidePath(argv[2]);
            }
        }
        else
        {
            showUsage();
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}