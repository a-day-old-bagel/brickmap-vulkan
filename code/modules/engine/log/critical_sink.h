#pragma once

#include <spdlog/sinks/base_sink.h>

namespace rebel_road {

    template<typename mutex_type>
    class critical_sink : public spdlog::sinks::base_sink<mutex_type>
    {
    public:
        void sink_it_( const spdlog::details::log_msg& msg ) override
        {
            if ( spdlog::level::critical == msg.level )
            {
                abort();
            }
        }

        void flush_() override {}
    };

    using critical_sink_mt = critical_sink<std::mutex>;

} // namespace rebel_road