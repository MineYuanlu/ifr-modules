//
// Created by yuanlu on 2022/9/22.
//

#include "API.h"

#include <memory>
#include <utility>

using namespace std;
namespace ifr {
    namespace API {
#define COMMON_JSON_HEADER "Access-Control-Allow-Origin:*\nContent-Type: application/json;charset:utf-8;\n" //通用头(json)
#define COMMON_TEXT_HEADER "Access-Control-Allow-Origin:*\nContent-Type: text/plain;charset:utf-8;\n" //通用头(text)
#define STR_MG2STD(s) std::string((s).ptr,(s).len)
#define STR_RBUF2MG(buf) mg_str_n((buf).GetString(),(buf).GetLength())
        const size_t npos = -1;

        struct handler_data {
            const char *pattern;//匹配模式串
            const char *method;//限定请求类型
            mg_event_handler_t fn;//处理器
        };

        vector <handler_data> http_route; //所有的路由

        map<unsigned long, struct mg_connection *> wsClients;//ws客户端
        mutex ws_mtx;//websocket 同步锁


        namespace TimeWatcherHelper {
            const constexpr static clock_t max_cache = 300;

            struct Data {
                std::weak_ptr<TimeWatcher> tw;//
                clock_t cache_lst = -1;//上次缓存时间
                std::string cache;//缓存
            };

            static std::mutex mtx;//访问锁
            static std::map<std::string, Data> datas;

            std::string getTimeWatch(const std::string &type) {
                std::unique_lock<std::mutex> lock(mtx);
                if (!datas.count(type))return "";
                auto &data = datas[type];
                if (auto sp = data.tw.lock()) {
                    auto now = clock();
                    if (now - data.cache_lst > max_cache) {
                        data.cache_lst = now;
                        return data.cache = sp->getTime();
                    } else return data.cache;
                } else {
                    datas.erase(type);
                    return "";
                }
            }

            std::string getTimeWatchList() {
                std::unique_lock<std::mutex> lock(mtx);

                rapidjson::StringBuffer buf;
                rapidjson::Writer<rapidjson::StringBuffer> w(buf);

                w.StartArray();
                for (const auto &x: datas) {
                    if (!x.second.tw.expired()) w.String(x.first);
                }
                w.EndArray();

                w.Flush();
                return buf.GetString();
            }

            void registerTimeWatcher(const shared_ptr <TimeWatcher> &tw) {
                std::unique_lock<std::mutex> lock(mtx);
                if (datas.count(tw->type) && !datas[tw->type].tw.expired())
                    throw runtime_error("[TimeWatcher] " + tw->type + " has been registered");
                datas[tw->type] = {tw};
            }

        }


        void sendWsReal(const std::string &str) {
            std::unique_lock<mutex> lock(ws_mtx);
            for (const auto &client: wsClients) {
                mg_ws_send(client.second, str.c_str(), str.length(), WEBSOCKET_OP_TEXT);
            }
        }

        void sendWs(ifr::Plans::msgType wsType, const string &type, const std::string &subType, const string &msg) {
            rapidjson::StringBuffer buf;
            rapidjson::Writer<rapidjson::StringBuffer> w(buf);
            w.StartObject();
            w.Key("ws-type"), w.Int(wsType);
            w.Key("type"), w.String(type);
            w.Key("sub-type"), w.String(subType);
            w.Key("msg"), w.String(msg);
            w.EndObject();
            w.Flush();
            sendWsReal(buf.GetString());
        }


        static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
            switch (ev) {
                case MG_EV_WS_OPEN: {
                    {
                        std::unique_lock<mutex> lock(ws_mtx);
                        wsClients[c->id] = c;
                    }
                    sendWsReal("connected " + to_string(c->rem.ip) + ", " + to_string(c->id));
                    cout << "已连接ws " << c->rem.ip << endl;
                    break;
                }
                case MG_EV_CLOSE: {
                    std::unique_lock<mutex> lock(ws_mtx);
                    wsClients.erase(c->id);
                    break;
                }
                case MG_EV_HTTP_MSG: {
                    auto hm = (struct mg_http_message *) ev_data;

                    //OPTIONS
                    if (!mg_vcmp(&hm->method, "OPTIONS")) {
                        mg_http_reply(c, 204,
                                      "Access-Control-Allow-Methods: OPTIONS, GET, POST, DELETE\r\nAllow: OPTIONS, GET, POST, DELETE\r\nAccess-Control-Allow-Origin:*\r\nAccess-Control-Allow-Headers:*\r\n",
                                      "");
                        return;
                    }
                    //注册的路由
                    for (const auto &r: http_route) {
                        if (!mg_http_match_uri(hm, r.pattern))continue;
                        if (mg_vcmp(&hm->method, r.method))continue;

                        try {
                            r.fn(c, ev, ev_data, fn_data);
                        } catch (exception &e) {
                            if (c->is_resp)
                                mg_http_reply(c, 500, COMMON_TEXT_HEADER, e.what());
                        } catch (...) {
                            if (c->is_resp)
                                mg_http_reply(c, 500, COMMON_TEXT_HEADER, "An unknown error has occurred");
                        }

                        return;
                    }
                    mg_http_reply(c, 404, COMMON_TEXT_HEADER, "Not Found");
                    return;
                }
                default:break;
            }
        }

        void registerRoute() {
            http_route.push_back(
                    {"/time/detail", "GET", [](auto c, int ev, auto ev_data, auto fn_data) {
                        auto hm = (mg_http_message *) ev_data;
                        auto type = mgx_getquery(hm->query, "type");
                        auto r = TimeWatcherHelper::getTimeWatch(STR_MG2STD(type));
                        if (r.empty())
                            mg_http_reply(c, 404, COMMON_TEXT_HEADER, "Not Found");
                        else mg_http_reply(c, 200, COMMON_JSON_HEADER, r.c_str());
                    }
                    });
            http_route.push_back(
                    {"/time/list", "GET", [](auto c, int ev, auto ev_data, auto fn_data) {
                        mg_http_reply(c, 200, COMMON_JSON_HEADER, TimeWatcherHelper::getTimeWatchList().c_str());
                    }
                    });
            http_route.push_back(
                    {"/vars/enable", "GET", [](auto c, int ev, auto ev_data, auto fn_data) {
                        mg_http_reply(c, 200, COMMON_JSON_HEADER,
#if IFRAPI_HAS_VARIABLE
                                      "true"
#else
                                "false"
#endif
                        );
                    }
                    });
#if IFRAPI_HAS_VARIABLE
            http_route.push_back(
                    {"/vars/var", "GET", [](auto c, int ev, auto ev_data, auto fn_data) {
                        if (Variable::locked) {
                            auto key = mgx_getquery(((mg_http_message *) ev_data)->query, "key");
                            char buf[1024];
                            mg_url_decode(key.ptr, key.len, buf, sizeof(buf), 0);
                            const auto itr = Variable::vars.find(buf);
                            if (itr == Variable::vars.end())mg_http_reply(c, 404, COMMON_TEXT_HEADER, "Not Found key");
                            else mg_http_reply(c, 200, COMMON_TEXT_HEADER, itr->second.getValue().c_str());
                        } else mg_http_reply(c, 500, COMMON_TEXT_HEADER, "uninitialized");
                    }
                    });
            http_route.push_back(
                    {"/vars/var", "POST", [](auto c, int ev, auto ev_data, auto fn_data) {
                        if (Variable::locked) {
                            auto hm = (mg_http_message *) ev_data;
                            auto key = mgx_getquery(hm->query, "key");
                            char buf[1024];
                            mg_url_decode(key.ptr, key.len, buf, sizeof(buf), 0);
                            auto value = STR_MG2STD(hm->body);
                            const auto itr = Variable::vars.find(buf);
                            if (itr == Variable::vars.end())mg_http_reply(c, 404, COMMON_TEXT_HEADER, "Not Found key");
                            else {
                                itr->second.setValue(value);
                                mg_http_reply(c, 200, COMMON_TEXT_HEADER, itr->second.getValue().c_str());
                            }
                        } else mg_http_reply(c, 500, COMMON_TEXT_HEADER, "uninitialized");
                    }
                    });
            http_route.push_back(
                    {"/vars/descriptions", "GET", [](auto c, int ev, auto ev_data, auto fn_data) {
                        if (Variable::locked) {
                            static string json;
                            if (json.empty()) {
                                rapidjson::StringBuffer buf;
                                rapidjson::Writer<decltype(buf)> w(buf);
                                w.StartObject();
                                w.Key("types"), w.StartObject();
                                {
                                    w.Key("bool"), w.String(typeid(bool).name());
                                    w.Key("long"), w.String(typeid(long).name());
                                    w.Key("int"), w.String(typeid(int).name());
                                    w.Key("unsigned"), w.String(typeid(unsigned).name());
                                    w.Key("unsigned long"), w.String(typeid(unsigned long).name());
                                    w.Key("float"), w.String(typeid(float).name());
                                    w.Key("double"), w.String(typeid(double).name());
                                }
                                w.EndObject();
                                w.Key("vars"), w.StartArray();
                                for (const auto &e: Variable::vars) {
                                    w.StartObject();
                                    w.Key("group"), w.String(e.second.group_);
                                    w.Key("type"), w.String(e.second.type_);
                                    w.Key("name"), w.String(e.second.name_);
                                    w.Key("def"), w.String(e.second.def_);
                                    w.Key("min"), w.String(e.second.min_);
                                    w.Key("max"), w.String(e.second.max_);
                                    w.Key("key"), w.String(e.first);
                                    w.EndObject();
                                }
                                w.EndArray();
                                w.EndObject();
                                json = buf.GetString();
                            }
                            mg_http_reply(c, 200, COMMON_JSON_HEADER, json.c_str());
                        } else mg_http_reply(c, 500, COMMON_TEXT_HEADER, "uninitialized");
                    }
                    });
#endif
            http_route.push_back(
                    {"/task/descriptions", "GET", [](auto c, int ev, auto ev_data, auto fn_data) {
                        mg_http_reply(c, 200, COMMON_JSON_HEADER,
                                      ifr::Plans::getTaskDescriptionsJson().c_str());
                    }
                    });
            http_route.push_back(
                    {"/plan/list", "GET", [](auto c, int ev, auto ev_data, auto fn_data) {
                        mg_http_reply(c, 200, COMMON_JSON_HEADER, ifr::Plans::getPlanListJson().c_str());
                    }
                    });
            http_route.push_back(
                    {"/plan/state", "GET", [](auto c, int ev, auto ev_data, auto fn_data) {
                        mg_http_reply(c, 200, COMMON_JSON_HEADER, ifr::Plans::getPlanStateJson().c_str());
                    }
                    });
            http_route.push_back(
                    {"/plan/get", "GET", [](auto c, int ev, auto ev_data, auto fn_data) {
                        auto hm = (mg_http_message *) ev_data;
                        auto pname = mgx_getquery(hm->query, "pname");
                        if (!pname.len) {
                            mg_http_reply(c, 400, COMMON_TEXT_HEADER, "no query: pname");
                            return;
                        }
                        auto plan = ifr::Plans::getPlanInfo(STR_MG2STD(pname));
                        if (!plan.loaded)
                            mg_http_reply(c, 404, COMMON_TEXT_HEADER, "Not Found");
                        else {
                            rapidjson::StringBuffer buf;
                            rapidjson::Writer<StringBuffer> w(buf);
                            plan(w);
                            w.Flush();
                            mg_http_reply(c, 200, COMMON_JSON_HEADER, buf.GetString());
                        }
                    }
                    });
            http_route.push_back(
                    {"/plan/save", "POST", [](auto c, int ev, auto ev_data, auto fn_data) {
                        auto hm = (mg_http_message *) ev_data;
                        try {
                            rapidjson::Document d;
                            d.Parse(hm->body.ptr, hm->body.len);
                            auto plan = ifr::Plans::PlanInfo::read(d);
                            ifr::Plans::savePlanInfo(plan);
                            mg_http_reply(c, 200, COMMON_JSON_HEADER, "true");
                        } catch (...) {
                            mg_http_reply(c, 400, COMMON_TEXT_HEADER, "Can not parse PlanInfo");
                        }
                    }
                    });
            http_route.push_back(
                    {"/plan/remove", "DELETE", [](auto c, int ev, auto ev_data, auto fn_data) {
                        auto hm = (mg_http_message *) ev_data;
                        auto pname = mgx_getquery(hm->query, "pname");
                        if (!pname.len) {
                            mg_http_reply(c, 400, COMMON_TEXT_HEADER, "no query: pname");
                            return;
                        }
                        bool success = ifr::Plans::removePlanInfo(STR_MG2STD(pname));
                        mg_http_reply(c, 200, COMMON_JSON_HEADER, success ? "true" : "false");
                    }
                    });
            http_route.push_back(
                    {"/plan/use", "GET", [](auto c, int ev, auto ev_data, auto fn_data) {
                        auto hm = (mg_http_message *) ev_data;
                        auto pname = mgx_getquery(hm->query, "pname");
                        if (!pname.len) {
                            mg_http_reply(c, 400, COMMON_TEXT_HEADER, "no query: pname");
                            return;
                        }
                        ifr::Plans::usePlanInfo(STR_MG2STD(pname));
                        mg_http_reply(c, 204, COMMON_JSON_HEADER, "");
                    }
                    });
            http_route.push_back(
                    {"/plan/start", "GET", [](auto c, int ev, auto ev_data, auto fn_data) {
                        bool success = ifr::Plans::startPlan();
                        mg_http_reply(c, 200, COMMON_JSON_HEADER, success ? "true" : "false");
                    }
                    });
            http_route.push_back(
                    {"/plan/stop", "GET", [](auto c, int ev, auto ev_data, auto fn_data) {
                        ifr::Plans::stopPlan();
                        mg_http_reply(c, 204, COMMON_JSON_HEADER, "");
                    }
                    });
            http_route.push_back(
                    {"/api.json", "GET", [](auto c, int ev, auto ev_data, auto fn_data) {
                        static mutex mtx;
                        static std::string json;
                        static size_t amount = -1;
                        unique_lock<mutex> lock(mtx);
                        if (json.empty() || amount != http_route.size()) {
                            amount = http_route.size();
                            rapidjson::StringBuffer buf;
                            rapidjson::Writer<StringBuffer> w(buf);
                            w.StartArray();
                            for (const auto &e: http_route)
                                w.StartObject(), w.Key("pattern"), w.String(e.pattern), w.Key("method"), w.String(
                                        e.method), w.EndObject();
                            w.EndArray();
                            w.Flush();
                            json = string(buf.GetString(), buf.GetLength());
                        }
                        mg_http_reply(c, 200, COMMON_JSON_HEADER, json.c_str());
                    }
                    });
        }

        void init(const std::string &url, bool async) {
            if (async) {
                thread t(init, url, false);
                while (!t.joinable());
                t.detach();
            } else {
                ifr::Plans::registerMsgOut(sendWs);
                registerRoute();
#if IFRAPI_HAS_VARIABLE
                Variable::init();
#endif
                ifr::logger::log("api", "Start API");
                struct mg_mgr mgr{};
                mg_mgr_init(&mgr);
                mg_http_listen(&mgr, url.c_str(), fn, nullptr);     // Create listening connection


                ifr::logger::log("api", "Start listen", url);
                for (;;) mg_mgr_poll(&mgr, 1000);                   // Block forever
            }
        }

        FORCE_INLINE size_t getNext(const mg_str &str, const char &c, const size_t &start = 0) {
            for (size_t i = start; i < str.len; i++)if (str.ptr[i] == c)return i;
            return npos;
        }

        mg_str mgx_getquery(const mg_str &query, const std::string &key) {
            for (size_t i = 0; i < query.len; i++) {
                if (query.len - i <= key.length())return mg_str("");
                size_t j;
                for (j = 0; j < key.length(); i++, j++) {
                    if (query.ptr[i] != key[j])break;

                }
                if (j != key.length() || query.ptr[i] != '=') {
                    i = getNext(query, '&', i);
                    if (i == npos)return mg_str("");
                    else continue;
                } else {
                    auto next = getNext(query, '&', ++i);
                    if (next == npos)next = query.len;
                    return mg_str_n(query.ptr + i, next - i);
                }
            }
            return mg_str("");
        }


        std::shared_ptr<TimeWatcher>
        registerTimePoint(const string &type, const TimeWatcher::unit_t &unit_ms, const size_t &point_amount,
                          const size_t &worker_amount) {
            auto s = std::make_shared<TimeWatcher>(type, unit_ms, point_amount, worker_amount);
            TimeWatcherHelper::registerTimeWatcher(s);
            return s;
        }

#if IFRAPI_HAS_VARIABLE

        std::map<std::string, Variable> Variable::vars;
        std::mutex Variable::mutex;
        bool Variable::locked;
        ifr::Config::ConfigController Variable::cc;


        bool Variable::init() {
            std::unique_lock<decltype(mutex)> lock(mutex);
            if (locked)return false;
            static ifr::Config::ConfigInfo<void> info = {
                    [](void *a, auto &w) {
                        std::unique_lock<decltype(mutex)> lock(mutex);
                        w.StartObject();
                        for (const auto &e: vars)w.Key(e.first), w.String(e.second.getValue());
                        w.EndObject();
                    },
                    [](void *a, const rapidjson::Document &d) {
                        std::unique_lock<decltype(mutex)> lock(mutex);
                        for (const auto &e: d.GetObj()) {
                            const std::string name = e.name.GetString();
                            if (vars.count(name))
                                vars.find(name)->second.setValue(e.value.GetString());
                        }
                    }
            };
            cc = ifr::Config::createConfig<void>("variable", nullptr, info);
            cc.load();
            return locked = true;
        }

        void Variable::save() { cc.save(); }


#endif
    }
} // ifr