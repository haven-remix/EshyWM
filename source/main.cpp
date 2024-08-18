
#include "eshywm.h"

#include <cstring>

int main(int argc, char** argv)
{
    SET_GLOBAL_SEVERITY(LS_Warning);

    for(int i = 0; i < argc; i++)
    {
        if(!strcmp(argv[i], "--logging=verbose"))
            SET_GLOBAL_SEVERITY(LS_Verbose)
        else if(!strcmp(argv[i], "--logging=info"))
            SET_GLOBAL_SEVERITY(LS_Info)
        else if(!strcmp(argv[i], "--logging=warning"))
            SET_GLOBAL_SEVERITY(LS_Warning)
        else if(!strcmp(argv[i], "--logging=error"))
            SET_GLOBAL_SEVERITY(LS_Error)
        else if(!strcmp(argv[i], "--logging=fatal"))
            SET_GLOBAL_SEVERITY(LS_Fatal)
    }

    try
    {
        EshyWM::initialize();
    }
    catch(const std::logic_error& e)
    {
        std::cout << "This is an EshyMail error: START " << e.what() << " END " << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cout << "This is an EshyMail error: START " << e.what() << " END " << std::endl;
    }

    return 0;
}
