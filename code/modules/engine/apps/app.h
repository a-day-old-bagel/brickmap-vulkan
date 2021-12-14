#pragma once

#include <memory>

namespace rebel_road
{

    namespace apps
    {

        enum class app_state
        {
            invalid = 0,
            init,
            running,
            cleanup,
            destroy
        };

        class app
        {
        public:
            virtual app_state on_init();
            virtual app_state on_running();
            virtual app_state on_cleanup();
            virtual app_state on_destroy();
            virtual void on_frame();

            void main_loop();
            void request_quit();

        private:
            app_state current_state { app_state::init };
            app_state pending_state { app_state::init };
        };

    } // namespace app

} // namespace rebel_road
