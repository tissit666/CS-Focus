# CS2 专注守卫

使用 C++17 和 [EUI-NEO](https://sudoevolve.github.io/EUI-NEO/) 构建的 Windows 桌面工具。
程序每秒检测一次 `cs2.exe`。当 CS2 正在运行时，会关闭下列受控通讯软件：

- `QQ.exe`
- `WeChat.exe`
- `Weixin.exe`
- `WXWork.exe`
- `WeCom.exe`

检测到目标应用后，程序会立即调用 Windows 的 `TerminateProcess` 强制结束该进程。这会导致未保存的内容丢失。

## 构建

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
.\build\cs2_focus_guard.exe
```

EUI-NEO 源码保存在 `third_party/EUI-NEO`，可在本地重复构建。
