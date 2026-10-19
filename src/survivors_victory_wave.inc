// Native Fyrus attack models, sampled once per simulation tick. This visual
// deliberately has no collision object and never calls the damage pipeline.
struct VictoryFireModel {
    J3DModel* model=nullptr;
    mDoExt_bckAnm bck;
    mDoExt_btkAnm btk;
    mDoExt_brkAnm brk;
    bool colored=false;
    bool create(unsigned index) {
        constexpr int models[]={dRes_INDEX_E_FM_BMD_EF_FMATTACK_A_e,dRes_INDEX_E_FM_BMD_EF_FMATTACK_B_e};
        constexpr int motions[]={dRes_INDEX_E_FM_BCK_EF_FMATTACK_A_e,dRes_INDEX_E_FM_BCK_EF_FMATTACK_B_e};
        constexpr int textures[]={dRes_INDEX_E_FM_BTK_EF_FMATTACK_A_e,dRes_INDEX_E_FM_BTK_EF_FMATTACK_B_e};
        auto* data=static_cast<J3DModelData*>(dComIfG_getObjectRes("E_fm",models[index]));
        auto* motion=static_cast<J3DAnmTransform*>(dComIfG_getObjectRes("E_fm",motions[index]));
        auto* texture=static_cast<J3DAnmTextureSRTKey*>(dComIfG_getObjectRes("E_fm",textures[index]));
        if (!data || !motion || !texture) return false;
        model=mDoExt_J3DModel__create(data,0x80000,0x11000284);
        if (!model || !bck.init(motion,TRUE,J3DFrameCtrl::EMode_NONE,1,0,-1,false) ||
            !btk.init(data,texture,TRUE,J3DFrameCtrl::EMode_LOOP,1,0,-1)) return false;
        colored=index==1;
        if (!colored) return true;
        auto* color=static_cast<J3DAnmTevRegKey*>(dComIfG_getObjectRes("E_fm",dRes_INDEX_E_FM_BRK_EF_FMATTACK_B_e));
        return color && brk.init(data,color,TRUE,J3DFrameCtrl::EMode_LOOP,1,0,-1);
    }
    void reset() { bck.setFrame(0); btk.setFrame(0); if (colored) brk.setFrame(0); }
    void sample(float frame) {
        bck.setFrame(std::min(frame,bck.getEndFrame()));
        btk.play(); if (colored) brk.play();
    }
    void draw(const cXyz& origin,float scale,dKy_tevstr_c& light) {
        mDoMtx_stack_c::transS(origin.x,origin.y+3,origin.z);
        // Keep the original flame height while expanding across the arena.
        mDoMtx_stack_c::scaleM(scale,2.5f,scale);
        model->setBaseTRMtx(mDoMtx_stack_c::get());
        g_env_light.setLightTevColorType_MAJI(model,&light);
        auto* data=model->getModelData();
        bck.entry(data); btk.entry(data); if (colored) brk.entry(data);
        mDoExt_modelUpdateDL(model);
        bck.remove(data); btk.remove(data); if (colored) brk.remove(data);
    }
};

struct VictoryWave {
    request_of_phase_process_class archive{};
    std::array<VictoryFireModel,2> fire;
    JKRSolidHeap* heap=nullptr;
    bool requested=false, ready=false, failed=false;
    cXyz origin{};
    float radius=1500, scale=2.5f;
    void prepare() {
        if (ready || failed) return;
        requested=true;
        const int phase=dComIfG_resLoad(&archive,"E_fm");
        if (phase!=cPhs_COMPLEATE_e) { failed=phase==cPhs_ERROR_e; return; }
        heap=mDoExt_createSolidHeapFromGame(0x40000,0x20);
        if (heap) {
            auto* previous=mDoExt_setCurrentHeap(heap);
            ready=fire[0].create(0) && fire[1].create(1);
            mDoExt_setCurrentHeap(previous);
            if (ready) mDoExt_adjustSolidHeap(heap);
            else { mDoExt_destroySolidHeap(heap); heap=nullptr; }
        }
        failed=!ready;
        if (failed) svc_log->warn(mod_ctx,"Survivors: could not allocate native Fyrus victory wave");
    }
    void release() {
        if (heap) mDoExt_destroySolidHeap(heap);
        heap=nullptr;
        for (auto& part:fire) part.model=nullptr;
        if (requested) dComIfG_resDelete(&archive,"E_fm");
        archive={}; requested=ready=failed=false;
    }
    void sample() {
        if (!ready) return;
        const float progress=std::min(1.f,float(s_victoryTicks)/60.f);
        const float fade=float(s_victoryTicks>60 ? s_victoryTicks-60 : 0);
        scale=2.5f*radius/(31.5f*std::max(1.f,std::min(47.5f,fire[1].bck.getEndFrame())));
        fire[0].sample(42*progress+fade);
        fire[1].sample(std::min(47.5f,fire[1].bck.getEndFrame())*progress+fade*.5f);
    }
} s_victoryWave;
void prepare_victory_wave() { s_victoryWave.prepare(); }
void draw_victory_wave() {
    if (!s_victory || !s_victoryWave.ready || s_victoryTicks>100) return;
    auto* link=daAlink_getAlinkActorClass();
    if (!link) return;
    for (auto& part:s_victoryWave.fire) part.draw(s_victoryWave.origin,s_victoryWave.scale,link->tevStr);
}
