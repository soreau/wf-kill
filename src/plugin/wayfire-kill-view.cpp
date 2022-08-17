/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Scott Moreau
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <sys/time.h>
#include <wayfire/core.hpp>
#include <wayfire/view.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/output.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/nonstd/wlroots-full.hpp>
#include <linux/input-event-codes.h>

#include "wayfire-kill-view.hpp"
#include "wayfire-kill-view-server-protocol.h"

static void bind_manager(wl_client *client, void *data,
    uint32_t version, uint32_t id);

void wf_kill_view::send_view_info()
{
    wayfire_view view = wf::get_core().get_cursor_focus_view();
    if (!view)
    {
        return;
    }
    pid_t pid = 0;
    wlr_surface *wlr_surface = view->get_wlr_surface();
#if WF_HAS_XWAYLAND
    bool is_xwayland_surface = wlr_surface_is_xwayland_surface(wlr_surface);
    if (is_xwayland_surface)
    {
        pid = wlr_xwayland_surface_from_wlr_surface(wlr_surface)->pid;
    } else
#endif
    {
        wl_client_get_credentials(view->get_client(), &pid, 0, 0);
    }

    view->close();

    for (auto r : client_resources)
    {
        wf_kill_view_base_send_view_pid(r, pid);
    }
}

void wf_kill_view::send_cancel()
{

    for (auto r : client_resources)
    {
        wf_kill_view_base_send_cancel(r);
    }
}

void wf_kill_view::deactivate(uint32_t b)
{
    for (auto& o : wf::get_core().output_layout->get_outputs())
    {
        o->deactivate_plugin(grab_interfaces[o]);
        grab_interfaces[o]->ungrab();
        grab_interfaces[o].reset();
    }

    idle_set_cursor.run_once([=] ()
    {
        if (b == BTN_LEFT)
        {
            send_view_info();
        }
        else
        {
            send_cancel();
        }
    });
}

wf_kill_view::wf_kill_view()
{
    manager = wl_global_create(wf::get_core().display,
        &wf_kill_view_base_interface, 1, this, bind_manager);

    if (!manager)
    {
        LOGE("Failed to create wf_kill_view interface");
        return;
    }
}

wf_kill_view::~wf_kill_view()
{
    wl_global_destroy(manager);

    for (auto& o : wf::get_core().output_layout->get_outputs())
    {
        grab_interfaces[o].reset();
    }
}

static void view_kill(struct wl_client *client, struct wl_resource *resource)
{
    wf_kill_view *wd = (wf_kill_view*)wl_resource_get_user_data(resource);

    bool set_cursor = false;
    for (auto& o : wf::get_core().output_layout->get_outputs())
    {
        wd->grab_interfaces[o] = std::make_unique<wf::plugin_grab_interface_t> (o);
        wd->grab_interfaces[o]->name = "wf-kill";
        wd->grab_interfaces[o]->capabilities = wf::CAPABILITY_GRAB_INPUT;

        if (!o->activate_plugin(wd->grab_interfaces[o]))
        {
            continue;
        }

        wd->grab_interfaces[o]->callbacks.pointer.button = [=] (uint32_t b, uint32_t s)
        {
            if (s == WL_POINTER_BUTTON_STATE_PRESSED)
            {
                wd->deactivate(b);
            }
        };

        wd->grab_interfaces[o]->grab();

        set_cursor = true;
    }

    if (set_cursor)
    {
        wd->idle_set_cursor.run_once([wd] ()
        {
            wf::get_core().set_cursor("pirate");
        });
    }
}

static const struct wf_kill_view_base_interface wf_kill_view_impl =
{
    .view_kill = view_kill,
};

static void destroy_client(wl_resource *resource)
{
    wf_kill_view *wd = (wf_kill_view*)wl_resource_get_user_data(resource);

    for (auto& r : wd->client_resources)
    {
        if (r == resource)
        {
            r = nullptr;
        }
    }
    wd->client_resources.erase(std::remove(wd->client_resources.begin(),
        wd->client_resources.end(), nullptr), wd->client_resources.end());
}

static void bind_manager(wl_client *client, void *data,
    uint32_t version, uint32_t id)
{
    wf_kill_view *wd = (wf_kill_view*)data;

    auto resource =
        wl_resource_create(client, &wf_kill_view_base_interface, 1, id);
    wl_resource_set_implementation(resource,
        &wf_kill_view_impl, data, destroy_client);
    wd->client_resources.push_back(resource);
    
}
