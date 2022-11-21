# API

> 调试接口

此模块用于网页调参, 开放端口(默认`0.0.0.0:8000`)提供http及websocket路由。

API模块目前主要用于Plan编辑, 请配合[前端](https://github.com/MineYuanlu/ifr-opencv-web)使用

## 安全

由于仅供内部人员使用, 故未添加任何安全措施(仅前端有数据验证), 可能后期会添加身份验证。

## 默认路由

默认路由在`registerRoute`中注册:

- `GET` /time/detail
- `GET` /time/list
- `GET` /task/descriptions
- `GET` /plan/list
- `GET` /plan/state
- `GET` /plan/get
- `POST` /plan/save
- `DELETE` /plan/remove
- `GET` /plan/use
- `GET` /plan/start
- `GET` /plan/stop
- `GET` /api.json

## 内部路由

所有`OPTIONS`都将返回如下信息:

```
HTTP/1.1 204 No Content
Access-Control-Allow-Methods: OPTIONS, GET, POST, DELETE
Allow: OPTIONS, GET, POST, DELETE
Access-Control-Allow-Origin: *
Access-Control-Allow-Headers: *
Content-Length: 0000000000
```

## WebSocket

目前Websocket预留了接口, 但还未实际使用。