# L-Engine

`L-Engine` 是一个面向西南大学校内步行导航的课程项目，采用：

- `AMPPS` 提供 `Apache + PHP + MySQL`
- `MinGW g++` 编译并运行 C++ 最短路引擎
- `Leaflet` 展示在线地图与导航结果
- `OpenStreetMap` 校园路网数据作为底层地图来源

当前第一版已经实现：

- 西南大学校内步行路网导入
- 前端点击地图选起点和终点
- 后端自动吸附到最近路网节点
- C++ 引擎按预计步行时间计算最短路
- 返回总距离、预计时间和路线几何并在前端显示

## 项目结构

- [sql/schema.sql](/D:/GitCode/year-2/sql/schema.sql)：数据库结构
- [tools/import_swu_osm.py](/D:/GitCode/year-2/tools/import_swu_osm.py)：OSM 导入脚本
- [data/swu.osm](/D:/GitCode/year-2/data/swu.osm)：当前使用的西南大学地图数据
- [data/southwest_university_boundary.geojson](/D:/GitCode/year-2/data/southwest_university_boundary.geojson)：校园边界裁剪文件
- [engine/build.bat](/D:/GitCode/year-2/engine/build.bat)：C++ 构建脚本
- [engine/config.ini](/D:/GitCode/year-2/engine/config.ini)：C++ 引擎数据库配置
- [web/config.php](/D:/GitCode/year-2/web/config.php)：PHP 端数据库配置
- [web/index.php](/D:/GitCode/year-2/web/index.php)：地图主页
- [web/dashboard.php](/D:/GitCode/year-2/web/dashboard.php)：任务面板

## 运行环境

- Windows
- AMPPS
- Python 3
- MinGW g++
- MySQL C API

AMPPS 默认用到的 MySQL 相关目录：

- `D:\ampps\Ampps\mysql\include`
- `D:\ampps\Ampps\mysql\lib`

## 部署步骤

下面步骤按当前项目实际目录编写，适用于本机路径 `D:\GitCode\year-2`。

### 1. 配置数据库

确认以下两个文件中的数据库连接信息一致：

- [web/config.php](/D:/GitCode/year-2/web/config.php)
- [engine/config.ini](/D:/GitCode/year-2/engine/config.ini)

当前默认配置为：

- Host: `127.0.0.1`
- Port: `3307`
- Database: `l_engine`
- User: `root`
- Password: `mysql`

如果你的 AMPPS 配置不同，需要先手动改掉这两个文件中的值。

### 2. 导入西南大学 OSM 路网数据

当前项目已经使用本地地图文件：

- [data/swu.osm](/D:/GitCode/year-2/data/swu.osm)

在项目根目录 `D:\GitCode\year-2` 打开 PowerShell，执行：

```powershell
python .\tools\import_swu_osm.py --input D:\GitCode\year-2\data\swu.osm
```

这一步会自动完成以下工作：

- 重建数据库表
- 清空旧节点、旧边和旧任务
- 按校园边界裁剪西南大学路网
- 导入可步行道路
- 计算每条边的真实距离和预计步行时间

如果你后续修改了校园边界文件 [data/southwest_university_boundary.geojson](/D:/GitCode/year-2/data/southwest_university_boundary.geojson)，重新执行这条命令即可重新导入。

### 3. 编译 C++ 路由引擎

在项目根目录执行：

```powershell
.\engine\build.bat engine
.\engine\build.bat test
```

成功后会在 [engine/bin](/D:/GitCode/year-2/engine/bin) 下生成：

- [engine/bin/l_engine.exe](/D:/GitCode/year-2/engine/bin/l_engine.exe)
- [engine/bin/test_connection.exe](/D:/GitCode/year-2/engine/bin/test_connection.exe)

### 4. 测试数据库连接

建议先运行一次连接测试：

```powershell
.\engine\bin\test_connection.exe
```

如果正常，会输出类似内容：

```text
MySQL connection succeeded.
Loaded nodes: 1751
```

### 5. 启动 C++ 路由引擎

在项目根目录执行：

```powershell
.\engine\bin\l_engine.exe
```

正常启动后会输出类似：

```text
Connected to MySQL. Loading graph...
Graph ready. Node count: 1751
```

这个窗口需要保持运行，不能关闭。  
因为网页提交的任务，都是由这个 C++ 程序轮询数据库后完成处理的。

### 6. 部署 PHP 页面

将 [web](/D:/GitCode/year-2/web) 目录复制到 AMPPS 的网站根目录，或者建立映射，使浏览器能够访问该目录中的 PHP 文件。

例如部署后可以通过下面地址访问：

- `http://localhost/你的目录/index.php`
- `http://localhost/你的目录/dashboard.php`

### 7. 使用系统

打开地图页面后：

1. 选择“起点”模式
2. 在地图上点击起点位置
3. 选择“终点”模式
4. 在地图上点击终点位置
5. 点击“提交步行路径任务”

系统会自动：

- 将点击位置吸附到最近路网节点
- 向数据库写入待处理任务
- 由 C++ 引擎计算最优步行路径
- 返回总距离、预计时间和路线结果

### 8. 查看任务状态

可以打开 [web/dashboard.php](/D:/GitCode/year-2/web/dashboard.php) 查看：

- 当前节点数
- 当前边数
- 已提交任务数
- 等待中任务
- 已完成任务
- 失败任务

## 常见问题

### 1. 页面一直显示“任务正在处理中”

通常是 `l_engine.exe` 没有运行。

解决方法：

```powershell
.\engine\bin\l_engine.exe
```

如果启动后立刻退出，先重新执行：

```powershell
.\engine\build.bat engine
```

然后再启动一次。

### 2. 地图能显示，但没有路线结果

请检查：

- MySQL 是否启动
- `web/config.php` 和 `engine/config.ini` 是否一致
- `test_connection.exe` 是否能正常连接数据库
- `l_engine.exe` 是否保持运行

### 3. 地图中出现校外道路

这是因为校园边界文件还不够精确。

解决方法：

- 修改 [data/southwest_university_boundary.geojson](/D:/GitCode/year-2/data/southwest_university_boundary.geojson)
- 重新执行导入命令

## 路径计算说明

当前第一版使用“预计步行时间”作为最短路权重。

不同道路类型采用固定步行速度：

- `steps`：`0.75 m/s`
- `footway` / `path` / `pedestrian`：`1.35 m/s`
- `service` / `living_street` / `residential` / `unclassified` / `tertiary`：`1.25 m/s`

系统输出的主要结果字段：

- `distance_m`：总距离，单位米
- `travel_time_sec`：预计步行时间，单位秒
- `route_geometry`：前端绘图使用的路线几何数据

## 当前版本说明

当前版本为第一版，定位是“西南大学校内步行导航原型系统”。

已完成：

- 校园路网导入
- 校内路径计算
- 前后端联动显示

后续可以继续扩展：

- 更精确的校园边界
- 校内建筑物名称标注
- 多种出行模式
- 校外遮罩层
- 更精细的路径动画与交互
