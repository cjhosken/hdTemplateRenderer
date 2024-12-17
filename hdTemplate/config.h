//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef HD_TEMPLATE_CONFIG_H
#define HD_TEMPLATE_CONFIG_H

#include "pxr/pxr.h"
#include "pxr/base/tf/singleton.h"

PXR_NAMESPACE_OPEN_SCOPE

// NOTE: types here restricted to bool/int/string, as also used for
// TF_DEFINE_ENV_SETTING
//constexpr int HdTemplateDefaultRandomNumberSeed = -1;

/// \class HdTemplateConfig
///
/// This class is a singleton, holding configuration parameters for HdTemplate.
/// Everything is provided with a default, but can be overridden using
/// environment variables before launching a hydra process.
///
/// Many of the parameters can be used to control quality/performance
/// tradeoffs, or to alter how HdTemplate takes advantage of parallelism.
///
/// At startup, this class will print config parameters if
/// *HDEMBREE_PRINT_CONFIGURATION* is true. Integer values greater than zero
/// are considered "true".
///
class HdTemplateConfig {
public:
    /// \brief Return the configuration singleton.
    static const HdTemplateConfig &GetInstance();
    
    /// Seed to give to the random number generator. A value of anything other
    /// than -1, combined with setting PXR_WORK_THREAD_LIMIT=1, should give
    /// deterministic / repeatable results. A value of -1 (the default) will
    /// allow the implementation to set a value that varies from invocation to
    /// invocation and thread to thread.
    ///
    /// Override with *HDEMBREE_RANDOM_NUMBER_SEED*.
    //int randomNumberSeed = HdTemplateDefaultRandomNumberSeed;

private:
    // The constructor initializes the config variables with their
    // default or environment-provided override, and optionally prints
    // them.
    HdTemplateConfig();
    ~HdTemplateConfig() = default;

    HdTemplateConfig(const HdTemplateConfig&) = delete;
    HdTemplateConfig& operator=(const HdTemplateConfig&) = delete;

    friend class TfSingleton<HdTemplateConfig>;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_TEMPLATe_CONFIG_H