# RetryIX SVM拓撲集成 - ComfyUI節點

這個模塊提供了在ComfyUI中集成RetryIX SVM（Shared Virtual Memory）拓撲發現和內存管理功能的完整解決方案。

## 功能特點

- 🔍 **SVM拓撲發現**: 自動檢測系統的記憶體拓撲結構
- 💾 **拓撲感知內存分配**: 根據拓撲優化內存分配策略
- 📊 **即時指標監控**: 監控內存使用統計和拓撲指標
- 🔄 **動態內存遷移**: 在不同NUMA域間遷移內存
- 🗑️ **安全內存清理**: 自動釋放已分配的SVM內存

## 支持的ComfyUI節點

### 1. 🔍 SVM拓撲發現 (SVMTologyDiscoveryNode)
- **輸入**: refresh_topology (布林值，可選)
- **輸出**:
  - topology_data: 完整的拓撲數據結構 (DICT)
  - topology_info: 拓撲摘要信息 (STRING)

### 2. 💾 拓撲感知內存分配 (SVMTopologyAwareAllocNode)
- **輸入**:
  - size_mb: 分配大小 (MB，默認1024)
  - set_affinity: 是否設置親和性 (布林值，默認True)
  - topology_data: 拓撲數據 (可選)
- **輸出**:
  - memory_pointer: 分配的內存指針 (POINTER)
  - alloc_info: 分配信息 (STRING)
  - memory_stats: 內存統計 (DICT)

### 3. 📊 SVM拓撲指標監控 (SVMTopologyMetricsNode)
- **輸入**: memory_pointer: 內存指針 (可選)
- **輸出**:
  - topology_metrics: 拓撲指標數據 (DICT)
  - metrics_info: 指標摘要 (STRING)

### 4. 🔄 SVM內存域遷移 (SVMMemoryMigrationNode)
- **輸入**:
  - memory_pointer: 要遷移的內存指針
  - from_domain: 來源域ID
  - to_domain: 目標域ID
- **輸出**:
  - migration_success: 遷移是否成功 (BOOLEAN)
  - migration_info: 遷移結果信息 (STRING)

### 5. 🗑️ SVM內存清理 (SVMMemoryCleanupNode)
- **輸入**: memory_pointer: 要清理的內存指針
- **輸出**:
  - cleanup_success: 清理是否成功 (BOOLEAN)
  - cleanup_info: 清理結果信息 (STRING)

## 使用方法

### 在ComfyUI中拖拽使用

1. 在ComfyUI節點面板中找到 "RetryIX" 分類
2. 拖拽需要的節點到工作區
3. 連接節點形成工作流程
4. 運行工作流程

### 典型工作流程

```
🔍 SVM拓撲發現 → 💾 拓撲感知內存分配 → 📊 SVM拓撲指標監控 → 🔄 SVM內存域遷移 → 🗑️ SVM內存清理
```

### Python代碼使用

```python
from pytorch_custom_backend_package.pytorch_retryix_svm_nodes import (
    SVMTologyDiscoveryNode,
    SVMTopologyAwareAllocNode,
    SVMMemoryCleanupNode,
)

# 發現拓撲
discovery = SVMTologyDiscoveryNode()
topology, info = discovery.discover_topology()

# 分配內存
allocator = SVMTopologyAwareAllocNode()
ptr, alloc_info, stats = allocator.alloc_memory(size_mb=1024)

# 清理內存
cleanup = SVMMemoryCleanupNode()
success, cleanup_info = cleanup.cleanup_memory(ptr)
```

## 依賴項

- ctypes (Python標準庫)
- RetryIX DLL (F:/cloud/254/retryix_production/lib/retryix.dll)
- PyTorch Custom Backend

## DLL函數映射

模塊自動映射以下RetryIX DLL函數：

- `retryix_discover_svm_topology_json`: 發現SVM拓撲
- `retryix_svm_alloc_topology_aware`: 拓撲感知內存分配
- `retryix_svm_free`: 釋放SVM內存
- `retryix_set_workload_topology_affinity`: 設置拓撲親和性
- `retryix_svm_get_statistics`: 獲取內存統計
- `retryix_svm_get_topology_metrics`: 獲取拓撲指標
- `retryix_svm_migrate_between_domains`: 內存域遷移

## 錯誤處理

- 如果RetryIX DLL不可用，模塊會自動切換到模擬模式
- 所有內存操作都有完整的錯誤檢查
- 提供詳細的錯誤信息和恢復建議

## 性能優化

- 拓撲感知的內存分配可提高記憶體訪問效率
- NUMA優化減少跨節點延遲
- 動態遷移支持工作負載適應性調整

## 測試

運行測試腳本驗證功能：

```bash
python test_retryix_svm_integration.py
```

運行演示腳本：

```bash
python demo_retryix_svm.py
```

## 工作流程範例

參見 `svm_topology_workflow.json` 文件，包含完整的工作流程配置。

## 注意事項

- 確保RetryIX DLL路徑正確
- 在生產環境中使用前請充分測試
- 內存操作需要小心管理，避免內存泄漏
- 拓撲信息可能因系統配置而異

## 授權

MIT License