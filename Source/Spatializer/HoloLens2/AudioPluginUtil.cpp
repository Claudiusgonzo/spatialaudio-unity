#include "AudioPluginUtil.h"
#include <string.h>
#include <exception>
#include <cstdlib>

char* strnew(const char* src)
{
    const auto newstr_size = strlen(src) + 1;
    auto newstr = new char[newstr_size];
#if _MSC_VER
    strcpy_s(newstr, newstr_size, src);
#else
    strcpy(newstr, src);
#endif
    return newstr;
}

void RegisterParameter(
    UnityAudioEffectDefinition& definition, const char* name, const char* unit, float minval, float maxval,
    float defaultval, float displayscale, float displayexponent, int enumvalue, const char* description)
{
    if (defaultval < minval || defaultval > maxval)
    {
        std::abort();
    }
    strcpy_s(definition.paramdefs[enumvalue].name, name);
    strcpy_s(definition.paramdefs[enumvalue].unit, unit);
    definition.paramdefs[enumvalue].description =
        (description != nullptr) ? strnew(description) : (name != nullptr) ? strnew(name) : nullptr;
    definition.paramdefs[enumvalue].defaultval = defaultval;
    definition.paramdefs[enumvalue].displayscale = displayscale;
    definition.paramdefs[enumvalue].displayexponent = displayexponent;
    definition.paramdefs[enumvalue].min = minval;
    definition.paramdefs[enumvalue].max = maxval;
    if (enumvalue >= static_cast<int>(definition.numparameters))
    {
        definition.numparameters = enumvalue + 1;
    }
}

// Helper function to fill default values from the effect definition into the params array -- called by Create callbacks
void InitParametersFromDefinitions(
    InternalEffectDefinitionRegistrationCallback registereffectdefcallback, float* params)
{
    UnityAudioEffectDefinition definition;
    memset(&definition, 0, sizeof(definition));
    registereffectdefcallback(definition);
    for (auto n = 0u; n < definition.numparameters; n++)
    {
        params[n] = definition.paramdefs[n].defaultval;
        delete[] definition.paramdefs[n].description;
    }
    // assumes that definition.paramdefs was allocated by registereffectdefcallback or is NULL
    delete[] definition.paramdefs;
}

void DeclareEffect(
    UnityAudioEffectDefinition& definition, const char* name, UnityAudioEffect_CreateCallback createcallback,
    UnityAudioEffect_ReleaseCallback releasecallback, UnityAudioEffect_ProcessCallback processcallback,
    UnityAudioEffect_SetFloatParameterCallback setfloatparametercallback,
    UnityAudioEffect_GetFloatParameterCallback getfloatparametercallback,
    UnityAudioEffect_GetFloatBufferCallback getfloatbuffercallback,
    InternalEffectDefinitionRegistrationCallback registereffectdefcallback)
{
    memset(&definition, 0, sizeof(definition));
    strcpy_s(definition.name, name);
    definition.structsize = sizeof(UnityAudioEffectDefinition);
    definition.paramstructsize = sizeof(UnityAudioParameterDefinition);
    definition.apiversion = UNITY_AUDIO_PLUGIN_API_VERSION;
    definition.pluginversion = 0x010000;
    definition.create = createcallback;
    definition.release = releasecallback;
    definition.process = processcallback;
    definition.setfloatparameter = setfloatparametercallback;
    definition.getfloatparameter = getfloatparametercallback;
    definition.getfloatbuffer = getfloatbuffercallback;
    registereffectdefcallback(definition);
}

#define DECLARE_EFFECT(namestr, ns)                                                                                    \
    namespace ns                                                                                                       \
    {                                                                                                                  \
        UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK CreateCallback(UnityAudioEffectState* state);                    \
        UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ReleaseCallback(UnityAudioEffectState* state);                   \
        UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ProcessCallback(                                                 \
            UnityAudioEffectState* state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels,      \
            int outchannels);                                                                                          \
        UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK                                                                  \
        SetFloatParameterCallback(UnityAudioEffectState* state, int index, float value);                               \
        UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK                                                                  \
        GetFloatParameterCallback(UnityAudioEffectState* state, int index, float* value, char* valuestr);              \
        UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK                                                                  \
        GetFloatBufferCallback(UnityAudioEffectState* state, const char* name, float* buffer, int numsamples);         \
        int InternalRegisterEffectDefinition(UnityAudioEffectDefinition& definition);                                  \
    }
#include "PluginList.h"
#undef DECLARE_EFFECT

#define DECLARE_EFFECT(namestr, ns)                                                                                    \
    DeclareEffect(                                                                                                     \
        definition[numeffects++],                                                                                      \
        namestr,                                                                                                       \
        ns::CreateCallback,                                                                                            \
        ns::ReleaseCallback,                                                                                           \
        ns::ProcessCallback,                                                                                           \
        ns::SetFloatParameterCallback,                                                                                 \
        ns::GetFloatParameterCallback,                                                                                 \
        ns::GetFloatBufferCallback,                                                                                    \
        ns::InternalRegisterEffectDefinition);

extern "C" UNITY_AUDIODSP_EXPORT_API int UnityGetAudioEffectDefinitions(UnityAudioEffectDefinition*** definitionptr)
{
    static UnityAudioEffectDefinition definition[256];
    static UnityAudioEffectDefinition* definitionp[256];
    static int numeffects = 0;
    if (numeffects == 0)
    {
#include "PluginList.h"
    }
    for (int n = 0; n < numeffects; n++)
    {
        definitionp[n] = &definition[n];
    }
    *definitionptr = definitionp;
    return numeffects;
}
