# VirtualDrv 安全安裝與測試指南

## ⚠️ 重要安全提醒

**VirtualDrv現在已經過安全修復**，移除了所有可能導致BSOD的危險操作：
- ✅ 物理記憶體映射操作被安全拒絕
- ✅ 物理記憶體讀取返回模擬數據
- ✅ 解除映射操作安全模擬成功

## 安裝步驟

### 1. 啟用測試簽章（必需）
```cmd
bcdedit /set testsigning on
shutdown /r /t 0
```

### 2. 安裝驅動程式
```cmd
# 使用pnputil安裝
pnputil /add-driver F:\1208\drivers\virtualdrv\VirtualDrv.inf /install

# 或使用devcon（如果WDK已安裝）
devcon install F:\1208\drivers\virtualdrv\VirtualDrv.inf Root\VirtualRetryix
```

### 3. 驗證安裝
```cmd
# 檢查驅動程式服務
sc query VirtualDrv

# 檢查設備
driverquery | findstr Virtual
```

## 安全測試

運行安全測試腳本：
```cmd
cd F:\1208\drivers\test_driver
python test_virtual_safe.py
```

### 測試結果解釋

- **✅ 模式操作**: 設定和查詢驅動程式模式
- **✅ 安全調用操作**: 測試安全的命令（0,1,2）
- **✅ 危險操作阻止**: 確認危險命令被安全處理
- **✅ 失敗模式**: 測試錯誤注入功能

## 驅動程式功能

### 模式
- **模擬模式 (0)**: 返回預定義的模擬數據
- **失敗模式 (1)**: 總是返回錯誤，用於測試錯誤處理
- **轉發模式 (2)**: 將請求轉發到真實驅動程式（需要配置目標）

### 安全命令
- **命令 0**: 基本測試，返回處理過的payload
- **命令 1**: 常數返回測試
- **命令 2**: 延遲測試（支援毫秒級延遲）
- **命令 3**: 錯誤代碼注入測試

### 受保護的危險命令
- **命令 14 (MAP_PHYSICAL)**: 返回 `STATUS_ACCESS_DENIED`
- **命令 15 (UNMAP_PHYSICAL)**: 安全模擬成功
- **命令 16 (READ_PHYS_MEMORY)**: 返回安全模擬數據

## 故障排除

### 驅動程式無法載入
1. 確認測試簽章已啟用
2. 檢查驅動程式檔案是否存在
3. 查看系統事件日誌的錯誤信息

### 測試失敗
1. 確認驅動程式已正確安裝
2. 檢查Python腳本的執行權限
3. 確認使用管理員權限運行

## 移除驅動程式

```cmd
# 停止服務
sc stop VirtualDrv

# 刪除驅動程式
pnputil /delete-driver VirtualDrv.inf /uninstall

# 重新啟動
shutdown /r /t 0
```

## 開發者注意事項

- 此驅動程式僅供測試使用
- 所有危險操作已被安全移除
- 建議在虛擬機中進行測試
- 生產環境請勿使用此驅動程式