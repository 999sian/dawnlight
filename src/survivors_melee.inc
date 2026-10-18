// Native sword targeting already uses Reach. Extend the actual swept sword
// capsules too, rebuilding from the native blade points on every active frame.
// Leave radius, hit flags, attack windows and the real model/animation intact.
void after_sword_collision(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!link || !playable() || s_run.phase!=Phase::Running || fopAcM_GetID(link)!=s_player ||
       s_swordAction!=SwordAction::Normal || !normal_sword_proc(link) || !s_run.rank(Upgrade::Reach))return;
    const float scale=s_run.radius(link->field_0x33d0);
    const auto tip=[&](const cXyz& base,const cXyz& native) {
        cXyz end=base+(native-base)*scale;
        dBgS_LinChk wall;wall.Set(&base,&end,link);
        if(dComIfG_Bgsp().LineCross(&wall))end=wall.GetCross();
        return end;
    };
    const cXyz current=tip(link->field_0x3498,link->mSwordTopPos);
    const cXyz previous=tip(link->field_0x34bc,link->field_0x34b0);
    link->mAtCps[0].SetStartEnd(link->field_0x3498,current);
    link->mAtCps[1].SetStartEnd(previous,current);
    link->mAtCps[2].SetStartEnd(link->field_0x3498,previous);
}
