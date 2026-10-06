/* Exercise production parser with firmware-like buffers on the build host. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "dynamic_info.c"

static void put16(UINT8 *p, UINT16 x) { memcpy(p,&x,2); }
static void put32(UINT8 *p, UINT32 x) { memcpy(p,&x,4); }
static UINT8 table[1024], entry[24];
static EFI_CONFIGURATION_TABLE config;
static EFI_SYSTEM_TABLE system_table;
static void install(UINTN length) {
    UINT64 address=(UINTN)table; UINTN i; UINT8 sum=0;
    memset(entry,0,sizeof(entry)); memcpy(entry,"_SM3_",5); entry[6]=24;
    put32(entry+12,length); memcpy(entry+16,&address,8);
    for(i=0;i<24;i++) sum+=entry[i]; entry[5]=(UINT8)(0-sum);
    config.VendorGuid=Smbios3; config.VendorTable=entry;
    memset(&system_table,0,sizeof(system_table));
    system_table.ConfigurationTable=&config; system_table.NumberOfTableEntries=1;
    ST=&system_table;
}
static void dimm(UINT8 *p,UINT16 size,UINT32 extended,UINT8 kind,UINT16 speed) {
    memset(p,0,94); p[0]=17; p[1]=92; put16(p+12,size); p[18]=kind;
    put32(p+28,extended); put16(p+32,speed);
}
int main(void) {
    CHAR16 cpu[64]; UINTN i;
    dimm(table,2048,0,0x22,6000); dimm(table+94,2048,0,0x22,5600);
    install(188); assert(SmbiosMemory()==4096ULL*1024*1024);
    assert(StrCmp(MemoryDetail,L"DDR5 2/2 DIMMs 5600 MT/s")==0);
    dimm(table,0x7fff,32768,0x1a,3200); install(94);
    assert(SmbiosMemory()==32768ULL*1024*1024);
    dimm(table,0x8400,0,0x18,1600); install(94);
    assert(SmbiosMemory()==1024ULL*1024);
    dimm(table,0xffff,0,0x22,6000); install(94); assert(!SmbiosMemory());
    dimm(table,0,0,0,0); install(94); assert(!SmbiosMemory());
    table[1]=3; assert(!SmbiosMemory());
    dimm(table,2048,0,0x22,6000); table[92]=1; table[93]=1;
    install(94); assert(!SmbiosMemory());
    memset(table,0,sizeof(table)); table[0]=4; table[1]=17; table[16]=1;
    memcpy(table+17,"Synthetic CPU fallback",22); install(41);
    assert(!SmbiosMemory()); assert(SmbiosCpu(cpu));
    assert(StrCmp(cpu,L"Synthetic CPU fallback")==0);
    entry[5]++; assert(!SmbiosMemory()); assert(!SmbiosCpu(cpu));
    /* Every truncation of a valid record must fail without out-of-bounds reads. */
    dimm(table,2048,0,0x22,6000);
    for(i=4;i<94;i++) { install(i); assert(!SmbiosMemory()); }
    puts("PASS: DIMMs, speed minimum, extended/KiB/unknown size, CPU fallback, checksum, malformed/truncated records");
    return 0;
}
