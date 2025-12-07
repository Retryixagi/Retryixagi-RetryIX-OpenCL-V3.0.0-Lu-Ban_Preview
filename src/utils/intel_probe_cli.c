// intel_probe_cli.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "retryix_platform.h"
#include "retryix_bridge_intel_l0.h"
#include "retryix_bridge_opencl_intel.h"
#include "retryix_bridge_cpu_intel.h"

static void print_json_escaped(const char* s){
  for(const char* p=s; *p; ++p){
    unsigned char c=(unsigned char)*p;
    if(c=='"'||c=='\\') { putchar('\\'); putchar(c); }
    else if(c<0x20) printf("\\u%04x", c);
    else putchar(c);
  }
}

int main(void){
  retryix_platform_t plats[16]; int cnt=0;

  // L0
  int c_l0=0;
  if (retryix_discover_intel_l0_platforms(plats+cnt, 16-cnt, &c_l0)==RETRYIX_SUCCESS) cnt+=c_l0;

  // OpenCL
  int c_ocl=0;
  if (retryix_discover_opencl_intel_platforms(plats+cnt, 16-cnt, &c_ocl)==RETRYIX_SUCCESS) cnt+=c_ocl;

  // CPU
  int c_cpu=0;
  if (retryix_discover_cpu_intel(plats+cnt, 16-cnt, &c_cpu)==RETRYIX_SUCCESS) cnt+=c_cpu;

  // 推導 door / cap（簡易規則）
  const char* door = "None";
  const char* type = "AGI";   // 你的語意類型標籤
  const char* sub  = "Unknown";
  const char* cap  = "None";
  int has_gpu_l0=0, has_gpu_usm=0, has_gpu_svm=0, has_cpu=0;

  for(int i=0;i<cnt;++i){
    if (strcmp(plats[i].profile,"LevelZero")==0 && strcmp(plats[i].vendor,"Intel")==0) { has_gpu_l0=1; }
    if (strcmp(plats[i].profile,"OpenCL")==0 && strcmp(plats[i].vendor,"Intel")==0) {
      if (strstr(plats[i].version,"USM:1")) has_gpu_usm=1;
      if (strstr(plats[i].version,"SVM:1")) has_gpu_svm=1;
    }
    if (strcmp(plats[i].profile,"CPU")==0 && strcmp(plats[i].vendor,"Intel")==0) has_cpu=1;
  }

  if (has_gpu_l0) { door="USM"; sub="USM"; cap="ZeroCopy"; }
  else if (has_gpu_usm) { door="USM"; sub="USM"; cap="ZeroCopy"; }
  else if (has_gpu_svm) { door="SVM"; sub="SVM"; cap="ZeroCopy"; }
  else if (has_cpu)     { door="Pinned"; sub="Pinned"; cap="Alignment4K"; }

  // JSON 輸出
  puts("{");
  printf("  \"platforms\": [\n");
  for(int i=0;i<cnt;++i){
    printf("    {\"vendor\":\""); print_json_escaped(plats[i].vendor);
    printf("\",\"name\":\"");      print_json_escaped(plats[i].name);
    printf("\",\"version\":\"");   print_json_escaped(plats[i].version);
    printf("\",\"profile\":\"");   print_json_escaped(plats[i].profile);
    printf("\",\"device_count\":%d}", plats[i].device_count);
    if (i!=cnt-1) printf(",");
    printf("\n");
  }
  printf("  ],\n");
  printf("  \"semantic_profile\": {\"type\":\"%s\",\"sub\":\"%s\",\"cap\":\"%s\",\"door\":\"%s\"}\n", type, sub, cap, door);
  puts("}");
  return 0;
}
