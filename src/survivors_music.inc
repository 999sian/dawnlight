// The intact throne room has no ambient combat cue. Borrow the native final
// battle sequence only during its Survivors run; never stop a replacement cue.
constexpr u32 kSurvivorsThroneMusic=Z2BGM_VS_GANON_04;
bool s_throneMusic=false;
void stop_survivors_music() {
    if(!s_throneMusic)return;
    s_throneMusic=false;
    auto* audio=Z2GetAudioMgr();
    if(audio && audio->getMainBgmID()==kSurvivorsThroneMusic)audio->bgmStop(0,0);
}
void update_survivors_music() {
    auto* link=daAlink_getAlinkActorClass();
    const bool wanted=s_active && s_gameStarted && !s_hub && s_map==4 && in_arena() &&
        !transitioning() && !s_restart && !s_deathPending && link && !link->checkDeadHP() &&
        s_run.phase!=Phase::Ready && s_run.phase!=Phase::Loading && s_run.phase!=Phase::Defeat;
    if(!wanted){stop_survivors_music();return;}
    if(s_throneMusic)return;
    auto* audio=Z2GetAudioMgr();if(!audio)return;
    // Claim the cue only once. Native game-over/scene audio may replace it.
    audio->bgmStart(kSurvivorsThroneMusic,0,0);s_throneMusic=true;
}
