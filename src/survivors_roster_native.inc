struct NativeSpecies { ProfileName profile; u32 parameters; s8 argument; float height; unsigned hp; };
constexpr std::array<NativeSpecies,species_count> native_roster{{
    {fpcNm_E_OC_e,0x00ff0000,-1,0,8}, {fpcNm_E_BA_e,0xffff0f01,-1,180,4},
    {fpcNm_E_MS_e,0xffff0000,-1,0,4}, {fpcNm_E_RD_e,0xff000100,-1,0,8},
    {fpcNm_E_RD_e,0xff000200,-1,0,8}, {fpcNm_E_TT_e,0x000000ff,-1,0,8},
    {fpcNm_E_DN_e,0xff000000,-1,0,16}, {fpcNm_E_MM_e,0x0000ff00,-1,0,12},
    {fpcNm_E_MM_e,0x0000ff00,1,0,24}, {fpcNm_E_GE_e,0x00ffff01,-1,180,6},
    {fpcNm_E_KR_e,0xffffff00,-1,220,12}, {fpcNm_E_CR_e,0,-1,0,10},
    {fpcNm_E_BS_e,0x00ffff00,-1,0,8},
    {fpcNm_E_GI_e,0x00ff01ff,-1,0,20}, {fpcNm_E_SF_e,0x0000ff00,-1,0,24},
    {fpcNm_E_FS_e,0,-1,0,8}, {fpcNm_E_BU_e,0xffff0f00,-1,180,6},
    {fpcNm_E_BU_e,0xffff1f00,-1,180,10}, {fpcNm_E_SM2_e,0xffff0060,-1,0,12},
    {fpcNm_E_MF_e,0xff000000,-1,0,24}, {fpcNm_B_GG_e,0xffff0002,-1,300,1200},
    {fpcNm_B_TN_e,0x000001ff,-1,0,1200},
    {fpcNm_E_DD_e,0xffff0000,-1,0,16}, {fpcNm_E_SH_e,0x0014ff00,-1,0,10},
    {fpcNm_E_RDY_e,0xff000107,-1,0,8}, {fpcNm_E_RDY_e,0xff000207,-1,0,8},
    {fpcNm_E_S1_e,0xffffffF0,-1,0,20}, {fpcNm_E_YK_e,0xffff0f01,-1,180,4},
    {fpcNm_E_YC_e,0,-1,260,12}, {fpcNm_B_DS_e,0xffffff02,-1,300,1200},
    {fpcNm_E_ZH_e,0xffffff00,-1,240,1200}
}};
bool extended_enemy(ProfileName profile) {
    return std::any_of(native_roster.begin()+5,native_roster.end(),
        [profile](const auto& native){return native.profile==profile;});
}
// Suppress only native puzzle/instant-kill damage rules for run-owned variants.
#define ROSTER_HIT(Name, Symbol, Type) DEFINE_HOOK_SYMBOL(Symbol,void(Type*),Name)
ROSTER_HIT(RosterTT,"daE_TT_c::damage_check",daE_TT_c);
ROSTER_HIT(RosterDN,"src/d/actor/d_a_e_dn.cpp#damage_check",e_dn_class);
ROSTER_HIT(RosterMM,"src/d/actor/d_a_e_mm.cpp#damage_check",e_mm_class);
ROSTER_HIT(RosterMMArmor,"src/d/actor/d_a_e_mm.cpp#damage_checkMetOn",e_mm_class);
ROSTER_HIT(RosterGE,"daE_GE_c::damage_check",daE_GE_c);
ROSTER_HIT(RosterKR,"src/d/actor/d_a_e_kr.cpp#damage_check",e_kr_class);
ROSTER_HIT(RosterCR,"src/d/actor/d_a_e_cr.cpp#damage_check",e_cr_class);
ROSTER_HIT(RosterBS,"src/d/actor/d_a_e_bs.cpp#damage_check",e_bs_class);
ROSTER_HIT(RosterGI,"daE_GI_c::damage_check",daE_GI_c);
ROSTER_HIT(RosterSF,"src/d/actor/d_a_e_sf.cpp#damage_check",e_sf_class);
ROSTER_HIT(RosterFS,"src/d/actor/d_a_e_fs.cpp#damage_check",e_fs_class);
ROSTER_HIT(RosterBU,"src/d/actor/d_a_e_bu.cpp#damage_check",e_bu_class);
ROSTER_HIT(RosterSM,"src/d/actor/d_a_e_sm2.cpp#damage_check",e_sm2_class);
ROSTER_HIT(RosterMF,"src/d/actor/d_a_e_mf.cpp#damage_check",e_mf_class);
ROSTER_HIT(RosterGG,"daB_GG_c::At_Check",daB_GG_c);
ROSTER_HIT(RosterTN,"daB_TN_c::damage_check",daB_TN_c);
ROSTER_HIT(RosterDD,"src/d/actor/d_a_e_dd.cpp#damage_check",e_dd_class);
ROSTER_HIT(RosterSH,"src/d/actor/d_a_e_sh.cpp#damage_check",e_sh_class);
ROSTER_HIT(RosterRDY,"src/d/actor/d_a_e_rdy.cpp#damage_check",e_rdy_class);
ROSTER_HIT(RosterS1,"src/d/actor/d_a_e_s1.cpp#damage_check",e_s1_class);
ROSTER_HIT(RosterYK,"src/d/actor/d_a_e_yk.cpp#damage_check",e_yk_class);
#undef ROSTER_HIT
DEFINE_HOOK_SYMBOL("cCcS::Set",void(cCcS*,cCcD_Obj*),RosterTargets);
DEFINE_HOOK_SYMBOL("daE_GI_c::setCryStop",bool(daE_GI_c*),RosterGibdoCry);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_mm_mt.cpp#action",void(e_mm_mt_class*),RosterShell);
DEFINE_HOOK_SYMBOL("daE_TT_c::checkPlayerSearch",bool(daE_TT_c*),RosterTTSearch);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_bu.cpp#pl_check",int(e_bu_class*,float,s16),RosterBubbleSearch);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_dd.cpp#pl_check",int(e_dd_class*,float,s16),RosterDDSearch);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_rdy.cpp#pl_check",int(e_rdy_class*,float,s16),RosterRDYSearch);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_rdy.cpp#e_rdy_normal",void(e_rdy_class*),RosterRDYIdle);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_yc.cpp#action",void(e_yc_class*),RosterYCAction);
DEFINE_HOOK_SYMBOL("daB_DS_c::mBattle2Action",void(daB_DS_c*),RosterHeadAction);
DEFINE_HOOK_SYMBOL("daE_ZH_c::action",void(daE_ZH_c*),RosterHandAction);
DEFINE_HOOK_SYMBOL("daE_ZH_c::cc_set",void(daE_ZH_c*),RosterHandCollision);
DEFINE_HOOK_SYMBOL("daB_DS_c::create",cPhs_Step(daB_DS_c*),RosterHeadCreate);
DEFINE_HOOK(static_cast<s32 (*)()>(&dComIfGs_isStageBossEnemy),RosterHeadCleared);
DEFINE_HOOK_SYMBOL("dComIfGs_onZoneSwitch",void(int,int),RosterHeadZone);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_yk.cpp#pl_check",int(e_yk_class*,float,s16),RosterYKSearch);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_s1.cpp#pl_check",int(e_s1_class*,float),RosterS1Search);
