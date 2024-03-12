#include "emulator.h"

using namespace hls4mlEmulator;

ModelLoader::ModelLoader(std::string model_name)
{
    model_lib_ = nullptr; // Set to null for defined destructor behavior in case of failed load
    model_name_ = std::move(model_name);
    model_name_.append(".so");
}

ModelLoader::~ModelLoader(){
  if (model_lib_ != nullptr)
    dlclose(model_lib_);
}


std::shared_ptr<Model> ModelLoader::load_model()
{
    //Open the model .so
    model_lib_ = dlopen(model_name_.c_str(), RTLD_LAZY | RTLD_LOCAL);
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
