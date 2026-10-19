#pragma once
#include "dusk/config_var.hpp"

namespace dawnlight::survivors {
// Own only an override we installed. Stored preferences are never written.
// All Cheats-tab defaults are zero/false, including both enum selectors.
template<class T> struct CheatGuard {
    dusk::config::ConfigVar<T>* var = nullptr;
    bool owned = false;
    bool restore_override = false;
    T previous{};
    void enforce() {
        using Layer = dusk::config::ConfigVarLayer;
        if (!var) return;
        // A UI edit replaces our override. Keep that newly saved preference;
        // suppress its effect again synchronously through the change subscription.
        if (owned && (var->getLayer()!=Layer::Override || var->getValue()!=T{})) owned=false;
        if (owned || var->getValue()==T{}) return;
        previous=var->getValue();
        restore_override=var->getLayer()==Layer::Override;
        owned=true;
        var->setOverrideValue(T{});
    }
    void release(bool speedrun) {
        using Layer = dusk::config::ConfigVarLayer;
        const bool ours=owned && var && var->getLayer()==Layer::Override && var->getValue()==T{};
        owned=false;
        if (!ours) return;
        if (restore_override) var->setOverrideValue(previous);
        else {
            var->clearOverride();
            // Native speedrun policy remains in force after our run ends.
            if (speedrun) var->setSpeedrunValue(T{});
        }
    }
};
}
