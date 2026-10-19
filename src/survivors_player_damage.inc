// Use the native no-reaction branch only while processing a real damage hit.
// Native damage, difficulty scaling, hurt sounds and death handling still run.
struct PlayerDamageFrame {
    void* args;
    daAlink_c* link;
    s16 timer;
    bool addedFlag=false;
};
std::vector<PlayerDamageFrame> s_playerDamageFrames;
void begin_player_damage(void* args,daAlink_c* link) {
    if (link && fopAcM_GetID(link)==s_player && playable() && s_run.phase==Phase::Running)
        s_playerDamageFrames.push_back({args,link,link->mDamageTimer});
}
void suppress_player_reaction(daAlink_c* link) {
    if (s_playerDamageFrames.empty() || s_playerDamageFrames.back().link!=link) return;
    if (!link->checkModeFlg(0x4000000)) {
        link->onModeFlg(0x4000000);
        s_playerDamageFrames.back().addedFlag=true;
    }
}
void after_player_damage(ModContext*,void* args,void*,void*) {
    if (s_playerDamageFrames.empty() || s_playerDamageFrames.back().args!=args) return;
    const auto frame=s_playerDamageFrames.back();s_playerDamageFrames.pop_back();
    if (frame.addedFlag) frame.link->offModeFlg(0x4000000);
    // Match No Normal-Hit Invulnerability independently of post-hook order.
    if (frame.link->mDamageTimer>frame.timer) frame.link->mDamageTimer=0;
}
