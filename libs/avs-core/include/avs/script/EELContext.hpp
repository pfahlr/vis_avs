#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "avs/core.hpp"
#if AVS_ENABLE_EEL2
#include "avs/eel.hpp"
#endif

namespace avs {

class EELContext {
 public:
  EELContext();
  ~EELContext();

  bool isEnabled() const;

  void setVariable(std::string_view name, double value);
  double getVariable(std::string_view name) const;

  bool compile(std::string_view name, std::string_view code);
  bool execute(std::string_view name);
  void remove(std::string_view name);

  void updateAudio(const AudioFeatures& audio, double engineTimeSeconds);

 private:
#if AVS_ENABLE_EEL2
  struct Program {
    std::string name;
    std::string code;
    NSEEL_CODEHANDLE handle{nullptr};
  };

  std::unique_ptr<EelVm> vm_;
  std::unordered_map<std::string, Program> programs_;
  std::unordered_map<std::string, EEL_F*> variables_;
  std::vector<std::uint8_t> oscBuffer_;
  std::vector<std::uint8_t> specBuffer_;
#else
  std::unordered_map<std::string, double> variables_;
  std::unordered_map<std::string, std::string> programs_;
#endif
};

}  // namespace avs

