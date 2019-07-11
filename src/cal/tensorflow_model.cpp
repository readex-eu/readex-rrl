/*
 * tensorflow_model.cpp
 *
 *  Created on: 27.02.2018
 *      Author: gocht
 */

#include <cal/tensorflow_model.hpp>

namespace rrl
{
namespace cal
{
namespace tensorflow
{
// static void free_destructor(void* data, size_t length, void* arg)
//{
//    free(data);
//}

static void empty_destructor(void *data, size_t length, void *arg)
{
}

modell::modell(std::string model_path)
{
    session_options = TF_NewSessionOptions();
    graph = TF_NewGraph();
    meta_graph_def = NULL;
    status = TF_NewStatus();

    const char *tag[] = {"serve"};

    session = TF_LoadSessionFromSavedModel(
        session_options, NULL, model_path.c_str(), tag, 1, graph, meta_graph_def, status);
    if (TF_GetCode(status) != TF_OK)
    {
        throw tf_error("Error in TF_LoadSessionFromSavedModel", status);
    }

    x_op = TF_GraphOperationByName(graph, "00_input");
    if (x_op == nullptr)
    {
        throw error("could not get x_op");
    }

    input_signature = (TF_Output *) calloc(3, sizeof(TF_Output));
    input_signature[0].index = 0;
    input_signature[0].oper = x_op;

    auto f_c = TF_GraphOperationByName(graph, "70_c_linear/BiasAdd");
    if (f_c == nullptr)
    {
        throw error("could not get 70_c_linear/BiasAdd");
    }

    auto f_u = TF_GraphOperationByName(graph, "70_u_linear/BiasAdd");
    if (f_u == nullptr)
    {
        throw error("could not get 70_u_linear/BiasAdd");
    }

    output_signature = (TF_Output *) calloc(2, sizeof(TF_Output));
    output_signature[0].index = 0;
    output_signature[0].oper = f_c;
    output_signature[1].index = 0;
    output_signature[1].oper = f_u;
}

std::vector<float> modell::get_prediction(std::vector<float> input_values)
{
    long int x_dim[] = {1, 0};
    x_dim[1] = input_values.size();
    auto x_tensor = TF_NewTensor(TF_FLOAT,
        x_dim,
        2,
        input_values.data(),
        input_values.size() * sizeof(float),
        &empty_destructor,
        0);
    if (x_tensor == nullptr)
    {
        throw error("could not get set up x_tensor");
    }

    TF_Tensor **input_tensors = (TF_Tensor **) calloc(1, sizeof(TF_Tensor *));
    input_tensors[0] = x_tensor;

    TF_Tensor **output_tensors = (TF_Tensor **) calloc(2, sizeof(TF_Tensor *));

    TF_SessionRun(session,
        NULL,
        input_signature,
        input_tensors,
        1,
        output_signature,
        output_tensors,
        2,
        nullptr,
        0,
        nullptr,
        status);
    if (TF_GetCode(status) != TF_OK)
    {
        throw tf_error("Error in TF_SessionRun", status);
    }

    std::vector<float> result;
    float *f_c = static_cast<float *>(TF_TensorData(output_tensors[0]));
    result.push_back(f_c[0]);
    float *f_u = static_cast<float *>(TF_TensorData(output_tensors[1]));
    result.push_back(f_u[0]);

    free(input_tensors);
    TF_DeleteTensor(x_tensor);
    TF_DeleteTensor(output_tensors[0]);
    TF_DeleteTensor(output_tensors[1]);
    free(output_tensors);

    return result;
}
modell::~modell()
{
    TF_DeleteGraph(graph);
    TF_DeleteSessionOptions(session_options);
    TF_DeleteStatus(status);
    free(input_signature);
    free(output_signature);
}
} // namespace tensorflow
} // namespace cal
} // namespace rrl
