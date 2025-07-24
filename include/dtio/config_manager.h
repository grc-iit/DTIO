/*
 * Copyright (C) 2024 Gnosis Research Center <grc@iit.edu>,
 * Keith Bateman <kbateman@hawk.iit.edu>, Neeraj Rajesh
 * <nrajesh@hawk.iit.edu> Hariharan Devarajan
 * <hdevarajan@hawk.iit.edu>, Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of DTIO
 *
 * DTIO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef DTIO_INCLUDE_DTIO_CONFIG_MANAGER_H_
#define DTIO_INCLUDE_DTIO_CONFIG_MANAGER_H_

#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "chimaera/api/chimaera_client.h"
#include "dtiomod/dtiomod_client.h"
#include "hermes_shm/util/config_parse.h"
#include "hermes_shm/util/singleton.h"

namespace dtio {

struct PathEntry {
  std::string path;
  bool do_include;

  PathEntry(const std::string& p, bool include)
      : path(p), do_include(include) {}
};

class ConfigurationManager : public hshm::BaseConfig {
 public:
  chi::dtiomod::Client dtio_mod_;
  std::vector<PathEntry> path_entries_;

  ConfigurationManager() {
    // Read DTIO configuration
    std::string dtio_conf_path = hshm::SystemInfo::Getenv(
        "DTIO_CONF_PATH", hshm::Unit<size_t>::Megabytes(1));
    if (!dtio_conf_path.empty()) {
      LoadFromFile(dtio_conf_path);
    } else {
      LoadDefault();
    }

    // Connect to DTIO
    CHIMAERA_CLIENT_INIT();
    dtio_mod_.Create(
        HSHM_MCTX,
        chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
        chi::DomainQuery::GetGlobalBcast(), "dtio_runtime");
  }

  bool ShouldIntercept(const std::string& absolute_path) const {
    // Check against path entries (already sorted by descending length)
    for (const auto& entry : path_entries_) {
      if (absolute_path.find(entry.path) == 0) {  // Path starts with entry.path
        return entry.do_include;
      }
    }
    return false;  // Default to not intercept
  }

  void LoadDefault() override {
    // Default: intercept everything under /tmp
    path_entries_.clear();
    path_entries_.emplace_back("/tmp", true);
    path_entries_.emplace_back("/", false);
  }

 private:
  void ParseYAML(YAML::Node& yaml_conf) override {
    std::vector<std::string> include_paths;
    std::vector<std::string> exclude_paths;

    if (yaml_conf["include"]) {
      BaseConfig::ParseVector<std::string>(yaml_conf["include"], include_paths);
    }

    if (yaml_conf["exclude"]) {
      BaseConfig::ParseVector<std::string>(yaml_conf["exclude"], exclude_paths);
    }

    // Convert to expanded, absolute paths and combine
    path_entries_.clear();

    for (const auto& path : include_paths) {
      std::string expanded = hshm::ConfigParse::ExpandPath(path);
      std::string abs_path = std::filesystem::absolute(expanded).string();
      path_entries_.emplace_back(abs_path, true);
    }

    for (const auto& path : exclude_paths) {
      std::string expanded = hshm::ConfigParse::ExpandPath(path);
      std::string abs_path = std::filesystem::absolute(expanded).string();
      path_entries_.emplace_back(abs_path, false);
    }

    // Sort by descending path length to ensure longest matches are checked
    // first
    std::sort(path_entries_.begin(), path_entries_.end(),
              [](const PathEntry& a, const PathEntry& b) {
                return a.path.length() > b.path.length();
              });
  }
};

}  // namespace dtio

// Global singleton macros
HSHM_DEFINE_GLOBAL_PTR_VAR_H(dtio::ConfigurationManager, kDtioConf);

// Convenience macro
#define DTIO_CONF HSHM_GET_GLOBAL_PTR_VAR(dtio::ConfigurationManager, kDtioConf)

#endif  // DTIO_INCLUDE_DTIO_CONFIG_MANAGER_H_