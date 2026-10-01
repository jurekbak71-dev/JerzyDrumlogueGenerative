#include "unit.h"
#define P(lo,hi,init,type,name) {lo,hi,0,init,type,0,0,0,{name}}
#define E P(0,0,0,k_unit_param_type_none,"")
const __unit_header unit_header_t unit_header={
 .header_size=sizeof(unit_header_t),.target=UNIT_TARGET_PLATFORM|k_unit_module_synth,
 .api=UNIT_API_VERSION,.dev_id=0x4A424B31U,.unit_id=0x47454E31U,
 .version=0x00010000U,.name="JerzyGen",.num_presets=0,.num_params=20,
 .params={
 P(0,1,1,k_unit_param_type_onoff,"RUN"),
 P(0,1,0,k_unit_param_type_strings,"CLOCK"),
 P(30,240,90,k_unit_param_type_bpm,"BPM"),
 P(0,3,2,k_unit_param_type_strings,"RATE"),
 P(0,5,0,k_unit_param_type_strings,"SCALE"),
 P(24,84,48,k_unit_param_type_midi_note,"ROOT"),
 P(1,3,2,k_unit_param_type_none,"OCTAVES"),
 P(0,100,75,k_unit_param_type_percent,"DENSITY"),
 P(0,100,20,k_unit_param_type_percent,"MUTATION"),
 P(0,32767,71,k_unit_param_type_none,"SEED"),
 P(0,2,0,k_unit_param_type_strings,"WAVE"),
 P(100,16000,5000,k_unit_param_type_hertz,"TONE"),
 P(2,2000,10,k_unit_param_type_msec,"ATTACK"),
 P(50,8000,1400,k_unit_param_type_msec,"DECAY"),
 P(0,100,20,k_unit_param_type_percent,"FM"),
 P(0,30,7,k_unit_param_type_cents,"DETUNE"),
 P(0,75,30,k_unit_param_type_percent,"ECHO"),
 P(0,85,45,k_unit_param_type_percent,"FEEDBACK"),
 P(0,100,80,k_unit_param_type_percent,"WIDTH"),
 P(0,100,70,k_unit_param_type_percent,"LEVEL"),
 E,E,E,E
 }};
