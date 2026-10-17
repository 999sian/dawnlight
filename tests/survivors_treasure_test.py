"""Exercise production chest interaction and native item-event lifecycle."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/survivors_treasure.inc').read_text()
core=(root/'src/survivors.cpp').read_text()
start=core.index('bool pause_run_actor(')
source+=core[start:core.index('\nHookAction before_process', start)]
fixture=r'''

#include <cassert>
#include <cstdint>
#include <cmath>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using s16=short;
constexpr int kNone=-1;
using u8=uint8_t;
using ActorId=int;
constexpr int fpcM_ERROR_PROCESS_ID_e=-1, fpcNm_Demo_Item_e=20;
constexpr int dItemNo_BOMB_5_e=10, dItemNo_BOW_e=11,dItemNo_BOOMERANG_e=12,dItemNo_IRONBALL_e=13, dEvtCnd_CANGETITEM_e=8, BUTTON_STATUS_OPEN=6;
struct ModContext{};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
struct cXyz {float x=0,y=0,z=0;
    cXyz operator+(cXyz p)const{return {x+p.x,y+p.y,z+p.z};}
    cXyz operator-(cXyz p)const{return {x-p.x,y-p.y,z-p.z};}
    float absXZ()const{return std::sqrt(x*x+z*z);}};
using Vec=cXyz;
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.14159265);}

struct leafdraw_class {int id=0;};
struct layer_class {bool deleting=false;};
layer_class rootLayer, sceneLayer, callbackLayer;
layer_class* currentLayer=&rootLayer;
layer_class* fpcLy_RootLayer(){return &rootLayer;}
layer_class* fpcLy_CurrentLayer(){return currentLayer;}
void fpcLy_SetCurrentLayer(layer_class* layer){currentLayer=layer;}
bool fpcLy_IsDeletingMesg(layer_class* layer){return layer->deleting;}
struct Actor:leafdraw_class {void* sub_method=nullptr;int name=0;struct {layer_class* layer=&sceneLayer;} layer_tag;};
using fopAc_ac_c=Actor;
struct EventInfo {bool command=false; int condition=0;
    bool checkCommandItem(){return command;}
    void onCondition(int c){condition|=c;}};
using process_method_func=int(*)(void*);
struct process_method_class {process_method_func execute_method;};
int execute(void*){return 1;}
process_method_class methods{execute};
void* dispatchArg=nullptr;
struct daDitem_c:Actor {
    enum {ACTION_END_e,ACTION_WAIT_LIGHT_END_e};
    daDitem_c(){sub_method=&methods;}
    int itemNo=0;int getItemNo(){return itemNo;}int mAction=ACTION_END_e;
    void setAction(int a){mAction=a;}
    EventInfo eventInfo; bool visible=true,dead=false;
    bool chkDraw(){return visible;} bool chkDead(){return dead;}
} prop;
struct daAlink_c:Actor {
    enum {PROC_WAIT,PROC_MOVE,PROC_ATN_MOVE,PROC_TIRED_WAIT,PROC_GET_ITEM,OTHER,ANM_GET_A};
    int mProcID=PROC_WAIT,animation=-1;bool wolf=false,dead=false,inEvent=false;
    struct {Vec pos;} current;
    struct {int p0=90,p1=90;void setParam0(int n){p0=n;} void setParam1(int n){p1=n;}} mDemo;
    struct {int field_0x300a=-7;} mProcVar1;
    unsigned field_0x32cc=0;
    struct {s16 y=0;}shape_angle;int doStatus=0;bool trigger=false;
    void setDoStatus(int s){doStatus=s;}bool doTrigger(){return trigger;}
    bool checkWolf(){return wolf;}bool checkDeadHP(){return dead;}bool checkEventRun(){return inEvent;}
    void setSingleAnimeBase(int a){animation=a;}
} player;
struct {int mPt2=-1;} event;
bool alive=false,creating=false,doing=false,deleteAllowed=true,mode=true,stageChange=false;
bool running=false,paused=false,talking=false,eventEnded=false,playerPresent=true,createFails=false;
int window=0,pauseStatus=0,created=0,ordered=0,reset=0,deleted=0,drawn=0,cleared=0,itemPartner=-1;
Actor* fopAcM_SearchByID(int id){return alive&&id==prop.id?&prop:nullptr;}
int fopAcM_GetName(Actor* a){return a->name;}
int fopAcM_GetID(leafdraw_class* a){return a->id;}
bool fpcM_IsCreating(int){return creating;}
daAlink_c* daAlink_getAlinkActorClass(){return playerPresent?&player:nullptr;}
auto* dComIfGp_getEvent(){return &event;}
bool dComIfGp_event_runCheck(){return running;}
bool dComIfGp_isEnableNextStage(){return stageChange;}
bool dComIfGp_isPauseFlag(){return paused;}
int dMeter2Info_getWindowStatus(){return window;}
int dMeter2Info_getPauseStatus(){return pauseStatus;}
bool dMsgObject_isTalkNowCheck(){return talking;}
void dComIfGp_event_setItemPartnerId(int id){itemPartner=id;}
void dComIfGp_event_setGtItm(int item){assert(item==prop.itemNo);}
bool dComIfGp_evmng_endCheck(const char*){return eventEnded;}
void dComIfGp_event_reset(){++reset;running=false;event.mPt2=-1;}
int fopAcM_GetRoomNo(Actor*){return 0;}
int fopAcM_delete(Actor*){if(!deleteAllowed)return 0;alive=false;++deleted;return 1;}
int fopAcM_createDemoItem(Vec*,int item,int bit,void*,int,void*,u8 flags){
    assert(item>=10&&item<=13&&bit==-1&&flags==1);prop.itemNo=item; // Presentation-only, no inventory grant.
    assert(currentLayer==player.layer_tag.layer&&currentLayer!=&rootLayer);
    ++created;if(createFails)return -1;
    alive=true;prop.id=100;prop.name=fpcNm_Demo_Item_e;return prop.id;
}
int fopAcM_orderItemEvent(daDitem_c* p,int,int){assert(p==&prop);++ordered;return 1;}
struct Node{Node* next=nullptr;};
struct create_tag {struct {Node node;void* mpTagData=nullptr;} base;};
struct create_request {int id=100;};
create_request request;
create_tag tag;
struct {Node* mpHead=nullptr;} g_fpcCtTg_Queue;
#define NODE_GET_NEXT(n) ((n)->next)
bool fpcCtRq_IsDoing(create_request*){return doing;}
bool fpcCtRq_Cancel(create_request*){creating=false;alive=false;g_fpcCtTg_Queue.mpHead=nullptr;return true;}
namespace mods {
template<class T> T arg(void* args,int n){
    if(n==1)return reinterpret_cast<T>(dispatchArg);
    return reinterpret_cast<T>(args);
}
}


Run s_run;unsigned rupees=0;int s_player=1;
struct Chest{bool active=false,opened=false,opening=false;cXyz position{};unsigned age=0;Upgrade reward=Upgrade::Count;};
constexpr unsigned kChestCount=2;unsigned s_selectedChest=kChestCount;
std::array<Chest,2>s_chests;
struct {bool ready=true;}s_loot;
bool blocked=false;
struct dBgS_LinChk {void Set(const cXyz*,const cXyz*,daAlink_c*){}};
struct Background {bool LineCross(dBgS_LinChk*){return blocked;}}background;
Background& dComIfG_Bgsp(){return background;}
Actor* actor_by_id(int id){return !creating?fopAcM_SearchByID(id):nullptr;}
bool in_arena(){return mode;}bool transitioning(){return stageChange;}
bool ui_visible(){return window!=0;}
bool playable(){return mode&&!stageChange&&!running&&!paused;}
bool sword_action_available(daAlink_c* link){return link->mProcID==daAlink_c::PROC_WAIT;}
unsigned dComIfGs_getRupee(){return rupees;}void dComIfGs_setRupee(unsigned n){rupees=n;}
void clear_attacks(){++cleared;}
'''
main=r'''

void fresh(){
    s_run.start(123);s_treasure={};s_chests={};s_chests[0].active=true;s_chests[0].position={0,0,100};
    s_selectedChest=kChestCount;rupees=100;s_loot.ready=true;
    prop={};player={};player.id=1;event.mPt2=-1;
    currentLayer=&rootLayer;sceneLayer.deleting=false;
    alive=creating=doing=running=paused=talking=stageChange=eventEnded=createFails=blocked=false;
    mode=playerPresent=deleteAllowed=true;window=pauseStatus=0;
    created=ordered=reset=deleted=drawn=cleared=0;itemPartner=-1;
    g_fpcCtTg_Queue.mpHead=nullptr;
}
void accept(){event.mPt2=prop.id;prop.eventInfo.command=true;running=true;}
int main(){
    fresh();int result=0;
    for(auto phase:{Phase::Ready,Phase::Running,Phase::Treasure,Phase::Upgrade,Phase::Defeat}){
        s_run.phase=phase;
        assert(pause_run_actor(true)==(phase!=Phase::Running));
        assert(pause_run_actor(false)==(phase!=Phase::Running&&phase!=Phase::Treasure));
    }
    s_run.phase=Phase::Treasure;window=1;assert(pause_run_actor(false));
    fresh();
    rupees=19;player.trigger=true;
    assert(before_chest_action(nullptr,&player,&result,nullptr)==HOOK_CONTINUE);
    assert(player.doStatus==0 && created==0 && s_run.phase==Phase::Running);
    rupees=20;player.trigger=false;
    assert(before_chest_action(nullptr,&player,&result,nullptr)==HOOK_CONTINUE);
    assert(player.doStatus==BUTTON_STATUS_OPEN && created==0);
    for(unsigned reason=0;reason<8;++reason){
        fresh();
        if(reason==0)player.shape_angle.y=0x4000;
        if(reason==1)player.current.pos.z=-100;
        if(reason==2)player.current.pos.y=50;
        if(reason==3)blocked=true;
        if(reason==4)s_loot.ready=false;
        if(reason==5)paused=true;
        if(reason==6)stageChange=true;
        if(reason==7)for(unsigned i=8;i<12;++i)s_run.ranks[i]=5;
        assert(!begin_treasure_demo(0,&player)&&created==0&&rupees==100);
    }
    fresh();createFails=true;assert(!begin_treasure_demo(0,&player));
    assert(currentLayer==&rootLayer&&s_run.phase==Phase::Running&&rupees==100);
    fresh();player.trigger=true;
    assert(before_chest_action(nullptr,&player,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==1);
    assert(created==1&&cleared==1&&s_run.phase==Phase::Treasure&&rupees==100);
    assert(!begin_treasure_demo(0,&player));
    const auto reward=s_chests[0].reward;
    assert(!s_run.tick(true));
    update_treasure_demo(&player);assert(ordered==1&&prop.eventInfo.condition==dEvtCnd_CANGETITEM_e);
    before_treasure_get(nullptr,&player,nullptr,nullptr);assert(player.mDemo.p0==90);
    accept();before_treasure_get(nullptr,&player,nullptr,nullptr);
    assert(s_treasure.started&&itemPartner==prop.id&&player.mDemo.p0==0&&player.mDemo.p1==0);
    after_treasure_get(nullptr,&player,nullptr,nullptr);assert(!s_treasure.poseStarted);
    player.mProcID=daAlink_c::PROC_GET_ITEM;
    after_treasure_get(nullptr,&player,nullptr,nullptr);
    assert(s_treasure.poseStarted&&player.animation==daAlink_c::ANM_GET_A&&player.mProcVar1.field_0x300a==0);
    player.animation=99;after_treasure_get(nullptr,&player,nullptr,nullptr);assert(player.animation==99);
    retain_treasure_actor(&prop,reinterpret_cast<void*>(execute));
    assert(prop.mAction==daDitem_c::ACTION_WAIT_LIGHT_END_e);
    update_treasure_demo(&player);assert(s_chests[0].opening&&s_chests[0].age==1&&rupees==100);
    eventEnded=true;update_treasure_demo(&player);
    assert(s_run.phase==Phase::Running&&rupees==80&&s_run.chest_cost()==40);
    assert(s_chests[0].opened&&s_run.rank(reward)==1&&reset==1&&!alive);
    update_treasure_demo(&player);assert(rupees==80&&s_run.chests_opened==1);
    // Async failure, order timeout, interrupted event: no charge or reward.
    for(unsigned reason=0;reason<3;++reason){
        fresh();assert(begin_treasure_demo(0,&player));
        if(reason==0)alive=false;
        if(reason==1)s_treasure.waitFrames=300;
        if(reason==2){accept();before_treasure_get(nullptr,&player,nullptr,nullptr);event.mPt2=999;}
        update_treasure_demo(&player);
        assert(s_run.phase==Phase::Running&&rupees==100&&s_run.chests_opened==0&&!s_chests[0].opened);
        if(reason==2)assert(reset==0&&event.mPt2==999);
    }
    fresh();assert(begin_treasure_demo(0,&player));creating=true;doing=true;
    tag.base.mpTagData=&request;g_fpcCtTg_Queue.mpHead=&tag.base.node;
    cancel_treasure_demo();assert(s_treasure.retiring&&creating);
    doing=false;retire_treasure_actor();assert(!creating&&s_treasure.item==kNone);
    fresh();assert(begin_treasure_demo(0,&player));deleteAllowed=false;
    cancel_treasure_demo();assert(s_treasure.retiring&&alive);
    deleteAllowed=true;retire_treasure_actor();assert(!alive&&s_treasure.item==kNone);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';binary=Path(tmp)/'test'
    cpp.write_text(fixture+source+main)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Survivors treasure: affordability, native interaction, item demo, single commit and cleanup passed')
