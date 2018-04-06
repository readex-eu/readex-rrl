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
static void free_destructor(void* data, size_t length, void* arg)
{
    free(data);
}

static void empty_destructor(void* data, size_t length, void* arg)
{
}

modell::modell(std::string model_path)
{
    session_options = TF_NewSessionOptions();
    graph = TF_NewGraph();
    meta_graph_def = NULL;
    status = TF_NewStatus();

    const char* tag[] = {"serve"};

    session = TF_LoadSessionFromSavedModel(
        session_options, NULL, model_path.c_str(), tag, 1, graph, meta_graph_def, status);
    if (TF_GetCode(status) != TF_OK)
    {
        throw tf_error("Error in TF_LoadSessionFromSavedModel", status);
    }

    x_op = TF_GraphOperationByName(graph, "x");
    if (x_op == nullptr)
    {
        throw error("could not get x_op");
    }
    keep_prob_op = TF_GraphOperationByName(graph, "keep_prob");
    if (keep_prob_op == nullptr)
    {
        throw error("could not get keep_prob_op");
    }
    keep_prob2_op = TF_GraphOperationByName(graph, "keep_prob2");
    if (keep_prob2_op == nullptr)
    {
        throw error("could not get keep_prob2_op");
    }

    input_signature = (TF_Output*) calloc(3, sizeof(TF_Output));
    input_signature[0].index = 0;
    input_signature[0].oper = x_op;
    input_signature[1].index = 0;
    input_signature[1].oper = keep_prob_op;
    input_signature[2].index = 0;
    input_signature[2].oper = keep_prob2_op;

    auto output_op = TF_GraphOperationByName(graph, "04_lay/Wx_plus_b/y_result");
    if (output_op == nullptr)
    {
        throw error("could not get output_op");
    }

    output_signature = (TF_Output*) calloc(1, sizeof(TF_Output));
    output_signature[0].index = 0;
    output_signature[0].oper = output_op;
}

std::vector<float> modell::get_prediction(
    std::vector<float> input_values, size_t expected_output_values)
{
    long int x_dim[] = {1, 0};
    x_dim[1] = input_values.size();
    auto x_tensor = TF_NewTensor(
        TF_FLOAT, x_dim, 2, input_values.data(), input_values.size(), &empty_destructor, 0);

    long int keep_prob_dim[] = {1};
    float* keep_prob_value = (float*) calloc(1, sizeof(float));
    keep_prob_value[0] = 1;
    auto keep_prob_tensor =
        TF_NewTensor(TF_FLOAT, keep_prob_dim, 1, keep_prob_value, 1, &free_destructor, 0);

    long int keep_prob2_dim[] = {1};
    float* keep_prob2_value = (float*) calloc(1, sizeof(float));
    keep_prob2_value[0] = 1;
    auto keep_prob2_tensor =
        TF_NewTensor(TF_FLOAT, keep_prob2_dim, 1, keep_prob2_value, 1, &free_destructor, 0);

    TF_Tensor** input_tensors = (TF_Tensor**) calloc(3, sizeof(TF_Tensor*));
    input_tensors[0] = x_tensor;
    input_tensors[1] = keep_prob_tensor;
    input_tensors[2] = keep_prob2_tensor;

    TF_Tensor** output_tensors = (TF_Tensor**) calloc(1, sizeof(TF_Tensor*));

    TF_SessionRun(session,
        NULL,
        input_signature,
        input_tensors,
        3,
        output_signature,
        output_tensors,
        1,
        nullptr,
        0,
        nullptr,
        status);
    if (TF_GetCode(status) != TF_OK)
    {
        throw tf_error("Error in TF_SessionRun", status);
    }

    float* out_vals = static_cast<float*>(TF_TensorData(output_tensors[0]));
    std::vector<float> result;
    for (size_t i = 0; i < expected_output_values; i++)
    {
        result.push_back(out_vals[i]);
    }

    free(input_tensors);
    free(output_tensors);
    TF_DeleteTensor(x_tensor);
    TF_DeleteTensor(keep_prob_tensor);
    TF_DeleteTensor(keep_prob2_tensor);
    TF_DeleteTensor(output_tensors[0]);

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
}
}
}
