/**
 * @file resnet.hpp
 * @author Aakash Kaushik
 * 
 * Definition of ResNet models.
 * 
 * For more information, kindly refer to the following paper.
 *
 * @code
 * @article{Kaiming He2015,
 *  author = {Kaiming He, Xiangyu Zhang, Shaoqing Ren, Jian Sun},
 *  title = {Deep Residual Learning for Image Recognition},
 *  year = {2015},
 *  url = {https://arxiv.org/pdf/1512.03385.pdf}
 * }
 * @endcode
 * 
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#ifndef MODELS_MODELS_RESNET_RESNET_HPP
#define MODELS_MODELS_RESNET_RESNET_HPP

#define MLPACK_ENABLE_ANN_SERIALIZATION
#include <mlpack.hpp>

#include "./../../utils/utils.hpp"

namespace mlpack {
namespace models {

/**
 * Definition of a ResNet CNN.
 * 
 * @tparam OutputLayerType The output layer type used to evaluate the network.
 * @tparam InitializationRuleType Rule used to initialize the weight matrix.
 * @tparam ResNetVersion Version of ResNet.
 */
template<
  typename OutputLayerType = CrossEntropyError<>,
  typename InitializationRuleType = RandomInitialization,
  size_t ResNetVersion = 18
>
class ResNet
{
 public:
  //! Create the ResNet model.
  ResNet();

  /**
   * ResNet constructor intializes input shape and number of classes.
   *
   * @param inputChannels Number of input channels of the input image.
   * @param inputWidth Width of the input image.
   * @param inputHeight Height of the input image.
   * @param includeTop Must be set to true if preTrained is set to true.
   * @param preTrained True for pre-trained weights of ImageNet,
   *    default is false.
   * @param numClasses Optional number of classes to classify images into,
   *     only to be specified if includeTop is true, default is 1000.
   */
  ResNet(const size_t inputChannel,
         const size_t inputWidth,
         const size_t inputHeight,
         const bool includeTop = true,
         const bool preTrained = false,
         const size_t numClasses = 1000);

  /**
   * ResNet constructor intializes input shape and number of classes.
   *
   * @param inputShape A three-valued tuple indicating input shape.
   *     First value is number of channels (channels-first).
   *     Second value is input height. Third value is input width.
   * @param preTrained True for pre-trained weights of ImageNet,
   *    default is false.
   * @param numClasses Optional number of classes to classify images into,
   *     only to be specified if includeTop is  true.
   */
  ResNet(std::tuple<size_t, size_t, size_t> inputShape,
         const bool includeTop = true,
         const bool preTrained = false,
         const size_t numClasses = 1000);

  //! Get Layers of the model.
  FFN<OutputLayerType, InitializationRuleType>& GetModel() { return resNet; }

  //! Load weights into the model and assumes the internal matrix to be
  //  named "ResNet".
  void LoadModel(const std::string& filePath);

  //! Save weights for the model and assumes the internal matrix to be
  //  named "ResNet".
  void SaveModel(const std::string& filepath);

 private:
  /**
   * Adds a Convolution Block depending on the configuration.
   *
   * @tparam SequentialType Layer type in which convolution block will
   *     be added.
   *
   * @param inSize Number of input maps.
   * @param outSize Number of output maps.
   * @param strideWidth Stride of filter application in the x direction.
   * @param strideHeight Stride of filter application in the y direction.
   * @param kernelWidth Width of the filter/kernel.
   * @param kernelHeight Height of the filter/kernel.
   * @param padW Padding width of the input.
   * @param padH Padding height of the input.
   * @param downSample Bool if it's a downsample block or not. Default is false.
   * @param downSampleInputWidth Input widht for downSample block.
   * @param downSampleInputHeight Input height for downSample block.
   */
  template<typename SequentialType = Sequential<>>
  void ConvolutionBlock(SequentialType* baseLayer,
                        const size_t inSize,
                        const size_t outSize,
                        const size_t strideWidth = 1,
                        const size_t strideHeight = 1,
                        const size_t kernelWidth = 3,
                        const size_t kernelHeight = 3,
                        const size_t padW = 1,
                        const size_t padH = 1,
                        const bool downSample = false,
                        const size_t downSampleInputWidth = 0,
                        const size_t downSampleInputHeight = 0)
  {
    if (downSample)
    {
      mlpack::Log::Info << "DownSample (" << std::endl;
      inputWidth = downSampleInputWidth;
      inputHeight = downSampleInputHeight;
    }

    Sequential<>* tempBaseLayer = new Sequential<>();
    tempBaseLayer->Add(new Convolution<>(inSize, outSize, kernelWidth,
        kernelHeight, strideWidth, strideHeight, padW, padH, inputWidth,
        inputHeight));
    mlpack::Log::Info << "Convolution: " << "(" << inSize << ", " <<
        inputWidth << ", " << inputHeight << ")" << " ---> (";

    // Updating input dimensions.
    inputWidth = ConvOutSize(inputWidth, kernelWidth, strideWidth, padW);
    inputHeight = ConvOutSize(inputHeight, kernelHeight, strideHeight,
        padH);

    mlpack::Log::Info << outSize << ", " << inputWidth << ", " <<
        inputHeight << ")" << std::endl;

    tempBaseLayer->Add(new BatchNorm<>(outSize, 1e-5));
    mlpack::Log::Info << "BatchNorm: " << "(" << outSize << ")" << " ---> ("
        << outSize << ")" << std::endl;
    baseLayer->Add(tempBaseLayer);
    if (downSample)
      mlpack::Log::Info << ")" <<std::endl;
  }

  /**
   * Adds a ReLU Layer.
   *
   * @param baseLayer Sequential layer type in which ReLU layer will be added.
   */
  void ReLULayer(Sequential<>* baseLayer)
  {
    baseLayer->Add(new ReLULayer<>);
    mlpack::Log::Info << "Relu" << std::endl;
  }

  /**
   * Adds basicBlock block for ResNet 18 and 34.
   *
   * It's represented as:
   * 
   * @code
   * resBlock - AddMerge layer
   * {
   *   sequentialBlock - sequentialLayer
   *   {
   *     ConvolutionBlock(inSize, outSize, strideWidth, strideHeight)
   *     ReLU
   *     ConvolutionBlock(inSize, outSize)
   *   }
   *
   *   sequentialLayer
   *   {
   *     if downsample == true
   *       ConvolutionBlock(inSize, outSize, strideWidth, strideHeight, 1, 1,
   *           0, 0, true, downSampleInputWidth, downSampleInputHeight);
   *     else
   *       IdentityLayer
   *   }
   * 
   *   ReLU
   * }
   * @endcode
   * 
   * @param inSize Number of input maps.
   * @param outSize Number of output maps.
   * @param strideWidth Stride of filter application in the x direction.
   * @param strideHeight Stride of filter application in the y direction.
   * @param downSample If there will be a downSample block or not, default
   *     false.
   */
  void BasicBlock(const size_t inSize,
                  const size_t outSize,
                  const size_t strideWidth = 1,
                  const size_t strideHeight = 1,
                  const bool downSample = false)
  {
    downSampleInputWidth = inputWidth;
    downSampleInputHeight = inputHeight;

    Sequential<>* basicBlock = new Sequential<>();
    AddMerge<>* resBlock = new AddMerge<>(true, true);
    Sequential<>* sequentialBlock = new Sequential<>();
    ConvolutionBlock(sequentialBlock, inSize, outSize, strideWidth,
        strideHeight);
    ReLULayer(sequentialBlock);
    ConvolutionBlock(sequentialBlock, outSize, outSize);

    resBlock->Add(sequentialBlock);

    if (downSample == true)
    {
      ConvolutionBlock(resBlock, inSize, outSize, strideWidth, strideHeight,
          1, 1, 0, 0, true, downSampleInputWidth, downSampleInputHeight);
    }
    else
    {
      mlpack::Log::Info << "IdentityLayer" << std::endl;
      resBlock->Add(new IdentityLayer<>);
    }

    basicBlock->Add(resBlock);
    ReLULayer(basicBlock);
    resNet.Add(basicBlock);
  }

  /**
   * Adds bottleNeck block for ResNet 50, 101 and 152.
   *
   * It's represented as:
   * 
   * @code
   * resBlock - AddMerge layer
   * {
   *   sequentialBlock
   *   {
   *     ConvolutionBlock(sequentialBlock, inSize, width, 1, 1, 1, 1, 0, 0)
   *     ReLU
   *     ConvolutionBlock(width, width, strideWidth, strideHeight)
   *     ReLU
   *     ConvolutionBlock(sequentialBlock, width, outSize * bottleNeckExpansion,
   *         1, 1, 1, 1, 0, 0)
   *   }
   *
   *   sequentialLayer
   *   {
   *     if downsample == true
   *       ConvolutionBlock(inSize, outSize * bottleNeckExpansion, strideWidth,
   *           strideHeight, 1, 1, 0, 0, true, downSampleInputWidth,
   *           downSampleInputHeight);
   *     else
   *       IdentityLayer
   *   }
   * 
   *   ReLU
   * }
   *@endcode
   *
   * @param inSize Number of input maps.
   * @param outSize Number of output maps.
   * @param strideWidth Stride of filter application in the x direction.
   * @param strideHeight Stride of filter application in the y direction.
   * @param downSample If there will be a downSample block or not, default
   *     false.
   * @param baseWidth Parameter for calculating width.
   * @param groups Parameter for calculating width.
   */
  void BottleNeck(const size_t inSize,
                  const size_t outSize,
                  const size_t strideWidth = 1,
                  const size_t strideHeight = 1,
                  const bool downSample = false,
                  const size_t baseWidth = 64,
                  const size_t groups = 1)
  {
    downSampleInputWidth = inputWidth;
    downSampleInputHeight = inputHeight;

    size_t width = int((baseWidth / 64.0) * outSize) * groups;
    Sequential<>* basicBlock = new Sequential<>();
    AddMerge<>* resBlock = new AddMerge<>(true, true);
    Sequential<>* sequentialBlock = new Sequential<>();
    ConvolutionBlock(sequentialBlock, inSize, width, 1, 1, 1, 1, 0, 0);
    ReLULayer(sequentialBlock);
    ConvolutionBlock(sequentialBlock, width, width, strideWidth,
        strideHeight);
    ReLULayer(sequentialBlock);
    ConvolutionBlock(sequentialBlock, width, outSize * bottleNeckExpansion, 1,
        1, 1, 1, 0, 0);
    resBlock->Add(sequentialBlock);

    if (downSample == true)
    {
      ConvolutionBlock(resBlock, inSize, outSize * bottleNeckExpansion,
           strideWidth, strideHeight, 1, 1, 0, 0, true, downSampleInputWidth,
           downSampleInputHeight);
    }
    else
    {
      mlpack::Log::Info << "IdentityLayer" << std::endl;
      resBlock->Add(new IdentityLayer<>);
    }

    basicBlock->Add(resBlock);
    ReLULayer(basicBlock);
    resNet.Add(basicBlock);
  }

  /**
   * Creates model layers based on the type of layer and parameters supplied.
   *
   * @param block Type of block to use for layer creation.
   * @param outSize Number of output maps.
   * @param numBlocks Number of layers to create.
   * @param stride Single parameter for StrideHeight and strideWidth.
   */
  void MakeLayer(const std::string& block,
                 const size_t outSize,
                 const size_t numBlocks,
                 const size_t stride = 1)
  {
    bool downSample = false;

    if (block == "basicblock")
    {
      if (stride != 1 || downSampleInSize != outSize * basicBlockExpansion)
        downSample = true;
      BasicBlock(downSampleInSize, outSize * basicBlockExpansion, stride,
          stride, downSample);
      downSampleInSize = outSize * basicBlockExpansion;
      for (size_t i = 1; i != numBlocks; ++i)
        BasicBlock(downSampleInSize, outSize);
      return;
    }

    if (stride != 1 || downSampleInSize != outSize * bottleNeckExpansion)
      downSample = true;
    BottleNeck(downSampleInSize, outSize, stride, stride, downSample);
    downSampleInSize = outSize * bottleNeckExpansion;
    for (size_t i = 1; i != numBlocks; ++i)
      BottleNeck(downSampleInSize, outSize);
  }

  /**
   * Return the convolution output size.
   *
   * @param size The size of the input (row or column).
   * @param k The size of the filter (width or height).
   * @param s The stride size (x or y direction).
   * @param padding The size of the padding (width or height) on one side.
   * @return The convolution output size.
   */
  size_t ConvOutSize(const size_t size,
                     const size_t k,
                     const size_t s,
                     const size_t padding)
  {
    return std::floor((size - k + 2 * padding) / s) + 1;
  }

  //! Locally stored DarkNet Model.
  FFN<OutputLayerType, InitializationRuleType> resNet;

  //! Locally stored number of channels in the image.
  size_t inputChannel;

  //! Locally stored width of the image.
  size_t inputWidth;

  //! Locally stored height of the image.
  size_t inputHeight;

  //! Locally stored number of output classes.
  size_t numClasses;

  //! Locally stored width of image for downSample block.
  size_t downSampleInputWidth;

  //! Locally stored height of image for downSample block.
  size_t downSampleInputHeight;

  //! Locally stored expansion for BasicBlock.
  size_t basicBlockExpansion = 1;

  //! Locally stored expansion for BottleNeck.
  size_t bottleNeckExpansion = 4;

  //! InSize for ResNet block creation.
  size_t downSampleInSize = 64;

  //! Locally stored map to constructor different ResNet versions.
  std::map<size_t, std::map<std::string, std::array<size_t, 4>>> ResNetConfig =
      {
        {18, {{"basicblock", {2, 2, 2, 2}}}},
        {34, {{"basicblock", {3, 4, 6, 3}}}},
        {50, {{"bottleneck", {3, 4, 6, 3}}}},
        {101, {{"bottleneck", {3, 4, 23, 3}}}},
        {152, {{"bottleneck", {3, 8, 36, 3}}}}
      };

  //! Locally stored array to constructor different ResNet versions.
  std::array<size_t , 4> numBlockArray;

  //! Locally stored block string from which to build the model.
  std::string builderBlock;

  //! Locally stored path string for pre-trained model.
  std::string preTrainedPath;
}; // ResNet class

// convenience typedefs for different ResNet models.
typedef ResNet<CrossEntropyError<>, RandomInitialization, 18> ResNet18;
typedef ResNet<CrossEntropyError<>, RandomInitialization, 34> ResNet34;
typedef ResNet<CrossEntropyError<>, RandomInitialization, 50> ResNet50;
typedef ResNet<CrossEntropyError<>, RandomInitialization, 101> ResNet101;
typedef ResNet<CrossEntropyError<>, RandomInitialization, 152> ResNet152;

} // namespace models
} // namespace mlpack

#include "resnet_impl.hpp"

#endif
