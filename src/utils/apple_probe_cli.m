// apple_probe_cli.m
#import <Foundation/Foundation.h>
#include "retryix_platform.h"
#include "retryix_bridge_metal.h"
#include <stdio.h>

static void jstr(const char* s){
  for(const unsigned char* p=(const unsigned char*)s; *p; ++p){
    unsigned char c=*p;
    if(c=='"'||c=='\\') { putchar('\\'); putchar(c); }
    else if(c<0x20) printf("\\u%04x", c);
    else putchar(c);
  }
}

int main(){
  retryix_platform_t plats[8]; int n=0;
  if (retryix_discover_apple_metal_platforms(plats, 8, &n)!=RETRYIX_SUCCESS || n<=0) {
    puts("{\"error\":\"no metal device\"}");
    return 1;
  }

  // 推導語意：Apple Silicon（hasUnifiedMemory=1）→ door=USM；否則視為 Pinned
  const char* door="Pinned", *sub="Pinned", *cap="Alignment4K";
  if (strstr(plats[0].version, "unified:1")) { door="USM"; sub="USM"; cap="ZeroCopy"; }

  puts("{");
  printf("  \"platforms\": [\n");
  for(int i=0;i<n;++i){
    printf("    {\"vendor\":\""); jstr(plats[i].vendor);
    printf("\",\"name\":\"");      jstr(plats[i].name);
    printf("\",\"version\":\"");   jstr(plats[i].version);
    printf("\",\"profile\":\"");   jstr(plats[i].profile);
    printf("\",\"device_count\":%d}", plats[i].device_count);
    if (i!=n-1) printf(",");
    printf("\n");
  }
  printf("  ],\n  \"semantic_profile\": {\"type\":\"AGI\",\"sub\":\"%s\",\"cap\":\"%s\",\"door\":\"%s\"}\n", sub, cap, door);
  puts("}");
  return 0;
}
