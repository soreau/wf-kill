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


#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include "wf-kill.hpp"

static void registry_add(void *data, struct wl_registry *registry,
    uint32_t id, const char *interface,
    uint32_t version)
{
    WfKill *wfk = (WfKill *) data;

    if (strcmp(interface, wf_kill_view_base_interface.name) == 0)
    {
        wfk->wf_kill_view_manager = (wf_kill_view_base *)
            wl_registry_bind(registry, id,
            &wf_kill_view_base_interface, 1);
    }
}

static void registry_remove(void *data, struct wl_registry *registry,
    uint32_t id)
{
    exit(0);
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_add,
    .global_remove = registry_remove,
};

static void receive_view_pid(void *data,
    struct wf_kill_view_base *wf_kill_view_base,
    const int view_pid)
{
    WfKill *wfk = (WfKill *) data;

    if (view_pid > 0)
    {
        if (wfk->send_kill)
        {
            kill(view_pid, SIGKILL);
            std::cout << "Sent SIGKILL to PID: " << view_pid << std::endl;
            wfk->running = 0;
            return;
        }
        std::cout << "Client PID: " << view_pid << std::endl;
    } else
    {
        std::cout << "Client PID: UNKNOWN" << std::endl;
    }
    wfk->running = 0;
}

static void cancel(void *data,
    struct wf_kill_view_base *wf_kill_view_base)
{
    exit(0);
}

static struct wf_kill_view_base_listener kill_view_listener {
	.view_pid = receive_view_pid,
	.cancel = cancel,
};

WfKill::WfKill(int argc, char *argv[])
{
    display = wl_display_connect(NULL);
    if (!display)
    {
        return;
    }

    wl_registry *registry = wl_display_get_registry(display);
    if (!registry)
    {
        return;
    }

    wl_registry_add_listener(registry, &registry_listener, this);

    wf_kill_view_manager = NULL;
    wl_display_roundtrip(display);
    wl_registry_destroy(registry);
    if (!wf_kill_view_manager)
    {
        std::cout << "Wayfire kill view protocol not advertised by compositor. Is wf-kill plugin enabled?" << std::endl;
        return;
    }
    wf_kill_view_base_view_kill(wf_kill_view_manager);
    wf_kill_view_base_add_listener(wf_kill_view_manager,
        &kill_view_listener, this);

    send_kill = (argc > 1 && !strcmp(argv[1], "-k"));
    running = 1;
    while(running)
        wl_display_dispatch(display);

    wl_display_flush(display);
    wl_display_disconnect(display);
}

WfKill::~WfKill()
{
}

int main(int argc, char *argv[])
{
    WfKill(argc, argv);

    return 0;
}
