// Move the host's actual sword image: its texture follows equipment changes.
// Restore before each host display pass and before destroying the document.
NativeTouch* s_swordTouchOwner=nullptr;
Rml::ElementPtr s_swordTouchLabel;
bool s_swordTouchHadItem=false;
void restore_survivors_touch_sword() {
    if(!s_swordTouchOwner)return;
    auto& b=s_swordTouchOwner->mControlElements[size_t(dusk::ui::Control::B)];
    auto& l=s_swordTouchOwner->mControlElements[size_t(dusk::ui::Control::L)];
    if(b.icon&&s_touchApi.parent(b.icon)==l.root) {
        auto icon=s_touchApi.remove(l.root,b.icon);
        if(icon)s_touchApi.append(b.root,std::move(icon),true);
    }
    const std::string hasItem="has-item";
    s_touchApi.setClass(b.root,&hasItem,s_swordTouchHadItem);
    if(s_swordTouchLabel)s_touchApi.append(l.root,std::move(s_swordTouchLabel),true);
    s_swordTouchOwner=nullptr;s_swordTouchLabel=nullptr;
}
void move_survivors_touch_sword(NativeTouch* controls) {
    restore_survivors_touch_sword();
    if(!s_extraHooksReady||!controls||controls->mWasSuppressed||!survivors_switch_attack_lock())return;
    auto& b=controls->mControlElements[size_t(dusk::ui::Control::B)];
    auto& l=controls->mControlElements[size_t(dusk::ui::Control::L)];
    const std::string hidden="hidden",visible="visible",hasItem="has-item";
    if(!b.root||!l.root||!b.icon||s_touchApi.parent(b.icon)!=b.root||
       s_touchApi.isPseudoClassSet(l.root,&hidden)||!s_touchApi.isClassSet(b.icon,&visible))return;
    auto icon=s_touchApi.remove(b.root,b.icon);if(!icon)return;
    // Only button roots have a :hidden rule in the host stylesheet. Detach
    // the L text while its sword is shown, retaining all native label styling.
    if(auto* label=s_touchApi.child(l.root,0))s_swordTouchLabel=s_touchApi.remove(l.root,label);
    s_swordTouchHadItem=s_touchApi.isClassSet(b.root,&hasItem);
    s_touchApi.setClass(b.root,&hasItem,false);
    s_touchApi.append(l.root,std::move(icon),true);s_swordTouchOwner=controls;
}
