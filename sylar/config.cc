#include "config.h"
#include <list>
#include <sstream>
#include <utility>
namespace mysylar
{

// 将yaml那种格式变为 "A.B", 10

static void ListAllMember(const std::string& prefix, const YAML::Node& node,
                          std::list<std::pair<std::string, const YAML::Node>>& output)
{
    if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos)
    {
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name" << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix, node));
    if (node.IsMap())

    {
        for (auto it = node.begin(); it != node.end(); it++)
        {
            // 递归处理
            ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(),
                          it->second, output);
        }
    }
}

void Config::LoadFromYaml(const YAML::Node& node)
{
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    ListAllMember("", node, all_nodes);

    // 将获取的YAML配置参数注册到Config中
    for (auto& i : all_nodes)
    {
        std::string key = i.first;
        if (key.empty())
            continue;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        auto it = GetDatas().find(key);
        ConfigVarBase::ptr var = it == GetDatas().end() ? nullptr : it->second;

        if (var)
        {
            if (i.second.IsScalar())
            {
                var->fromString(i.second.Scalar());
            }
            else
            {
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

}; // namespace mysylar