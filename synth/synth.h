#pragma once
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "unit.h"

// Self-running four-voice synthesizer. The audio thread owns all DSP and
// sequence state; control callbacks communicate through lock-free atomics.
class Synth {
 public:
  enum { Run, Clock, Bpm, Rate, Scale, Root, Octaves, Density, Mutation, Seed,
         Wave, Tone, Attack, Decay, FM, Detune, Echo, Feedback, Width, Level, Count };
  Synth() { for(unsigned i=0;i<Count;++i) p_[i].store(defaults()[i]); }
  int8_t Init(const unit_runtime_desc_t* d) {
    if(!d)return k_unit_err_undef;
    if(d->samplerate!=48000)return k_unit_err_samplerate;
    if(d->output_channels!=2)return k_unit_err_geometry;
    for(unsigned i=0;i<Table;++i)sine_[i]=std::sin(6.28318530718f*i/Table);
    Reset(); return k_unit_err_none;
  }
  void Reset() {
    for(auto& v:voices_)v=Voice{};
    write_=0;filled_=0;step_=0;nextVoice_=0;timer_=0;
    master_=0;playing_=false;seed_=-1;
    restart_.store(false);halt_.store(false);bend_.store(8192);
  }
  void Teardown(){Reset();}
  void Resume(){restart_.store(true);}
  void Suspend(){Reset();}
  void setParameter(uint8_t i,int32_t v){
    if(i>=Count)return;
    v=clamp(v,minimum()[i],maximum()[i]);
    const int32_t previous=p_[i].exchange(v);
    if(i==Run && v && !previous)restart_.store(true);
  }
  int32_t getParameterValue(uint8_t i)const{return i<Count?p_[i].load():0;}
  const char* getParameterStrValue(uint8_t i,int32_t v)const{
    static const char* clocks[]={"Device","Internal"};
    static const char* rates[]={"1/2","1/4","1/8","1/16"};
    static const char* scales[]={"Minor pent","Major pent","Minor","Major","Dorian","Chromatic"};
    static const char* waves[]={"Sine","Triangle","Saw"};
    if(i==Clock)return clocks[clamp(v,0,1)];
    if(i==Rate)return rates[clamp(v,0,3)];
    if(i==Scale)return scales[clamp(v,0,5)];
    if(i==Wave)return waves[clamp(v,0,2)];
    return nullptr;
  }
  const uint8_t* getParameterBmpValue(uint8_t,int32_t)const{return nullptr;}
  void SetTempo(uint32_t t){if(t>=20u*65536u && t<=400u*65536u)tempo_.store(t);}
  void NoteOn(uint8_t n,uint8_t v){if(v){p_[Root].store(clamp(n,24,84));restart_.store(true);}}
  void NoteOff(uint8_t){}
  void GateOn(uint8_t v){if(v)restart_.store(true);}
  void GateOff(){}
  void AllNoteOff(){halt_.store(true);}
  void PitchBend(uint16_t b){bend_.store(b>16383?16383:b);}
  void ChannelPressure(uint8_t){}
  void Aftertouch(uint8_t,uint8_t){}
  void LoadPreset(uint8_t){}
  uint8_t getPresetIndex()const{return 0;}
  static const char* getPresetName(uint8_t){return nullptr;}

  void Render(float* out,size_t frames){
    if(!out)return;
    int32_t q[Count];for(unsigned i=0;i<Count;++i)q[i]=p_[i].load();
    if(restart_.exchange(false)){halt_.store(false);timer_=0;step_=0;seed_=-1;}
    const bool run=q[Run] && !halt_.load();
    if(run && !playing_){timer_=0;step_=0;seed_=-1;}
    playing_=run;
    if(seed_!=q[Seed]){seed_=q[Seed];rng_=uint32_t(seed_)+0x9e3779b9u;makeMotif(q[Octaves],q[Scale]);}
    const double bpm=q[Clock]?q[Bpm]:double(tempo_.load())/65536.0;
    const double beat=48000.0*60.0/bpm;
    const double lengths[]={2.0,1.0,.5,.25};
    const double interval=beat*lengths[q[Rate]];
    const unsigned delayFrames=unsigned(beat*.75<Delay-1?beat*.75:Delay-1);
    const float alpha=1.f-std::exp(-6.28318530718f*q[Tone]/48000.f);
    const float detune=std::pow(2.f,float(q[Detune])/1200.f);
    const float bend=std::pow(2.f,(int(bend_.load())-8192)/49152.f);
    const float wet=q[Echo]*.01f,feedback=q[Feedback]*.01f;
    const float level=q[Level]*.01f;
    for(size_t f=0;f<frames;++f){
      if(run){
        if(timer_<=0){
          if(random()%100<unsigned(q[Mutation])) {
            // Local, bounded melodic change instead of random octave jumps.
            const int n=degrees(q[Scale])*q[Octaves];
            const int delta=int(random()%5)-2;
            int candidate=int(motif_[step_])+delta;
            // Reflect at phrase boundaries rather than sticking to the extremes.
            if(candidate<0)candidate=-candidate;
            if(candidate>=n)candidate=2*(n-1)-candidate;
            motif_[step_]=uint8_t(clamp(candidate,0,n-1));
          }
          if(random()%100<unsigned(q[Density]))trigger(q,motif_[step_]);
          step_=(step_+1)%16;timer_+=interval;
        }
        timer_-=1;
      }
      master_+=((run?1.f:0.f)-master_)*.004f;
      if(!run && master_<.000001f){master_=0;for(auto& v:voices_)v.active=false;}
      float mix[2]={0,0};
      for(auto& v:voices_){
        if(!v.active)continue;
        if(v.attacking){v.env+=v.attack;if(v.env>=1){v.env=1;v.attacking=false;}}
        else{v.env*=v.decay;if(v.env<.00001f){v.active=false;continue;}}
        const float inc=v.increment*bend;
        const float mod=sine(v.modphase)*q[FM]*.0025f;
        float x=.5f*(osc(v.phase,inc,q[Wave],mod)+osc(v.other,inc*detune,q[Wave],mod));
        v.filter+=alpha*(x-v.filter);
        x=(q[Tone]>=16000?x:v.filter)*v.env*v.velocity*.24f;
        mix[0]+=x*v.left;mix[1]+=x*v.right;
        v.phase=wrap(v.phase+inc);v.other=wrap(v.other+inc*detune);v.modphase=wrap(v.modphase+inc*2);
      }
      const unsigned read=(write_+Delay-delayFrames)%Delay;
      const float dl=filled_>=delayFrames?delay_[0][read]:0.f,dr=filled_>=delayFrames?delay_[1][read]:0.f;
      // Stabilized feedback: saturate only the recirculating signal and clear denormals.
      const float fl=dr*feedback,fr=dl*feedback;
      const float wl=mix[0]+fl/(1.f+std::fabs(fl));
      const float wr=mix[1]+fr/(1.f+std::fabs(fr));
      delay_[0][write_]=std::fabs(wl)<1.e-19f?0.f:wl;
      delay_[1][write_]=std::fabs(wr)<1.e-19f?0.f:wr;
      write_=(write_+1)%Delay;if(filled_<Delay)++filled_;
      for(unsigned c=0;c<2;++c){const float x=(mix[c]+(c?dr:dl)*wet)*level*master_;out[2*f+c]=x/(1.f+std::fabs(x));}
    }
  }
 private:
  static constexpr unsigned Table=1024,Delay=48000;
  struct Voice {bool active=false,attacking=false;float phase=0,other=0,modphase=0,increment=0,env=0,attack=0,decay=0,velocity=0,filter=0,left=0,right=0;};
  std::atomic<int32_t> p_[Count];
  std::atomic<uint32_t> tempo_{120u*65536u},bend_{8192};
  std::atomic<bool> restart_{false},halt_{false};
  Voice voices_[4]{};
  float sine_[Table]{},delay_[2][Delay]{},master_=0;
  unsigned write_=0,filled_=0,step_=0,nextVoice_=0;
  uint8_t motif_[16]{};
  uint32_t rng_=1;int32_t seed_=-1;
  double timer_=0;bool playing_=false;
  static int32_t clamp(int32_t v,int32_t lo,int32_t hi){return v<lo?lo:(v>hi?hi:v);}
  static const int32_t* defaults(){static const int32_t v[]={1,0,90,2,0,48,2,75,20,71,0,5000,10,1400,20,7,30,45,80,70};return v;}
  static const int32_t* minimum(){static const int32_t v[]={0,0,30,0,0,24,1,0,0,0,0,100,2,50,0,0,0,0,0,0};return v;}
  static const int32_t* maximum(){static const int32_t v[]={1,1,240,3,5,84,3,100,100,32767,2,16000,2000,8000,100,30,75,85,100,100};return v;}
  // Inputs are within [-0.25, 1.25] because note pitch is bounded. Avoid
  // a libm floor call for every oscillator/sample on the Cortex-A7.
  static float wrap(float p){return p<0?p+1:(p>=1?p-1:p);}
  float sine(float p)const{const float x=wrap(p)*Table;const unsigned i=unsigned(x);const float a=sine_[i];return a+(sine_[(i+1)%Table]-a)*(x-i);}
  static float blep(float p,float dt){
    if(p<dt){const float t=p/dt;return t+t-t*t-1;}
    if(p>1-dt){const float t=(p-1)/dt;return t*t+t+t+1;}
    return 0;
  }
  float osc(float p,float dt,int wave,float mod)const{
    if(wave==0)return sine(p+mod);
    if(wave==1)return (1.f-4.f*std::fabs(p-.5f))*(1.f-qmod(mod)) + sine(p+mod)*qmod(mod);
    return (2*p-1-blep(p,dt))*(1.f-qmod(mod))+sine(p+mod)*qmod(mod);
  }
  static float qmod(float m){return std::fabs(m)*2;}
  uint32_t random(){rng_^=rng_<<13;rng_^=rng_>>17;rng_^=rng_<<5;return rng_;}
  static int degrees(int scale){return scale<2?5:(scale<5?7:12);}
  static int semitone(int scale,int degree){
    static const uint8_t notes[6][12]={{0,3,5,7,10},{0,2,4,7,9},{0,2,3,5,7,8,10},{0,2,4,5,7,9,11},{0,2,3,5,7,9,10},{0,1,2,3,4,5,6,7,8,9,10,11}};
    const int n=degrees(scale);return 12*(degree/n)+notes[scale][degree%n];
  }
  void makeMotif(int octaves,int scale){
    int d=0;const int n=degrees(scale)*octaves;
    for(unsigned i=0;i<16;++i){d=clamp(d+int(random()%5)-2,0,n-1);motif_[i]=uint8_t(d);}
  }
  void trigger(const int32_t* q,unsigned degree){
    degree%=unsigned(degrees(q[Scale])*q[Octaves]);
    const int note=clamp(q[Root]+semitone(q[Scale],int(degree)),0,108);
    // Prefer an idle/quiet voice to avoid needlessly cutting an audible tail.
    unsigned chosen=nextVoice_;
    for(unsigned i=0;i<4;++i){
      const unsigned candidate=(nextVoice_+i)&3u;
      if(!voices_[candidate].active){chosen=candidate;break;}
      if(voices_[candidate].env<voices_[chosen].env)chosen=candidate;
    }
    nextVoice_=(chosen+1)&3u;
    Voice& v=voices_[chosen];v=Voice{};
    v.active=v.attacking=true;v.increment=440.f*std::pow(2.f,(note-69)/12.f)/48000.f;
    v.attack=1.f/(48.f*q[Attack]);v.decay=std::exp(-6.907755f/(48.f*q[Decay]));
    v.velocity=.55f+(random()%46)*.01f;
    const float pan=.5f+(int(random()%101)-50)*.01f*q[Width]*.01f;
    v.left=std::sqrt(1-pan);v.right=std::sqrt(pan);
  }
};
