#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nes_routines.h"

static int testBatchAdditional20(void);
static int testState2SetupCommands(void);
static int testSubEC99Exact(void);
static int testSubFF53Exact(void);

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
    if(s.ram00A8[2]!=0u||s.ram0058[2]!=0u||s.ram0370[2]!=0u||s.ram03A6_flag!=1u){fprintf(stderr,"E456 state fail: A8=%u 58=%u 370=%u flag=%u\n",s.ram00A8[2],s.ram0058[2],s.ram0370[2],s.ram03A6_flag);return 1;}
    if(oam[0x40]!=0xF8u||oam[0x44]!=0xF8u||oam[0x48]!=0xF8u||oam[0x30]!=0xF8u||oam[0x38]!=0xF8u){fprintf(stderr,"E456 OAM fail: %02X %02X %02X %02X %02X\n",oam[0x40],oam[0x44],oam[0x48],oam[0x30],oam[0x38]);return 2;}
    if(oam[0x70]!=0u){fprintf(stderr,"E456 unexpected OAM[70]=%02X\n",oam[0x70]);return 3;}
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
    if(oam[0x97]!=0x19u||oam[0x9B]!=0x11u||oam[0x98]!=0x38u||oam[0x94]!=0x40u||oam[0x95]!=0xF6u||oam[0x99]!=0xF7u){fprintf(stderr,"E2F6 primary fail: %02X %02X %02X %02X %02X %02X\n",oam[0x97],oam[0x9B],oam[0x98],oam[0x94],oam[0x95],oam[0x99]);return 1;}
    for(unsigned i=0;i<256u;i++)oam[i]=0u;
    nesSubE2F6DrawWheelSprites(0u,0u,r36,r94,r90,rbc,r84,r80,r8c,4u,oam);
    if(oam[0x95]!=0xF7u||oam[0x99]!=0xF6u){fprintf(stderr,"E2F6 swap fail: %02X %02X\n",oam[0x95],oam[0x99]);return 2;}
    r94[0]=0u;r90[0]=1u;for(unsigned i=0;i<256u;i++)oam[i]=0u;
    nesSubE2F6DrawWheelSprites(0u,0u,r36,r94,r90,rbc,r84,r80,r8c,0u,oam);
    if(oam[0x98]!=0xF8u){fprintf(stderr,"E2F6 hide fail: %02X\n",oam[0x98]);return 3;}
    rbc[0]=1u;for(unsigned i=0;i<256u;i++)oam[i]=0u;
    nesSubE2F6DrawWheelSprites(0u,0u,r36,r94,r90,rbc,r84,r80,r8c,0u,oam);
    if(oam[0x97]!=0u){fprintf(stderr,"E2F6 gate fail: %02X\n",oam[0x97]);return 4;}
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
    uint8_t z[4]={0u},r9c[4]={0u,1u,0u,0u},r36[4]={0u},rac[4]={3u},r80[4]={0u,0x20u,0u,0u},r8c[4]={0u,0x30u,0u,0u};
    uint8_t d892[23],d8a9[23],oam[256]={0},ram039C3=0xFFu,temp=0xFFu,render=0u;NesPreparedSpriteCommand out={0};
    for(unsigned i=0;i<23u;i++){d892[i]=(uint8_t)(0x40u+i);d8a9[i]=(uint8_t)(0x80u+i);}
    if(!nesSubE1A3PrepareObjectSprite(1u,0x20u,r80,r8c,r9c,z,z,r36,rac,z,z,z,z,z,z,z,0u,0u,d892,d8a9,oam,&ram039C3,&temp,&render,&out))return 1;
    if(out.scratch0!=0x20u||out.scratch1!=0x38u||out.scratch2!=d892[0x16]||out.scratch3!=0x23u||out.scratch4!=0x2Cu||ram039C3!=0x20u||render!=d8a9[0x16])return 1;
    r9c[1]=0u;rac[1]=0x0Au;for(unsigned i=0;i<256u;i++)oam[i]=0u;
    if(!nesSubE1A3PrepareObjectSprite(1u,0x40u,r80,r8c,r9c,z,z,r36,rac,z,z,z,z,z,z,z,1u,0u,d892,d8a9,oam,&ram039C3,&temp,&render,&out))return 1;
    if(temp!=2u||oam[0x51]!=0xB2u||oam[0x4D]!=0xB1u||oam[0x45]!=0xB0u||oam[0x41]!=0xAFu)return 1;
    puts("PASS: sub_E1A3 table selection, special offset, and overlay tiles.");return 0;
}
static int testLocE96EVerticalDelta(void)
{
    unsigned int state, y, delta;
    unsigned long checked = 0ul;
    for (state = 0u; state < 256u; ++state) {
        for (y = 0u; y < 256u; ++y) {
            for (delta = 0u; delta < 256u; ++delta) {
                uint8_t states[4] = {(uint8_t)state, 0x14u, 0x15u, 0x00u};
                uint8_t posY[4] = {(uint8_t)y, 0xA5u, 0x5Au, 0xFEu};
                uint8_t deltas[4] = {(uint8_t)delta, 0x11u, 0x22u, 0x03u};
                uint8_t expected = (state == 0x14u || state == 0x15u)
                    ? (uint8_t)y : (uint8_t)(y + delta);
                nesLocE96EApplyVerticalDelta(states, posY, deltas);
                if (posY[0] != expected || posY[1] != 0xA5u ||
                    posY[2] != 0x5Au || posY[3] != (uint8_t)0x01u) {
                    fprintf(stderr, "sub_E96C loc_E96E mismatch: state=%02X y=%02X delta=%02X\n",
                            state, y, delta);
                    return 1;
                }
                ++checked;
            }
        }
    }
    printf("PASS: loc_E96E vertical delta matched byte-level reference for %lu combinations.\n", checked);
    return 0;
}


static int testE84FStorePair(void)
{
    unsigned int x, value;
    for (x = 0; x < 4u; ++x) for (value = 0; value < 256u; ++value) {
        uint8_t a[4] = {1u,2u,3u,4u}, b[4] = {5u,6u,7u,8u};
        uint8_t oldA[4], oldB[4]; unsigned int i;
        memcpy(oldA,a,4); memcpy(oldB,b,4);
        nesSubE84FStorePair((uint8_t)x,(uint8_t)value,a,b);
        for(i=0;i<4u;++i) {
            uint8_t ea=(i==x)?(uint8_t)value:oldA[i], eb=(i==x)?(uint8_t)value:oldB[i];
            if(a[i]!=ea || b[i]!=eb) return 1;
        }
    }
    return 0;
}
static int testE86FSubtractFromE4(void)
{
    unsigned int x, a, e;
    for(x=0;x<4u;++x) for(a=0;a<256u;++a) for(e=0;e<256u;++e) {
        uint8_t src[4]={0,0,0,0}, dst[4]={0xAA,0xAA,0xAA,0xAA};
        src[x]=(uint8_t)e;
        if(nesSubE86FSubtractFromE4((uint8_t)x,(uint8_t)a,src,dst)!=(uint8_t)(e-a) ||
           dst[x]!=(uint8_t)(e-a)) return 1;
    }
    return 0;
}
static int testE893SetOffsetsAndSubtract(void)
{
    unsigned int x,a,c,d;
    for(x=0;x<4u;++x) for(a=0;a<256u;++a) for(c=0;c<256u;++c) for(d=0;d<256u;++d) {
        uint8_t cv[4]={0},dv[4]={0},out1[4]={0},out2[4]={0};
        cv[x]=(uint8_t)c; dv[x]=(uint8_t)d;
        if(nesSubE893SetOffsetsAndSubtract((uint8_t)x,(uint8_t)a,cv,dv,out1,out2)!=(uint8_t)(c-d) ||
           out1[x]!=(uint8_t)a || out2[x]!=(uint8_t)a) return 1;
    }
    return 0;
}
static int testE73BAddXLow(void)
{
    unsigned int x,delta,pos;
    for(x=0;x<4u;++x) for(delta=0;delta<256u;++delta) for(pos=0;pos<256u;++pos) {
        uint8_t d[4]={0},p[4]={11,22,33,44},before[4]; unsigned int i;
        d[x]=(uint8_t)delta; memcpy(before,p,4);
        nesSubE73BAddXLow((uint8_t)x,d,p);
        for(i=0;i<4u;++i) if(p[i]!=(i==x?(uint8_t)(before[i]+delta):before[i])) return 1;
    }
    return 0;
}
static int testE733UpdateThreeXLowSlots(void)
{
    unsigned int slot, value;
    for (slot = 1u; slot <= 3u; ++slot) {
        for (value = 0u; value < 256u; ++value) {
            uint8_t delta[4] = {0x19u,0x29u,0x39u,0x49u};
            uint8_t pos[4] = {0xF0u,0xF1u,0xF2u,0xF3u};
            uint8_t before[4];
            unsigned int i;
            delta[slot] = (uint8_t)value;
            memcpy(before, pos, sizeof(pos));
            nesSubE733UpdateThreeXLowSlots(delta, pos);
            for (i = 0u; i < 4u; ++i) {
                uint8_t expected = (i == 0u) ? before[i] : (uint8_t)(before[i] + delta[i]);
                if (pos[i] != expected) return 1;
            }
        }
    }
    return 0;
}


static int testE7FCAndE7FF(void)
{
    unsigned int x,v;
    for(x=0;x<4u;++x) for(v=0;v<256u;++v) {
        uint8_t a[4]={0,0,0,0},b[4]={0,0,0,0};
        a[x]=(uint8_t)v; b[x]=(uint8_t)(255u-v);
        if(nesSubE7FCLoadDataIndex((uint8_t)x,a)!=(uint8_t)v ||
           nesSubE7FFLoadDataIndex((uint8_t)x,b)!=(uint8_t)(255u-v)) return 1;
    }
    return 0;
}
static int testE80DPreparePointer(void)
{
    static const uint8_t lo[6]={0x00u,0x40u,0x80u,0xC0u,0x00u,0x40u};
    static const uint8_t hi[6]={0x04u,0x04u,0x04u,0x04u,0x05u,0x05u};
    unsigned int i;
    for(i=0;i<6u;++i) { uint8_t l=0xAAu,h=0xBBu; nesSubE80DPointerBytes((uint8_t)i,&l,&h); if(l!=lo[i]||h!=hi[i]) return 1; }
    return 0;
}
static int testE803ReadIndexedRam(void)
{
    uint8_t ram[1024]; unsigned int i,t,y;
    for(i=0;i<1024u;++i) ram[i]=(uint8_t)((i*37u+11u)&0xFFu);
    for(t=0;t<6u;++t) for(y=0;y<256u;++y) {
        static const unsigned int base[6]={0u,0x40u,0x80u,0xC0u,0x100u,0x140u};
        if(nesSubE803ReadIndexedRam((uint8_t)t,(uint8_t)y,ram)!=ram[base[t]+y]) return 1;
    }
    return 0;
}
static int testOfsE81807Apply(void)
{
    unsigned int b,ac,s,other;
    for(b=0;b<2u;++b) for(ac=0;ac<256u;++ac) for(s=0;s<4u;++s) for(other=0;other<2u;++other) {
        uint8_t rb[4]={0,0,0,0},rac[4]={0,0,0,0},r94[4]={0,0,0,0},r90[4]={0,0,0,0},r98[4]={0x11,0x22,0x33,0x44};
        uint8_t expected=(uint8_t)(s<3u ? ((s==2u && other!=0u)?0xFFu:r98[0]) : 0xFFu);
        rb[0]=(uint8_t)b; rac[0]=(uint8_t)ac; r94[0]=(uint8_t)s; r90[0]=(uint8_t)(other?0x80u:0u);
        if(b!=0u) expected=r98[0];
        if((int8_t)(uint8_t)(ac-7u)>=0) expected=r98[0];
        nesOfsE81807Apply(0u,rb,rac,r94,r90,r98);
        if(r98[0]!=expected) return 1;
    }
    return 0;
}


static int testE8xxDispatchEntries(void)
{
    static const uint8_t probes[]={0u,1u,2u,7u,0x7Fu,0x80u,0xFEu,0xFFu};
    unsigned int x,c,d,p,q;
    for(x=0;x<4u;++x) for(c=0;c<256u;++c) for(d=0;d<256u;++d) for(p=0;p<sizeof(probes);++p) for(q=0;q<2u;++q) {
        uint8_t c8[4]={0},d0[4]={0},e4[4]={0},a0[4]={0},d8[4]={9,8,7,6},d4[4]={0},b4[4]={0},bc[4]={0},e4out[4]={0};
        uint8_t diff=(uint8_t)(c-d);
        c8[x]=(uint8_t)c; d0[x]=(uint8_t)d; e4[x]=probes[p]; a0[x]=(uint8_t)q;
        d8[x]=0xAAu; bc[x]=0xBBu; e4out[x]=0xCCu;
        nesOfsE84504Apply((uint8_t)x,c8,d0,d8,d4,b4,bc,e4out);
        if(d8[x]!=0x80u||d4[x]!=5u||b4[x]!=5u||bc[x]!=(uint8_t)(diff>>1)||e4out[x]!=(uint8_t)(diff>>1)) return 1;
        d8[x]=0xAAu;d4[x]=0;b4[x]=0;bc[x]=0xBBu;
        nesOfsE85405Apply((uint8_t)x,c8,d0,e4,d4,b4,bc);
        if(d4[x]!=2u||b4[x]!=2u||bc[x]!=(uint8_t)(probes[p]-(uint8_t)(diff>>1))) return 1;
        d8[x]=0xAAu;d4[x]=0;b4[x]=0;bc[x]=0xBBu;e4out[x]=0xCCu;
        nesOfsE85D02Apply((uint8_t)x,c8,d0,d8,d4,b4,bc,e4out);
        if(d8[x]!=0x60u||d4[x]!=6u||b4[x]!=6u||bc[x]!=diff||e4out[x]!=diff) return 1;
        d4[x]=0;b4[x]=0;bc[x]=0xBBu;
        nesOfsE86A03Apply((uint8_t)x,c8,d0,e4,d4,b4,bc);
        if(d4[x]!=1u||b4[x]!=1u||bc[x]!=(uint8_t)(probes[p]-diff)) return 1;
        d8[x]=0xAAu;d4[x]=0;b4[x]=0;bc[x]=0xBBu;e4out[x]=0xCCu;
        nesOfsE87909Apply((uint8_t)x,c8,d0,a0,d8,d4,b4,bc,e4out);
        { uint8_t expected=(uint8_t)(diff<<1); if(q!=0u) expected=(uint8_t)(expected+0x10u);
          if(d8[x]!=0x40u||d4[x]!=4u||b4[x]!=4u||bc[x]!=expected||e4out[x]!=expected) return 1; }
    }
    return 0;
}


static int testE8xxConstantAndResetEntries(void)
{
    unsigned int x,y;
    for(x=0;x<4u;++x) for(y=0;y<256u;++y) {
        uint8_t a4[4]={1,2,3,4},a0[4]={5,6,7,8},bc[4]={9,10,11,12},e4[4]={13,14,15,16},cc[4]={17,18,19,20},d4[4]={21,22,23,24},b4[4]={25,26,27,28};
        uint8_t oldA4[4],oldA0[4],oldBC[4],oldE4[4],oldCC[4],oldD4[4],oldB4[4]; unsigned int i;
        memcpy(oldA4,a4,4);memcpy(oldA0,a0,4);memcpy(oldBC,bc,4);memcpy(oldE4,e4,4);memcpy(oldCC,cc,4);memcpy(oldD4,d4,4);memcpy(oldB4,b4,4);
        nesOfsE8BF0EReset((uint8_t)x,a4,a0);
        if(a4[x]!=0u||a0[x]!=0u) return 1;
        for(i=0;i<4u;++i) if(i!=x&&(a4[i]!=oldA4[i]||a0[i]!=oldA0[i])) return 1;
        memcpy(a4,oldA4,4); nesOfsE8C60FSetStateFromY((uint8_t)x,(uint8_t)y,a4);
        if(a4[x]!=(y<0x20u?2u:1u)) return 1;
        memcpy(a0,oldA0,4); nesOfsE8E301SetA0((uint8_t)x,a0);
        if(a0[x]!=1u) return 1;
        memcpy(bc,oldBC,4);memcpy(e4,oldE4,4);nesOfsE8F514SetPair30((uint8_t)x,bc,e4);
        if(bc[x]!=0x30u||e4[x]!=0x30u) return 1;
        memcpy(bc,oldBC,4);memcpy(e4,oldE4,4);nesOfsE8FA10SetPair10((uint8_t)x,bc,e4);
        if(bc[x]!=0x10u||e4[x]!=0x10u) return 1;
        nesOfsE96300ClearThree((uint8_t)x,cc,d4,b4);
        if(cc[x]!=0u||d4[x]!=0u||b4[x]!=0u) return 1;
        for(i=0;i<4u;++i) if(i!=x&&(cc[i]!=oldCC[i]||d4[i]!=oldD4[i]||b4[i]!=oldB4[i])) return 1;
    }
    return 0;
}


static int testBatchC333C3B1(void) {
    uint8_t ctrl=0u,r2000=0u,a=0xFFu,b=0xA5u;
    nesSubC333WritePpuCtrl(0xA5u,&r2000,&ctrl);
    if(ctrl!=0xA5u||r2000!=0xA5u)return 1;
    nesSubC3AFIncrementRaceCounter(&a); if(a!=0u)return 2;
    nesSubC3B1ClearCounters(&a,&b); if(a!=0u||b!=0u)return 3;
    puts("PASS: sub_C333/C3AF/C3B1 register and RAM effects."); return 0;
}
static int testBatchC621C68A(void) {
    uint8_t source[256],dest[256],lo=0xFFu,hi=0x34u; unsigned int i;
    for(i=0;i<256u;++i){source[i]=(uint8_t)(i^0xA5u);dest[i]=0u;}
    nesSubC621CopyPage(source,dest);
    if(memcmp(source,dest,256u)!=0)return 1;
    nesSubC68AIncrementPointer(&lo,&hi); if(lo!=0u||hi!=0x35u)return 2;
    puts("PASS: sub_C621 full page copy and C68A pointer carry."); return 0;
}
static int testBatchC971CA1B(void) {
    uint8_t out=0u,x=0u,held=0u;
    if(nesSubC971AddCommandLength(0xF0u,0x30u,&out,&x)!=0x20u||out!=0x20u||x!=0x30u)return 1;
    nesSubCA1BCopyButtonHold(0xA6u,&held); if(held!=0xA6u)return 2;
    puts("PASS: sub_C971 addition and CA1B button copy."); return 0;
}
static int testBatchCB13(void) {
    if(nesSubCB13ComputeSpriteCode(5u,0u)!=0x20u)return 1;
    if(nesSubCB13ComputeSpriteCode(4u,0u)!=0x0Cu)return 2;
    if(nesSubCB13ComputeSpriteCode(0u,1u)!=0x10u)return 3;
    if(nesSubCB13ComputeSpriteCode(1u,1u)!=0x13u)return 4;
    if(nesSubCB13ComputeSpriteCode(0x55u,0u)!=0xFFu)return 5;
    puts("PASS: sub_CB13 exact Y loop and flag branch."); return 0;
}
static int testBatchCFE3D24F(void) {
    uint8_t flags[256]={0},mask=0xFFu; flags[0xF2u]=0xAFu;
    nesSubCFE3ClearObjectFlags(0xF2u,flags); if(flags[0xF2u]!=0xA0u)return 1;
    nesSubD24FDisableRendering(&mask); if(mask!=0u)return 2;
    puts("PASS: sub_CFE3 flag mask and D24F rendering mask."); return 0;
}
static int testBatchCF8BECBF(void) {
    unsigned int i;
    for(i=0;i<256u;++i) {
        uint8_t expected=(uint8_t)(i+1u); if(expected==0x40u)expected=0u;
        if(nesSubECBFIncrementAndWrap((uint8_t)i)!=expected)return 1;
        if(nesSubCF8BCarryOut((uint8_t)i)!=((i>=0x3Bu&&i<0x3Eu)?0u:1u))return 2;
    }
    puts("PASS: sub_CF8B carry and ECBF increment/wrap over all A values."); return 0;
}
static int testBatchF676F826(void) {
    uint8_t ram[22]; uint8_t b7=0u,c5=0xAAu; unsigned int i;
    for(i=0;i<22u;++i)ram[i]=(uint8_t)i;
    nesSubF676FillCommandPayload(ram);
    for(i=0;i<22u;++i)if(ram[i]!=((i>=4u&&i<=13u)?0xFCu:(uint8_t)i))return 1;
    if(nesSubF826AdvanceObject(0xFFu,&b7,&c5)!=0u||b7!=0u||c5!=0u)return 2;
    puts("PASS: sub_F676 payload fill and F826 byte wrap/state writes."); return 0;
}
static int testBatchF82E839(void) {
    uint8_t loA[256],hiA[256],loB[256],hiB[256],lo=0u,hi=0u; unsigned int i;
    for(i=0;i<256u;++i){loA[i]=(uint8_t)i;hiA[i]=(uint8_t)(255u-i);loB[i]=(uint8_t)(i^0x5Au);hiB[i]=(uint8_t)(i^0xA5u);}
    nesSubF82EPreparePointer(0x91u,loA,hiA,&lo,&hi);if(lo!=0x91u||hi!=(uint8_t)(255u-0x91u))return 1;
    nesSubF839PreparePointer(0x3Cu,loB,hiB,&lo,&hi);if(lo!=(uint8_t)(0x3Cu^0x5Au)||hi!=(uint8_t)(0x3Cu^0xA5u))return 2;
    puts("PASS: sub_F82E/F839 indexed pointer-table reads."); return 0;
}
static int testBatchC62D(void) {
    uint8_t strobe=0u; nesSubC62DControllerStrobe(&strobe);
    if(strobe!=0x05u)return 1;
    puts("PASS: sub_C62D controller strobe value."); return 0;
}
static int testBatchF6FB(void) {
    uint8_t a=0xA3u,b=0x1Eu;
    nesSubF6FBAdvanceAudioPhase(&a,&b); if(a!=0xA3u||b!=0x1Fu)return 1;
    nesSubF6FBAdvanceAudioPhase(&a,&b); if(a!=0xA7u||b!=0u)return 2;
    a=0xFFu;b=0xFFu;nesSubF6FBAdvanceAudioPhase(&a,&b);if(a!=0xFFu||b!=0u)return 3;
    puts("PASS: sub_F6FB increment, equality branch and XOR bit."); return 0;
}
static int testBatchF871F885(void) {
    uint8_t audio[16]={0},table[256]={0}; unsigned int i;
    for(i=0;i<256u;++i)table[i]=(uint8_t)(i^0x21u);
    table[0x11u]=0x44u; table[0x10u]=0x21u;
    nesSubF871WriteAudioTimer(0x10u,table,0u,audio);
    if(audio[2]!=0x44u||audio[3]!=0x29u)return 1;
    audio[6]=0xAAu; audio[7]=0xBBu; table[0x21u]=0u;
    nesSubF885WriteAudioTimer(0x20u,table,0u,audio);
    if(audio[6]!=0xAAu||audio[7]!=0xBBu)return 2;
    table[0x31u]=0x22u; table[0x30u]=0x11u;
    nesSubF885WriteAudioTimer(0x30u,table,0u,audio);
    if(audio[6]!=0x22u||audio[7]!=0x19u)return 3;
    table[0xFFu]=0x12u;
    nesSubF871WriteAudioTimer(0xFFu,table,0x33u,audio);
    if(audio[2]!=0x33u||audio[3]!=0x1Au)return 4;
    puts("PASS: sub_F871/F885 indexed ROM reads, zero exit and address wrap."); return 0;
}



static int testD0F4PrepareCommandStream(void)
{
    uint8_t src[7]={1u,2u,3u,4u,5u,6u,7u},dst[8]={0u};
    uint8_t slo=0u,shi=0u,dlo=0u,dhi=0u;unsigned i;
    nesSubD0F4PrepareCommandStream(src,dst,&slo,&shi,&dlo,&dhi);
    if(dst[0]!=0x17u||dst[2]!=0xFBu||dst[5]!=0xFBu||
       slo!=0x56u||shi!=0x23u||dlo!=0xD0u||dhi!=0x03u)return 1;
    for(i=1u;i<8u;i++) {
        uint8_t expected=src[i-1u];
        if(i==2u||i==5u)expected=0xFBu;
        if(dst[i]!=expected)return 2;
    }
    puts("PASS: sub_D0F4 command-stream copy and explicit pointers.");
    return 0;
}
static int testD1A9ScrollWrites(void)
{
    uint8_t writes[2]={0xAAu,0xBBu},reads=4u;
    nesSubD1A9PrepareScrollWrites(writes,&reads);
    if(writes[0]!=0u||writes[1]!=0u||reads!=5u)return 1;
    puts("PASS: sub_D1A9 two zero scroll writes and status-read count.");
    return 0;
}
static int testD1B3ScrollWrites(void)
{
    uint8_t ctrl=0u,writes[2]={0u,0u},reads=7u;
    nesSubD1B3PrepareScrollWrites(0xA0u,0x02u,0x35u,0x71u,&ctrl,writes,&reads);
    if(ctrl!=0xA2u||writes[0]!=0x35u||writes[1]!=0x71u||reads!=8u)return 1;
    puts("PASS: sub_D1B3 control and ordered scroll-write effects.");
    return 0;
}

static int testBatchC1FDC229(void) {
    uint8_t table[256], ram[2048]; unsigned i;
    for(i=0;i<256u;i++) table[i]=(uint8_t)(i^0x5Au);
    for(i=0;i<2048u;i++) ram[i]=0xCCu;
    nesSubC1FDInitializeRam(table,ram);
    for(i=0;i<=0x1Eu;i++) if(ram[0x580u+i]!=table[i]||ram[0x5A3u+i]!=table[i]) return 1;
    if(ram[0x6E0]!=1u||ram[0x6E1]!=0x40u||ram[0x7AE]!=0x40u||ram[0x7AF]!=9u||ram[0x5CE]!=0xA5u||ram[0x5CF]!=0x5Au) return 2;
    nesSubC229InitializeRows(ram);
    if(ram[0x5A0]!=0x0Au||ram[0x5A1]!=0u||ram[0x5A2]!=0u||ram[0x5C3]!=0x0Au||ram[0x5C4]!=0u||ram[0x5C5]!=0u) return 3;
    return 0;
}
static int testBatchC32F(void) {
    uint8_t ram=0xD5u,ppu=0u; nesSubC32FMaskPpuCtrl(&ram,&ppu);
    return (ram==0x55u&&ppu==0x55u)?0:1;
}
static int testBatchC53Family(void) {
    uint8_t ram[2048]={0},r2=0u,r5=0u,r6=0u,r7=0u;
    uint8_t idx=nesSubCB06ComputeTableIndex(2u,0u,0u);
    ram[0x580u+idx]=0x12u;ram[0x581u+idx]=0x34u;ram[0x582u+idx]=0x56u;
    nesSubC53ESelectTableEntry(2u,0u,0u,ram,&r5,&r6,&r7);
    if(r5!=0x12u||r6!=0x34u||r7!=0x56u)return 1;
    nesSubC53ASelectTableEntry(2u,0u,0u,&r2,ram,&r5,&r6,&r7);
    return (r2==2u&&r5==0x12u&&r6==0x34u&&r7==0x56u)?0:2;
}
static int testBatchC716C726(void) {
    uint8_t d,e,f,g,h;
    nesSubC716ConfigureLoop(&d,&e,&f,&g,&h);
    if(d!=0x40u||e!=0x60u||f!=0u||g!=0x28u||h!=0x28u)return 1;
    nesSubC726ConfigureLoop(&d,&e,&f,&g,&h);
    return (d==0u&&e==0xE0u&&f==5u&&g==0x14u&&h==0x14u)?0:2;
}
static int testBatchDC97(void) {
    uint8_t bc[4]={1u,2u,0x80u,4u},py[4]={3u,4u,0x7Fu,8u},c1=0u,c2=0u;
    uint8_t a=nesSubDC97SubtractObjectY(0x20u,2u,bc,py,&c1);
    uint8_t b=nesSubDC9A(0x20u,bc[2],py[2]);
    c2=1u; /* 6502 carry after 0x20 - 0x80 - 0x7F is set. */
    return (a==b&&c1==c2&&a==0x20u)?0:1;
}
static int testBatchEC25EC5D(void) {
    uint8_t oam[256]={0},tbl[256];unsigned i;for(i=0;i<256u;i++)tbl[i]=(uint8_t)(i+7u);
    nesSubEC25WriteSprite(0x22u,3u,tbl,oam);
    if(oam[0x30]!=0x22u||oam[0x31]!=0xCAu||oam[0x32]!=0u||oam[0x33]!=10u)return 1;
    nesSubEC5DWriteSprite(0x10u,4u,tbl,oam);
    return (oam[0x30]==0xF8u&&oam[0x28]==0xF8u&&oam[0x2C]==0xF8u&&oam[0x33]==11u)?0:2;
}
static int testBatchEC3B(void) {
    uint8_t ram[2048];unsigned i;for(i=0;i<2048u;i++)ram[i]=(uint8_t)(i^0xA5u);
    /* Use a disjoint expected source snapshot to verify the full 256-byte copy. */
    uint8_t expected[256];for(i=0;i<256u;i++)expected[i]=ram[0x5E0u+i];
    nesSubEC3BCopyPageDescending(ram);
    for(i=0;i<256u;i++)if(ram[0x6E0u+i]!=expected[i])return 1;
    return 0;
}
static int testBatchEC75(void) {
    uint8_t b0[4]={0x10u,0x20u,0x30u,0x40u},c0[4]={0u,0u,2u,0u},tbl[256]={0};
    uint8_t x[4]={9u,9u,9u,9u},y[4]={0u,0u,0u,0u},bc[4]={0u,0u,0u,0u};
    tbl[2]=0x1Au;nesSubEC75InitializeObjectPositions(b0,c0,tbl,x,y,bc);
    return (x[0]==0x0Au&&x[1]==0u&&x[2]==0u&&y[0]==0x1Au&&y[2]==0x30u&&y[3]==0x10u&&bc[0]==0x20u)?0:1;
}
static int testBatchF681(void) {
    static uint8_t memory[65536];uint8_t ram[2048]={0},xo=0u,yo=0u;unsigned i;
    for(i=0;i<65536u;i++)memory[i]=(uint8_t)((i*13u)^0x5Au);
    nesSubF681CopyCommandBytes(0xFFFCu,0x0Cu,0xFEu,memory,ram,&xo,&yo);
    if(xo!=0x0Fu||yo!=(uint8_t)(0xFEu+3u))return 1;
    for(i=0;i<3u;i++)if(ram[0x030Fu+i] != memory[(uint16_t)(0xFFFCu+(uint8_t)(0xFFu+i))])return 2;
    return 0;
}
static int testBatchF821F86B(void) {
    uint8_t ram[2048]={0},tbl[256]={0},audio[16]={0};unsigned i;
    if(nesSubF821InitializeBufferAndAdvance(0xE0u,ram)!=0xE1u||ram[0x6C0]!=0x40u||ram[0xB7]!=0xE1u||ram[0xC5]!=0u)return 1;
    for(i=0;i<256u;i++)tbl[i]=(uint8_t)(i+1u);
    nesSubF86BWriteAudioRegisters(0x80u,0x9Cu,3u,tbl,0x77u,audio);
    return (audio[0]==0x80u&&audio[1]==0x9Cu&&audio[2]==5u&&audio[3]==0x0Cu)?0:2;
}
static int testBatchF889(void) {
    uint8_t tbl[256],audio[16];unsigned i;for(i=0;i<256u;i++)tbl[i]=1u;for(i=0;i<16u;i++)audio[i]=0xAAu;
    tbl[0x11]=0u;nesSubF889WriteThirdAudioTimer(0x10u,tbl,0x66u,audio);
    if(audio[10]!=0xAAu||audio[11]!=0xAAu)return 1;
    tbl[0x11]=0x22u;tbl[0x10]=0x31u;nesSubF889WriteThirdAudioTimer(0x10u,tbl,0x66u,audio);
    return (audio[10]==0x22u&&audio[11]==0x39u)?0:2;
}
static int testBatchC94B(void) {
    uint8_t length=0x10u,table[257],ram[267];uint8_t x=0u;unsigned i;
    for(i=0;i<257u;i++) table[i]=(uint8_t)(i^0xA5u);
    for(i=0;i<267u;i++) ram[i]=0xCCu;
    nesSubC94BBuildCommand(&length,0xFEu,table,ram,&x);
    if(x!=0x10u||length!=0x1Au)return 1;
    if(ram[0x11]!=table[0xFE]||ram[0x12]!=table[0xFF]||ram[0x13]!=0x07u||ram[0x15]!=0xFBu||ram[0x18]!=0xFBu||ram[0x1B]!=0u)return 2;
    return 0;
}
static int testBatchC64E653(void) {
    uint8_t a=0x91u,ao=0u,r7=0xFFu,rc=0xFFu,w[2]={0u,0u},loops[2]={0u,0u};
    nesSubC64EControllerDelay(a,&ao,&r7,&rc,w,loops);
    if(ao!=a||r7!=0u||rc!=0u||w[0]!=4u||w[1]!=0xFFu||loops[0]!=0x34u||loops[1]!=0x34u)return 1;
    nesSubC653ControllerDelay(a,&ao,&r7,&rc,w,loops);
    return (ao==a&&r7==0u&&rc==0u&&w[0]==4u&&w[1]==0xFFu&&loops[0]==0x6Au&&loops[1]==0x6Au)?0:2;
}


static int testSubCFE3MaskFlags(void)
{
    unsigned int value;
    for (value = 0u; value < 256u; ++value) {
        uint8_t actual = (uint8_t)value;
        nesSubCFE3MaskFlags(&actual);
        if (actual != (uint8_t)(value & 0xF0u)) {
            fprintf(stderr, "sub_CFE3 mismatch: input=%02X actual=%02X\n", value, actual);
            return 1;
        }
    }
    return 0;
}

static int testSubC7DEMenuInput(void)
{
    uint8_t selection, guard, timer30, timer31, changed, result;
    unsigned int i;

    selection=4u; guard=0u; timer30=0u; timer31=0u; changed=0u;
    result=nesSubC7DEMenuInput(0x04u,5u,&selection,&guard,&timer30,&timer31,&changed);
    if(result!=0u || selection!=1u || guard!=1u || timer31!=0x5Du || changed!=1u) {
        fputs("sub_C7DE down/wrap mismatch\n",stderr); return 1;
    }
    selection=2u; guard=1u; timer31=0u; changed=1u;
    result=nesSubC7DEMenuInput(0x04u,5u,&selection,&guard,&timer30,&timer31,&changed);
    if(result!=0u || selection!=2u || guard!=1u || timer31!=0x5Du || changed!=0u) {
        fputs("sub_C7DE repeat guard mismatch\n",stderr); return 2;
    }
    guard=1u;
    result=nesSubC7DEMenuInput(0x01u,5u,&selection,&guard,&timer30,&timer31,&changed);
    if(result!=0u || guard!=0u) { fputs("sub_C7DE idle reset mismatch\n",stderr); return 3; }
    selection=1u; guard=0u; changed=0u;
    result=nesSubC7DEMenuInput(0x08u,6u,&selection,&guard,&timer30,&timer31,&changed);
    if(result!=0u || selection!=0u || changed!=1u) { fputs("sub_C7DE up-to-zero mismatch\n",stderr); return 4; }
    selection=3u; guard=0u; timer30=0u; changed=1u;
    result=nesSubC7DEMenuInput(0x10u,6u,&selection,&guard,&timer30,&timer31,&changed);
    if(result!=4u || selection!=3u || timer30!=4u || changed!=0u) { fputs("sub_C7DE start mismatch\n",stderr); return 5; }
    selection=2u; guard=0u; changed=0u;
    result=nesSubC7DEMenuInput(0x24u,6u,&selection,&guard,&timer30,&timer31,&changed);
    if(result!=0u || selection!=3u || changed!=1u) { fputs("sub_C7DE Select priority mismatch\\n",stderr); return 7; }
    selection=5u; guard=0u; changed=0u;
    result=nesSubC7DEMenuInput(0x04u,6u,&selection,&guard,&timer30,&timer31,&changed);
    if(result!=0u || selection!=1u || changed!=1u) { fputs("sub_C7DE six-entry down wrap mismatch\\n",stderr); return 8; }
    selection=0u; guard=0u; changed=0u;
    result=nesSubC7DEMenuInput(0x08u,6u,&selection,&guard,&timer30,&timer31,&changed);
    if(result!=0u || selection!=5u || changed!=1u) { fputs("sub_C7DE zero up wrap mismatch\\n",stderr); return 9; }
    for(i=0u;i<1000u;++i) {
        uint8_t press=(i&1u)?0x04u:0x08u;
        guard=0u;
        (void)nesSubC7DEMenuInput(press,6u,&selection,&guard,&timer30,&timer31,&changed);
        if(selection>5u) { fputs("sub_C7DE range mismatch\n",stderr); return 6; }
    }
    return 0;
}


static int testSubC3BDConfirmSelection(void)
{
    uint8_t state, selection, mode, clear, flag40, flag44;

    state=8u; selection=0u; mode=0u; clear=0u; flag40=0xAAu; flag44=0xBBu;
    nesSubC3BDConfirmSelection(4u,&state,&selection,&mode,&clear,&flag40,&flag44);
    if(state!=10u || selection!=0u || mode!=0u || clear!=0u || flag40!=0u || flag44!=0u) {
        fputs("sub_C3BD selection 0 mismatch\n",stderr); return 9;
    }
    state=8u; selection=1u; mode=0u; clear=0u; flag40=0xAAu; flag44=0xBBu;
    nesSubC3BDConfirmSelection(4u,&state,&selection,&mode,&clear,&flag40,&flag44);
    if(state!=10u || selection!=1u || mode!=1u || clear!=0u || flag40!=0u || flag44!=0u) {
        fputs("sub_C3BD selection 1 / sub_C3B1 mismatch\n",stderr); return 1;
    }
    state=8u; selection=2u; mode=7u; clear=0u; flag40=0xAAu; flag44=0xBBu;
    nesSubC3BDConfirmSelection(4u,&state,&selection,&mode,&clear,&flag40,&flag44);
    if(state!=9u || mode!=7u || clear!=0u || flag40!=0u || flag44!=0u) {
        fputs("sub_C3BD selection 2 mismatch\n",stderr); return 2;
    }
    state=8u; selection=3u; mode=0u; clear=0u; flag40=0xAAu; flag44=0xBBu;
    nesSubC3BDConfirmSelection(4u,&state,&selection,&mode,&clear,&flag40,&flag44);
    if(state!=12u || selection!=0u || clear!=0u || flag40!=0u || flag44!=0u) {
        fputs("sub_C3BD selection 3 mismatch\n",stderr); return 3;
    }
    state=8u; selection=4u; mode=0u; clear=0u; flag40=0xAAu; flag44=0xBBu;
    nesSubC3BDConfirmSelection(4u,&state,&selection,&mode,&clear,&flag40,&flag44);
    if(state!=13u || selection!=0u || clear!=0u || flag40!=0u || flag44!=0u) {
        fputs("sub_C3BD selection 4 mismatch\n",stderr); return 4;
    }
    state=8u; selection=5u; mode=0u; clear=0u; flag40=0xAAu; flag44=0xBBu;
    nesSubC3BDConfirmSelection(4u,&state,&selection,&mode,&clear,&flag40,&flag44);
    if(state!=8u || selection!=2u || clear!=1u || flag40!=0xAAu || flag44!=0xBBu) {
        fputs("sub_C3BD selection 5 / clear branch mismatch\n",stderr); return 5;
    }
    state=8u; selection=3u; mode=0u; clear=0u; flag40=0xAAu; flag44=0xBBu;
    nesSubC3BDConfirmSelection(0u,&state,&selection,&mode,&clear,&flag40,&flag44);
    if(state!=8u || selection!=3u || clear!=0u || flag40!=0xAAu || flag44!=0xBBu) {
        fputs("sub_C3BD no-action mismatch\n",stderr); return 6;
    }
    state=0xFFu;
    nesSubC3AFIncrementState(&state);
    if(state!=0u) { fputs("sub_C3AF 8-bit wrap mismatch\n",stderr); return 7; }
    flag40=0x12u; flag44=0x34u;
    nesSubC3B1ResetFrameFlags(&flag40,&flag44);
    if(flag40!=0u || flag44!=0u) { fputs("sub_C3B1 flag reset mismatch\n",stderr); return 8; }
    puts("PASS: sub_C3BD confirmation branches and sub_C3AF/sub_C3B1 side effects.");
    return 0;
}


typedef struct {
    uint8_t carry;
    uint8_t zero;
    uint8_t negative;
} ReferenceCmpFlags;

static ReferenceCmpFlags referenceCmpFlags6502(uint8_t lhs, uint8_t rhs)
{
    uint16_t raw = (uint16_t)lhs + (uint16_t)(rhs ^ 0xFFu) + 1u;
    uint8_t result = (uint8_t)raw;
    ReferenceCmpFlags flags;
    flags.carry = (uint8_t)(raw > 0xFFu);
    flags.zero = (uint8_t)(result == 0u);
    flags.negative = (uint8_t)((result & 0x80u) != 0u);
    return flags;
}

/* Exhaustive comparison/flag tests with an independent 6502 CMP reference. */
static int testBatchAdditional20Boundaries(void)
{
    static const uint8_t edge[] = {0x00u,0x01u,0x7Fu,0x80u,0xFEu,0xFFu};
    unsigned int a,b,f;
    unsigned long checked=0u;

    /* sub_CFDB: zero leaves the byte untouched; nonzero ANDs with $0F. */
    for(a=0u;a<256u;++a) for(f=0u;f<256u;++f) {
        uint8_t actual=(uint8_t)f;
        uint8_t expected=(uint8_t)f;
        nesSubCFDBClearUpperFlags((uint8_t)a,&actual);
        if(a!=0u) expected=(uint8_t)(expected&0x0Fu);
        if(actual!=expected) { fprintf(stderr,"sub_CFDB mismatch A=%02X flags=%02X\n",a,f); return 1; }
        ++checked;
    }

    /* sub_CFCB: exhaustive high-byte compares and all low bytes against edges. */
    for(a=0u;a<256u;++a) for(b=0u;b<256u;++b) {
        ReferenceCmpFlags cmp=referenceCmpFlags6502((uint8_t)a,(uint8_t)b);
        uint8_t actual=0x5Au,expected=0x5Au;
        nesSubCFCBCompareObjectPosition((uint8_t)a,(uint8_t)b,0x80u,0x80u,&actual);
        if(!cmp.carry) expected=(uint8_t)(expected|0x80u);
        else if(a!=0u) expected=(uint8_t)(expected&0x0Fu);
        if(actual!=expected) { fprintf(stderr,"sub_CFCB high mismatch %02X/%02X\n",a,b); return 2; }
        ++checked;
    }
    for(a=0u;a<256u;++a) for(b=0u;b<256u;++b) {
        uint8_t other=edge[b%6u];
        ReferenceCmpFlags cmp=referenceCmpFlags6502((uint8_t)b,other);
        uint8_t actual=0xA5u,expected=0xA5u;
        nesSubCFCBCompareObjectPosition((uint8_t)a,(uint8_t)a,(uint8_t)b,other,&actual);
        if(!cmp.carry) expected=(uint8_t)(expected|0x80u);
        else if(a!=0u) expected=(uint8_t)(expected&0x0Fu);
        if(actual!=expected) { fprintf(stderr,"sub_CFCB low mismatch %02X/%02X\n",b,other); return 3; }
        ++checked;
    }

    /* sub_CFEA: all A, operand, and initial flag-byte combinations. */
    for(a=0u;a<256u;++a) for(b=0u;b<256u;++b) {
        ReferenceCmpFlags cmp=referenceCmpFlags6502((uint8_t)a,(uint8_t)b);
        for(f=0u;f<256u;++f) {
            uint8_t actual=(uint8_t)f;
            uint8_t expected=(uint8_t)(f&0xF0u);
            nesSubCFEAUpdateComparisonFlags((uint8_t)a,(uint8_t)b,&actual);
            if(a!=0u && !cmp.zero)
                expected=(uint8_t)(expected|(cmp.carry?2u:1u));
            if(actual!=expected) { fprintf(stderr,"sub_CFEA mismatch A=%02X operand=%02X flags=%02X\n",a,b,f); return 4; }
            ++checked;
        }
    }
    printf("PASS: comparison/flag boundary audit (%lu cases).\n",checked);
    return 0;
}

static int testSubD310MenuTimers(void)
{
    uint8_t frame=0u, timer30=4u, timer31=0x38u, timer34=0x2Du;
    unsigned int i;
    nesSubD310DecreaseMenuTimers(&frame,&timer30,&timer31,&timer34);
    if(frame!=0x0Au || timer30!=3u || timer31!=0x37u || timer34!=0x2Cu) {
        fputs("sub_D310 menu timer initial tick mismatch\n",stderr); return 1;
    }
    for(i=0u;i<10u;++i) nesSubD310DecreaseMenuTimers(&frame,&timer30,&timer31,&timer34);
    if(timer30!=3u || timer31!=0x37u || timer34!=0x2Cu) {
        fputs("sub_D310 menu timer divider mismatch\n",stderr); return 2;
    }
    nesSubD310DecreaseMenuTimers(&frame,&timer30,&timer31,&timer34);
    if(timer30!=2u || timer31!=0x36u || timer34!=0x2Bu) {
        fputs("sub_D310 menu timer rollover mismatch\n",stderr); return 3;
    }
    puts("PASS: sub_D310 timer projection for menu $0030/$0031/$0034.");
    return 0;
}

static int testState13Completion(void)
{
    uint8_t source[256], destination[256], state;
    unsigned int i;
    for (i = 0u; i < 256u; ++i) { source[i] = (uint8_t)(i ^ 0xA5u); destination[i] = 0u; }
    nesSubEC3BCopy256(source, destination);
    if (memcmp(source, destination, sizeof(source)) != 0) return 1;
    state = 13u; nesLocCC6BUpdateMenuState(&state, 0u); if (state != 8u) return 2;
    state = 8u; nesLocCC6BUpdateMenuState(&state, 1u); if (state != 9u) return 3;
    state = 0xFFu; nesLocCC6BUpdateMenuState(&state, 1u); if (state != 0u) return 4;
    puts("PASS: state-13 sub_EC3B copy and loc_CC6B clamp/wrap paths.");
    return 0;
}



static int testSubC55EC56A(void)
{
    uint8_t state = 0xA5u;
    uint8_t timer = 0xA5u;
    nesSubC55ESetState06(&state);
    if (state != 0x06u || timer != 0xA5u) return 1;
    state = 0xA5u;
    nesSubC56ASetState04(&timer, &state);
    if (timer != 0x04u || state != 0x04u) return 2;
    puts("PASS: menu command entries C55E and C56A preserve exact state/timer writes.");
    return 0;
}

static int testSubC455SelectState(void)
{
    uint8_t state = 0xA5u;
    nesSubC455SelectState(0x07u, 0u, &state);
    if (state != 0xA5u) return 1;
    nesSubC455SelectState(0x07u, 1u, &state);
    if (state != 0x07u) return 2;
    state = 0xA5u;
    nesSubC455SelectState(0x08u, 0u, &state);
    if (state != 0x08u) return 3;
    state = 0xA5u;
    nesSubC455SelectState(0xFFu, 0u, &state);
    if (state != 0x08u) return 4;
    puts("PASS: C455 menu state selection, unsigned threshold, and untouched-byte path.");
    return 0;
}

static int testState2SetupCommands(void)
{
    static uint8_t ram[65536];
    unsigned int i;
    memset(ram, 0xA5, sizeof(ram));
    ram[0x0041u]=0x07u; ram[0x0043u]=0x03u;
    nesSubC046Command1(ram);
    if(ram[0x0045u]!=0x15u)return 1;
    ram[0x0041u]=0x08u; ram[0x0045u]=0u;
    nesSubC046Command1(ram);
    if(ram[0x0045u]!=0x12u)return 2;
    ram[0x0041u]=0x07u; ram[0x0043u]=0xEEu;
    nesSubC046Command1(ram);
    if(ram[0x0045u]!=0x06u)return 17; /* ADC wraps to zero: BNE falls through C55E. */

    memset(ram,0xA5,sizeof(ram));
    nesSubC046Command2(ram);
    for(i=0x004Du;i<=0x00EFu;++i)if(ram[i]!=0u)return 3;
    if(ram[0x004Cu]!=0xA5u||ram[0x00F0u]!=0xA5u)return 4;

    memset(ram,0xA5,sizeof(ram));
    nesSubC046Command3(ram);
    if(ram[0x0000u]!=0u||ram[0x0001u]!=3u||ram[0x0045u]!=1u||ram[0x004Bu]!=0u)return 5;
    if(ram[0x0002u]!=0xC0u||ram[0x0003u]!=0xFCu||ram[0x0004u]!=0x83u||ram[0x0005u]!=0x21u)return 6;
    for(i=0x0340u;i<=0x03EFu;++i)if(ram[i]!=0u)return 7;
    if(ram[0x033Fu]!=0xA5u||ram[0x03F0u]!=0xA5u)return 8;
    for(i=0u;i<0x80u;++i)
        if(ram[0x0400u+i]!=0x3Bu||ram[0x0480u+i]!=0x3Du||ram[0x0500u+i]!=0x3Du)return 9;
    if(ram[0x0200u]!=0xBFu||ram[0x0201u]!=0xFFu||ram[0x0203u]!=0xF7u||
       ram[0x00E8u]!=0x3Fu||ram[0x00E9u]!=0x1Fu||ram[0x00EBu]!=0x0Au||ram[0x00EAu]!=0x25u)return 10;

    ram[0x0045u]=0u; nesSubC046Command4(ram); if(ram[0x0045u]!=0x17u)return 11;
    nesSubC046Command5(ram); if(ram[0x0045u]!=0x0Eu)return 12;

    memset(ram,0,sizeof(ram));
    ram[0x0034u]=3u; ram[0x03F2u]=1u; ram[0x0019u]=0x03u;
    if(nesSubC046Command6InitPrefix(ram)!=0u)return 13;
    if(ram[0x0047u]!=1u||ram[0x0040u]!=1u||ram[0x0034u]!=3u||
       ram[0x00E8u]!=0x34u||ram[0x00E9u]!=0x14u||ram[0x03F1u]!=0xA0u||
       ram[0x03B4u]!=0x80u||ram[0x03B0u]!=0x80u||ram[0x03BDu]!=0xFFu||
       ram[0x0370u]!=1u||ram[0x03BEu]!=2u)return 14;
    if(ram[0x00B8u]!=0x0Eu||ram[0x00B9u]!=0x1Au||ram[0x00Bau]!=0x26u||
       ram[0x00BBu]!=0x32u)return 15;
    if(ram[0x0080u]!=0x38u||ram[0x0081u]!=0x48u||ram[0x0082u]!=0x58u||
       ram[0x0083u]!=0x68u)return 16;
    if(ram[0x0300u]!=0x14u||ram[0x030Bu]!=0x24u||ram[0x030Cu]!=0xCFu||
       ram[0x030Du]!=0x07u||ram[0x030Fu]!=0xFBu||ram[0x0312u]!=0xFBu||
       ram[0x0315u]!=0u)return 18;

    /* C8E0 alternate-mode branch and C53A's additive $23 RAM-table offset. */
    memset(ram,0,sizeof(ram));
    ram[0x0041u]=0x08u; ram[0x0043u]=0x04u; ram[0x0046u]=1u;
    ram[0x03F2u]=1u; ram[0x03F3u]=42u; ram[0x03F8u]=0x20u;
    ram[0x05AFu]=2u; ram[0x05B0u]=3u; ram[0x05B1u]=4u;
    if(nesSubC046Command6InitPrefix(ram)!=0u)return 19;
    if(ram[0x0300u]!=0x1Bu||ram[0x0306u]!=4u||ram[0x0307u]!=2u)return 20;
    if(ram[0x0312u]!=0x24u||ram[0x0313u]!=0xCFu||
       ram[0x0314u]!=0x07u||ram[0x0316u]!=0xFBu||
       ram[0x0319u]!=0xFBu||ram[0x031Cu]!=0u)return 21;
    puts("PASS: all six state-2 setup commands, including normal/alternate C875 PPU command-buffer paths.");
    return 0;
}

static int testLocCBA3SetTimerAndAdvance(void)
{
    uint8_t timer = 0xA5u;
    uint8_t index = 0xFEu;
    nesLocCBA3SetTimerAndAdvance(0x31u, &timer, &index);
    if (timer != 0x31u || index != 0xFFu) return 1;
    nesLocCBA3SetTimerAndAdvance(0x00u, &timer, &index);
    if (timer != 0x00u || index != 0x00u) return 2;
    return 0;
}

int main(void)
{
    if (testLocCBA3SetTimerAndAdvance() != 0) { fputs("FAIL: loc_CBA3 timer/state advance\\n", stderr); return 1; }
    if (testSubC455SelectState() != 0) { fputs("FAIL: menu command C455\\n", stderr); return 1; }
    if (testSubC55EC56A() != 0) { fputs("FAIL: menu commands C55E/C56A\\n", stderr); return 1; }
    {
        int state2Result = testState2SetupCommands();
        if (state2Result != 0) {
            fprintf(stderr, "FAIL: state-2 setup regression vector %d\\n", state2Result);
            return 90 + state2Result;
        }
    }
    if (testState13Completion() != 0) { fputs("FAIL: state-13 completion\n", stderr); return 1; }
    if (testSubD310MenuTimers() != 0) { fputs("FAIL: sub_D310 menu timers\n", stderr); return 1; }
    if (testSubCFE3MaskFlags() != 0) { fputs("FAIL: sub_CFE3\n", stderr); return 1; }
    if (testSubC3BDConfirmSelection() != 0) { fputs("FAIL: sub_C3BD menu confirm\n", stderr); return 1; }
    if (testSubC7DEMenuInput() != 0) { fputs("FAIL: sub_C7DE menu input\n", stderr); return 1; }
    if (testBatchAdditional20Boundaries() != 0) { fputs("FAIL: comparison/flag boundary audit\n",stderr); return 1; }
    if (testBatchAdditional20() != 0) { fputs("FAIL: additional 20 ASM routines\n", stderr); return 1; }
    if (testSubEC99Exact() != 0) { fputs("FAIL: sub_EC99\n", stderr); return 1; }
    if (testSubFF53Exact() != 0) { fputs("FAIL: sub_FF53\n", stderr); return 1; }
    if(testBatchC1FDC229()!=0) { fputs("FAIL: sub_C1FD/sub_C229\n",stderr); return 1; }
    if(testBatchC32F()!=0) { fputs("FAIL: sub_C32F\n",stderr); return 1; }
    if(testBatchC53Family()!=0) { fputs("FAIL: sub_C53A/sub_C53E/sub_CB06\n",stderr); return 1; }
    if(testBatchC716C726()!=0) { fputs("FAIL: sub_C716/sub_C726\n",stderr); return 1; }
    if(testBatchDC97()!=0) { fputs("FAIL: sub_DC97/sub_DC9A\n",stderr); return 1; }
    if(testBatchEC25EC5D()!=0) { fputs("FAIL: sub_EC25/sub_EC5D\n",stderr); return 1; }
    if(testBatchEC3B()!=0) { fputs("FAIL: sub_EC3B\n",stderr); return 1; }
    if(testBatchEC75()!=0) { fputs("FAIL: sub_EC75\n",stderr); return 1; }
    if(testBatchF681()!=0) { fputs("FAIL: sub_F681\n",stderr); return 1; }
    if(testBatchF821F86B()!=0) { fputs("FAIL: sub_F821/sub_F86B\n",stderr); return 1; }
    if(testBatchF889()!=0) { fputs("FAIL: sub_F889\n",stderr); return 1; }
    if(testBatchC94B()!=0) { fputs("FAIL: sub_C94B\n",stderr); return 1; }
    if(testBatchC64E653()!=0) { fputs("FAIL: sub_C64E/sub_C653\n",stderr); return 1; }
    if(testBatchC333C3B1()!=0||testBatchC621C68A()!=0||testBatchC971CA1B()!=0||testBatchCB13()!=0||testBatchCFE3D24F()!=0||testBatchCF8BECBF()!=0||testBatchF676F826()!=0||testBatchF82E839()!=0||testBatchC62D()!=0||testBatchF6FB()!=0||testBatchF871F885()!=0) { fputs("FAIL: 20-routine batch\n",stderr); return 1; }
    if(testD0F4PrepareCommandStream()!=0) { fputs("FAIL: sub_D0F4 setup\n",stderr); return 1; }
    if(testD1A9ScrollWrites()!=0) { fputs("FAIL: sub_D1A9 scroll writes\n",stderr); return 1; }
    if(testD1B3ScrollWrites()!=0) { fputs("FAIL: sub_D1B3 scroll writes\n",stderr); return 1; }
    if(testE8xxConstantAndResetEntries()) { fputs("FAIL: E8BF/E8C6/E8E3/E8F5/E8FA/E963\n",stderr); return 1; }
    if(testE8xxDispatchEntries()) { fputs("FAIL: E845/E854/E85D/E86A/E879 dispatch entries\n",stderr); return 1; }
    if(testE7FCAndE7FF()) { fputs("FAIL: sub_E7FC/E7FF\n",stderr); return 1; }
    if(testE80DPreparePointer()) { fputs("FAIL: sub_E80D\n",stderr); return 1; }
    if(testE803ReadIndexedRam()) { fputs("FAIL: sub_E803\n",stderr); return 1; }
    if(testOfsE81807Apply()) { fputs("FAIL: ofs_E818_07\n",stderr); return 1; }
    if (testE84FStorePair()) { fputs("FAIL: sub_E84F\n",stderr); return 1; }
    if (testE86FSubtractFromE4()) { fputs("FAIL: sub_E86F\n",stderr); return 1; }
    if (testE893SetOffsetsAndSubtract()) { fputs("FAIL: sub_E893\n",stderr); return 1; }
    if (testE73BAddXLow()) { fputs("FAIL: sub_E73B\n",stderr); return 1; }
    if (testE733UpdateThreeXLowSlots()) { fputs("FAIL: sub_E733\n",stderr); return 1; }
    if (testLocE96EVerticalDelta() != 0) return 1;
    if(testSubE1A3Exact()) { fputs("FAIL: sub_E1A3 reference test\n",stderr); return 1; }
    if(testSubE2AFCountdownExact()) { fputs("FAIL: E2AF countdown reference test\n",stderr); return 1; }
    if(testSubEA44Exact()) { fputs("FAIL: sub_EA44 reference test\n",stderr); return 1; }
    if(testSubE2F6Exact()) { fputs("FAIL: sub_E2F6 reference test\n",stderr); return 1; }
    if(testSubE359Exact()) { fputs("FAIL: sub_E359 reference test\n",stderr); return 1; }
    if(testSubE26AExact()) { fputs("FAIL: sub_E26A reference test\n",stderr); return 1; }
    if(testSubE456Exact()) { fputs("FAIL: sub_E456 reference test\n",stderr); return 1; }
    if(testSubE4C8Exact()) { fputs("FAIL: sub_E4C8 exact reference test\n",stderr); return 1; }
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
static int testSubFF53Exact(void)
{
    static uint8_t memory[65536];
    unsigned int a, carry, offset;
    for (unsigned int address = 0u; address < 65536u; ++address)
        memory[address] = (uint8_t)((address ^ (address >> 8)) & 0xFFu);
    for (a = 0u; a < 256u; ++a) {
        for (carry = 0u; carry < 2u; ++carry) {
            for (offset = 0u; offset < 256u; ++offset) {
                uint8_t x = (uint8_t)a, value = (uint8_t)a;
                uint8_t c = (uint8_t)carry, expectedY, actualY = 0u, expected;
                uint16_t address;
                c = (uint8_t)(value & 1u);
                value = (uint8_t)((value >> 1) | (uint8_t)(carry << 7));
                value = x;
                for (unsigned int bit = 0u; bit < 3u; ++bit) {
                    uint8_t next = (uint8_t)(value >> 7);
                    value = (uint8_t)((uint8_t)(value << 1) | c);
                    c = next;
                }
                value = (uint8_t)((value & 0x07u) + (uint8_t)offset);
                expectedY = value;
                address = (uint16_t)(0xFF3Eu + (uint16_t)expectedY);
                expected = memory[address];
                {
                    uint8_t actual = nesSubFF53Lookup((uint8_t)a, (uint8_t)carry,
                                                       (uint8_t)offset, memory, &actualY);
                    if (actual != expected || actualY != expectedY) {
                        fprintf(stderr, "sub_FF53 mismatch A=%02X C=%u offset=%02X Y=%02X/%02X value=%02X/%02X\n",
                                a, carry, offset, actualY, expectedY, actual, expected);
                        return 1;
                    }
                }
            }
        }
    }
    puts("PASS: sub_FF53 ROR/ROL carry flow and indexed 6502 memory read.");
    return 0;
}

static int testSubEC99Exact(void)
{
    uint8_t y=3u,c0=0u,b0=0x10u,b1=0x1Fu,o2=0x3Fu,o3=0x20u;
    uint8_t out2=0xAAu,out3=0xBBu;
    nesSubEC99AdvanceObjectPhase(0u,&y,&c0,o2,o3,&b0,&b1,&out2,&out3);
    if(y!=3u||c0!=0u||b0!=0x10u||b1!=0x1Fu||out2!=0xAAu||out3!=0xBBu)return 1;
    nesSubEC99AdvanceObjectPhase(4u,&y,&c0,o2,o3,&b0,&b1,&out2,&out3);
    if(y!=7u||c0!=0u||b0!=0x10u||out2!=0xAAu)return 2;
    nesSubEC99AdvanceObjectPhase(1u,&y,&c0,o2,o3,&b0,&b1,&out2,&out3);
    if(y!=0u||c0!=1u||b1!=0u||b0!=(uint8_t)(0x10u^0x04u)||out2!=0u||out3!=0x21u)return 3;
    puts("PASS: sub_EC99 early exits, exact $08 trigger, phase helper, and $40 wrap.");
    return 0;
}

static int testNewE3AA(void){uint8_t t=0u,l=0u,f=1u;nesLocE3AAUpdateHeatFlags(0x20u,1u,&t,&l,&f);return(t==0x20u&&l==0x20u&&f==0xB1u)?0:1;}


/* Regression vectors for the additional 20 byte-level routine translations. */
static int testBatchAdditional20(void)
{
    uint8_t f, oam[256], buf[263], ram[256], source7[7], table8[8], table256[256];
    uint8_t statusV[3]={0x80u,0xFFu,0x00u}, statusD[3]={0x00u,0x80u,0x40u};
    uint8_t ppu=0x55u, ctrl=0u, ctrlRam=0u, hold0=0u, press0=0u, hold1=0u, press1=0u, strobe[2];
    uint8_t samples0[8]={0u,0u,0u,0u,0u,0u,0u,0u}, samples1[8]={1u,1u,1u,1u,1u,1u,1u,1u};
    unsigned int used=0u, i;
    for (i=0u;i<256u;++i) { table256[i]=(uint8_t)i; ram[i]=(uint8_t)(i^0xA5u); }
    for (i=0u;i<263u;++i) buf[i]=0x11u;
    for (i=0u;i<8u;++i) { table8[i]=(uint8_t)(0x20u+i); oam[0xC0u+i]=0u; }
    for (i=0u;i<7u;++i) source7[i]=(uint8_t)(0x70u+i);

    f=0x5Au; nesSubCFDBClearUpperFlags(1u,&f); if(f!=0x0Au)return 1;
    f=0xA5u; nesSubCFDBClearUpperFlags(0u,&f); if(f!=0xA5u)return 2;
    f=0x35u; nesSubCFCBCompareObjectPosition(4u,5u,0u,0u,&f); if(f!=0xB5u)return 3;
    f=0xA5u; nesSubCFCBCompareObjectPosition(7u,6u,0u,0u,&f); if(f!=0x05u)return 4;
    f=0x5Fu; nesSubCFEAUpdateComparisonFlags(2u,3u,&f); if(f!=0x51u)return 5;
    f=0x5Fu; nesSubCFEAUpdateComparisonFlags(3u,3u,&f); if(f!=0x50u)return 6;
    f=0x5Fu; nesSubCFEAUpdateComparisonFlags(4u,3u,&f); if(f!=0x52u)return 7;
    if(!nesSubCE58ObjectIndexMatches(3u,3u)||nesSubCE58ObjectIndexMatches(3u,4u))return 8;
    if(nesLocCAFAFillCommand(2u,buf)!=9u)return 9;
    for(i=6u;i<13u;++i)if(buf[i]!=0xFEu)return 10;
    nesLocCD13CopySpriteTemplate(oam,table8);
    for(i=0u;i<8u;++i)if(oam[0xC0u+i]!=table8[i])return 11;
    if(nesLocD0A6StoreAndReturn(0xA0u,0x6Cu,0x91u,ram)!=0x91u||ram[0xA0u]!=0x6Cu)return 12;
    nesLocD197WriteScrollRegister(0x42u,&ppu); if(ppu!=0x42u)return 13;
    memset(oam,0,sizeof(oam)); nesLocEC47WriteFixedSprite(oam,0x7Bu);
    if(oam[0x34u]!=0xCFu||oam[0x35u]!=0x7Bu||oam[0x36u]!=0u||oam[0x37u]!=0xE4u)return 14;
    memset(ram,0xCC,sizeof(ram)); ram[0x20u]=0x44u;ram[0x42u]=0x77u;
    nesLocCA85ClearRam(ram);
    if(ram[0x20u]!=0x44u||ram[0x42u]!=0x77u||ram[0x49u]!=1u||ram[0x21u]!=0u||ram[0xFFu]!=0u)return 15;
    if(!nesSubC339WaitForVblank(statusV,3u,0x25u,&ctrl,&ctrlRam,&used)||used!=3u||ctrl!=0xA5u||ctrlRam!=0xA5u)return 16;
    ctrl=0x33u;ctrlRam=0x44u;
    if(nesSubC339WaitForVblank(statusV,2u,0x25u,&ctrl,&ctrlRam,&used)!=0u||ctrl!=0x33u||ctrlRam!=0x44u)return 17;
    if(!nesSubD19BWaitForStatus(statusD,3u,0xABu,&ctrl,&ctrlRam,&used)||used!=3u||ctrl!=0xA0u||ctrlRam!=0xA0u)return 18;
    nesSubD347ReadControllers(samples0,samples1,0u,0u,&hold0,&press0,&hold1,&press1,strobe);
    if(strobe[0]!=1u||strobe[1]!=0u)return 19;
    if(nesLocC4FCComputeScaledValue(10u,20u,2u,&f)!=15u||f!=15u)return 20;
    if(nesLocC4FCComputeScaledValue(30u,10u,0u,&f)!=14u||f!=14u)return 21;
    {
        uint8_t cpuRam[2048]={0}, r2=0u,r3=0u,r5=0u,r6=0u,r7=0u;
        cpuRam[0x5A9u]=0xA1u;cpuRam[0x5AAu]=0xA2u;cpuRam[0x5ABu]=0xA3u;
        cpuRam[0x5B9u]=0xB1u;cpuRam[0x5BAu]=0xB2u;cpuRam[0x5BBu]=0xB3u;
        cpuRam[0x5C3u]=0xC1u;cpuRam[0x5C4u]=0xC2u;cpuRam[0x5C5u]=0xC3u;
        /* Y=2 => base 6; $0046 ORs $10; $03F2 then adds $23. */
        nesSubC92CSelectTableEntry(0x37u,2u,1u,1u,cpuRam,&r2,&r3,&r5,&r6,&r7);
        if(r2!=2u||r3!=0x37u||r5!=0xB1u||r6!=0xB2u||r7!=0xB3u)return 22;
        /* C5A7 is exactly LDA #$00 / JMP loc_C92C. */
        nesSubC5A7SelectMenuTableEntry(2u,1u,1u,cpuRam,&r2,&r3,&r5,&r6,&r7);
        if(r2!=2u||r3!=0u||r5!=0xB1u||r6!=0xB2u||r7!=0xB3u)return 43;
        /* C53A preserves $0003 while writing $0002 and the tuple. */
        r3=0x66u;
        nesSubC53AReadMenuTuple(2u,1u,1u,cpuRam,&r2,&r5,&r6,&r7);
        if(r2!=2u||r3!=0x66u||r5!=0xB1u||r6!=0xB2u||r7!=0xB3u)return 44;
        /* Without $0046, the same path selects base 6 + $23 = $29. */
        nesSubC92CSelectTableEntry(0x00u,2u,0u,1u,cpuRam,&r2,&r3,&r5,&r6,&r7);
        if(r2!=2u||r3!=0u||r5!=0xA1u||r6!=0xA2u||r7!=0xA3u)return 37;
        /* Y=5 takes the special $20 path and ignores the $0046 bank bit. */
        nesSubC92CSelectTableEntry(0x00u,5u,1u,1u,cpuRam,&r2,&r3,&r5,&r6,&r7);
        if(r2!=5u||r5!=0xC1u||r6!=0xC2u||r7!=0xC3u)return 38;
        nesSubC92CSelectTableEntry(0x00u,0xFFu,0u,0u,cpuRam,&r2,&r3,&r5,&r6,&r7);
        if(r2!=0xFFu||r3!=0u||r5!=cpuRam[0x580u]||
           r6!=cpuRam[0x581u]||r7!=cpuRam[0x582u])return 42;
    }
    {
        uint8_t optionTable[256], v5=0u,v6=0u,v7=0u;
        for(i=0u;i<256u;++i) optionTable[i]=(uint8_t)(i^0x5Au);
        nesSubC522SelectOptionTuple(2u,1u,optionTable,&v5,&v6,&v7);
        if(v5!=(uint8_t)(0x16u^0x5Au)||v6!=(uint8_t)(0x17u^0x5Au)||
           v7!=(uint8_t)(0x18u^0x5Au))return 39;
        /* $0043=5 selects $20 directly, regardless of $0046. */
        nesSubC522SelectOptionTuple(5u,1u,optionTable,&v5,&v6,&v7);
        if(v5!=(uint8_t)(0x20u^0x5Au)||v6!=(uint8_t)(0x21u^0x5Au)||
           v7!=(uint8_t)(0x22u^0x5Au))return 40;
        /* DEY immediately sets N for $FF, so the BPL loop adds nothing. */
        nesSubC522SelectOptionTuple(0xFFu,0u,optionTable,&v5,&v6,&v7);
        if(v5!=optionTable[0]||v6!=optionTable[1]||v7!=optionTable[2])return 41;
    }
    {
        uint8_t ramMenu[65536], tblC091[258];
        memset(ramMenu,0,sizeof(ramMenu));
        memset(tblC091,0,sizeof(tblC091));
        ramMenu[0x0041u]=8u;
        ramMenu[0x0068u]=1u; ramMenu[0x0069u]=2u; ramMenu[0x006Au]=3u;
        ramMenu[0x0580u]=1u; ramMenu[0x0581u]=2u; ramMenu[0x0582u]=3u;
        nesSubC485UpdateMenuSelection(ramMenu,tblC091);
        if(ramMenu[0x0053u]!=1u||ramMenu[0x0045u]!=0x18u||
           ramMenu[0x0049u]!=1u||ramMenu[0x0040u]!=1u||ramMenu[0x0044u]!=0u)return 45;

        /* A candidate tuple greater than $0068-$006A is replaced in $0580. */
        memset(ramMenu,0,sizeof(ramMenu));
        ramMenu[0x0041u]=8u;
        ramMenu[0x0068u]=1u; ramMenu[0x0069u]=2u; ramMenu[0x006Au]=3u;
        ramMenu[0x0580u]=2u; ramMenu[0x0581u]=0u; ramMenu[0x0582u]=0u;
        nesSubC485UpdateMenuSelection(ramMenu,tblC091);
        if(ramMenu[0x0053u]!=1u||ramMenu[0x0580u]!=1u||
           ramMenu[0x0581u]!=2u||ramMenu[0x0582u]!=3u)return 46;

        /* If the first tuple is lower, state < 8 compares the ROM tuple. */
        memset(ramMenu,0,sizeof(ramMenu));
        ramMenu[0x0041u]=0u;
        ramMenu[0x0068u]=1u; ramMenu[0x0069u]=0u; ramMenu[0x006Au]=0u;
        ramMenu[0x0580u]=0u; ramMenu[0x0581u]=0u; ramMenu[0x0582u]=0u;
        tblC091[0]=2u;
        nesSubC485UpdateMenuSelection(ramMenu,tblC091);
        if(ramMenu[0x0053u]!=2u)return 47;

        /* State >= 8 skips sub_C522; this exercises C4DC-C510 byte math. */
        memset(ramMenu,0,sizeof(ramMenu));
        ramMenu[0x0041u]=8u; ramMenu[0x03F8u]=1u;
        ramMenu[0x0068u]=2u; ramMenu[0x0069u]=1u; ramMenu[0x006Au]=0u;
        ramMenu[0x0580u]=1u; ramMenu[0x0581u]=0u; ramMenu[0x0582u]=0u;
        nesSubC485UpdateMenuSelection(ramMenu,tblC091);
        if(ramMenu[0x0053u]!=130u||ramMenu[0x0008u]!=60u||
           ramMenu[0x0009u]!=2u)return 48;
    }
    {
        uint8_t ramCopy[65536];
        memset(ramCopy,0,sizeof(ramCopy));
        for(i=0u;i<256u;++i) ramCopy[0x06E0u+i]=(uint8_t)(i^0xA5u);
        nesSubC621CopyRaceData(ramCopy);
        for(i=0u;i<256u;++i)
            if(ramCopy[0x05E0u+i]!=(uint8_t)(i^0xA5u))return 49;
        if(ramCopy[0x05DFu]!=0u||ramCopy[0x07E0u]!=0u)return 50;
    }
    {
        uint8_t state44=0u,state48=0u,fb=0x22u,state41=3u;
        nesLocC43DHandleMenuState(&state44,&state48,&fb,&state41,0u);
        if(state44!=1u||state48!=1u||fb!=0x22u||state41!=3u)return 51;
        state44=1u; state48=0x22u; fb=0x11u; state41=5u;
        nesLocC43DHandleMenuState(&state44,&state48,&fb,&state41,0u);
        if(state44!=1u||state48!=0x22u||fb!=0x11u||state41!=5u)return 52;
        state44=1u; state48=0xFFu; fb=0u; state41=8u;
        nesLocC43DHandleMenuState(&state44,&state48,&fb,&state41,0u);
        if(fb!=0x40u||state41!=8u)return 53; /* INC then clamp at 8. */
        state41=8u; state48=0xFFu; fb=0u;
        nesLocC43DHandleMenuState(&state44,&state48,&fb,&state41,1u);
        if(fb!=0x40u||state41!=9u)return 54; /* $004B nonzero disables clamp. */
    }
    {
        uint8_t romC12C[260]={0}, low=0u, high=0u;
        const uint8_t lows[4]={0x3Bu,0x77u,0xA4u,0x6Bu};
        const uint8_t highs[4]={0xCBu,0xCBu,0xCBu,0xCCu};
        for(i=0u;i<4u;++i) { romC12C[i]=lows[i]; romC12C[4u+i]=highs[i]; }
        for(i=0u;i<4u;++i) {
            const uint16_t target=(uint16_t)((uint16_t)lows[i]|((uint16_t)highs[i]<<8u));
            if(nesSubCB2CDispatchAddress((uint8_t)i,romC12C,&low,&high)!=target||
               low!=lows[i]||high!=highs[i])return 55;
        }
    }
    source7[1]=0xA2u; source7[4]=0xB4u;
    if(nesSubC935BuildCommand(25u,34u,68u,1u,source7,buf)!=8u)return 23;
    if(source7[0]!=5u||source7[1]!=0xA2u||source7[2]!=3u||source7[3]!=4u||
       source7[4]!=0xB4u||source7[5]!=6u||source7[6]!=8u)return 24;
    for(i=0u;i<7u;++i)if(buf[5u+i]!=source7[i])return 30;
    for(i=0u;i<256u;++i) { ram[i]=0x3Du; oam[i]=0x3Du; table256[i]=0x3Bu; }
    if(nesSubECC9CheckSpriteBytes(9u,ram,oam,table256,table256)!=0u)return 25;
    table256[9u]=0u; if(nesSubECC9CheckSpriteBytes(9u,ram,oam,table256,table256)!=1u)return 26;
    {
        uint8_t sprite[4]={0x11u,0x22u,0x33u,0x44u}, flag=0xFFu;
        nesLocC411WriteSpriteX(0x99u,sprite,&flag);
        if(sprite[0]!=0x11u||sprite[1]!=0x22u||sprite[2]!=0u||sprite[3]!=0x99u||flag!=0u)return 27;
        nesLocC3EFWriteSpriteY(0x68u,&sprite[0]); if(sprite[0]!=0x68u)return 28;
        memset(table256,0,sizeof(table256)); table256[2]=0xACu;
        memset(sprite,0,sizeof(sprite)); flag=0xFFu;
        if(nesSubC409SetupStaticSprite(2u,table256,sprite,&flag)!=0u||
           sprite[1]!=0xFAu||sprite[2]!=0u||sprite[3]!=0xACu||flag!=0u)return 29;
    }
    {
        uint8_t initOam[256], nt[2048], sx=0x12u, sy=0x34u, pctrl=0xFFu, pmask=0xFFu;
        unsigned int j;
        memset(initOam,0,sizeof(initOam)); memset(nt,0,sizeof(nt));
        nesSubC318InitializePpu(initOam,nt,&sx,&sy,&pctrl,&pmask);
        for(j=0u;j<256u;++j) if(initOam[j]!=0xF8u)return 31;
        for(j=0u;j<2048u;++j) {
            const uint8_t expected=((j%1024u)<960u)?0xFCu:0u;
            if(nt[j]!=expected)return 32;
        }
        if(sx!=0u||sy!=0u||pctrl!=0x10u||pmask!=0u)return 33;
    }
    {
        uint8_t index=0u, value=0u;
        if(nesSubC41CState13Entry(&index,&value)!=0u||index!=1u||value!=2u)return 34;
        if(nesSubC41CState13Entry(&index,&value)!=0u||index!=1u||value!=2u)return 35;
        value=0xFFu;
        if(nesSubC41CState13Entry(&index,&value)!=1u||index!=1u||value!=0xFFu)return 36;
    }
    puts("PASS: additional ASM batch: compare branches, buffer writes, RAM clearing, PPU polling, controller latch and sprite setup.");
    return 0;
}
