#pragma once

#include <deque>

namespace rebel_road
{
    namespace util
    {
        class deletion_queue
        {
        public:
            void push_function( std::function<void()>&& function )
            {
                deletors.push_back( function );
            }

            void flush()
            {
                for ( auto it = deletors.rbegin(); it != deletors.rend(); it++ )
                {
                    ( *it )( );
                }

                deletors.clear();
            }

        private:
            std::deque<std::function<void()>> deletors;
        };
    }
}