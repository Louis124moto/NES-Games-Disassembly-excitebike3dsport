#include <stdint.h>
#include <stdio.h>

#include "nes_routines.h"

static int testSubDA58(void);
static int testSubE091(void);
static int testSubE098(void);
static int testSubDFA4(void);
static int testSubDFA2(void);
static int testSubD0AB(void);
static int testSubDFB2(void);
static int testSubD000(void);
static int testSubD11F(void);
static int testSubD255(void);
static int testSubD2FF(void);
static int testSubE70B(void);
static int testSubE7FCFamily(void);
static int testSubE836(void);
static int testNewE16C(void);
static int testNewE3F4(void);
static int testNewE4(void);
static int testNewE2(void);
static int testNewE1F9(void);
static int testNewE3AA(void);

/* Independent instruction-by-instruction 6502 reference for sub_DA15. */
static uint8_t referenceSubDA15(uint8_t ram0390, uint8_t ram0080, uint8_t ramY)
{
    uint8_t a = ram0390;
    unsigned int carry = 1u;
    int difference;
    unsigned int sum;

    /* SEC; SBC ram_0080_obj,X */
    difference = (int)a - (int)ram0080 - (int)(1u - carry);
    a = (uint8_t)difference;
    carry = difference >= 0 ? 1u : 0u;

    /* SBC #$08, consuming the carry from the previous SBC. */
    difference = (int)a - 0x08 - (int)(1u - carry);
    a = (uint8_t)difference;
    carry = difference >= 0 ? 1u : 0u;

    /* ASL; CLC; ADC ram_obj_pos_Y_lo,X */
    a = (uint8_t)(a << 1);
    carry = 0u;
    sum = (unsigned int)a + (unsigned int)ramY + carry;
    a = (uint8_t)sum;
    carry = sum > 0xFFu ? 1u : 0u;

    /* ADC #$04 consumes the carry produced by ADC ramY. */
    sum = (unsigned int)a + 0x04u + carry;
    a = (uint8_t)sum;

    return a;
}

static int testSubD310(void);
static int testSubD35B(void);
static int testSubD326(void);
static int testSubDA7A(void);
static int testSubDBFE(void);
static int testSubDC9A(void);
static int testSubDCDE(void);
static int testSubDD38(void);
static int testSubDD6F(void);
static int testSubDB95(void);
static int testSubDA6A(void);
static int testSubDB50(void);
static int testSubDCA0(void);
static int testLocDCC7(void);



static int testSubE4C8Exact(void)
{
    uint8_t flags[4]={0u,1u,1u,1u};
    uint8_t y[4]={0u,40u,41u,42u};
    uint8_t rel[4]={0u,20u,22u,24u};
    uint8_t packed=0u,found=0xAAu;
    if(nesSubE52CCheckVerticalOverlap(1u,2u,rel)!=(uint8_t)0xFEu) return 1;
    rel[2]=0x50u;
    if(nesSubE52CCheckVerticalOverlap(1u,2u,rel)!=0u) return 1;
    rel[2]=22u;
    nesSubE4C8CheckOverlappingObjects(flags,y,rel,&packed,&found);
    if(found!=1u||packed!=11u) return 1;
    flags[1]=0u; flags[2]=0u; flags[3]=0u;
    packed=0xA5u; found=0xAAu;
    nesSubE4C8CheckOverlappingObjects(flags,y,rel,&packed,&found);
    if(found!=0u||packed!=0xA5u) return 1;
    puts("PASS: sub_E4C8/sub_E52C object-overlap logic and preserved scratch byte.");
    return 0;
}

static int testSubE456Exact(void){
    uint8_t oam[256]={0},oi[4]={0x10u,0x20u,0x30u,0x40u},off[3]={0u,0x30u,0x60u};
    NesSpriteCullState s={0};
    s.ram0088[0]=0u;s.ram0088[1]=1u;s.ram0088[2]=2u;s.ram0088[3]=3u;
    s.ram0084[1]=1u;s.ram0080[1]=0x50u;
    s.ram0084[2]=0u;s.ram0080[2]=0x80u;s.ram00A8[2]=9u;s.ram0058[2]=9u;s.ram0370[2]=9u;
    s.ram0084[3]=0u;s.ram0080[3]=0xFAu;
    nesSubE456CullObjectSprites(oam,&s,oi,off);
    if(s.ram00A8[2]!=0u||s.ram0058[2]!=0u||s.ram0370[2]!=0u||s.ram03A6_flag!=1u){fprintf(stderr,"E456 state fail: A8=%u 58=%u 370=%u flag=%u\\n",s.ram00A8[2],s.ram0058[2],s.ram0370[2],s.ram03A6_flag);return 1;}
    if(oam[0x40]!=0xF8u||oam[0x44]!=0xF8u||oam[0x48]!=0xF8u||oam[0x30]!=0xF8u||oam[0x38]!=0xF8u){fprintf(stderr,"E456 OAM fail: %02X %02X %02X %02X %02X\\n",oam[0x40],oam[0x44],oam[0x48],oam[0x30],oam[0x38]);return 2;}
    if(oam[0x70]!=0u){fprintf(stderr,"E456 unexpected OAM[70]=%02X\\n",oam[0x70]);return 3;}
    puts("PASS: sub_E456 culling, opponent reset, and three-write sprite groups.");
    return 0;
}
static int testSubE26AExact(void){
    uint8_t r84[4]={0u},r390[4]={0u,0x20u,0u,0u},r80[4]={0u,0x10u,0u,0u};
    uint8_t r6c[4]={0u,0x10u,0u,0u},ra4[4]={0u,1u,0u,0u},r398[4]={0u,2u,0u,0u};
    uint8_t tbl[3]={0xBFu,0xC3u,0xC6u};
    NesPreparedSpriteCommand o={0};
    if(!nesSubE26APrepareSpecialSprite(1u,0x50u,r84,r390,r80,r6c,ra4,r398,tbl,0x04u,&o))return 1;
    if(o.scratch0!=0x20u||o.scratch1!=0x98u||o.scratch2!=0xC6u||o.scratch3!=0x22u||o.scratch4!=0x50u||o.scratchB!=0x24u||o.transformY!=1u)return 1;
    if(nesSubE26APrepareSpecialSprite(1u,0x50u,r84,(uint8_t[4]){0u,0xF8u,0u,0u},r80,r6c,ra4,r398,tbl,0u,&o))return 1;
    puts("PASS: sub_E26A gate and exact sprite-command parameters.");return 0;
}
static int testSubE359Exact(void){
    uint8_t timer=0u,frac=0xF0u,coarse=4u,latch=0u,flags=0u,oam[256]={0};
    nesSubE359UpdateTemperatureMeter(0u,0u,0u,&timer,&frac,&coarse,&latch,&flags,oam);
    if(frac!=0x2Fu||coarse!=5u||latch!=0u||oam[0xFC]!=0xCFu||oam[0xFD]!=0xFEu||oam[0xFE]!=0x20u||oam[0xFF]!=0x55u)return 1;
    timer=0u;frac=5u;coarse=9u;latch=0u;flags=0u;
    nesSubE359UpdateTemperatureMeter(0u,0u,0u,&timer,&frac,&coarse,&latch,&flags,oam);
    if(frac!=0xFAu||coarse!=8u)return 1;
    timer=2u;frac=0u;coarse=0x20u;latch=1u;flags=0u;
    nesSubE359UpdateTemperatureMeter(0u,0u,0u,&timer,&frac,&coarse,&latch,&flags,oam);
    if(oam[0xFD]!=0xFCu)return 1;
    timer=1u;frac=0u;coarse=0x20u;latch=1u;flags=0u;
    nesSubE359UpdateTemperatureMeter(1u,0u,0u,&timer,&frac,&coarse,&latch,&flags,oam);
    if(oam[0xFD]!=0xFEu)return 1;
    puts("PASS: sub_E359 heat arithmetic, overheat tile selection, and four OAM sprites.");return 0;
}
static int testSubE2F6Exact(void){
    uint8_t r36[4]={1u,0u,0u,0u},r94[4]={1u,0u,0u,0u},r90[4]={0u},rbc[4]={0u},r84[4]={1u,0u,0u,0u};
    uint8_t r80[4]={0x20u,0u,0u,0u},r8c[4]={0x30u,0u,0u,0u},oam[256]={0};
    nesSubE2F6DrawWheelSprites(0u,0u,r36,r94,r90,rbc,r84,r80,r8c,0u,oam);
    if(oam[0x97]!=0x19u||oam[0x9B]!=0x11u||oam[0x98]!=0x38u||oam[0x94]!=0x40u||oam[0x95]!=0xF6u||oam[0x99]!=0xF7u){fprintf(stderr,"E2F6 primary fail: %02X %02X %02X %02X %02X %02X\\n",oam[0x97],oam[0x9B],oam[0x98],oam[0x94],oam[0x95],oam[0x99]);return 1;}
    for(unsigned i=0;i<256u;i++)oam[i]=0u;
    nesSubE2F6DrawWheelSprites(0u,0u,r36,r94,r90,rbc,r84,r80,r8c,4u,oam);
    if(oam[0x95]!=0xF7u||oam[0x99]!=0xF6u){fprintf(stderr,"E2F6 swap fail: %02X %02X\\n",oam[0x95],oam[0x99]);return 2;}
    r94[0]=0u;r90[0]=1u;for(unsigned i=0;i<256u;i++)oam[i]=0u;
    nesSubE2F6DrawWheelSprites(0u,0u,r36,r94,r90,rbc,r84,r80,r8c,0u,oam);
    if(oam[0x98]!=0xF8u){fprintf(stderr,"E2F6 hide fail: %02X\\n",oam[0x98]);return 3;}
    rbc[0]=1u;for(unsigned i=0;i<256u;i++)oam[i]=0u;
    nesSubE2F6DrawWheelSprites(0u,0u,r36,r94,r90,rbc,r84,r80,r8c,0u,oam);
    if(oam[0x97]!=0u){fprintf(stderr,"E2F6 gate fail: %02X\\n",oam[0x97]);return 4;}
    puts("PASS: sub_E2F6 full wheel-sprite gates and OAM writes.");return 0;
}
static int testSubEA44Exact(void){
    uint8_t r84[4]={1u,1u,0u,0u},r80[4]={0x20u,0xF4u,0u,0u},b0[4]={1u,0u,0u,0u},b4[4]={2u,0u,0u,0u};
    uint8_t bc[4]={5u,0u,0u,0u},ry[4]={1u,0u,0u,0u},tbl[7]={0xCBu,0xCDu,0xCCu,0xCEu,0xCBu,0xCBu,0xCBu};
    uint8_t r36[4]={0u,1u,0u,0u},r94[4]={0u,1u,0u,0u},r90[4]={0u},r8c[4]={0u,0x20u,0u,0u},oam[256]={0};
    nesSubEA44UpdateWheelSprites(oam,r84,r80,b0,b4,bc,ry,tbl,r36,r94,r90,r8c,0u);
    if(oam[0x94]!=0xB0u||oam[0x95]!=0xCCu||oam[0x96]!=0u||oam[0x97]!=0x2Au)return 1;
    if(oam[0x9F]!=0xEDu||oam[0xA3]!=0xE5u||oam[0xA0]!=0x28u)return 1;
    puts("PASS: sub_EA44 special wheel tile path and fallback to sub_E2F6.");return 0;
}
static int testSubE2AFCountdownExact(void){
    uint8_t low=4u,high=1u,blink=0u,tbl[2]={0xE2u,0xE8u};NesPreparedSpriteCommand out={0};
    if(nesSubE2AFUpdateFinishCountdown(1u,1u,0u,&low,&high,&blink,tbl,&out)!=1u)return 1;
    if(low!=3u||high!=1u||blink!=1u||out.scratch0!=3u||out.scratch1!=0x50u||out.scratch2!=0xE8u||out.scratch3!=0x32u||out.scratch4!=0xC0u||out.scratchB!=0u)return 1;
    low=2u;high=1u;blink=0u;
    if(nesSubE2AFUpdateFinishCountdown(3u,0u,0u,&low,&high,&blink,tbl,&out)!=2u||low!=0xFFu||high!=0u)return 1;
    low=5u;high=1u;blink=0u;
    if(nesSubE2AFUpdateFinishCountdown(1u,2u,0u,&low,&high,&blink,tbl,&out)!=0u||low!=4u||blink!=0u)return 1;
    puts("PASS: countdown decrement/borrow, finish blink, and draw-command branch.");return 0;
}
static int testSubE1A3Exact(void){
    uint8_t z[4]={0u},r9c[4]={1u,0u,0u,0u},r36[4]={0u},rac[4]={3u},r80[4]={0u,0x20u,0u,0u},r8c[4]={0u,0x30u,0u,0u};
    uint8_t d892[23],d8a9[23],oam[256]={0},ram039C3=0xFFu,temp=0xFFu,render=0u;NesPreparedSpriteCommand out={0};
    for(unsigned i=0;i<23u;i++){d892[i]=(uint8_t)(0x40u+i);d8a9[i]=(uint8_t)(0x80u+i);}
    if(!nesSubE1A3PrepareObjectSprite(1u,0x20u,r80,r8c,r9c,z,z,r36,rac,z,z,z,z,z,z,z,0u,0u,d892,d8a9,oam,&ram039C3,&temp,&render,&out))return 1;
    if(out.scratch0!=0x20u||out.scratch1!=0x38u||out.scratch2!=d892[0x16]||out.scratch3!=0x23u||out.scratch4!=0x2Cu||ram039C3!=0x20u||render!=d8a9[0x16])return 1;
    r9c[1]=0u;rac[1]=0x0Au;for(unsigned i=0;i<256u;i++)oam[i]=0u;
    if(!nesSubE1A3PrepareObjectSprite(1u,0x40u,r80,r8c,r9c,z,z,r36,rac,z,z,z,z,z,z,z,1u,0u,d892,d8a9,oam,&ram039C3,&temp,&render,&out))return 1;
    if(temp!=2u||oam[0x51]!=0xB2u||oam[0x4D]!=0xB1u||oam[0x45]!=0xB0u||oam[0x41]!=0xAFu)return 1;
    puts("PASS: sub_E1A3 table selection, special offset, and overlay tiles.");return 0;
}
int main(void)
{
    if(testSubE1A3Exact()) { fputs("FAIL: sub_E1A3 reference test\\n",stderr); return 1; }
    if(testSubE2AFCountdownExact()) { fputs("FAIL: E2AF countdown reference test\\n",stderr); return 1; }
    if(testSubEA44Exact()) { fputs("FAIL: sub_EA44 reference test\\n",stderr); return 1; }
    if(testSubE2F6Exact()) { fputs("FAIL: sub_E2F6 reference test\\n",stderr); return 1; }
    if(testSubE359Exact()) { fputs("FAIL: sub_E359 reference test\\n",stderr); return 1; }
    if(testSubE26AExact()) { fputs("FAIL: sub_E26A reference test\\n",stderr); return 1; }
    if(testSubE456Exact()) { fputs("FAIL: sub_E456 reference test\\n",stderr); return 1; }
    if(testSubE4C8Exact()) { fputs("FAIL: sub_E4C8 exact reference test\\n",stderr); return 1; }
    unsigned int x, y, z;
    unsigned long checked = 0;

    for (x = 0; x < 256u; ++x) {
        for (y = 0; y < 256u; ++y) {
            for (z = 0; z < 256u; ++z) {
                uint8_t expected = referenceSubDA15((uint8_t)x, (uint8_t)y, (uint8_t)z);
                uint8_t actual = nesSubDA15DerivedY((uint8_t)x, (uint8_t)y, (uint8_t)z);
                if (actual != expected) {
                    fprintf(stderr,
                            "Mismatch: ram0390=%02X ram0080=%02X ramY=%02X expected=%02X actual=%02X\n",
                            x, y, z, expected, actual);
                    return 1;
                }
                ++checked;
            }
        }
    }

    printf("PASS: %lu input combinations matched the 6502 reference.\n", checked);
    if (testSubD310() != 0) return 1;
    if (testSubD35B() != 0) return 1;
    if (testSubD326() != 0) return 1;
    if (testSubDA7A() != 0) return 1;
    if (testSubDBFE() != 0) return 1;
    if (testSubDC9A() != 0) return 1;
    if (testSubDCDE() != 0) return 1;
    if (testSubDD38() != 0) return 1;
    if (testSubDD6F() != 0) return 1;
    if (testSubDB95() != 0) return 1;
    if (testSubDA6A() != 0) return 1;
    if (testSubDB50() != 0) return 1;
    if (testSubDCA0() != 0) return 1;
    if (testLocDCC7() != 0) return 1;
    if (testSubDA58() != 0) return 1;
    if (testSubE091() != 0) return 1;
    if (testSubE098() != 0) return 1;
    if (testSubD000() != 0) return 1;
    if (testSubD11F() != 0) return 1;
    if (testSubD255() != 0) return 1;
    if (testSubD2FF() != 0) return 1;
    if (testSubE70B() != 0) return 1;
    if (testSubE7FCFamily() != 0) return 1;
    if (testSubE836() != 0) return 1;
    if (testSubDFA4() != 0) return 1;
    if (testSubDFA2() != 0) return 1;
    if (testSubD0AB() != 0) return 1;
    if (testSubDFB2() != 0) return 1;
    if(testNewE16C()!=0||testNewE3F4()!=0||testNewE4()!=0||testNewE2()!=0||testNewE1F9()!=0||testNewE3AA()!=0)return 1;
    return 0;
}


/* Independent reference for sub_D310; model DEC/N flags and X loop bounds. */
static void referenceSubD310(uint8_t *timer0020, uint8_t timers[29])
{
    uint8_t decremented = (uint8_t)(*timer0020 - 1u);
    int x = 0x0E;

    *timer0020 = decremented;
    if ((decremented & 0x80u) != 0u) {
        *timer0020 = 0x0Au;
        x = 0x1C;
    }

    for (; x >= 0; --x) {
        if (timers[x] != 0u) {
            timers[x] = (uint8_t)(timers[x] - 1u);
        }
    }
}

static int testSubD310(void)
{
    unsigned int frame, timer_value;
    unsigned long checked = 0;

    for (frame = 0; frame < 256u; ++frame) {
        for (timer_value = 0; timer_value < 256u; ++timer_value) {
            uint8_t actual_frame = (uint8_t)frame;
            uint8_t expected_frame = (uint8_t)frame;
            uint8_t actual_timers[29];
            uint8_t expected_timers[29];
            unsigned int i;

            for (i = 0; i < 29u; ++i) {
                actual_timers[i] = (uint8_t)(timer_value + i * 37u + frame * 13u);
                expected_timers[i] = actual_timers[i];
            }
            if (((frame ^ timer_value) & 1u) != 0u) {
                for (i = 0; i < 29u; ++i) {
                    actual_timers[i] = (uint8_t)timer_value;
                    expected_timers[i] = (uint8_t)timer_value;
                }
            }

            referenceSubD310(&expected_frame, expected_timers);
            nesSubD310DecreaseAllTimers(&actual_frame, actual_timers);
            if (actual_frame != expected_frame) {
                fprintf(stderr, "sub_D310 frame mismatch: in=%02X expected=%02X actual=%02X\n",
                        frame, expected_frame, actual_frame);
                return 1;
            }
            for (i = 0; i < 29u; ++i) {
                if (actual_timers[i] != expected_timers[i]) {
                    fprintf(stderr,
                            "sub_D310 timer mismatch: frame=%02X value=%02X slot=%u expected=%02X actual=%02X\n",
                            frame, timer_value, i, expected_timers[i], actual_timers[i]);
                    return 1;
                }
            }
            ++checked;
        }
    }
    printf("PASS: sub_D310 matched reference for %lu frame/timer vectors.\n", checked);
    return 0;
}


/* Independent reference for sub_D326, preserving the initial SEC decision
 * and the carry chain across each memory ROR. */
static void referenceSubD326(uint8_t bytes[8])
{
    unsigned int carry = 0u;
    unsigned int i;
    uint8_t tmp0 = (uint8_t)(bytes[0] & 0x02u);
    uint8_t tmp1 = (uint8_t)(bytes[1] & 0x02u);
    uint8_t different = (uint8_t)(tmp0 ^ tmp1);

    if (different != 0u) carry = 1u;
    for (i = 0u; i < 8u; ++i) {
        uint8_t value = bytes[i];
        uint8_t outgoing = (uint8_t)(value & 1u);
        bytes[i] = (uint8_t)((value >> 1) | (uint8_t)(carry << 7));
        carry = outgoing;
    }
}

static int testSubD326(void)
{
    unsigned int first, second, pattern;
    unsigned long checked = 0;

    for (first = 0; first < 256u; ++first) {
        for (second = 0; second < 256u; ++second) {
            uint8_t actual[8];
            uint8_t expected[8];
            unsigned int i;
            for (i = 0; i < 8u; ++i) {
                actual[i] = (uint8_t)(first + second * (i + 1u) + i * 29u);
                expected[i] = actual[i];
            }
            actual[0] = expected[0] = (uint8_t)first;
            actual[1] = expected[1] = (uint8_t)second;
            pattern = (first ^ second) & 3u;
            if (pattern == 1u) {
                for (i = 2; i < 8u; ++i) actual[i] = expected[i] = (uint8_t)first;
            } else if (pattern == 2u) {
                for (i = 2; i < 8u; ++i) actual[i] = expected[i] = (uint8_t)(255u - second);
            } else if (pattern == 3u) {
                for (i = 2; i < 8u; ++i) actual[i] = expected[i] = (uint8_t)(i & 1u ? 0xFFu : 0x00u);
            }
            referenceSubD326(expected);
            nesSubD326RotateInputBits(actual);
            for (i = 0; i < 8u; ++i) {
                if (actual[i] != expected[i]) {
                    fprintf(stderr,
                            "sub_D326 mismatch: first=%02X second=%02X slot=%u expected=%02X actual=%02X\n",
                            first, second, i, expected[i], actual[i]);
                    return 1;
                }
            }
            ++checked;
        }
    }
    printf("PASS: sub_D326 matched reference for %lu input vectors.\n", checked);
    return 0;
}


/* Reference model for the RAM-level decode and A-button repeat suppression. */
static void referenceSubD35B(const uint8_t serial_samples[8], uint8_t previous_hold,
                             uint8_t *hold, uint8_t *press)
{
    uint8_t accumulator = 0u;
    unsigned int i;

    for (i = 0; i < 8u; ++i) {
        uint8_t saved = serial_samples[i];
        uint8_t shifted = (uint8_t)(saved >> 1);
        uint8_t combined = (uint8_t)(shifted | saved);
        uint8_t carry = (uint8_t)(combined & 1u);
        accumulator = (uint8_t)((accumulator << 1) | carry);
    }

    *hold = accumulator;
    *press = accumulator;
    if ((*hold & 0x80u) != 0u && (previous_hold & 0x80u) != 0u) {
        *press = (uint8_t)(accumulator & 0x7Fu);
    }
}

static int testSubD35B(void)
{
    unsigned int mask, previous, encoding;
    unsigned long checked = 0;

    for (mask = 0; mask < 256u; ++mask) {
        for (previous = 0; previous < 256u; ++previous) {
            for (encoding = 0; encoding < 3u; ++encoding) {
                uint8_t samples[8];
                uint8_t actual_hold, actual_press, expected_hold, expected_press;
                unsigned int i;
                for (i = 0; i < 8u; ++i) {
                    uint8_t active = (uint8_t)((mask >> (7u - i)) & 1u);
                    samples[i] = active ? (uint8_t)(encoding + 1u) : 0u;
                }
                referenceSubD35B(samples, (uint8_t)previous, &expected_hold, &expected_press);
                nesSubD35BDecodeController(samples, (uint8_t)previous, &actual_hold, &actual_press);
                if (actual_hold != expected_hold || actual_press != expected_press) {
                    fprintf(stderr,
                            "sub_D35B mismatch: mask=%02X previous=%02X encoding=%u hold=%02X/%02X press=%02X/%02X\n",
                            mask, previous, encoding, expected_hold, actual_hold, expected_press, actual_press);
                    return 1;
                }
                ++checked;
            }
        }
    }
    printf("PASS: sub_D35B matched reference for %lu controller vectors.\n", checked);
    return 0;
}


/* Independent reference: model each 6502 instruction and its carry flag. */
static void referenceSubDA7A(uint8_t active[4], const uint8_t delta[4],
                             uint8_t posXLo[4], uint8_t posXHi[4],
                             uint8_t ram0390[4])
{
    int x;
    for (x = 3; x >= 1; --x) {
        uint8_t a;
        unsigned int carry;
        int difference;
        if (active[x] == 0u) continue;

        a = delta[x];
        carry = 0u; /* CLC */
        {
            unsigned int sum = (unsigned int)a + posXLo[x] + carry;
            a = (uint8_t)sum;
            carry = sum > 0xFFu ? 1u : 0u;
        }
        if (carry != 0u) posXHi[x] = (uint8_t)(posXHi[x] + 1u);

        carry = 1u; /* SEC */
        difference = (int)a - (int)delta[0] - (int)(1u - carry);
        a = (uint8_t)difference;
        carry = difference >= 0 ? 1u : 0u;
        posXLo[x] = a;
        if (carry == 0u) posXHi[x] = (uint8_t)(posXHi[x] - 1u);

        carry = 1u; /* SEC before SBC ram_0060 */
        difference = (int)ram0390[x] - (int)delta[0] - (int)(1u - carry);
        ram0390[x] = (uint8_t)difference;
    }
}

static int testSubDA7A(void)
{
    unsigned int pattern;
    unsigned long checked = 0u;

    /* Deterministic exhaustive byte pairs for delta[0], delta[x], posXLo[x],
       plus edge patterns in the high and 0390 bytes and active flags. */
    for (pattern = 0; pattern < 65536u; ++pattern) {
        uint8_t activeA[4], activeB[4], deltaA[4], deltaB[4];
        uint8_t loA[4], loB[4], hiA[4], hiB[4], rA[4], rB[4];
        unsigned int i;
        deltaA[0] = deltaB[0] = (uint8_t)(pattern >> 8);
        for (i = 1; i < 4u; ++i) {
            deltaA[i] = deltaB[i] = (uint8_t)(pattern + i * 53u);
            loA[i] = loB[i] = (uint8_t)((pattern >> 1) + i * 71u);
            hiA[i] = hiB[i] = (uint8_t)((pattern >> 3) + i * 29u);
            rA[i] = rB[i] = (uint8_t)((pattern >> 5) + i * 97u);
            activeA[i] = activeB[i] = (uint8_t)(((pattern >> (i - 1u)) & 1u) ? 1u : 0u);
        }
        activeA[0] = activeB[0] = 0u;
        loA[0] = loB[0] = 0u;
        hiA[0] = hiB[0] = 0u;
        rA[0] = rB[0] = 0u;

        referenceSubDA7A(activeA, deltaA, loA, hiA, rA);
        nesSubDA7AUpdateScrollSlots(activeB, deltaB, loB, hiB, rB);
        for (i = 0; i < 4u; ++i) {
            if (loA[i] != loB[i] || hiA[i] != hiB[i] || rA[i] != rB[i]) {
                fprintf(stderr, "sub_DA7A mismatch: pattern=%u slot=%u\n", pattern, i);
                return 1;
            }
        }
        ++checked;
    }
    printf("PASS: sub_DA7A matched instruction reference for %lu patterns.\n", checked);
    return 0;
}


static uint8_t refSBC(uint8_t a, uint8_t value, unsigned int *carry)
{
    int d = (int)a - (int)value - (int)(1u - *carry);
    *carry = d >= 0 ? 1u : 0u;
    return (uint8_t)d;
}

static int testSubDC9A(void)
{
    unsigned int a, b, c;
    unsigned long checked = 0u;
    for (a = 0; a < 256u; ++a) {
        for (b = 0; b < 256u; ++b) {
            for (c = 0; c < 256u; ++c) {
                unsigned int carry = 1u;
                uint8_t expected = refSBC((uint8_t)a, (uint8_t)b, &carry);
                expected = refSBC(expected, (uint8_t)c, &carry);
                if (nesSubDC9A((uint8_t)a, (uint8_t)b, (uint8_t)c) != expected) {
                    fprintf(stderr, "sub_DC9A mismatch: %02X %02X %02X\n", a, b, c);
                    return 1;
                }
                ++checked;
            }
        }
    }
    printf("PASS: sub_DC9A matched 6502 SBC chain for %lu triples.\n", checked);
    return 0;
}

static int testSubDBFE(void)
{
    unsigned int a, b;
    unsigned long checked = 0u;
    for (a = 0; a < 256u; ++a) {
        for (b = 0; b < 256u; ++b) {
            uint8_t actualBf = (uint8_t)b, expectedBf = (uint8_t)b;
            uint8_t actualScroll = (uint8_t)(a * 37u + b), expectedScroll = actualScroll;
            uint8_t actualNt = (uint8_t)((a ^ b) & 1u), expectedNt = actualNt;
            uint8_t expectedA = (uint8_t)a;
            unsigned int carry = expectedA & 1u;
            unsigned int i, sum;
            expectedA = (uint8_t)(expectedA >> 1);
            for (i = 0; i < 3u; ++i) {
                unsigned int next = expectedA & 1u;
                expectedA = (uint8_t)((expectedA >> 1) | (uint8_t)(carry << 7));
                carry = next;
            }
            sum = (unsigned int)expectedA + expectedBf;
            expectedBf = (uint8_t)sum;
            carry = sum > 0xFFu ? 1u : 0u;
            sum = (unsigned int)(uint8_t)a + expectedScroll + carry;
            expectedScroll = (uint8_t)sum;
            if (sum > 0xFFu) expectedNt = (uint8_t)(expectedNt ^ 1u);
            nesSubDBFEUpdateScroll((uint8_t)a, &actualBf, &actualScroll, &actualNt);
            if (actualBf != expectedBf || actualScroll != expectedScroll || actualNt != expectedNt) {
                fprintf(stderr, "sub_DBFE mismatch: %02X %02X\n", a, b);
                return 1;
            }
            ++checked;
        }
    }
    printf("PASS: sub_DBFE matched instruction reference for %lu pairs.\n", checked);
    return 0;
}

static int testSubDCDE(void)
{
    unsigned int timer, sample;
    unsigned long checked = 0u;
    for (timer = 0; timer < 256u; ++timer) {
        for (sample = 0; sample < 256u; ++sample) {
            uint8_t actual = (uint8_t)timer;
            uint8_t expected = actual;
            if (expected != 0u) {
                expected = (uint8_t)(expected - 1u);
                if ((sample & 0xC0u) != 0u) expected = 0u;
            }
            nesSubDCDE(&actual, (uint8_t)sample);
            if (actual != expected) {
                fprintf(stderr, "sub_DCDE mismatch: timer=%02X sample=%02X\n", timer, sample);
                return 1;
            }
            ++checked;
        }
    }
    printf("PASS: sub_DCDE matched timer/flag reference for %lu pairs.\n", checked);
    return 0;
}

static int testSubDD38(void)
{
    unsigned int x, lo, hi, mode;
    unsigned long checked = 0u;
    for (x = 0; x < 4u; ++x) {
        for (lo = 0; lo < 256u; ++lo) {
            for (hi = 0; hi < 256u; ++hi) {
                for (mode = 0; mode < 2u; ++mode) {
                    uint8_t r90[4] = {0x11u, 0x22u, 0x33u, 0x44u};
                    uint8_t r94[4] = {0u, 0u, 0u, 0u};
                    uint8_t r388[4] = {0u, 0u, 0u, 0u};
                    uint8_t r378[4] = {0u, 0u, 0u, 0u};
                    uint8_t r37c[4] = {0u, 0u, 0u, 0u};
                    uint8_t r380[4] = {0u, 0u, 0u, 0u};
                    uint8_t r00b0[4] = {0u, 0u, 0u, 0u};
                    uint8_t rff = 0xA5u;
                    unsigned int sum, carry;
                    uint8_t expectedLo, expectedHi;
                    r90[x] = (uint8_t)lo;
                    r94[x] = (uint8_t)hi;
                    r388[x] = mode ? 2u : 0u;
                    sum = lo + 0xAFu;
                    expectedLo = (uint8_t)sum;
                    carry = sum > 0xFFu ? 1u : 0u;
                    expectedHi = (uint8_t)(hi + carry);
                    if (mode) {
                        carry = expectedHi & 1u;
                        expectedHi = (uint8_t)(expectedHi >> 1);
                        expectedLo = (uint8_t)((expectedLo >> 1) | (uint8_t)(carry << 7));
                    }
                    nesSubDD38SetupObject((uint8_t)x, r90, r94, r388, r378, r37c, r380, r00b0, &rff);
                    if (r378[x] != expectedLo || r37c[x] != expectedHi ||
                        r380[x] != 0x0Fu || r00b0[x] != 2u) {
                        fprintf(stderr, "sub_DD38 mismatch: x=%u lo=%02X hi=%02X mode=%u\n", x, lo, hi, mode);
                        return 1;
                    }
                    if (x == 0u) {
                        uint8_t e = (uint8_t)(((uint8_t)hi << 4) ^ 0x30u);
                        if (rff != (e != 0u ? e : 0x08u)) {
                            fprintf(stderr, "sub_DD38 ram_00FF mismatch: hi=%02X\n", hi);
                            return 1;
                        }
                    } else if (rff != 0xA5u) {
                        fprintf(stderr, "sub_DD38 unexpectedly changed ram_00FF for X=%u\n", x);
                        return 1;
                    }
                    ++checked;
                }
            }
        }
    }
    printf("PASS: sub_DD38 matched reference for %lu slot/value/mode cases.\n", checked);
    return 0;
}

static int testSubDD6F(void)
{
    unsigned int pattern, x, carryIn;
    unsigned long checked = 0u;
    for (pattern = 0; pattern < 65536u; ++pattern) {
        for (x = 0; x < 4u; ++x) {
            for (carryIn = 0; carryIn < 2u; ++carryIn) {
                uint8_t r380[4], r38c[4], r384[4], r37c[4];
                uint8_t e380, e384, a, expected;
                unsigned int sum, carry = carryIn;
                int d;
                unsigned int i;
                for (i = 0; i < 4u; ++i) {
                    r380[i] = (uint8_t)(pattern + i * 43u);
                    r38c[i] = (uint8_t)((pattern >> 1) + i * 71u);
                    r384[i] = (uint8_t)((pattern >> 3) + i * 19u);
                    r37c[i] = (uint8_t)((pattern >> 5) + i * 97u);
                }
                e380 = (uint8_t)((unsigned int)r380[x] + r38c[x] + carry);
                sum = (unsigned int)r380[x] + r38c[x] + carry;
                carry = sum > 0xFFu ? 1u : 0u;
                sum = (unsigned int)r384[x] + carry;
                e384 = (uint8_t)sum;
                carry = sum > 0xFFu ? 1u : 0u;
                d = (int)(uint8_t)pattern - (int)r37c[x] - (int)(1u - carry);
                a = (uint8_t)d;
                carry = d >= 0 ? 1u : 0u;
                sum = (unsigned int)a + e384 + carry;
                expected = (uint8_t)sum;
                if (nesSubDD6F((uint8_t)x, (uint8_t)carryIn, r380, r38c, r384, r37c, (uint8_t)pattern) != expected ||
                    r380[x] != e380 || r384[x] != e384) {
                    fprintf(stderr, "sub_DD6F mismatch: pattern=%u x=%u carry=%u\n", pattern, x, carryIn);
                    return 1;
                }
                ++checked;
            }
        }
    }
    printf("PASS: sub_DD6F matched reference for %lu state combinations.\n", checked);
    return 0;
}

static int testSubDB95(void)
{
    unsigned int x, i;
    for (x = 1u; x <= 3u; ++x) {
        uint8_t arrays[20][4];
        uint8_t *p[20];
        for (i = 0; i < 20u; ++i) {
            unsigned int j;
            p[i] = arrays[i];
            for (j = 0; j < 4u; ++j) arrays[i][j] = (uint8_t)(0x20u + i * 7u + j);
        }
        nesSubDB95ClearOpponentBikeData((uint8_t)x,
            p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[9],
            p[10],p[11],p[12],p[13],p[14],p[15],p[16],p[17],p[18],p[19]);
        for (i = 0; i < 20u; ++i) {
            unsigned int j;
            for (j = 0; j < 4u; ++j) {
                uint8_t expected = (j == x) ? 0u : (uint8_t)(0x20u + i * 7u + j);
                if (arrays[i][j] != expected) {
                    fprintf(stderr, "sub_DB95 mismatch: field=%u slot=%u\n", i, j);
                    return 1;
                }
            }
        }
    }
    printf("PASS: sub_DB95 clears exactly the 20 documented fields for slots 1..3.\n");
    return 0;
}


static int testSubDA6A(void)
{
    unsigned int scroll, delta, nt;
    unsigned long checked = 0u;
    for (scroll = 0; scroll < 256u; ++scroll) {
        for (delta = 0; delta < 256u; ++delta) {
            for (nt = 0; nt < 2u; ++nt) {
                uint8_t actualScroll = (uint8_t)scroll;
                uint8_t actualNt = (uint8_t)nt;
                unsigned int sum = scroll + delta;
                uint8_t expectedScroll = (uint8_t)sum;
                uint8_t expectedNt = (uint8_t)(nt ^ (sum > 0xFFu ? 1u : 0u));
                nesSubDA6AUpdateScroll(&actualScroll, (uint8_t)delta, &actualNt);
                if (actualScroll != expectedScroll || actualNt != expectedNt) {
                    fprintf(stderr, "sub_DA6A mismatch: scroll=%02X delta=%02X nt=%u\n", scroll, delta, nt);
                    return 1;
                }
                ++checked;
            }
        }
    }
    printf("PASS: sub_DA6A matched 131072 scroll/carry cases.\n");
    return 0;
}

static int testSubDCA0(void)
{
    unsigned int x, index, finished, flags;
    unsigned long checked = 0u;
    for (x = 0; x < 4u; ++x) {
        for (index = 0; index < 256u; ++index) {
            for (finished = 0; finished < 2u; ++finished) {
                for (flags = 0; flags < 8u; ++flags) {
                    uint8_t y[4] = {0u, 0u, 0u, 0u};
                    uint8_t a4[4] = {0u, 0u, 0u, 0u};
                    uint8_t h[4] = {0u, 0u, 0u, 0u};
                    uint8_t table[256];
                    uint8_t expected;
                    unsigned int i;
                    for (i = 0; i < 256u; ++i) table[i] = (uint8_t)(i ^ 0x5Au);
                    y[x] = (uint8_t)index;
                    a4[x] = (uint8_t)((flags & 1u) ? 1u : ((flags & 2u) ? 2u : 0u));
                    expected = (finished == 1u && x == 0u && flags == 0u) ? 0x0Au : table[index];
                    if (a4[x] == 1u) expected = 0x06u;
                    nesSubDCA0SetupSpriteHeight((uint8_t)x, y, (uint8_t)finished,
                        (uint8_t)((flags & 2u) ? 1u : 0u),
                        (uint8_t)((flags & 4u) ? 1u : 0u),
                        (uint8_t)((flags & 1u) ? 1u : 0u), a4, table, h);
                    if (h[x] != expected) {
                        fprintf(stderr, "sub_DCA0 mismatch: x=%u index=%u finished=%u flags=%u\n", x, index, finished, flags);
                        return 1;
                    }
                    ++checked;
                }
            }
        }
    }
    printf("PASS: sub_DCA0 matched %lu table/flag combinations.\n", checked);
    return 0;
}

static int testSubDB50(void)
{
    unsigned int x, scenario;
    uint8_t d8d5[256], d8cd[16];
    unsigned int i;
    unsigned long checked = 0u;
    for (i = 0; i < 256u; ++i) d8d5[i] = (uint8_t)(i ^ 0xA5u);
    for (i = 0; i < 16u; ++i) d8cd[i] = (uint8_t)(i * 7u);
    for (x = 0; x < 4u; ++x) {
        for (scenario = 0; scenario < 256u; ++scenario) {
            uint8_t r84[4], r1b[4], r70[4], r78[4], r74[4], r80[4];
            uint8_t r94 = (uint8_t)(scenario ^ 0x37u);
            uint8_t r90 = (uint8_t)(scenario ^ 0x92u);
            uint8_t finished = (uint8_t)((scenario >> 7) & 1u);
            uint8_t y = (uint8_t)(scenario & 0x0Fu);
            uint8_t expected78, expected74;
            for (i = 0; i < 4u; ++i) {
                r84[i] = (uint8_t)(scenario + i * 29u);
                r1b[i] = (uint8_t)(scenario + i * 17u);
                r70[i] = (uint8_t)(scenario + i * 31u);
                r78[i] = (uint8_t)(scenario ^ (i * 11u));
                r74[i] = (uint8_t)(scenario ^ (i * 23u));
                r80[i] = (uint8_t)(scenario + i * 47u);
            }
            expected78 = finished ? 3u : d8d5[y];
            expected74 = d8cd[r1b[x] & 0x0Fu];
            if (!finished && x == 3u && (r84[x] & 2u) == 0u && r94 != 0u &&
                (r84[x] == 0u || r80[x] < 0x40u)) {
                expected78 = r94;
                expected74 = r90;
                if (!(r84[x] != 0u && r70[0] == r70[3])) {
                    if (expected78 < 4u) expected78 = (uint8_t)(expected78 + 1u);
                } else {
                    expected78 = r94; /* assembly returns after initial stores */
                }
            }
            nesSubDB50SetupObject((uint8_t)x, y, finished, r84, r1b, r70, r78, r74, r80,
                                  r94, r90, d8d5, d8cd);
            if (r78[x] != expected78 || r74[x] != expected74) {
                fprintf(stderr, "sub_DB50 mismatch: x=%u scenario=%u\n", x, scenario);
                return 1;
            }
            ++checked;
        }
    }
    printf("PASS: sub_DB50 matched branch/table reference for %lu cases.\n", checked);
    return 0;
}


static int testLocDCC7(void)
{
    unsigned int x, target, current, timerMode;
    unsigned long checked = 0u;
    for (x = 0; x < 4u; ++x) {
        for (target = 0; target < 256u; ++target) {
            for (current = 0; current < 256u; ++current) {
                for (timerMode = 0; timerMode < 2u; ++timerMode) {
                    uint8_t timers[4] = {0u, 0u, 0u, 0u};
                    uint8_t values[4] = {0u, 0u, 0u, 0u};
                    uint8_t duration = (uint8_t)(target ^ 0xA7u);
                    uint8_t expected = (uint8_t)current;
                    unsigned int i;
                    for (i = 0; i < 4u; ++i) {
                        timers[i] = 0u;
                        values[i] = (uint8_t)(0x31u + i);
                    }
                    timers[x] = timerMode ? 1u : 0u;
                    values[x] = (uint8_t)current;
                    if (timerMode == 0u && current != target) {
                        if (current > target) expected = (uint8_t)(expected - 1u);
                        else expected = (uint8_t)(expected + 1u);
                    }
                    nesLocDCC7ApproachValue((uint8_t)x, (uint8_t)target, duration, timers, values);
                    if (values[x] != expected ||
                        timers[x] != (timerMode ? 1u : duration)) {
                        fprintf(stderr, "loc_DCC7 mismatch: x=%u target=%02X current=%02X timerMode=%u\n",
                                x, target, current, timerMode);
                        return 1;
                    }
                    ++checked;
                }
            }
        }
    }
    printf("PASS: loc_DCC7 matched %lu timer/target/value cases.\n", checked);
    return 0;
}


/* Independent arithmetic reference for sub_DA58. */
static int testSubDA58(void)
{
    unsigned int x, fraction, velocity;
    unsigned long checked = 0u;
    for (x = 0; x < 4u; ++x) {
        for (fraction = 0; fraction < 256u; ++fraction) {
            for (velocity = 0; velocity < 256u; ++velocity) {
                unsigned int carryByte = (fraction * 17u + velocity * 29u) & 0xFFu;
                    uint8_t r94[4] = {0x11u,0x22u,0x33u,0x44u};
                    uint8_t r60[4] = {0x55u,0x66u,0x77u,0x88u};
                    uint8_t r90[4] = {0u,0u,0u,0u};
                    uint8_t r394[4] = {0u,0u,0u,0u};
                    uint8_t expected60, expected394;
                    unsigned int sum;
                    r94[x] = (uint8_t)carryByte;
                    r90[x] = (uint8_t)velocity;
                    r394[x] = (uint8_t)fraction;
                    sum = velocity + fraction;
                    expected60 = (uint8_t)(carryByte + (sum > 0xFFu ? 1u : 0u));
                    expected394 = (uint8_t)sum;
                    nesSubDA58UpdateObjectFraction((uint8_t)x, r94, r60, r90, r394);
                    if (r60[x] != expected60 || r394[x] != expected394) {
                        fprintf(stderr, "sub_DA58 mismatch: x=%u fraction=%u velocity=%u base=%u\n",
                                x, fraction, velocity, carryByte);
                        return 1;
                    }
                ++checked;
            }
        }
    }
    printf("PASS: sub_DA58 matched byte/carry reference for %lu cases.\n", checked);
    return 0;
}

static int testSubE091(void)
{
    unsigned int value;
    for (value = 0; value < 256u; ++value) {
        uint8_t expected = (uint8_t)value;
        if (value < 0xE7u) expected = (uint8_t)(value + 0x0Cu);
        if (nesSubE091AdjustValue((uint8_t)value) != expected) {
            fprintf(stderr, "sub_E091 mismatch: value=%02X\n", value);
            return 1;
        }
    }
    puts("PASS: sub_E091 matched all 256 CMP/ADC input values.");
    return 0;
}

static int testSubE098(void)
{
    unsigned int value;
    for (value = 0; value < 256u; ++value) {
        uint8_t expected = (uint8_t)value;
        if (value < 0xE7u) expected = (uint8_t)(value + 0x02u);
        if (nesSubE098AdjustValue((uint8_t)value) != expected) {
            fprintf(stderr, "sub_E098 mismatch: value=%02X\n", value);
            return 1;
        }
    }
    puts("PASS: sub_E098 matched all 256 CMP/ADC input values.");
    return 0;
}


/* Independent 6502 register/flag reference for sub_DFA4. */
static void referenceSubDFA4(uint8_t inputA, uint8_t divisor,
                             uint8_t *outX, uint8_t *outA)
{
    uint8_t a = inputA;
    uint8_t x = 0u;
    uint8_t carry;
    do {
        uint16_t difference;
        x = (uint8_t)(x + 1u);
        difference = (uint16_t)a - (uint16_t)divisor;
        a = (uint8_t)difference;
        carry = (uint8_t)(difference <= 0xFFu);
    } while ((a & 0x80u) == 0u);
    x = (uint8_t)(x - 1u);
    a = (uint8_t)((uint16_t)a + (uint16_t)divisor + (uint16_t)carry);
    *outX = x;
    *outA = a;
}

static int testSubDFA4(void)
{
    unsigned int a, divisor;
    unsigned long checked = 0u;
    for (a = 0; a < 256u; ++a) {
        for (divisor = 1; divisor < 256u; ++divisor) {
            uint8_t expectedX, expectedA, actualX = 0u, actualA = 0u;
            referenceSubDFA4((uint8_t)a, (uint8_t)divisor, &expectedX, &expectedA);
            nesSubDFA4((uint8_t)a, (uint8_t)divisor, &actualX, &actualA);
            if (actualX != expectedX || actualA != expectedA) {
                fprintf(stderr, "sub_DFA4 mismatch: A=%02X divisor=%02X expected X/A=%02X/%02X actual=%02X/%02X\n",
                        a, divisor, expectedX, expectedA, actualX, actualA);
                return 1;
            }
            ++checked;
        }
    }
    printf("PASS: sub_DFA4 matched instruction-level reference for %lu input pairs.\n", checked);
    return 0;
}

static int testSubDFA2(void)
{
    unsigned int a;
    for (a = 0; a < 256u; ++a) {
        uint8_t expectedX, expectedA, actualX = 0u, actualA = 0u;
        referenceSubDFA4((uint8_t)a, 0x0Au, &expectedX, &expectedA);
        nesSubDFA2((uint8_t)a, &actualX, &actualA);
        if (actualX != expectedX || actualA != expectedA) {
            fprintf(stderr, "sub_DFA2 mismatch: A=%02X\n", a);
            return 1;
        }
    }
    puts("PASS: sub_DFA2 matched sub_DFA4 with X initialized to $0A for all 256 A values.");
    return 0;
}


static uint8_t referenceD0ABOne(uint8_t input)
{
    uint8_t a = (uint8_t)(input - 0x10u);
    uint8_t y = 5u;
    if ((a & 0x80u) == 0u) {
        do {
            y = (uint8_t)(y - 1u);
            if (y == 0u) break;
            a = (uint8_t)(a - 0x08u);
        } while ((a & 0x80u) == 0u);
    }
    return y;
}

static int testSubD0AB(void)
{
    unsigned int value, x;
    for (value = 0; value < 256u; ++value) {
        uint8_t input[4] = {0x00u,0x01u,0x02u,0x03u};
        uint8_t output[4] = {0xAAu,0xAAu,0xAAu,0xAAu};
        input[0] = (uint8_t)value;
        input[1] = (uint8_t)value;
        input[2] = (uint8_t)value;
        input[3] = (uint8_t)value;
        nesSubD0ABComputeVerticalBands(input, output);
        for (x = 0; x < 4u; ++x) {
            uint8_t expected = referenceD0ABOne((uint8_t)value);
            if (output[x] != expected) {
                fprintf(stderr, "sub_D0AB mismatch: input=%02X x=%u expected=%u actual=%u\n",
                        value, x, expected, output[x]);
                return 1;
            }
        }
    }
    puts("PASS: sub_D0AB matched the SBC/N-flag reference for all 256 input bytes and four slots.");
    return 0;
}

static uint8_t referenceDFB2One(uint8_t ram009C, uint8_t ram00B8,
                                uint8_t ram00A4, const uint8_t table[5])
{
    uint8_t y = 0u;
    if (ram009C == 0u) {
        y = 0u;
        while (1) {
            uint8_t cmp = (uint8_t)(ram00B8 - table[y]);
            if ((cmp & 0x80u) != 0u) break;
            y = (uint8_t)(y + 1u);
            if (y >= 5u) { y = 0u; break; }
        }
        if (y != 0u && ram00A4 == 1u) y = (uint8_t)(y + 2u);
    }
    return y;
}

static int testSubDFB2(void)
{
    static const uint8_t table[5] = {0x08u,0x14u,0x20u,0x2Cu,0x38u};
    unsigned int yValue, state, a4, x;
    unsigned long checked = 0u;
    for (yValue = 0; yValue < 256u; ++yValue) {
        for (state = 0; state < 256u; ++state) {
            for (a4 = 0; a4 < 2u; ++a4) {
                for (x = 0; x < 4u; ++x) {
                    uint8_t r9c[4] = {0u,0u,0u,0u};
                    uint8_t rb8[4] = {0u,0u,0u,0u};
                    uint8_t ra4[4] = {0u,0u,0u,0u};
                    uint8_t r70[4] = {0xCCu,0xCCu,0xCCu,0xCCu};
                    uint8_t expected;
                    r9c[x] = (uint8_t)state;
                    rb8[x] = (uint8_t)yValue;
                    ra4[x] = (uint8_t)a4;
                    expected = referenceDFB2One((uint8_t)state,(uint8_t)yValue,(uint8_t)a4,table);
                    nesSubDFB2SelectBand((uint8_t)x,r9c,rb8,ra4,r70,table);
                    if (r70[x] != expected) {
                        fprintf(stderr, "sub_DFB2 mismatch: B8=%02X 9C=%02X A4=%02X x=%u expected=%u actual=%u\n",
                                yValue,state,a4,x,expected,r70[x]);
                        return 1;
                    }
                    ++checked;
                }
            }
        }
    }
    printf("PASS: sub_DFB2 matched CMP/N-flag reference for %lu cases.\n", checked);
    return 0;
}


static int testSubD000(void)
{
    uint8_t flags[4] = {0x10u,0x10u,0x00u,0xFFu};
    uint8_t state[4] = {0xBFu,0xC0u,0xF0u,0x00u};
    uint8_t y = 0xAAu;
    if (nesSubD000ShouldSetupObject(0u, flags, state, &y) != 0u) return 1;
    if (nesSubD000ShouldSetupObject(1u, flags, state, &y) != 1u || y != 1u) return 1;
    if (nesSubD000ShouldSetupObject(2u, flags, state, &y) != 0u) return 1;
    if (nesSubD000ShouldSetupObject(3u, flags, state, &y) != 1u || y != 0u) return 1;
    puts("PASS: sub_D000 gate and exact-$C0 Y increment.");
    return 0;
}

static int testSubD11F(void)
{
    uint8_t ram[19];
    unsigned int i;
    for (i=0u;i<19u;++i) ram[i]=0x55u;
    nesSubD11FInitializeUpdateBuffer(ram);
    if (ram[0]!=0x11u || ram[1]!=0x23u || ram[2]!=0xA0u ||
        ram[3]!=0x0Eu || ram[18]!=0u) return 1;
    for (i=4u;i<=17u;++i) if (ram[i]!=0xFEu) return 1;
    puts("PASS: sub_D11F exact RAM $0300-$0312 writes.");
    return 0;
}

static int testSubD255(void)
{
    uint8_t oam[256];
    unsigned int i;
    for (i=0u;i<256u;++i) oam[i]=0u;
    nesSubD255HideAllSprites(oam);
    for (i=0u;i<256u;++i) if (oam[i]!=0xF8u) return 1;
    puts("PASS: sub_D255 hides all 256 OAM bytes.");
    return 0;
}

static int testSubD2FF(void)
{
    uint8_t x=0x3Eu, base=0x05u, memory[256], discarded=0u;
    unsigned int i;
    for (i=0u;i<256u;++i) memory[i]=0xAAu;
    nesSubD2FFAdvanceBufferIndex(&x,base,memory,&discarded);
    if (x!=base || memory[base]!=0u || discarded!=2u) return 1;
    x=0x10u; discarded=0u;
    nesSubD2FFAdvanceBufferIndex(&x,base,memory,&discarded);
    if (x!=0x11u || discarded!=0u) return 1;
    puts("PASS: sub_D2FF normal and stack-discard paths.");
    return 0;
}

static int testSubE70B(void)
{
    uint8_t a[4]={1u,2u,0u,0x10u}, b[4]={4u,2u,0x77u,0x05u};
    uint8_t c[4]={0x3Fu,0x3Eu,0x22u,0xFFu}, d[4]={1u,1u,1u,1u};
    nesSubE70BUpdateObjectOffsets(a,b,c,d);
    if (b[0]!=3u || c[0]!=0x3Fu || d[0]!=0u) return 1;
    if (b[1]!=8u || c[1]!=0x3Fu || d[1]!=0u) return 1;
    if (b[2]!=0x77u || c[2]!=0x22u || d[2]!=0u) return 1;
    if (b[3]!=0xFDu || c[3]!=0u || d[3]!=0u) return 1;
    puts("PASS: sub_E70B N/Z flag branches and six-bit wrap.");
    return 0;
}

static int testSubE7FCFamily(void)
{
    static uint8_t pages[6][256];
    const uint8_t *pagePtrs[6];
    uint8_t ram360[4]={0u,1u,2u,5u}, ramE0[4]={4u,5u,6u,7u};
    uint8_t saved=0xFFu, value=0xFFu;
    const uint8_t *selected=NULL;
    unsigned int p,i;
    for (p=0u;p<6u;++p) {
        pagePtrs[p]=pages[p];
        for (i=0u;i<256u;++i) pages[p][i]=(uint8_t)(p+i);
    }
    if (!nesSubE80DPreparePointer(4u,pagePtrs,&selected) || selected!=pagePtrs[4]) {
        /* The direct pointer test below uses its own output variable. */
        return 1;
    }
    if (!nesSubE803ReadData(3u,0x22u,pagePtrs,&saved,&value) ||
        saved!=3u || value!=(uint8_t)(3u+0x22u)) return 1;
    if (!nesSubE7FFReadObjectData(2u,1u,ramE0,pagePtrs,&saved,&value) ||
        saved!=1u || value!=7u) return 1;
    if (!nesSubE7FCReadObjectBandData(3u,ram360,ramE0,pagePtrs,&saved,&value) ||
        saved!=5u || value!=12u) return 1;
    puts("PASS: sub_E80D/E803/E7FF/E7FC pointer and indexed reads.");
    return 0;
}

static int testSubE836(void)
{
    uint8_t b0[4]={0u,1u,0u,0u}, bc[4]={1u,2u,3u,4u};
    uint8_t by[4]={5u,6u,7u,8u}, out[4]={0xAAu,0xAAu,0xAAu,0xAAu};
    nesSubE836UpdateObjectDifferences(b0,bc,by,out,100u);
    if (out[0]!=(uint8_t)(100u-1u-5u) || out[1]!=0xAAu ||
        out[2]!=(uint8_t)(100u-3u-7u) || out[3]!=(uint8_t)(100u-4u-8u)) return 1;
    puts("PASS: sub_E836 skips nonzero $00B0 slots and uses shared SBC helper.");
    return 0;
}

static int testNewE16C(void){uint8_t o[256]={0};nesSubE16CWriteSpriteStrip(o,0x20u,0x33u);return(o[0x26]==0x60u&&o[0x25]==0x33u&&o[0x22]==0x60u&&o[0x21]==0x32u)?0:1;}
static int testNewE3F4(void){uint8_t o[4],y[4]={30u,10u,20u,40u};nesSubE3F4SortObjectsByY(o,y,0u,0u);if(o[0]!=1u||o[1]!=2u||o[2]!=0u||o[3]!=3u)return 1;nesSubE3F4SortObjectsByY(o,y,1u,5u);return(o[0]==3u&&o[1]==1u&&o[2]==2u&&o[3]==0u)?0:1;}
static int testNewE4(void){uint8_t o[256]={0},f[3]={0u,1u,2u},off[3]={0x10u,0x20u,0x30u};nesSubE4ADHideFourSprites(o,0x40u,8u);if(o[0x48]!=0xF8u||o[0x4C]!=0xF8u||o[0x50]!=0xF8u)return 1;for(unsigned i=0;i<256u;i++)o[i]=0u;nesSubE49BHideSelectedSpriteGroups(o,0x40u,f,off);return(o[0x50]==0xF8u&&o[0x54]==0xF8u&&o[0x58]==0xF8u&&o[0x60]==0u&&o[0x70]==0xF8u&&o[0x78]==0xF8u)?0:1;}
static int testNewE2(void){uint8_t lo=3u,hi=9u,b=0u;uint8_t one[4]={1u},z[4]={0u},bc[4]={0u},d[4]={1u},e[4]={0x10u};nesSubE2AFDecrementCountdown(&lo,&hi,4u);if(lo!=0xFFu||hi!=8u)return 1;nesLocE2BDUpdateFinishBlink(1u,0u,&b);if(b!=1u||!nesLocE2D0CountdownReachedOne(1u))return 1;return nesSubE2F6ShouldDrawWheel(0u,one,one,z,bc,d,e)?0:1;}
static int testNewE1F9(void){
    uint8_t r9c[4]={1u,0u,0u,0u},rbc[4]={0u},r370[4]={0u},r36[4]={0u};
    uint8_t rac[4]={3u,0x0Au,6u,6u},r98[4]={0u},rb0[4]={0u},r58[4]={0u},rb8[4]={0u};
    uint8_t r90[4]={0u},r94[4]={0u},rdc[4]={0u},temp=0xFFu,ram039C3=0xFFu;
    if(nesSubE1F9SelectObjectSprite(0u,r9c,rbc,r370,r36,rac,r98,rb0,r58,rb8,r90,r94,rdc,0u,0u,&ram039C3,&temp)!=0x16u||temp!=0u||ram039C3!=0u)return 1;
    r36[0]=3u;
    if(nesSubE1F9SelectObjectSprite(0u,r9c,rbc,r370,r36,rac,r98,rb0,r58,rb8,r90,r94,rdc,0u,0u,&ram039C3,&temp)!=0x12u)return 2;
    r9c[0]=0u;
    if(nesSubE1F9SelectObjectSprite(1u,r9c,rbc,r370,r36,rac,r98,rb0,r58,rb8,r90,r94,rdc,1u,0u,&ram039C3,&temp)!=2u||temp!=2u)return 3;
    r98[2]=1u;
    if(nesSubE1F9SelectObjectSprite(2u,r9c,rbc,r370,r36,rac,r98,rb0,r58,rb8,r90,r94,rdc,0u,0u,&ram039C3,&temp)!=6u||temp!=0u)return 4;
    r98[2]=0u;r90[2]=1u;rdc[2]=0x80u;
    if(nesSubE1F9SelectObjectSprite(2u,r9c,rbc,r370,r36,rac,r98,rb0,r58,rb8,r90,r94,rdc,0u,0u,&ram039C3,&temp)!=0x15u)return 5;
    puts("PASS: sub_E1F9 selection branches, timer return, temp and RAM side effect.");return 0;
}
static int testNewE3AA(void){uint8_t t=0u,l=0u,f=1u;nesLocE3AAUpdateHeatFlags(0x20u,1u,&t,&l,&f);return(t==0x20u&&l==0x20u&&f==0xB1u)?0:1;}
