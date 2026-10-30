# Smart Shopping System · 在线商城系统

一个**不用任何 Web 框架**、用 C 语言从零写出来的在线商城系统。HTTP 服务器、路由、请求解析、响应封装全部自己实现，后端逻辑直接对接 SQLite，前端是原生 HTML/CSS/JS 页面。

> 适合作为网络编程 / Linux 系统编程课程设计参考。

---

## ✨ 功能特性

| 模块 | 说明 |
| --- | --- |
| 用户注册 | `POST /register`，表单数据经 URL 解码后写入 `users` 表 |
| 用户登录 | `POST /login`，SQLite 回调校验账号密码，成功进入搜索页 |
| 商品搜索 | `POST /search`，按关键词模糊匹配 `goods` 表，返回 JSON |
| 商品详情 | `GET /goods_detail?id=<id>`，查询单条商品记录并返回 JSON |
| 静态资源 | 自研静态文件服务，自动识别 `Content-Type`，支持图片/HTML/JS |
| 路径安全 | `is_path_safe()` 拦截 `../` 路径穿越，越权访问返回 403 |

## 🏗 技术架构

```
浏览器 (HTML + AJAX)
        │  HTTP/1.1
        ▼
┌───────────────────────────────┐
│  webserver.c  自研 HTTP 服务器 │
│  ├─ CreateListenSocket() 监听 8080
│  ├─ RecvHttpRequest()    收包
│  ├─ ParseHttpRequest()   解析请求行/头/体
│  └─ SendHttpResponse()   路由分发 + 响应
└───────────────┬───────────────┘
                │  sqlite3 C API
                ▼
        operation.c  业务逻辑
        （注册 / 登录 / 搜索 / 详情）
                │
                ▼
           123.db  SQLite 数据库
```

- **纯 C 实现**：只用 POSIX socket + pthread + SQLite，无第三方 Web 库
- **单进程阻塞模型**：`accept → recv → parse → 响应 → close` 循环处理
- **前后端分离思路**：页面负责渲染，数据通过 `fetch` 请求 JSON 接口

## 📁 目录结构

```
.
├── main.c                 # 入口：创建监听、accept 循环、分发请求
├── webserver.c / .h       # HTTP 协议实现：收包、解析、路由、响应
├── operation.c / .h       # 业务逻辑：注册、登录、搜索、商品详情
├── head.h                 # 公共头文件集合
├── Makefile               # 构建脚本
├── homepage.html          # 首页 / 登录页
├── register.html          # 注册页
├── search.html            # 商品搜索页
├── goods_detail.html      # 商品详情页
├── goods_detail.json      # 详情接口返回数据（运行时写入）
├── search_result.json     # 搜索接口返回数据（运行时写入）
├── images/200905/         # 商品图片资源（source_img / goods_img / thumb_img）
├── sqlite3.c / sqlite3.h  # SQLite 官方源码（静态编入）
└── 演示视频.mp4            # 功能演示录屏
```

## 🚀 快速开始

### 环境要求

- Linux / macOS（依赖 `unistd.h`、`pthread`、POSIX socket，**Windows 原生环境需 WSL**）
- gcc、make

### 编译

```bash
make
```

生成可执行文件 `a.out`。

```bash
make clean   # 清理编译产物
```

### 运行

```bash
./a.out
# Server listening on port 8080...
```

浏览器打开 <http://127.0.0.1:8080/> 即可访问首页。

### 准备数据库

仓库未包含数据库文件（含用户数据），运行时需要把 ECShop 的 `123.db` 放到**与 `a.out` 同级**的目录：

```bash
# 把 123.db 复制到项目根目录后启动
./a.out
```

程序直接以相对路径打开 `123.db`，缺失时注册/登录/搜索会失败。

## 🔌 接口一览

| 方法 | 路径 | 说明 | 返回 |
| --- | --- | --- | --- |
| GET | `/` | 首页 | `homepage.html` |
| GET | `/register.html` | 注册页 | `register.html` |
| GET | `/search.html` | 搜索页 | `search.html` |
| GET | `/goods?id=<id>` | 商品详情页 | `goods_detail.html` |
| GET | `/goods_detail?id=<id>` | 商品详情**数据** | JSON / 404 JSON |
| GET | `/<静态路径>` | 静态资源 | 文件内容或 403 |
| POST | `/register` | 用户注册 | 成功 → 首页，失败 → 注册页 |
| POST | `/login` | 用户登录 | 成功 → 搜索页，失败 → 首页 |
| POST | `/search` | 关键词搜索 | JSON |

## 🗄 数据库

`123.db` 采用 ECShop 表结构（120+ 张表），本系统实际使用：

- `users` —— 用户账号（`user_name`、`password`、`email` 等）
- `goods` —— 商品主表（`goods_id`、`goods_name`、`shop_price`、图片字段等）

## 📌 已知限制

- 单线程阻塞式处理，不支持并发连接，仅适用于学习与演示
- 密码明文/MD5 存储，未做加盐与 HTTPS
- 路由为硬编码 `if/else` 分发，新增接口需改源码重编译

## 📄 License

仅供学习交流使用。
