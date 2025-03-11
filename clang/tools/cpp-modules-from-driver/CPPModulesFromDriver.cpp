#include "clang/Tooling/DependencyScanning/DependencyScanningTool.h"
#include "clang/Tooling/DependencyScanning/DependencyScanningWorker.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/LLVMDriver.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace clang;
using namespace clang::tooling;
using namespace clang::tooling::dependencies;
using namespace llvm;

struct EnsureFinishedConsumer : public DiagnosticConsumer {
  bool Finished = false;
  void finish() override { Finished = true; }
};

int cpp_modules_from_driver_main(int argc, char **, const llvm::ToolContext &) {
  StringRef CWD = "/root";

  auto VFS = new llvm::vfs::InMemoryFileSystem();
  VFS->setCurrentWorkingDirectory(CWD);
  auto Sept = llvm::sys::path::get_separator();

  std::vector<std::string> Files = {
      std::string(llvm::formatv("{0}root{0}header.h", Sept)),
      std::string(llvm::formatv("{0}root{0}test.cpp", Sept)),
      std::string(llvm::formatv("{0}root{0}test.s", Sept)),
      std::string(llvm::formatv("{0}root{0}module.cppm", Sept)),
      std::string(llvm::formatv("{0}root{0}main.cpp", Sept))
  };

  VFS->addFile(Files[0], 0, llvm::MemoryBuffer::getMemBuffer("\n"));
  VFS->addFile(Files[1], 0, llvm::MemoryBuffer::getMemBuffer("#include \"header.h\"\n"));
  VFS->addFile(Files[2], 0, llvm::MemoryBuffer::getMemBuffer(""));
  VFS->addFile(Files[3], 0, llvm::MemoryBuffer::getMemBuffer("export module M;\nexport int foo();\n"));
  VFS->addFile(Files[4], 0, llvm::MemoryBuffer::getMemBuffer("#include \"header.h\"\nimport M;\nint main() { return foo(); }\n"));

  DependencyScanningService Service(ScanningMode::DependencyDirectivesScan,
                                    ScanningOutputFormat::Make);
  DependencyScanningWorker Worker(Service, VFS);

  for (const auto &File : Files) {

    if (File == Files[2]) {
      llvm::outs() << "Skipping assembly file: " << File << "\n";
      continue;
    }

    llvm::DenseSet<ModuleID> AlreadySeen;
    FullDependencyConsumer DC(AlreadySeen);
    CallbackActionController AC(nullptr);

    std::vector<std::string> Args = {
        "clang++",
        "-target",
        "x86_64-unknown-linux-gnu",
        "-c",
        File,
        "-o",
        File + ".o"
    };

    EnsureFinishedConsumer DiagConsumer;
    bool Success = Worker.computeDependencies(CWD, Args, DC, AC, DiagConsumer);

    if (Success) {
      TranslationUnitDeps TU = DC.takeTranslationUnitDeps();
      llvm::outs() << "Dependencies for " << File << ":\n";
      llvm::outs() << "  File Dependencies:\n";
      for (const auto &FileDep : TU.FileDeps) {
        llvm::outs() << "    - " << FileDep << "\n";
      }
      llvm::outs() << "  Module Dependencies:\n";
      for (const auto &ModuleDep : TU.ClangModuleDeps) {
        llvm::outs() << "[log] inside the Module Dependencies loop : ";
        llvm::outs() << "    - " << "ModuleID{ModuleName: " << ModuleDep.ModuleName
                     << ", ContextHash: " << ModuleDep.ContextHash << "}" << "\n";
      }
    } else {
      llvm::errs() << "Failed to compute dependencies for " << File << ".\n";
    }

    llvm::outs() << "Success: " << Success << "\n";
    llvm::outs() << "Diagnostic Consumer Finished: " << DiagConsumer.Finished << "\n";
  }
  return 0;
}