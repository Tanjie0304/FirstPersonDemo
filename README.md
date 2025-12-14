# UE5 多人联机射击游戏 Demo - 箐英班大作业

## 简介

这是一个基于 **Unreal Engine 5** 和 **C++** 开发的多人联机第一人称射击游戏 Demo。
项目包含了完整的网络同步框架，实现了从主菜单建立主机、客户端通过 IP 加入、主菜单等待到跳转至主关卡的完整流程。

> **核心技术栈**：Unreal Engine 5.x, C++, Blueprints, UMG, Replication (Listen Server)

## 主要功能

* **多人联机系统**：
    * 基于 Listen Server（侦听服务器）架构。
    * 支持 **Host (建立主机)** 与 **Join (加入游戏)** 功能。
    * 支持输入 IP 地址进行局域网或公网直连。
* **关卡切换流程**：
    * 实现了主菜单 -> 战斗关卡的无缝切换。
    * 使用 `ServerTravel` 确保所有已连接客户端同步跳转。
    * 利用demo自带的资产重新调整了关卡布局。
* **战斗系统**：
    * 完整的生命值、弹药射击、场景物品的网络复制与同步。
    * 射击打靶（方块）计分机制（定时分波次的方块随机生成于场景中，并分配不同分值与赋予不同颜色）。
    * 击杀计分系统（窃取被击杀玩家一半分数）与死亡重生机制。
    * 游戏预开始倒计时机制（倒计时时玩家与NPC敌人无法射击与移动）。
    * 游戏结算机制以及结束重开机制。
    * 倒计时角色狂暴机制（玩家移动速度加快）。
    * 移动的敌人与攻击玩家，击杀敌人得大量分。
    * 场景中央定时生成回血包，并有简易动态特效。
* **UI 交互**：
    * 动态网络状态检测（根据是房主还是客户端显示不同按钮）。
    * 实时血条与弹药显示。
    * 实时游戏剩余时间显示。
    * 简易的计分板功能（按Tab键呼出）。
    * 狂暴状态简易的边框动态动画。
    * 开始游戏与结束游戏中央简易的动画文字特效。

## 📸 游戏截图 (Screenshots)

### 主菜单 (Main Menu)
这里是建立主机和输入 IP 加入的地方。
![主菜单](./screenshots/main_menu.jpg)

### 战斗画面 (Gameplay)
双人联机对战测试。
![战斗画面](./Screenshots/combat_test.png)

*(注：请确保你实际创建了 Screenshots 文件夹并放入了对应的图片，否则这里会裂图)*

## 🛠️ 如何运行 (How to Run)

1.  **克隆仓库**：
    ```bash
    git clone [https://github.com/你的用户名/你的仓库名.git](https://github.com/你的用户名/你的仓库名.git)
    ```
2.  **生成项目文件**：
    * 右键点击 `FirstPersonDemo.uproject`。
    * 选择 **Generate Visual Studio project files**。
3.  **编译**：
    * 打开生成的 `.sln` 文件。
    * 在 Visual Studio 中选择 `Development Editor` 模式进行编译。
4.  **运行**：
    * 双击 `.uproject` 打开 UE5 编辑器。
    * **测试联机**：建议在编辑器设置中将 `Number of Players` 设为 2，并选择 `Play as Listen Server` 模式运行。

## 📦 打包与联机 (Packaging & Networking)

如果要打包成 `.exe` 进行局域网联机：
1.  在项目设置中，确保 **List of maps to include** 包含了 `MenuLevel`, `LobbyMap` 和 `ShooterMap`。
2.  打包 Windows 版本。
3.  **房主**：运行游戏 -> 点击 "建立主机"。
4.  **客户端**：运行游戏 -> 输入房主 IP (局域网通常为 `192.168.x.x`) -> 点击 "加入游戏"。

## 📝 待办列表 (To-Do)

- [x] 基础移动与射击
- [x] 网络同步与大厅系统
- [ ] 添加更多武器类型
- [ ] 击杀计分板 UI

---
*Created by [你的名字]*
