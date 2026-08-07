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
    // release ModelLoader's ownership of model_lib_
    model_lib_.reset();
}

std::shared_ptr<Model> ModelLoader::load_model() {
    if (model_name_.empty()) {
      throw std::runtime_error("uninitialised model name: failed to load hls4ml emulator model !");
    }

    //Open the model .so
    std::string const model_lib_name = model_name_ + ".so";
    raw_handle = dlopen(model_lib_name.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!raw_handle) {
        std::cerr << "Cannot load library: " << dlerror() << std::endl;
        throw std::runtime_error("hls4ml emulator model library dlopen failure!");
    }

    //set up shared pointer to close model only when all owners release ownership
    model_lib_ = std::shared_ptr<void>(raw_handle, [](void* handle){
        if (handle != nullptr) {
            dlclose(handle);
        }
    });

    //bind the model .so's "create_model" function to "create_model" so we can make a model later
    create_model_cls* create_model = (create_model_cls*) dlsym(model_lib_.get(), "create_model");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        throw std::runtime_error("hls4ml emulator failed to load 'create_model' symbol!");
    }

    //bind the model .so's "destroy_model" function to "destroy" so the model can also be destroyed later
    destroy_model_cls* destroy = (destroy_model_cls*) dlsym(model_lib_.get(), "destroy_model");
    dlsym_error = dlerror();
    if (dlsym_error) {
        throw std::runtime_error("hls4ml emulator failed to load 'destroy_model' symbol!");
    }

    //Create a/the model from our specific implementation in the loaded .so
    Model* model = create_model();
    //capture model_lib_ by value -> Model shares ownership
    std::shared_ptr<void> lib_keepalive = model_lib_;
    //Hand over a shared pointer to the model
    //Also hand over its destructor, which is, in essence, the "destroy" function/symbol we created earlier
    //bind the keepalive pointer in the destructor to prevent early dlclose call
    return std::shared_ptr<Model>(
        model,
        [lib_keepalive, destroy](Model* m)
        {
            if (m != nullptr)
            {
                destroy(m);
            }
        }
    );
}
