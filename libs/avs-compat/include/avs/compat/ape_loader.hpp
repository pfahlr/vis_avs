#pragma once

#include <avs/effects.hpp>
#include <avs/compat/preset.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace avs::ape {

// Forward declarations of Windows types used by APE interface
#ifdef _WIN32
#include <windows.h>
#else
// Mock Windows types for Linux
typedef void* HWND;
typedef void* HINSTANCE;
#endif

// APE C_RBASE interface (matches original APE SDK)
class C_RBASE {
 public:
  C_RBASE() = default;
  virtual ~C_RBASE() = default;

  // Core rendering function - returns 1 if fbout has dest, 0 if framebuffer has dest
  virtual int render(char visdata[2][2][576],
                     int isBeat,
                     int* framebuffer,
                     int* fbout,
                     int w,
                     int h) = 0;

  // Configuration dialog (not supported on Linux)
  virtual HWND conf(HINSTANCE hInstance, HWND hwndParent) {
    (void)hInstance;
    (void)hwndParent;
    return nullptr;
  }

  // Effect display name
  virtual char* get_desc() = 0;

  // Binary preset serialization
  virtual void load_config(unsigned char* data, int len) {
    (void)data;
    (void)len;
  }
  virtual int save_config(unsigned char* data) {
    (void)data;
    return 0;
  }
};

// APE extended interface with SMP support
class C_RBASE2 : public C_RBASE {
 public:
  virtual int smp_getflags() { return 0; }  // return 1 to enable SMP
  virtual int smp_begin(int max_threads, char visdata[2][2][576], int isBeat,
                       int* framebuffer, int* fbout, int w, int h) {
    (void)max_threads; (void)visdata; (void)isBeat;
    (void)framebuffer; (void)fbout; (void)w; (void)h;
    return 0;
  }
  virtual void smp_render(int this_thread, int max_threads, char visdata[2][2][576],
                         int isBeat, int* framebuffer, int* fbout, int w, int h) {
    (void)this_thread; (void)max_threads; (void)visdata; (void)isBeat;
    (void)framebuffer; (void)fbout; (void)w; (void)h;
  }
  virtual int smp_finish(char visdata[2][2][576], int isBeat,
                        int* framebuffer, int* fbout, int w, int h) {
    (void)visdata; (void)isBeat; (void)framebuffer; (void)fbout; (void)w; (void)h;
    return 0;
  }
};

// EEL VM types
typedef void* VM_CONTEXT;
typedef void* VM_CODEHANDLE;

// APEinfo structure - provides callbacks to AVS internals
struct APEinfo {
  int ver;  // Start with ver=1
  double* global_registers;  // 100 global registers
  int* lineblendmode;  // Line blend mode configuration

  // EEL VM interface
  VM_CONTEXT (*allocVM)();
  void (*freeVM)(VM_CONTEXT);
  void (*resetVM)(VM_CONTEXT);
  double* (*regVMvariable)(VM_CONTEXT, char* name);
  VM_CODEHANDLE (*compileVMcode)(VM_CONTEXT, char* code);
  void (*executeCode)(VM_CODEHANDLE, char visdata[2][2][576]);
  void (*freeCode)(VM_CODEHANDLE);

  // Extended functions (ver >= 2)
  void (*doscripthelp)(HWND hwndDlg, char* mytext);

  // Buffer management (ver >= 3)
  void* (*getNbuffer)(int w, int h, int n, int do_alloc);
};

// Wine-based APE DLL loader
class WineAPELoader {
 public:
  WineAPELoader() = default;
  ~WineAPELoader();

  // Disable copy/move
  WineAPELoader(const WineAPELoader&) = delete;
  WineAPELoader& operator=(const WineAPELoader&) = delete;

  // Load an APE DLL from file system
  bool load(const std::filesystem::path& dllPath);

  // Set APEinfo structure (must be called before createEffectInstance)
  bool setAPEinfo(APEinfo* info);

  // Check if loader is initialized
  bool isLoaded() const { return dllHandle_ != nullptr; }

  // Get APE identifier string from DLL
  std::string getIdentifier() const { return identifier_; }

  // Create an effect instance from the loaded DLL
  std::unique_ptr<C_RBASE> createEffectInstance();

  // Get last error message
  std::string getLastError() const { return lastError_; }

  // Get DLL handle (for keeping DLL alive)
  void* getDllHandle() const { return dllHandle_; }

  // Transfer ownership of DLL handle (caller must free it)
  void* releaseDllHandle() {
    void* handle = dllHandle_;
    dllHandle_ = nullptr;
    return handle;
  }

 private:
  void* dllHandle_ = nullptr;
  typedef C_RBASE* (*CreateFunc)();
  typedef void (*SetExtInfoFunc)(APEinfo*);
  CreateFunc createFunc_ = nullptr;
  SetExtInfoFunc setExtInfoFunc_ = nullptr;
  std::string identifier_;
  std::string lastError_;
};

// Effect wrapper that bridges Wine APE to avs::Effect interface
class WineAPEEffect : public Effect {
 public:
  explicit WineAPEEffect(std::unique_ptr<C_RBASE> apeInstance,
                         const std::vector<std::uint8_t>& config,
                         void* dllHandle);
  ~WineAPEEffect() override;

  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;

 private:
  std::unique_ptr<C_RBASE> apeInstance_;
  void* dllHandle_;  // Keep DLL loaded for the lifetime of this effect
  APEinfo apeInfo_;
  std::vector<double> globalRegisters_;
  int lineBlendMode_ = 0;
  int width_ = 0;
  int height_ = 0;

  // Legacy audio data buffer (576 samples per channel)
  char visdata_[2][2][576];

  void setupAPEinfo();
  void updateVisdata(const Framebuffer& in);
};

// Factory function to create APE effect from DLL
std::unique_ptr<Effect> createWineAPEEffect(const std::string& apeIdentifier,
                                            const LegacyEffectEntry& entry,
                                            ParsedPreset& result,
                                            const std::filesystem::path& presetPath = std::filesystem::path());

// Check if Wine APE loader is available on this system
bool isWineAPESupported();

// Set APE DLL search paths (defaults to ~/.config/vis_avs/ape_plugins)
void setAPESearchPaths(const std::vector<std::filesystem::path>& paths);

// Get current APE search paths
std::vector<std::filesystem::path> getAPESearchPaths();

// Find an APE DLL by identifier string
// If presetPath is provided, searches in preset directory tree first
std::filesystem::path findAPEDLL(const std::string& identifier,
                                  const std::filesystem::path& presetPath = std::filesystem::path());

// Setup APEinfo with EEL VM callbacks
void setupAPEinfoCallbacks(APEinfo* info);

// Get global registers array
double* getGlobalRegisters();

}  // namespace avs::ape
