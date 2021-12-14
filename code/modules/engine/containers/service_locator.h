#pragma once

namespace rebel_road
{

    template <typename service_class>
    class service_locator
    {
    public:
        static service_class* get() { return service; }
        static void provide( service_class* serv ) { service = serv; }
    private:
        static service_class* service;
    };

    template <typename service_class>
    class service_locator_by_value
    {
    public:
        static service_class get() { return service; }
        static void provide( service_class* serv ) { service = serv; }
    private:
        static service_class service;
    };

}