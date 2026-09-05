#include "pch.h"

#include <list>
#include <regex>
#include <filesystem>

#include "push-widget.h"
#include "plugin-support.h"

#include "output-config.h"

//added for websocket
#include "ws_vendor.hpp"
#include "multi-output-widget.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#define ConfigSection "obs-multi-rtmp"

static class GlobalServiceImpl : public GlobalService
{
public:
    bool RunInUIThread(std::function<void()> task) override {
        if (uiThread_ == nullptr)
            return false;
        QMetaObject::invokeMethod(uiThread_, [func = std::move(task)]() {
            func();
        });
        return true;
    }

    QThread* uiThread_ = nullptr;
} s_service;


GlobalService& GetGlobalService() {
    return s_service;
}

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-multi-rtmp", "en-US")
OBS_MODULE_AUTHOR("雷鳴 (@sorayukinoyume)")

bool obs_module_load()
{
    auto mainwin = (QMainWindow*)obs_frontend_get_main_window();
    if (mainwin == nullptr)
        return false;
    QMetaObject::invokeMethod(mainwin, []() {
        s_service.uiThread_ = QThread::currentThread();
    });

    // ✅ FIXED: Don't initialize vendor here - wait until obs-websocket is loaded
    // Just create the instance but don't initialize yet
    MultiRTMPWebsocketVendor::Instance();

    blog(LOG_INFO, TAG "Websocket vendor instance created (will register when obs-websocket loads)");

    auto dock = new MultiOutputWidget();
    dock->setObjectName("obs-multi-rtmp-dock");
    if (!obs_frontend_add_dock_by_id("obs-multi-rtmp-dock", obs_module_text("Title"), dock))
    {
        delete dock;
        return false;
    }

    blog(LOG_INFO, TAG "version: %s by SoraYuki https://github.com/sorayuki/obs-multi-rtmp/", PLUGIN_VERSION);

    // ✅ ADD THIS: Listen for module load events to detect when obs-websocket loads
    obs_frontend_add_event_callback(
        [](enum obs_frontend_event event, void *private_data) {
            auto dock = static_cast<MultiOutputWidget*>(private_data);

            for(auto x: dock->GetAllPushWidgets())
                x->OnOBSEvent(event);
            dock->OnOBSEvent(event);

            if (event == obs_frontend_event::OBS_FRONTEND_EVENT_EXIT)
            {   
                dock->SaveConfig();
            }
            else if (event == obs_frontend_event::OBS_FRONTEND_EVENT_PROFILE_CHANGED)
            {
                dock->LoadConfig();
            }
            // ✅ ADD THIS: Check if obs-websocket is now loaded
            else if (event == obs_frontend_event::OBS_FRONTEND_EVENT_FINISHED_LOADING)
            {
                // Try to initialize vendor after all modules are loaded
                MultiRTMPWebsocketVendor::Instance()->Initialize();
            }
        }, dock
    );

    return true;
}

void obs_module_unload(void)
{
#ifdef ENABLE_WEBSOCKET
    MultiRTMPWebsocketVendor::Instance()->Shutdown();
#endif
}

const char *obs_module_description(void)
{
    return "Multiple RTMP Output Plugin";
}
