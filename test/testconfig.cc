#include "../sylar/config.h"
#include "../sylar/log.h"
#include <iostream>
#include <set>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>
mysylar::ConfigVar<int>::ptr g_int_value_config =
    mysylar::Config::Lookup("system.port", (int) 800, "system port");
mysylar::ConfigVar<float>::ptr g_float_value_config =
    mysylar::Config::Lookup("system.value", (float) 10.2, "system ");
mysylar::ConfigVar<std::vector<int>>::ptr g_vector_value_config =
    mysylar::Config::Lookup("system.vector", std::vector<int>{1, 2, 3}, "system.vector");
mysylar::ConfigVar<std::vector<int>>::ptr g_list_value_config =
    mysylar::Config::Lookup("system.list", std::vector<int>{101, 202, 303}, "system.list");
mysylar::ConfigVar<std::set<int>>::ptr g_set_value_config =
    mysylar::Config::Lookup("system.set", std::set<int>{1, 2, 3}, "system.set");
mysylar::ConfigVar<std::unordered_set<int>>::ptr g_unordered_set_value_config =
    mysylar::Config::Lookup("system.unordered_set", std::unordered_set<int>{1, 2, 3},
                            "system.unordered_set");
mysylar::ConfigVar<std::map<std::string, int>>::ptr g_map_value_config = mysylar::Config::Lookup(
    "system.map", std::map<std::string, int>{{"key1", 1}, {"key2", 2}}, "system.map");
mysylar::ConfigVar<std::unordered_map<std::string, int>>::ptr g_umap_value_config =
    mysylar::Config::Lookup("system.unordered_map",
                            std::unordered_map<std::string, int>{{"key1", 3}, {"key2", 4}},
                            "system.unordered_map");
void printYaml(const YAML::Node& node, int level)
{
    if (node.IsScalar())
    {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << node.Scalar() << " - " << node.Type() << " - " << level;
    }
    else if (node.IsNull())
    {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "NULL - " << node.Type() << " - " << level;
    }
    else if (node.IsMap())
    {
        for (auto it = node.begin(); it != node.end(); ++it)
        {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
                << it->first << " - " << it->second.Type() << " - " << level;
            printYaml(it->second, level + 1);
        }
    }
    else if (node.IsSequence())
    {
        for (size_t i = 0; i < node.size(); ++i)
        {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << i << " - " << node[i].Type() << " - " << level;
            printYaml(node[i], level + 1);
        }
    }
}

void testYaml()
{
    YAML::Node node = YAML::LoadFile("/home/lunisolar/cpp/mysylar/config/log.yaml");
    printYaml(node, 0);
}

void testConfig()
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before :" << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before :" << g_float_value_config->getValue();
#define XX(g_val, name, prefix)                                                                    \
    {                                                                                              \
        auto& v = g_val->getValue();                                                               \
        for (auto& i : v)                                                                          \
        {                                                                                          \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " :" << i;                       \
        }                                                                                          \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " ymal: " << g_val->toString();      \
    }
#define XX_M(g_val, name, prefix)                                                                  \
    {                                                                                              \
        auto& v = g_val->getValue();                                                               \
        for (auto& i : v)                                                                          \
        {                                                                                          \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())                                                       \
                << #prefix " " #name " :" << i.first << " - " << i.second;                         \
        }                                                                                          \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " ymal: " << g_val->toString();      \
    }
    XX(g_vector_value_config, vector, before);
    XX(g_list_value_config, list, before);
    XX(g_set_value_config, set, before);
    XX(g_unordered_set_value_config, unordered_set, before);
    XX_M(g_map_value_config, map, before);
    XX_M(g_umap_value_config, unordered_map, before);
    YAML::Node node = YAML::LoadFile("/home/lunisolar/cpp/mysylar/config/log.yaml");
    mysylar::Config::LoadFromYaml(node);
    XX(g_vector_value_config, vector, after);
    XX(g_list_value_config, list, after);
    XX(g_set_value_config, set, after);
    XX(g_unordered_set_value_config, unordered_set, after);
    XX_M(g_map_value_config, map, after);
    XX_M(g_umap_value_config, unordered_map, after);
}

int main(int argc, char** argv)
{
    // testYaml();
    testConfig();
    return 0;
}
