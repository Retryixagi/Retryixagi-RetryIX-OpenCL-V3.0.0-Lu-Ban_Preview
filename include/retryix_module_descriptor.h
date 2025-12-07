
#ifndef RETRYIX_MODULE_DESCRIPTOR_H
#define RETRYIX_MODULE_DESCRIPTOR_H

// 模組描述結構，可根據需求擴充


typedef struct {
    char name[128];
    int version_major;
    int version_minor;
    int status;
    char author[64];
    char description[256];
    char path[256];
    int loaded;
    int dependency_count;
    char dependencies[8][64];
    // ...可擴充欄位...
} module_descriptor_t;


// 模組狀態/錯誤碼
#define RETRYIX_MODULE_OK           0
#define RETRYIX_MODULE_NOT_FOUND   -1
#define RETRYIX_MODULE_LOAD_ERROR  -2
#define RETRYIX_MODULE_ALREADY_LOADED -3
#define RETRYIX_MODULE_DEP_ERROR   -4

typedef void (*retryix_module_event_cb)(const module_descriptor_t* desc, int event_code);

// 模組管理 API
int retryix_register_module(const module_descriptor_t* desc);
int retryix_unregister_module(const char* name);
int retryix_query_module(const char* name, module_descriptor_t* out_desc);
int retryix_list_modules(module_descriptor_t* out_list, int max_count);
int retryix_load_module(const char* path);
int retryix_unload_module(const char* name);
int retryix_reload_module(const char* name);
int retryix_set_module_event_callback(retryix_module_event_cb cb);
int retryix_is_module_loaded(const char* name);

#endif // RETRYIX_MODULE_DESCRIPTOR_H

// 其他模組相關定義、API 聲明
// ...
