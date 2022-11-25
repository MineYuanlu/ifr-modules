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
#include "logger/logger.hpp"
#include "tools/tools.hpp"

using namespace rapidjson;

#define IFR_CONFIG_USE_FS RAPIDJSON_HAS_CXX17

#if IFR_CONFIG_USE_FS

#include <filesystem>

#define IFR_CONFIG_PATH2STR(path) ((path).string())
#else
#define IFR_CONFIG_PATH2STR(path) (path)
#endif

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

#if !IFR_CONFIG_USE_FS
        /**
         * 创建文件夹 (递归创建, 调用系统命令)
         * @param path 文件夹路径
         */
        void mkDir(std::string path);

        /**
        * 获取路径的文件夹
        * @param fname 文件路径
        * @return 所在文件夹
        */
        std::string getDir(std::string fname);
#endif

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
#if IFR_CONFIG_USE_FS
            std::filesystem::path path(dir + name + ".json");
            path = absolute(path);
            {
                const auto parent = path.parent_path();
                if (!parent.empty() && !exists(parent))std::filesystem::create_directories(parent);
            }
#else
            const auto path = dir + name + ".json";
            mkDir(getDir(path));
#endif
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
                                                     IFR_CONFIG_PATH2STR(path) + ", err: " + err.what());
                                } catch (...) {
                                    ifr::logger::err("Config", "Can not serialize file", path);
                                }

                                w.Flush();
                                osw.Flush();
                                fout.flush();
                                fout.close();

                            } else {
                                ifr::logger::err("Config", "Can not open file (w)", path);
                                return;
                            }
                        } catch (std::exception &err) {
                            ifr::logger::err("Config", "Can not write file",
                                             IFR_CONFIG_PATH2STR(path) + ", err: " + err.what());
                        } catch (...) {
                            ifr::logger::err("Config", "Can not write file", path);
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
                                                     IFR_CONFIG_PATH2STR(path) + ", err: " + err.what());
                                } catch (...) {
                                    ifr::logger::err("Config", "Can not deserialize file", path);
                                }
                                fin.close();
                            } else {
                                ifr::logger::err("Config", "Can not open file (r)", path);
                                return;
                            }
                        } catch (std::exception &err) {
                            ifr::logger::err("Config", "Can not read file",
                                             IFR_CONFIG_PATH2STR(path) + ", err: " + err.what());
                        } catch (...) {
                            ifr::logger::err("Config", "Can not read file", path);
                        }
                    }
            };
            return cc;
        }

    }
} // ifr

#endif //IFR_OPENCV_CONFIG_H
