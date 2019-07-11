/*
 * tensorflow_model.hpp
 *
 *  Created on: 27.02.2018
 *      Author: gocht
 */

#ifndef INCLUDE_CAL_TENSORFLOW_MODEL_HPP_
#define INCLUDE_CAL_TENSORFLOW_MODEL_HPP_

#include <stdexcept>
#include <string>
#include <tensorflow/c/c_api.h>
#include <vector>

namespace rrl
{
namespace cal
{
namespace tensorflow
{
class tf_error : public std::runtime_error
{
public:
    tf_error(const std::string &what_arg, const TF_Status *status) : std::runtime_error(what_arg)
    {
        message = what_arg;
        message += " : ";
        message += TF_Message(status);
    }

    const char *what() const noexcept override
    {
        return message.c_str();
    }

private:
    std::string message;
};

class error : public std::runtime_error
{
public:
    error(const std::string &what_arg) : std::runtime_error(what_arg)
    {
    }
};

class modell
{
public:
    modell(std::string model_path);
    ~modell();

    std::vector<float> get_prediction(std::vector<float> input_values);

private:
    TF_SessionOptions *session_options;
    TF_Graph *graph;
    TF_Buffer *meta_graph_def;
    TF_Status *status;
    TF_Session *session;
    TF_Operation *x_op;
    TF_Output *input_signature;
    TF_Output *output_signature;
};
} // namespace tensorflow
} // namespace cal
} // namespace rrl

#endif /* INCLUDE_CAL_TENSORFLOW_MODEL_HPP_ */
