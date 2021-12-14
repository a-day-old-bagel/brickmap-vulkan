#pragma once

#include "apps/app.h"

#ifdef _WIN32
    #undef APIENTRY
    #include <windows.h>

    #define application( app_type ) \
    \
    \
    int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR lpCmdLine, int nShowCmd ) \
    { \
    \
        rebel_road::apps::app_type app; \
        app.main_loop(); \
    \
    \
        return EXIT_SUCCESS; \
    }
#else
    #define application( app_type ) \
    \
    \
    void main() \
    { \
        rebel_road::apps::app_type app; \ 
        app.main_loop(); \
    }
#endif
