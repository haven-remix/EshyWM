
// #pragma once

// #include "window.h"

// #include <vector>

// struct window_list_container
// {
// public:

//     /**@return was the element put in*/
//     bool push_back(EshyWMWindow* new_element);

//     /**Checks for EshyWMWindow*/
//     EshyWMWindow* contains(EshyWMWindow* test);

//     /**Checks for Window*/
//     Window contains_window(Window test);

// private:

//     std::vector<EshyWMWindow*> window_list_internal;
// };

// bool window_list_container::push_back(EshyWMWindow* new_element)
// {
//     if(!contains(new_element))
//     {
//         window_list_internal.push_back(new_element);
//         return true;
//     }

//     return false;
// }

// EshyWMWindow* window_list_container::contains(EshyWMWindow* test)
// {
//     return nullptr;
// }

// Window window_list_container::contains_window(Window test)
// {
//     Window final = 0L;

//     for(EshyWMWindow* x : window_list_internal)
//     {
//         if(x->get_window() == test)
//         {
//             final = test;
//             break;
//         }
//     }

//     return final;
// }