#include <jlcxx/jlcxx.hpp>

int main()
{
  jl_init();
  jl_eval_string(R"(
    include("setup-test.jl")
  )");

  #ifdef _OS_WINDOWS_
  const std::string libdir = "bin";
  const std::string libext = "dll";
  #elif defined _OS_DARWIN_
  const std::string libext = "dylib";
  const std::string libdir = "lib";
  #else
  const std::string libext = "so";
  const std::string libdir = "lib";
  #endif

  const std::string libpath = "../" + libdir + "/libjlqml." + libext;

  auto lib =  jl_dlopen(libpath.c_str(), JL_RTLD_GLOBAL);
  if(lib == nullptr)
  {
    throw std::runtime_error("Error opening jlqml lib " + libpath);
  }
  void* regfunc = nullptr;
  if(!jl_dlsym(lib, "define_julia_module", &regfunc, false))
  {
    throw std::runtime_error("Error finding entrypoint define_julia_module");
  };

  jl_value_t* mod = jl_eval_string(R"(
    module QML
      const __cxxwrap_pointers = Ptr{Cvoid}[]
    end
  )");
  JL_GC_PUSH1(&mod);

  register_julia_module((jl_module_t*)mod, reinterpret_cast<void (*)(jlcxx::Module&)>(regfunc));
  jl_call1(jl_get_function(jlcxx::get_cxxwrap_module(), "wraptypes"), mod);

  jl_value_t* dt = jl_eval_string("QML.QVariant");
  if(dt == nullptr)
  {
    throw std::runtime_error("Datatype QVariant not found");
  }
  
  std::cout << "Successfully loaded jlqml library and found QVariant type" << std::endl;
  
  JL_GC_POP();
  
  jl_atexit_hook(0);

  return 0;
}