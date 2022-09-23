
#include "window_data.h"
#include "window.h"

void window_data::update_internal_window_data(window_data* new_window_data)
{
    get_eshywm_window()->set_window_data(new_window_data);
}