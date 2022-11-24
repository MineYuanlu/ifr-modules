# CONFIG

> 简单的配置文件管理器

## create

```cpp
/**
 * 注册一个配置文件
 * @tparam T 配置文件数据类型
 * @param name 配置文件名称
 * @param data 数据指针
 * @param info 配置文件信息
 * @return 配置文件控制器
 */
template<typename T>
ConfigController createConfig(const std::string &name, T *data, const ConfigInfo<T> &info)
```

创建一个配置文件需要提供文件名、数据指针、配置文件信息

## ConfigInfo

> 配置文件信息

配置文件信息用于决定如何序列化及反序列化数据

```cpp
/**
 * 配置文件信息
 * @tparam T 配置文件数据类型
 */
template<typename T>
struct ConfigInfo {
std::function<void(T *, rapidjson::Writer<OStreamWrapper> &)> serialize;//序列化
std::function<void(T *, const rapidjson::Document &)> deserialize;//反序列化
};
```

### serialize

序列化函数, 传入数据指针及rapidjson的Writer, 用于将数据写入json文件

### deserialize

反序列化函数, 传入数据指针及rapidjson的Document, 用于从json文件读出数据

## ConfigController

> 配置文件控制器

配置文件控制器用于控制配置文件的加载及保存

```cpp
/**
* 配置文件控制器
*/
struct ConfigController {
std::function<void()> save;//保存
std::function<void()> load;//加载
};
```

## 辅助函数

简单的辅助函数, 用于在低标准C++上实现文件系统操作

```cpp
/**
 * 创建文件夹 (递归创建, 调用系统命令)
 * @param path 文件夹路径
 */
void mkDir(std::string path);
```

```cpp
/**
 * 获取路径的文件夹
 * @param fname 文件路径
 * @return 所在文件夹
 */
std::string getDir(std::string fname);
```

## 样例

```cpp
struct Point{
    int x,y;
};
Point p;
void init() {
	static ifr::Config::ConfigInfo<Point> info = {
		[](auto *a, auto &w) {
			std::unique_lock<std::recursive_mutex> lock(mtx);
			w.StartObject();
			w.Key("list");
			w.StartArray();
			for (const auto &e: plans)w.String(e.first);
			w.EndArray();
			w.Key("current"), w.String(currentPlans);
			w.EndObject();
		},
		[](auto *a, const rapidjson::Document &d) {
			std::unique_lock<std::recursive_mutex> lock(mtx);
			for (const auto &item: d["list"].GetArray()) {
				std::string name = item.GetString();
				if (!checkPlanName(name))continue;
				readPlanInfo(name);
			}
			planListJson = "";
			currentPlans = d["current"].GetString();
		}
	};
	cc = ifr::Config::createConfig("xy", &p, info);
	cc.load();
}
```