"""Apply the production Intro Skip preset to native save structs with custom names."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1] if len(sys.argv) > 1 else root / "build").resolve()
source = (root / "src/new_save_modes.cpp").read_text()
start = source.index("void apply_intro_skip_preset(")
end = source.index("\nvoid set_item(", start)
production = source[start:end]
fixture = r'''
#include "helpers/string.hpp"
#include "d/d_save.h"
#include <array>
#include <cassert>
#include <cstring>
#include <string>
// Native constructors call these before every byte is replaced by our fixture.
void dSv_memory_c::init() {}
void dSv_memory2_c::init() {}
bool intro=false,boss=true;
void set_intro_skipped(bool enabled){intro=enabled;}
void set_boss_rush(bool enabled){boss=enabled;}
constexpr std::array<u8,sizeof(dSv_save_c)> kIntroSkipReferenceSave={
#include "intro_skip_reference_save.inc"
};
static_assert(sizeof(dSv_save_c)==0x958);
'''
cases = r'''
int main(){
 apply_intro_skip_preset(nullptr);assert(!intro&&boss);
 for(const char* player:{"René","Hero","1234567890123456","Link"})
 for(const char* horse:{"Épona","Storm","abcdefghijklmnop","Epona"}) {
  dSv_save_c save;
  std::memset(static_cast<void*>(&save),0,sizeof(save));
  auto& info=save.getPlayer().getPlayerInfo();
  std::strcpy(info.mPlayerName,player);std::strcpy(info.mHorseName,horse);
  info.setTotalTime(123456);save.getPlayer().getPlayerStatusA().setRupee(3);
  intro=false;boss=true;
  apply_intro_skip_preset(&save);
  assert(std::string(info.mPlayerName)==player&&std::string(info.mHorseName)==horse);
  assert(info.getTotalTime()==0&&save.getPlayer().getPlayerStatusA().getRupee()==150);
  assert(intro&&!boss);
  // Story/inventory bytes still match the intended preset. Only the two names,
  // fresh play time and starting rupees differ from the reference save.
  auto normalized=save;
  dSv_save_c reference;
  std::memcpy(&reference,kIntroSkipReferenceSave.data(),sizeof(reference));
  auto& normalizedInfo=normalized.getPlayer().getPlayerInfo();
  const auto& referenceInfo=reference.getPlayer().getPlayerInfo();
  std::memcpy(normalizedInfo.mPlayerName,referenceInfo.mPlayerName,sizeof(normalizedInfo.mPlayerName));
  std::memcpy(normalizedInfo.mHorseName,referenceInfo.mHorseName,sizeof(normalizedInfo.mHorseName));
  normalizedInfo.setTotalTime(referenceInfo.getTotalTime());
  normalized.getPlayer().getPlayerStatusA().setRupee(reference.getPlayer().getPlayerStatusA().getRupee());
  assert(std::memcmp(&normalized,&reference,sizeof(reference))==0);
  const auto once=save;apply_intro_skip_preset(&save);
  assert(std::memcmp(&save,&once,sizeof(save))==0);
 }
}
'''
ninja = (build / "build.ninja").read_text()
block = ninja.split("build CMakeFiles/dawnlight_mod.dir/src/config.cpp.o:", 1)[1].split("\nbuild ", 1)[0]
args = [os.environ.get("CXX", "c++"), "-I" + str(root / "src")]
for key in ("DEFINES", "FLAGS", "INCLUDES"):
    line = next(x for x in block.splitlines() if x.startswith("  " + key + " = "))
    args += shlex.split(line.split(" = ", 1)[1])
args = [x for x in args if x != "-DNDEBUG"]
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / "test.cpp"
    exe = Path(tmp) / "test"
    cpp.write_text(fixture + production + cases)
    subprocess.run(args + [str(cpp), "-o", str(exe)], cwd=build, check=True)
    subprocess.run([str(exe)], check=True)
print("Intro Skip: custom player/horse names, UTF-8, full-length names and native preset integrity passed")
