#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>
#include "synth.h"

static unit_runtime_desc_t descriptor(){unit_runtime_desc_t d{};d.samplerate=48000;d.output_channels=2;return d;}
static double energy(const std::vector<float>& x){double e=0;for(float f:x){assert(std::isfinite(f));assert(std::fabs(f)<=1);e+=f*f;}return e;}
static std::vector<float> render(Synth& s,size_t n){std::vector<float> x(n*2);size_t i=0;while(i<n){const size_t block=n-i<128?n-i:128;s.Render(x.data()+i*2,block);i+=block;}return x;}
int main(){
 auto s=std::unique_ptr<Synth>(new Synth);auto d=descriptor();assert(s->Init(&d)==0);
 // No NoteOn or GateOn: selecting/rendering the unit must generate sound.
 auto automatic=render(*s,48000*4);assert(energy(automatic)>1);
 bool stereo=false;for(size_t i=0;i<automatic.size();i+=2)if(std::fabs(automatic[i]-automatic[i+1])>.0001f)stereo=true;assert(stereo);
 s->Reset();auto repeated=render(*s,48000*4);assert(automatic==repeated);
 s->setParameter(Synth::Seed,72);s->Reset();auto changed=render(*s,48000*4);assert(changed!=automatic);assert(energy(changed)>1);
 s->setParameter(Synth::Run,0);render(*s,48000);assert(energy(render(*s,48000))==0);
 s->setParameter(Synth::Run,1);assert(energy(render(*s,48000))>0);
 s->AllNoteOff();render(*s,48000);assert(energy(render(*s,48000))==0);
 s->NoteOn(48,100);assert(energy(render(*s,48000))>0);
 s->setParameter(Synth::Density,0);s->Reset();assert(energy(render(*s,48000*2))==0);
 s->setParameter(Synth::Density,100);s->setParameter(Synth::Clock,1);s->setParameter(Synth::Echo,0);
 s->setParameter(Synth::Bpm,60);s->Reset();auto slow=render(*s,48000*3);
 s->setParameter(Synth::Bpm,240);s->Reset();auto quickTempo=render(*s,48000*3);assert(slow!=quickTempo);
 s->setParameter(Synth::Clock,0);s->SetTempo(60u*65536);s->Reset();assert(render(*s,48000*3)==slow);
 s->SetTempo(240u*65536);s->Reset();assert(render(*s,48000*3)==quickTempo);
 // Parameter extremes, every scale/wave, high pitch, maximum feedback.
 s->setParameter(Synth::Root,84);s->setParameter(Synth::Octaves,3);s->setParameter(Synth::FM,100);
 s->setParameter(Synth::Detune,30);s->setParameter(Synth::Echo,75);s->setParameter(Synth::Feedback,85);
 s->setParameter(Synth::Attack,2);s->setParameter(Synth::Decay,8000);s->setParameter(Synth::Level,100);
 s->setParameter(Synth::Mutation,100);s->setParameter(Synth::Rate,3);s->PitchBend(16383);
 for(int scale=0;scale<6;++scale)for(int wave=0;wave<3;++wave){s->setParameter(Synth::Scale,scale);s->setParameter(Synth::Wave,wave);energy(render(*s,48000));}
 s->setParameter(Synth::Root,-10000);assert(s->getParameterValue(Synth::Root)==24);
 s->setParameter(Synth::Feedback,10000);assert(s->getParameterValue(Synth::Feedback)==85);
 s->Suspend();s->Resume();assert(energy(render(*s,48000))>0);
 d.samplerate=44100;assert(s->Init(&d)==k_unit_err_samplerate);d=descriptor();d.output_channels=1;assert(s->Init(&d)==k_unit_err_geometry);
 std::cout<<"Passed: autonomous sound without note triggers, stereo, deterministic seed, mutation, RUN, panic/restart, density, tempo clocks and bounded output at parameter extremes.\n";
}
