// Six short horizontal steps, at most one push every twelve active ticks.
// Simultaneous pellets refresh only to the strongest push, never add impulses.
void queue_knockback(Enemy& enemy, fopAc_ac_c* actor, bool heavy) {
    if (!actor || actor->health<=0 || enemy.elite || native_boss_profile(enemy.profile)) return;
    const float distance=s_run.knockback_distance(heavy,enemy.elite);
    if (distance<=0) return;
    auto& effect=effect_for(enemy);
    if (effect.knockCooldown && effect.knockCooldown!=12) return;
    if (effect.knockCooldown==12 && effect.knockStep.absXZ()*6>=distance) return;
    auto* link=daAlink_getAlinkActorClass();
    if (!link) return;
    cXyz direction=actor->current.pos-link->current.pos;
    direction.y=0;
    const float length=direction.absXZ();
    if (length<1.f) direction=cXyz(0,0,1);
    else direction=direction*(1.f/length);
    effect.knockStep=direction*(distance/6.f);
    effect.knockFrames=6;effect.knockCooldown=12;
}

void update_knockback(Enemy& enemy, fopAc_ac_c* actor) {
    auto& effect=effect_for(enemy);
    if(enemy.elite || native_boss_profile(enemy.profile)) {effect.knockFrames=effect.knockCooldown=0;return;}
    if (effect.knockCooldown) --effect.knockCooldown;
    if (!effect.knockFrames) return;
    --effect.knockFrames;
    cXyz from=actor->current.pos+cXyz(0,30,0), to=from+effect.knockStep;
    dBgS_LinChk wall;wall.Set(&from,&to,actor);
    cXyz floor=actor->current.pos;
    // Keep ground enemies on their floor and flying enemies at their altitude;
    // never push through walls, across arena barriers or off a ledge.
    if (dComIfG_Bgsp().LineCross(&wall) || !dry_arena_floor(floor,60,5000)) {
        effect.knockFrames=0;return;
    }
    cXyz destination=floor+effect.knockStep;
    if (!walkable_spawn_path(floor,destination,20)) { effect.knockFrames=0;return; }
    cXyz next=actor->current.pos+effect.knockStep;
    // Follow a safe slope without snapping a low-flying or leaping actor down
    // to the floor. Keep its current height above the floor and native velocity;
    // knockback must not end a leap or alter the attack/animation clock.
    if (std::abs(actor->current.pos.y-floor.y)<=60) next.y+=destination.y-floor.y;
    const cXyz delta=next-actor->current.pos;
    actor->current.pos=next;
    actor->old.pos+=delta;
    actor->eyePos+=delta;
}
