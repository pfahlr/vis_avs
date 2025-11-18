#include <avs/compat/ape_loader.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

#ifndef _WIN32
#include <dlfcn.h>
#endif

namespace avs::ape {

namespace {

// Default APE search paths
std::vector<std::filesystem::path> gAPESearchPaths = {
    std::filesystem::path(std::getenv("HOME") ? std::getenv("HOME") : ".") /
        ".config/vis_avs/ape_plugins",
    "/usr/local/share/vis_avs/ape_plugins",
    "/usr/share/vis_avs/ape_plugins",
};

// Check if we have Wine available
bool checkWineAvailable() {
#ifdef _WIN32
  return true;  // Native Windows, no Wine needed
#else
  // Try to dlopen a test function to see if Wine interop works
  // For now, just check if we can load shared libraries
  return true;  // Assume available, will fail gracefully if not
#endif
}

}  // namespace

// ============================================================================
// WineAPELoader Implementation
// ============================================================================

WineAPELoader::~WineAPELoader() {
  if (dllHandle_) {
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(dllHandle_));
#else
    dlclose(dllHandle_);
#endif
  }
}

bool WineAPELoader::load(const std::filesystem::path& dllPath) {
  if (!std::filesystem::exists(dllPath)) {
    lastError_ = "APE DLL not found: " + dllPath.string();
    return false;
  }

#ifdef _WIN32
  // Native Windows: use LoadLibrary
  dllHandle_ = LoadLibraryW(dllPath.c_str());
  if (!dllHandle_) {
    lastError_ = "Failed to load DLL: " + dllPath.string();
    return false;
  }

  // Find the APE SetExtInfo function (must be called before factory)
  setExtInfoFunc_ = reinterpret_cast<SetExtInfoFunc>(
      GetProcAddress(static_cast<HMODULE>(dllHandle_), "_AVS_APE_SetExtInfo"));

  if (!setExtInfoFunc_) {
    // Try without underscore (some compilers)
    setExtInfoFunc_ = reinterpret_cast<SetExtInfoFunc>(
        GetProcAddress(static_cast<HMODULE>(dllHandle_), "AVS_APE_SetExtInfo"));
  }

  // Find the APE factory function
  createFunc_ = reinterpret_cast<CreateFunc>(
      GetProcAddress(static_cast<HMODULE>(dllHandle_), "_AVS_APE_RetrFunc"));

  if (!createFunc_) {
    // Try without underscore (some compilers)
    createFunc_ = reinterpret_cast<CreateFunc>(
        GetProcAddress(static_cast<HMODULE>(dllHandle_), "AVS_APE_RetrFunc"));
  }

  if (!createFunc_) {
    lastError_ = "APE factory function not found in DLL";
    FreeLibrary(static_cast<HMODULE>(dllHandle_));
    dllHandle_ = nullptr;
    return false;
  }

#else
  // Linux with Wine: use dlopen (requires winelib or similar)
  // First, try to load the DLL directly with RTLD_NOW
  dllHandle_ = dlopen(dllPath.c_str(), RTLD_NOW | RTLD_LOCAL);

  if (!dllHandle_) {
    const char* error = dlerror();
    lastError_ = std::string("Failed to load APE DLL: ") +
                 (error ? error : "unknown error") +
                 "\n\nNote: Wine APE loader requires winelib or compatible PE loader.\n" +
                 "This is a proof-of-concept implementation. For production use:\n" +
                 "1. Compile with winelib support\n" +
                 "2. Reimplement APE effect natively\n" +
                 "3. Use Windows or Wine prefix with vis_avs.exe";
    return false;
  }

  // Try to find the SetExtInfo function (must be called before factory)
  setExtInfoFunc_ = reinterpret_cast<SetExtInfoFunc>(dlsym(dllHandle_, "_AVS_APE_SetExtInfo"));

  if (!setExtInfoFunc_) {
    // Try without underscore
    setExtInfoFunc_ = reinterpret_cast<SetExtInfoFunc>(dlsym(dllHandle_, "AVS_APE_SetExtInfo"));
  }

  // Try to find the factory function
  createFunc_ = reinterpret_cast<CreateFunc>(dlsym(dllHandle_, "_AVS_APE_RetrFunc"));

  if (!createFunc_) {
    // Try without underscore
    createFunc_ = reinterpret_cast<CreateFunc>(dlsym(dllHandle_, "AVS_APE_RetrFunc"));
  }

  // Also try C++ mangled name (MSVC style)
  if (!createFunc_) {
    createFunc_ = reinterpret_cast<CreateFunc>(
        dlsym(dllHandle_, "?AVS_APE_RetrFunc@@YAPAVC_RBASE@@XZ"));
  }

  if (!createFunc_) {
    const char* error = dlerror();
    lastError_ = std::string("APE factory function not found: ") +
                 (error ? error : "unknown symbol");
    dlclose(dllHandle_);
    dllHandle_ = nullptr;
    return false;
  }
#endif

  identifier_ = dllPath.stem().string();
  return true;
}

bool WineAPELoader::setAPEinfo(APEinfo* info) {
  if (!isLoaded() || !info) {
    lastError_ = "Cannot set APEinfo: loader not initialized or info is null";
    return false;
  }

  if (setExtInfoFunc_) {
    try {
      setExtInfoFunc_(info);
      return true;
    } catch (const std::exception& e) {
      lastError_ = std::string("Exception setting APEinfo: ") + e.what();
      return false;
    } catch (...) {
      lastError_ = "Unknown exception setting APEinfo";
      return false;
    }
  }

  // SetExtInfo is optional in some older APE plugins
  return true;
}

std::unique_ptr<C_RBASE> WineAPELoader::createEffectInstance() {
  if (!isLoaded() || !createFunc_) {
    return nullptr;
  }

  try {
    C_RBASE* instance = createFunc_();
    return std::unique_ptr<C_RBASE>(instance);
  } catch (const std::exception& e) {
    lastError_ = std::string("Exception creating APE instance: ") + e.what();
    return nullptr;
  } catch (...) {
    lastError_ = "Unknown exception creating APE instance";
    return nullptr;
  }
}

// ============================================================================
// WineAPEEffect Implementation
// ============================================================================

WineAPEEffect::WineAPEEffect(std::unique_ptr<C_RBASE> apeInstance,
                             const std::vector<std::uint8_t>& config,
                             void* dllHandle)
    : apeInstance_(std::move(apeInstance)),
      dllHandle_(dllHandle),
      globalRegisters_(100, 0.0) {
  // Initialize visdata to zero
  std::memset(visdata_, 0, sizeof(visdata_));

  // Load configuration if provided
  if (!config.empty() && apeInstance_) {
    apeInstance_->load_config(
        const_cast<unsigned char*>(config.data()),
        static_cast<int>(config.size()));
  }

  setupAPEinfo();
}

WineAPEEffect::~WineAPEEffect() {
  // Destroy APE instance first (while DLL is still loaded)
  apeInstance_.reset();

  // Now free the DLL
  if (dllHandle_) {
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(dllHandle_));
#else
    dlclose(dllHandle_);
#endif
  }
}

void WineAPEEffect::setupAPEinfo() {
  // Setup APEinfo with global callbacks
  setupAPEinfoCallbacks(&apeInfo_);

  // Override global registers with our instance copy
  // This allows each effect instance to have its own local state
  apeInfo_.global_registers = globalRegisters_.data();
  apeInfo_.lineblendmode = &lineBlendMode_;

  // Note: EEL VM callbacks are now provided by ape_eel_bridge.cpp
}

void WineAPEEffect::init(int w, int h) {
  width_ = w;
  height_ = h;
}

void WineAPEEffect::updateVisdata(const Framebuffer& in) {
  (void)in;  // TODO: Extract audio data from framebuffer metadata
  // For now, just zero it out
  // In a full implementation, this would come from AudioState
  std::memset(visdata_, 0, sizeof(visdata_));
}

void WineAPEEffect::process(const Framebuffer& in, Framebuffer& out) {
  if (!apeInstance_) {
    // No APE instance, just copy input to output
    out = in;
    return;
  }

  // Ensure output buffer is sized correctly
  if (out.w != in.w || out.h != in.h) {
    out.w = in.w;
    out.h = in.h;
    out.rgba.resize(in.rgba.size());
  }

  // Update audio visualization data
  updateVisdata(in);

  // Prepare buffers for APE render call
  // APE expects 32-bit RGBA pixels as int*
  std::vector<int> framebuffer(in.rgba.size() / 4);
  std::vector<int> fbout(in.rgba.size() / 4);

  // Copy input to framebuffer (convert uint8_t* to int*)
  std::memcpy(framebuffer.data(), in.rgba.data(), in.rgba.size());

  // Call APE render function
  // isBeat = 0 for now (would come from beat detection)
  int result = apeInstance_->render(
      visdata_,
      0,  // isBeat
      framebuffer.data(),
      fbout.data(),
      in.w,
      in.h);

  // Copy result to output
  // result == 1 means fbout has the result, 0 means framebuffer has it
  const int* srcBuffer = (result == 1) ? fbout.data() : framebuffer.data();
  std::memcpy(out.rgba.data(), srcBuffer, out.rgba.size());
}

// ============================================================================
// Utility Functions
// ============================================================================

bool isWineAPESupported() {
  return checkWineAvailable();
}

void setAPESearchPaths(const std::vector<std::filesystem::path>& paths) {
  gAPESearchPaths = paths;
}

std::vector<std::filesystem::path> getAPESearchPaths() {
  return gAPESearchPaths;
}

std::filesystem::path findAPEDLL(const std::string& identifier,
                                  const std::filesystem::path& presetPath) {
  // Normalize identifier (remove path separators, etc.)
  std::string normalizedId = identifier;
  std::replace(normalizedId.begin(), normalizedId.end(), '/', '_');
  std::replace(normalizedId.begin(), normalizedId.end(), '\\', '_');
  std::replace(normalizedId.begin(), normalizedId.end(), ':', '_');

  // Try various filename patterns
  std::vector<std::string> patterns = {
      normalizedId + ".dll",
      normalizedId + ".so",
      normalizedId + ".ape",
      "ape_" + normalizedId + ".dll",
      "ape_" + normalizedId + ".so",
      "ape_" + normalizedId + ".ape",
  };

  // Build search paths list:
  // 1. Preset directory tree (if provided) - highest priority
  // 2. Global APE search paths
  std::vector<std::filesystem::path> searchPaths;

  if (!presetPath.empty() && std::filesystem::exists(presetPath)) {
    std::filesystem::path presetDir;
    if (std::filesystem::is_directory(presetPath)) {
      presetDir = presetPath;
    } else {
      presetDir = presetPath.parent_path();
    }

    // Search in preset directory and all parent directories up to root
    std::filesystem::path current = presetDir;
    while (!current.empty() && current != current.root_path()) {
      searchPaths.push_back(current);
      searchPaths.push_back(current / "APE");
      searchPaths.push_back(current / "ape");
      searchPaths.push_back(current / "plugins");
      searchPaths.push_back(current / "Plugins");
      current = current.parent_path();
    }
  }

  // Add global search paths
  for (const auto& path : gAPESearchPaths) {
    searchPaths.push_back(path);
  }

  // Search in all paths
  for (const auto& searchPath : searchPaths) {
    if (!std::filesystem::exists(searchPath)) {
      continue;
    }

    // Try direct match first
    for (const auto& pattern : patterns) {
      auto candidate = searchPath / pattern;
      if (std::filesystem::exists(candidate)) {
        std::cerr << "INFO: Found APE DLL: " << candidate << std::endl;
        return candidate;
      }
    }

    // Try case-insensitive search in directory
    if (std::filesystem::is_directory(searchPath)) {
      std::string lowerIdentifier = normalizedId;
      std::transform(lowerIdentifier.begin(), lowerIdentifier.end(),
                     lowerIdentifier.begin(), ::tolower);

      try {
        for (const auto& entry : std::filesystem::directory_iterator(searchPath)) {
          if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            std::string lowerFilename = filename;
            std::transform(lowerFilename.begin(), lowerFilename.end(),
                          lowerFilename.begin(), ::tolower);

            // Check if filename matches identifier (case-insensitive)
            for (const auto& pattern : patterns) {
              std::string lowerPattern = pattern;
              std::transform(lowerPattern.begin(), lowerPattern.end(),
                            lowerPattern.begin(), ::tolower);
              if (lowerFilename == lowerPattern) {
                std::cerr << "INFO: Found APE DLL (case-insensitive): " << entry.path() << std::endl;
                return entry.path();
              }
            }
          }
        }
      } catch (...) {
        // Ignore directory iteration errors
      }
    }
  }

  return std::filesystem::path();  // Not found
}

std::unique_ptr<Effect> createWineAPEEffect(const std::string& apeIdentifier,
                                            const LegacyEffectEntry& entry,
                                            ParsedPreset& result,
                                            const std::filesystem::path& presetPath) {
  // Try to find the APE DLL (search preset directory tree first)
  auto dllPath = findAPEDLL(apeIdentifier, presetPath);

  if (dllPath.empty()) {
    std::string warning = "APE DLL not found for: " + apeIdentifier;
    std::cerr << "WARNING: " << warning << std::endl;
    std::cerr << "Searched in preset directory tree";
    if (!presetPath.empty()) {
      std::cerr << " (from " << presetPath << ")";
    }
    std::cerr << " and global paths:" << std::endl;
    for (const auto& path : getAPESearchPaths()) {
      std::cerr << "  - " << path << std::endl;
    }
    result.warnings.push_back(warning);
    return nullptr;
  }

  // Load the DLL
  WineAPELoader loader;
  if (!loader.load(dllPath)) {
    std::string warning = "Failed to load APE DLL: " + loader.getLastError();
    std::cerr << "WARNING: " << warning << std::endl;
    result.warnings.push_back(warning);
    return nullptr;
  }

  // Setup APEinfo structure
  APEinfo apeInfo;
  setupAPEinfoCallbacks(&apeInfo);

  // Call SetExtInfo to pass APEinfo to the DLL (required before creating instances)
  if (!loader.setAPEinfo(&apeInfo)) {
    std::string warning = "Failed to set APEinfo: " + loader.getLastError();
    std::cerr << "WARNING: " << warning << std::endl;
    result.warnings.push_back(warning);
    // Continue anyway - some older APE plugins don't have SetExtInfo
  }

  // Create effect instance
  auto apeInstance = loader.createEffectInstance();
  if (!apeInstance) {
    std::string warning = "Failed to create APE instance: " + loader.getLastError();
    std::cerr << "WARNING: " << warning << std::endl;
    result.warnings.push_back(warning);
    return nullptr;
  }

  // Transfer DLL handle ownership to the effect (so DLL stays loaded)
  void* dllHandle = loader.releaseDllHandle();

  // Wrap in Effect interface
  return std::make_unique<WineAPEEffect>(std::move(apeInstance), entry.payload, dllHandle);
}

}  // namespace avs::ape
