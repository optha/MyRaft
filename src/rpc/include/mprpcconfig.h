#pragma once

#include <string>
#include <unordered_map>

class MpRpcConfig {
 public:
  // 解析加载配置文件
  void LoadConfigFile(const char *config_file);
  // 查询配置项信息
  std::string Load(const std::string &key);

 private:
  std::unordered_map<std::string, std::string> m_configMap;
  // 去除空格
  void Trim(std::string &src_buf);
};