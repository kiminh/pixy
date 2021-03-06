//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "ml/layers/convolution_2d.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/convolution_2d.hpp"
#include "ml/ops/placeholder.hpp"

namespace fetch {
namespace ml {
namespace layers {

template <typename TensorType>
Convolution2D<TensorType>::Convolution2D(SizeType const output_channels,
                                         SizeType const input_channels, SizeType const kernel_size,
                                         SizeType const                stride_size,
                                         details::ActivationType const activation_type,
                                         std::string const &name, WeightsInit const init_mode,
                                         SizeType const seed)
  : kernel_size_{kernel_size}
  , input_channels_{input_channels}
  , output_channels_{output_channels}
  , stride_size_{stride_size}
{
  FETCH_LOG_INFO(Descriptor(), "-- Convolution2D initialisation ... --");
  std::string input =
      this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});

  std::string weights =
      this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Weights", {});

  TensorType weights_data(
      std::vector<SizeType>{{output_channels_, input_channels_, kernel_size_, kernel_size_, 1}});
  fetch::ml::ops::Weights<TensorType>::Initialise(weights_data, 1, 1, init_mode, seed);
  this->SetInput(weights, weights_data);

  std::string output = this->template AddNode<fetch::ml::ops::Convolution2D<TensorType>>(
      name + "_Conv2D", {input, weights}, stride_size_);

  output = fetch::ml::details::AddActivationNode<TensorType>(activation_type, this,
                                                             name + "_Activation", output);

  this->GetNode(weights)->SetBatchOutputShape(
      {output_channels_, input_channels_, kernel_size_, kernel_size_, 1});

  // TODO(ML-470): Preliminary batch shape of a Conv2d layer (channels x 32(h) x 32(w) x
  // 1(batch) ), is used here now, however, real width and height are to be set later on graph
  // compilation (when expected Input shape of the Model/Graph is already known). Thus the
  // convolutional weight init can be done on constructions, but this->input shape has to be
  // inited only in this->CompleteInitialisation() override.
  static constexpr SizeType DEFAULT_HEIGHT = 32;
  static constexpr SizeType DEFAULT_WIDTH  = 32;
  this->GetNode(input)->SetBatchOutputShape({output_channels_, DEFAULT_HEIGHT, DEFAULT_WIDTH, 1});

  this->AddInputNode(input);
  this->SetOutputNode(output);

  this->Compile();
  FETCH_LOG_INFO(Descriptor(), "-- Convolution2D initialisation completed. --");
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Convolution2D<TensorType>::GetOpSaveableParams()
{
  // get all base classes saveable params
  std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

  auto ret = std::make_shared<SPType>();

  // copy subgraph saveable params over
  auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
  auto sg_ptr2 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
  *sg_ptr2     = *sg_ptr1;

  // asign layer specific params
  ret->kernel_size     = kernel_size_;
  ret->input_channels  = input_channels_;
  ret->output_channels = output_channels_;
  ret->stride_size     = stride_size_;

  return ret;
}

template <typename TensorType>
void Convolution2D<TensorType>::SetOpSaveableParams(SPType const &sp)
{
  // assign layer specific params
  kernel_size_     = sp.kernel_size;
  input_channels_  = sp.input_channels;
  output_channels_ = sp.output_channels;
  stride_size_     = sp.stride_size;
}

template <typename TensorType>
std::vector<math::SizeType> Convolution2D<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  TensorType weights_data(
      std::vector<SizeType>{{output_channels_, input_channels_, kernel_size_, kernel_size_, 1}});
  return fetch::ml::ops::Convolution2D<TensorType>(stride_size_)
      .ComputeOutputShape({inputs.at(0), std::make_shared<TensorType>(weights_data)});
}

template <class TensorType>
OperationsCount Convolution2D<TensorType>::ChargeForward()
{
  auto ptr = dynamic_cast<Graph<TensorType> *>(this);
  return ptr->ChargeForward(this->output_node_name_);
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Convolution2D<math::Tensor<int8_t>>;
template class Convolution2D<math::Tensor<int16_t>>;
template class Convolution2D<math::Tensor<int32_t>>;
template class Convolution2D<math::Tensor<int64_t>>;
template class Convolution2D<math::Tensor<float>>;
template class Convolution2D<math::Tensor<double>>;
template class Convolution2D<math::Tensor<fixed_point::fp32_t>>;
template class Convolution2D<math::Tensor<fixed_point::fp64_t>>;
template class Convolution2D<math::Tensor<fixed_point::fp128_t>>;
}  // namespace layers
}  // namespace ml
}  // namespace fetch
