#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include "log.h"
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cstddef>
#include <exception>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <yaml-cpp/node/type.h>
#include <yaml-cpp/yaml.h>
namespace mysylar
{

class ConfigVarBase
{
  public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string description = "")
        : m_name(name), m_description(description)
    {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    };
    virtual ~ConfigVarBase(){};

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& value) = 0;
    virtual std::string getTypeName() = 0;

    const std::string& getName() const
    {
        return m_name;
    }
    const std::string& getDescription() const
    {
        return m_description;
    }

  private:
    std::string m_name;
    std::string m_description;
};

// F from_type T to_Type

/**
 * @brief 通用类型转换模板
 *
 * @tparam F from_type 源类型
 * @tparam T to_Type 目标类型
 */
template <class F, class T>
class LexicalCast
{
  public:
    /**
     * @brief 运算符重载 通过lexical_cast将源类型转为目标类型
     *
     * @param v 源类型数据
     * @return T 目标类型数据
     */
    T operator()(const F& v)
    {
        return boost::lexical_cast<T>(v);
    }
};
/**
 * @brief 偏特化 vector容器支持 将string类型转为vector
 *
 * @tparam T
 */
template <class T>
class LexicalCast<std::string, std::vector<T>>
{
  public:
    std::vector<T> operator()(const std::string& val)
    {
        YAML::Node node = YAML::Load(val);
        std::vector<T> vec = {};
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i)
        {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }

        return vec;
    }
};
/**
 * @brief 偏特化 vector容器支持 将vector类型转为string
 *
 * @tparam T
 */
template <class T>
class LexicalCast<std::vector<T>, std::string>
{
  public:
    std::string operator()(const std::vector<T>& vec)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : vec)
        {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 偏特化 list容器支持 将string类型转为list
 *
 */
template <class T>
class LexicalCast<std::string, std::list<T>>
{
  public:
    std::list<T> operator()(const std::string& val)
    {
        YAML::Node node = YAML::Load(val);
        std::list<T> list = {};
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i)
        {
            ss.str("");
            ss << node[i];
            list.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return list;
    }
};
/**
 * @brief 偏特化 list容器支持 将list类型转为string
 *
 */

template <class T>
class LexicalCast<std::list<T>, std::string>
{
  public:
    std::string operator()(const std::list<T>& list)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : list)
        {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
/**
 * @brief 偏特化 set容器支持 将string类型转为set
 *
 */

template <class T>
class LexicalCast<std::string, std::set<T>>
{
  public:
    std::set<T> operator()(const std::string& val)
    {
        YAML::Node node = YAML::Load(val);
        std::set<T> set = {};
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i)
        {
            ss.str("");
            ss << node[i];
            set.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return set;
    }
};

/**
 * @brief 偏特化 set容器支持 将set类型转为string
 *
 */
template <class T>
class LexicalCast<std::set<T>, std::string>
{
  public:
    std::string operator()(const std::set<T>& set)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : set)
        {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
/**
 * @brief 偏特化 unordered_set容器支持 将string类型转为unordered_set
 *
 */
template <class T>
class LexicalCast<std::string, std::unordered_set<T>>
{
  public:
    std::unordered_set<T> operator()(const std::string& val)
    {
        YAML::Node node = YAML::Load(val);
        std::unordered_set<T> uset = {};
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i)
        {
            ss.str("");
            ss << node[i];
            uset.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return uset;
    }
};
/**
 * @brief 偏特化 unordered_set容器支持 将unordered_set类型转为string
 *
 */
template <class T>
class LexicalCast<std::unordered_set<T>, std::string>
{
  public:
    std::string operator()(const std::unordered_set<T>& uset)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : uset)
        {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
/**
 * @brief 偏特化 map容器支持 将string类型转为map
 *
 */
template <class T>
class LexicalCast<std::string, std::map<std::string, T>>
{
  public:
    std::map<std::string, T> operator()(const std::string& val)
    {
        YAML::Node node = YAML::Load(val);
        std::map<std::string, T> map = {};
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it)
        {
            ss.str("");
            ss << it->second;
            map.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return map;
    }
};
/**
 * @brief 偏特化 map容器支持 将map类型转为string
 *
 */
template <class T>
class LexicalCast<std::map<std::string, T>, std::string>
{
  public:
    std::string operator()(const std::map<std::string, T>& map)
    {
        YAML::Node node(YAML::NodeType::Map);
        for (auto& i : map)
        {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
/**
 * @brief 偏特化 unordered_map容器支持 将string类型转为unordered_map
 *
 */
template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>>
{
  public:
    std::unordered_map<std::string, T> operator()(const std::string& val)
    {
        YAML::Node node = YAML::Load(val);
        std::unordered_map<std::string, T> umap = {};
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); it++)
        {
            ss.str("");
            ss << it->second;
            umap.insert(
                std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return umap;
    }
};
/**
 * @brief 偏特化 unordered_map容器支持 将unordered_map类型转为string
 *
 */
template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string>
{
  public:
    std::string operator()(const std::unordered_map<std::string, T>& umap)
    {
        YAML::Node node(YAML::NodeType::Map);
        for (auto& i : umap)
        {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
// FromStr T operator()(const std:string)
// ToStr std::string operator(const T)

template <class T, class FromStr = LexicalCast<std::string, T>,
          class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase
{
  public:
    typedef std::shared_ptr<ConfigVar> ptr;

    ConfigVar(const std::string& name, const T& default_value, const std::string& description = "")
        : ConfigVarBase(name, description), m_val(default_value)
    {
    }
    std::string toString() override
    {
        try
        {
            // return boost::lexical_cast<std::string>(m_val);
            return ToStr()(m_val);
        }
        catch (std::exception& e)
        {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception " << e.what()
                                              << " convert " << getTypeName() << " toString";
        }
        return "";
    }
    bool fromString(const std::string& value) override
    {
        try
        {
            // m_val = boost::lexical_cast<T>(value);
            setValue(FromStr()(value));
        }
        catch (std::exception& e)
        {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception " << e.what()
                                              << " convert string  to" << getTypeName();
        }
        return false;
    }
    std::string getTypeName() override
    {
        return typeid(T).name();
    }
    const T getValue() const
    {
        return m_val;
    }
    void setValue(const T& value)
    {
        m_val = value;
    }

  private:
    T m_val;
};

class Config
{
  public:
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigValMap;

    /**
     * @brief 创建/获取对应参数名的配置参数
     *
     * @tparam T
     * @param name
     * @param description
     * @param default_value
     * @return ConfigVar<T>::ptr
     */
    template <class T>
    static ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value,
                                    const std::string& description)
    {
        auto it = GetDatas().find(name);
        // 存在这个参数名
        if (it != GetDatas().end())
        {
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if (tmp)
            {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists";
                return tmp;
            }
            else
            {
                // 这时虽然有参数名，但是Config为空
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                    << "Lookup name = " << name << "exists,but type not T real_type is"
                    << it->second->getTypeName();
            }
        }
        // 不存在这个数据，先验证name是否有效
        if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789") != std::string::npos)
        {
            // 无效
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid" << name;
            throw std::invalid_argument(name);
        }
        // typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        auto v = std::make_shared<ConfigVar<T>>(name, default_value, description);
        GetDatas()[name] = v;
        return v;
    }

    /**
     * @brief 查找配置参数
     *
     * @tparam T
     * @param name
     * @return ConfigVar<T>::ptr
     */
    template <class T>
    static ConfigVar<T>::ptr Lookup(const std::string& name)
    {
        auto it = GetDatas().find(name);
        if (it == GetDatas().end())
            return nullptr;
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    /**
     * @brief 使用YAML::NODE 初始化模块
     */
    static void LoadFromYaml(const YAML::Node& node);

  private:
    static ConfigValMap& GetDatas()
    {
        static ConfigValMap s_datas;
        return s_datas;
    }
};

}; // namespace mysylar

#endif