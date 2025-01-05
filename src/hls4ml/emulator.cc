#include "emulator.h"

using namespace hls4mlEmulator;

ModelLoader::ModelLoader(std::string const& model_name)
  : model_lib_{nullptr},
    model_name_{model_name} {}

ModelLoader::~ModelLoader() {
    reset();
}

void ModelLoader::reset(std::string const& model_name) {
    model_name_ = model_name;
    if (model_lib_ != nullptr) {
        dlclose(model_lib_);
        model_lib_ = nullptr;
    }
}

std::shared_ptr<Model> ModelLoader::load_model() {
    if (model_name_.empty()) {
      throw std::runtime_error("uninitialised model name: failed to load hls4ml emulator model !");
    }

    //Open the model .so
    std::string const model_lib_name = model_name_ + ".so";
    model_lib_ = dlopen(model_lib_name.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!model_lib_) {
        std::cerr << "Cannot load library: " << dlerror() << std::endl;
        throw std::runtime_error("hls4ml emulator model library dlopen failure!");
    }
    
    //bind the model .so's "create_model" function to "create_model" so we can make a model later
    create_model_cls* create_model = (create_model_cls*) dlsym(model_lib_, "create_model");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        throw std::runtime_error("hls4ml emulator failed to load 'create_model' symbol!");
    }

    //bind the model .so's "destroy_model" function to "destroy" so the model can also be destroyed later
    destroy_model_cls* destroy = (destroy_model_cls*) dlsym(model_lib_, "destroy_model");
    dlsym_error = dlerror();
    if (dlsym_error) {
        throw std::runtime_error("hls4ml emulator failed to load 'destroy_model' symbol!");
    }

    //Create a/the model from our specific implementation in the loaded .so
    Model* model = create_model();
    //Hand over a shared pointer to the model
    //Also hand over it's destructor, which is, in essence, the "destroy" function/symbol we created earlier
    return std::shared_ptr<Model>(
        model,
        [destroy](Model* m)
        {
            if (m != nullptr)
            {
                destroy(m);
            }
        }
    );
}
