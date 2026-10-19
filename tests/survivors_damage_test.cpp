#include "../src/survivors_rules.hpp"
#include <cassert>
#include <iostream>
using namespace dawnlight::survivors;
int main() {
    DamageQueue queue;
    queue.add(DamageSource::Sword,30);queue.add(DamageSource::Bow,10);
    queue.add(DamageSource::FireEdge,20,true);queue.consume();
    assert(!damage_sum(queue.pending)&&!damage_sum(queue.fixedPending));
    // Separate the 20 bonus HP from Dark Link; fixed burns are not multiplied twice.
    auto weights=queue.weights(40,60,20);DamageTotals totals;
    totals.record(weights,200,120);
    assert(totals.total()==80&&totals.dealt[0]==30&&totals.dealt[4]==10&&totals.dealt[14]==20&&totals.dealt[unsigned(DamageSource::DarkLink)]==20);
    totals.record(weights,120,120);totals.record(weights,120,150);totals.record(weights,0,-20);
    assert(totals.total()==80); // Block, healing and an already dead target.
    totals.record(weights,40,-400);assert(totals.total()==120); // No overkill.
    totals.record({},100,10);assert(totals.total()==120); // Unowned damage.
    for(unsigned hp=1;hp<1000;++hp) {
        DamageTotals test;test.record(weights,int(hp),-100);assert(test.total()==hp);
        assert(!test.dealt[unsigned(DamageSource::Other)]);
    }
    queue.add(DamageSource::FireBomb,7,true);queue.consume();
    weights=queue.weights(0,0,7);DamageTotals burn;burn.record(weights,20,13);
    assert(burn.total()==7&&burn.dealt[unsigned(DamageSource::FireBomb)]==7);
    queue={};queue.consume();weights=queue.weights(5,10,0);
    DamageTotals fallback;fallback.record(weights,20,10);
    assert(fallback.dealt[unsigned(DamageSource::Other)]==5&&fallback.dealt[unsigned(DamageSource::DarkLink)]==5);
    Run run;run.start(3);run.damage=totals;run.end(true);
    assert(run.damage.total()==120);run.start(4);assert(!run.damage.total());
    assert(weapon_damage_source(unsigned(Upgrade::BombArrow))==DamageSource::BombArrow);
    std::cout<<"Damage attribution: weighted mixed hits, fixed burns, overkill, rejection and run reset passed\n";
}
