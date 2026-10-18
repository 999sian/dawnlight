void draw_chest_prices() {
    if (s_run.phase!=Phase::Running || dComIfGp_event_runCheck() || !dComIfGd_getView()) return;
    char price[24]; std::snprintf(price,sizeof(price),"%u",s_run.chest_cost());
    for (unsigned i=0;i<kPaidChestCount;++i) {
        const auto& chest=s_chests[i];
        if (!chest.active || chest.opening) continue;
        cXyz world=chest.position+cXyz(0,150+8*cM_ssin(s16(s_run.ticks*700)),0);
        const auto& matrix=*dComIfGd_getProjViewMtx();
        const float w=matrix[3][0]*world.x+matrix[3][1]*world.y+matrix[3][2]*world.z+matrix[3][3];
        if (w<=1) continue;
        cXyz screen; mDoLib_project(&world,&screen);
        if (!std::isfinite(screen.x) || !std::isfinite(screen.y) ||
            screen.x<mDoGph_gInf_c::getMinXF() || screen.x>mDoGph_gInf_c::getMaxXF() ||
            screen.y<0 || screen.y>mDoGph_gInf_c::getHeightF()) continue;
        label(price,screen.x,screen.y,std::clamp(18000.f/w,12.f,22.f));
    }
}
