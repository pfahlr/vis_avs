#include <avs/compat/ape_loader.hpp>
#include <avs/compat/eel.hpp>

#include <cstring>
#include <unordered_map>
#include <mutex>

namespace avs::ape {

namespace {

// Global EEL VM registry for APE callbacks
// Maps VM_CONTEXT to EelVm instances
std::unordered_map<VM_CONTEXT, EelVm*> gVmRegistry;
std::mutex gVmRegistryMutex;

// Global megabuf for shared state across all effects
static constexpr int kGlobalRegisterCount = 100;
static double gGlobalRegisters[kGlobalRegisterCount] = {0.0};

// Register a VM instance
void registerVm(VM_CONTEXT ctx, EelVm* vm) {
  std::lock_guard<std::mutex> lock(gVmRegistryMutex);
  gVmRegistry[ctx] = vm;
}

// Unregister a VM instance
void unregisterVm(VM_CONTEXT ctx) {
  std::lock_guard<std::mutex> lock(gVmRegistryMutex);
  gVmRegistry.erase(ctx);
}

// Get EelVm from context
EelVm* getVm(VM_CONTEXT ctx) {
  std::lock_guard<std::mutex> lock(gVmRegistryMutex);
  auto it = gVmRegistry.find(ctx);
  return (it != gVmRegistry.end()) ? it->second : nullptr;
}

}  // namespace

// ============================================================================
// APEinfo EEL VM C Callback Implementations
// ============================================================================

extern "C" {

// Allocate a new EEL VM context
static VM_CONTEXT ape_allocVM() {
  EelVm* vm = new EelVm();
  VM_CONTEXT ctx = reinterpret_cast<VM_CONTEXT>(vm);
  registerVm(ctx, vm);
  return ctx;
}

// Free an EEL VM context
static void ape_freeVM(VM_CONTEXT ctx) {
  EelVm* vm = getVm(ctx);
  if (vm) {
    unregisterVm(ctx);
    delete vm;
  }
}

// Reset an EEL VM (clear all variables)
static void ape_resetVM(VM_CONTEXT ctx) {
  EelVm* vm = getVm(ctx);
  if (vm) {
    // Create a new VM to reset state
    unregisterVm(ctx);
    delete vm;
    vm = new EelVm();
    registerVm(ctx, vm);
  }
}

// Register a variable in the VM
static double* ape_regVMvariable(VM_CONTEXT ctx, char* name) {
  EelVm* vm = getVm(ctx);
  if (!vm || !name) {
    static double dummy = 0.0;
    return &dummy;
  }
  return vm->regVar(name);
}

// Compile EEL code
static VM_CODEHANDLE ape_compileVMcode(VM_CONTEXT ctx, char* code) {
  EelVm* vm = getVm(ctx);
  if (!vm || !code) {
    return nullptr;
  }
  try {
    NSEEL_CODEHANDLE handle = vm->compile(std::string(code));
    return reinterpret_cast<VM_CODEHANDLE>(handle);
  } catch (...) {
    return nullptr;
  }
}

// Execute compiled EEL code
static void ape_executeCode(VM_CODEHANDLE code, char visdata[2][2][576]) {
  (void)visdata;  // TODO: Pass visdata to VM context
  if (!code) {
    return;
  }

  // Extract VM context from code handle
  // Note: NSEEL_CODEHANDLE is opaque, so we need to track which VM owns it
  // For now, we'll execute it with the first available VM
  // In a full implementation, we'd need better tracking
  std::lock_guard<std::mutex> lock(gVmRegistryMutex);
  if (!gVmRegistry.empty()) {
    EelVm* vm = gVmRegistry.begin()->second;
    if (vm) {
      NSEEL_CODEHANDLE handle = reinterpret_cast<NSEEL_CODEHANDLE>(code);

      // Update visdata in VM (if needed)
      // This would require exposing setLegacySources or similar
      // For now, just execute the code
      vm->execute(handle);
    }
  }
}

// Free compiled code
static void ape_freeCode(VM_CODEHANDLE code) {
  if (!code) {
    return;
  }

  // Find the VM that owns this code and free it
  std::lock_guard<std::mutex> lock(gVmRegistryMutex);
  for (auto& pair : gVmRegistry) {
    EelVm* vm = pair.second;
    if (vm) {
      try {
        NSEEL_CODEHANDLE handle = reinterpret_cast<NSEEL_CODEHANDLE>(code);
        vm->freeCode(handle);
        return;
      } catch (...) {
        // Continue to next VM
      }
    }
  }
}

// Script help dialog (not supported on Linux)
static void ape_doscripthelp(HWND hwndDlg, char* mytext) {
  // Not implemented - would show a dialog on Windows
  (void)hwndDlg;
  (void)mytext;
}

// Get shared buffer (simplified implementation)
static void* ape_getNbuffer(int w, int h, int n, int do_alloc) {
  // Simplified: return nullptr for now
  // Full implementation would manage 8 shared buffers
  (void)w;
  (void)h;
  (void)n;
  (void)do_alloc;
  return nullptr;
}

}  // extern "C"

// ============================================================================
// APEinfo Setup Functions
// ============================================================================

void setupAPEinfoCallbacks(APEinfo* info) {
  if (!info) {
    return;
  }

  info->ver = 3;
  info->global_registers = gGlobalRegisters;

  // EEL VM callbacks
  info->allocVM = ape_allocVM;
  info->freeVM = ape_freeVM;
  info->resetVM = ape_resetVM;
  info->regVMvariable = ape_regVMvariable;
  info->compileVMcode = ape_compileVMcode;
  info->executeCode = ape_executeCode;
  info->freeCode = ape_freeCode;

  // Extended callbacks
  info->doscripthelp = ape_doscripthelp;
  info->getNbuffer = ape_getNbuffer;
}

double* getGlobalRegisters() {
  return gGlobalRegisters;
}

}  // namespace avs::ape
