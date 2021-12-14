
#include "app.h"

namespace rebel_road 
{

    namespace apps 
    {

        void app::main_loop()
        {
            while ( current_state != app_state::invalid )
            {
                on_frame();
            }
        }

        void app::request_quit()
        {
            if ( current_state == app_state::running )
            {
                pending_state = app_state::cleanup;
            }
            else
            {
                pending_state = app_state::destroy;
            }
        }

        void app::on_frame()
        {
            if ( pending_state != current_state )
            {
                current_state = pending_state;
            }

            //signals::app_signal::pre_frame_update();

            switch ( current_state )
            {
                case app_state::init:
                    pending_state = on_init();
                    break;
                case app_state::running:
                    pending_state = on_running();
                    break;
                case app_state::cleanup:
                    pending_state = on_cleanup();
                    break;
                case app_state::destroy:
                    pending_state = on_destroy();
                    current_state = app_state::invalid;
                    break;
            }

            //signals::app_signal::post_frame_update();
        }

        app_state app::on_init()
        {
            return app_state::running;
        }

        app_state app::on_running()
        {
            return pending_state;
        }

        app_state app::on_cleanup()
        {
            return app_state::destroy;
        }

        app_state app::on_destroy()
        {
            return app_state::invalid;
        }

    } // namespace apps

} // namespace rebel_road