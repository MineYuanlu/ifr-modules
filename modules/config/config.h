//
// Created by yuanlu on 2022/10/15.
//

#ifndef IFR_OPENCV_CONFIG_H
#define IFR_OPENCV_CONFIG_H

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include <algorithm>
#include <string>
#include <fstream>
#include <functional>
#include <filesystem>
#include "logger/logger.hpp"
#include "tools/tools.hpp"

using namespace rapidjson;
namespace ifr {
    namespace Config {
        static const std::string dir = "runtime/config/";

        /**
         * 配置文件控制器
         */
        struct ConfigController {
            std::function<void()> save;//保存
            std::function<void()> load;//加载
        };
        /**
         * 配置文件信息
         * @tparam T 配置文件数据类型
         */
        template<typename T>
        struct ConfigInfo {
            std::function<void(T *const, rapidjson::Writer<OStreamWrapper> &)> serialize;//序列化
            std::function<void(T *const, const rapidjson::Document &)> deserialize;//反序列化
        };

        /**
         * 注册一个配置文件
         * @tparam T 配置文件数据类型
         * @param name 配置文件名称
         * @param data 数据指针
         * @param info 配置文件信息
         * @return 配置文件控制器
         */
        template<typename T>
        ConfigController createConfig(const std::string &name, T *const data, const ConfigInfo<T> &info) {
            std::filesystem::path path(dir + name + ".json");
            path = absolute(path);
            {
                const auto parent = path.parent_path();
                if (!parent.empty() && !exists(parent))std::filesystem::create_directories(parent);
            }
            ConfigController cc = {
                    [&info, data, path]() {
                        try {
                            std::ofstream fout(path);
                            if (fout.is_open()) {
                                rapidjson::OStreamWrapper osw(fout);
                                rapidjson::Writer<OStreamWrapper> w(osw);

                                try {
                                    info.serialize(data, w);
                                } catch (std::exception &err) {
                                    ifr::logger::err("Config", "Can not serialize file",
                                                     path.string() + ", err: " + err.what());
                                } catch (...) {
                                    ifr::logger::err("Config", "Can not serialize file", path.string());
                                }

                                w.Flush();
                                osw.Flush();
                                fout.flush();
                                fout.close();

                            } else {
                                ifr::logger::err("Config", "Can not open file (w)", path.string());
                                return;
                            }
                        } catch (std::exception &err) {
                            ifr::logger::err("Config", "Can not write file", path.string() + ", err: " + err.what());
                        } catch (...) {
                            ifr::logger::err("Config", "Can not write file", path.string());
                        }
                    },
                    [&info, data, path]() {
                        try {
                            std::ifstream fin(path);
                            if (fin.is_open()) {
                                Document d;
                                rapidjson::IStreamWrapper isw(fin);
                                d.ParseStream(isw);

                                try {
                                    info.deserialize(data, d);
                                } catch (std::exception &err) {
                                    ifr::logger::err("Config", "Can not deserialize file",
                                                     path.string() + ", err: " + err.what());
                                } catch (...) {
                                    ifr::logger::err("Config", "Can not deserialize file", path.string());
                                }
                                fin.close();
                            } else {
                                ifr::logger::err("Config", "Can not open file (r)", path.string());
                                return;
                            }
                        } catch (std::exception &err) {
                            ifr::logger::err("Config", "Can not read file", path.string() + ", err: " + err.what());
                        } catch (...) {
                            ifr::logger::err("Config", "Can not read file", path.string());
                        }
                    }
            };
            return cc;
        }

    }
} // ifr

#endif //IFR_OPENCV_CONFIG_H
