//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "config.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/instantiateSingleton.h"

#include <algorithm>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// Instantiate the config singleton.
TF_INSTANTIATE_SINGLETON(HdTemplateConfig);

// Each configuration variable has an associated environment variable.
// The environment variable macro takes the variable name, a default value,
// and a description...

//TF_DEFINE_ENV_SETTING(
//    HDEMBREE_RANDOM_NUMBER_SEED,
//    HdTemplateDefaultRandomNumberSeed,
//    "Seed to give to the random number generator. A value of anything other"
//        " than -1, combined with setting PXR_WORK_THREAD_LIMIT=1, should"
//        " give deterministic / repeatable results. A value of -1 (the"
//        " default) will allow the implementation to set a value that varies"
//        " from invocation to invocation and thread to thread.");

HdTemplateConfig::HdTemplateConfig()
{
    // Read in values from the environment, clamping them to valid ranges.
    //randomNumberSeed = TfGetEnvSetting(HDEMBREE_RANDOM_NUMBER_SEED);

}

/*static*/
const HdTemplateConfig&
HdTemplateConfig::GetInstance()
{
    return TfSingleton<HdTemplateConfig>::GetInstance();
}

PXR_NAMESPACE_CLOSE_SCOPE