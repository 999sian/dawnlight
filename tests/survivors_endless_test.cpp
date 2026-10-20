#include "../src/survivors_rules.hpp"
#include <cassert>
#include <vector>
#include <iostream>
using namespace dawnlight::survivors;
int main() {
    RunOptions options;
    assert(options.twilit_rank()==4 && options.score_percent()==100);
    options.enemy_hp=10;options.enemy_damage=10;options.enemy_speed=3;options.horde=5;
    assert(options.twilit_rank()==28 && options.score_percent()==415);
    Records records;Run run;run.start(42,options);
    run.ranks[unsigned(Upgrade::Lightning)]=12;run.master_sword=8;run.xp=17;
    run.ticks=duration+300;run.kills=400;run.timed_elites=4;
    assert(!run.continue_endless());run.end(true);
    records.finish(run,0);const auto checkpoint=records;
    const auto score=run.score(),random=run.random;
    assert(run.continue_endless() && !run.continue_endless());
    assert(run.phase==Phase::Running && run.rank(Upgrade::Lightning)==12 && run.master_sword==8);
    assert(run.xp==17 && run.random==random && run.normal_score==score);
    assert(run.twilit_rank()==28 && run.endless_minutes()==0 && run.endless_factor()==1.0);
    for(unsigned minute=0;minute<=20;++minute) {
        run.ticks=run.endless_start+minute*1800;
        assert(run.endless_minutes()==minute);
        assert(std::abs(run.endless_factor()-std::pow(1.1,minute))<1e-8);
        if(minute){--run.ticks;assert(run.endless_minutes()==minute-1);++run.ticks;}
    }
    run.ticks=run.endless_start+1800;
    assert(run.incoming_damage(4,2)==88);
    assert(!run.tick(false));const auto ticks=run.ticks;assert(run.tick(true)&&run.ticks==ticks+1);
    run.horde_remaining=100;run.tick(true);assert(run.horde_remaining==99);
    ++run.kills;run.end(false);records.finish(run,0);
    assert(records.runs==1 && records.wins==1 && records.cleared_maps==1);
    assert(records.total_xp==run.score()); // Normal victory reward was not paid twice.
    assert(records.ranked[0].encode()==checkpoint.ranked[0].encode());
    assert(records.endless_ranked[0].present()&&records.endless_ranked[0].run.twilit_rank()==28);
    Records restored;assert(restored.decode(records.encode())&&restored.encode()==records.encode());
    restored.delete_high_score(0,false,true);
    assert(!restored.endless_ranked[0].present()&&restored.ranked[0].present()&&restored.map_unlocked(1));
    // Migration of the real v9 shape: ten snapshots, no endless fields or rank word.
    const auto bytes=checkpoint.encode();std::vector<uint8_t> v9(bytes.begin(),bytes.begin()+136);v9[4]=9;
    for(unsigned n=0;n<10;++n){
        const auto begin=bytes.begin()+136+n*HighScoreRun::word_count*4;
        v9.insert(v9.end(),begin,begin+HighScoreRun::legacy_word_count*4);
    }
    assert(v9.size()==Records::v9_byte_count && restored.decode(v9));
    assert(restored.ranked[0].score==score&&restored.ranked[0].run.twilit_rank()==28);
    assert(restored.total_xp==checkpoint.total_xp&&restored.cleared_maps==checkpoint.cleared_maps);
    for(unsigned i=0;i<5;++i)assert(!restored.endless_ranked[i].present()&&!restored.endless_cheated[i].present());
    // Both categories on every map, intact snapshots and rank after reload.
    for(unsigned map=0;map<5;++map)for(bool assisted:{false,true}){
        Run r;r.start(7);r.options.allow_cheats=assisted;r.ticks=duration;r.end(true);
        restored.finish(r,map);assert(r.continue_endless());r.ticks+=9000;r.end(false);restored.finish(r,map);
        assert(restored.high_score(map,assisted,true).present());
    }
    assert(records.decode(restored.encode())&&records.encode()==restored.encode());
    auto bad=restored.encode();bad[136+HighScoreRun::legacy_word_count*4]=99;
    assert(!records.decode(bad)); // Invalid rank does not mutate existing records.
    assert(records.encode()==restored.encode());
    records.clear_high_scores();assert(records.cleared_maps==31);
    for(unsigned map=0;map<5;++map)for(bool assisted:{false,true})for(bool endless:{false,true})
        assert(!records.high_score(map,assisted,endless).present());
    // Normal curves remain capped at 20m; Endless adds both extrapolation and compounding.
    Run hp;hp.start(1);hp.ticks=duration;const auto normal=hp.enemy_health(8,false);
    hp.ticks+=1800;assert(hp.enemy_health(8,false)==normal);hp.end(true);hp.continue_endless();
    const auto initial=hp.enemy_health(8,false);assert(initial>normal);
    hp.ticks+=1800;assert(hp.enemy_health(8,false)>unsigned(initial*1.1));
    hp.ticks=UINT32_MAX;assert(hp.enemy_health(8,false)==32767&&hp.incoming_damage(4)==32767);
    std::cout<<"Endless: continuation, compound scaling, rank, separate scores, XP and v9 migration passed\n";
}
