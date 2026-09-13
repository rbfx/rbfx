// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Variant.h"
#include "Urho3D/IO/Archive.h"
#include "Urho3D/Engine/ApplicationFlavor.h"

namespace Urho3D
{

/// Configuration parameter description.
struct URHO3D_API ConfigVariableDefinition
{
    ConfigVariableDefinition& SetDefault(const Variant& value);
    ConfigVariableDefinition& SetOptional(VariantType type);
    ConfigVariableDefinition& Overridable();
    ConfigVariableDefinition& CommandLinePriority();

    template <class T> ConfigVariableDefinition& SetOptional() { return SetOptional(GetVariantType<T>()); }

    /// Whether to allow overriding this parameter in user configuration.
    bool overridable_{};
    /// Whether this parameter should be applied as soon as possible when specified in the command line.
    bool commandLinePriority_{};
    /// Default value of the variable. Also defines variable type.
    Variant defaultValue_{};
    /// Type of the variable. May be different from default value type if default value is null.
    VariantType type_{};
};

/// A class responsible for serializing configuration parameters.
class URHO3D_API ConfigFile : public Object
{
    URHO3D_OBJECT(ConfigFile, Object);

public:
    struct ConfigFlavor
    {
        ApplicationFlavorPattern flavor_;
        StringVariantMap variables_;

        void SerializeInBlock(Archive& archive);
    };

    using ConfigFlavorVector = ea::vector<ConfigFlavor>;
    using ConfigVariableDefinitionMap = ea::unordered_map<ea::string, ConfigVariableDefinition>;

    explicit ConfigFile(Context* context);

    /// Define variable supported by the config.
    ConfigVariableDefinition& DefineVariable(const ea::string& name, const Variant& defaultValue = Variant::EMPTY);
    /// Define new variables or update defaults for existing ones.
    void DefineVariables(const StringVariantMap& defaults);
    /// Update values for priority variables.
    void UpdatePriorityVariables(const StringVariantMap& defaults);
    /// Set variable value.
    void SetVariable(const ea::string& name, const Variant& value);
    /// Return whether variable is explicitly defined.
    bool HasVariable(const ea::string& name) const;
    /// Return variable definition (if present).
    const ConfigVariableDefinition* GetVariableDefinition(const ea::string& name) const;
    /// Return variable value or default.
    const Variant& GetVariable(const ea::string& name) const;

    /// Serialize persistent variable configuration. Current values are not serialized.
    void SerializeInBlock(Archive& archive) override;

    /// Load default variables from file.
    bool LoadDefaults(const ea::string& fileName, const ApplicationFlavor& flavor);
    /// Load variable overrides from file.
    bool LoadOverrides(const ea::string& fileName);
    /// Save variable overrides to file.
    bool SaveOverrides(const ea::string& fileName, const ApplicationFlavor& flavor) const;

    /// Evaluate variables which are explicitly configured for specifed flavor.
    /// Current values and defaults are ignored.
    StringVariantMap GetFlavorVariables(const ApplicationFlavor& flavor) const;
    /// Evaluate variables that are changed compared to flavor and global defaults.
    StringVariantMap GetChangedVariables(const ApplicationFlavor& flavor) const;

    /// Return internal data.
    /// @{
    ConfigFlavorVector& GetVariablesPerFlavor() { return variablesPerFlavor_; }
    const ConfigFlavorVector& GetVariablesPerFlavor() const { return variablesPerFlavor_; }
    ConfigVariableDefinitionMap& GetVariableDefinitions() { return definitions_; }
    const ConfigVariableDefinitionMap& GetVariableDefinitions() const { return definitions_; }
    StringVariantMap& GetVariables() { return variables_; }
    const StringVariantMap& GetVariables() const { return variables_; }
    /// @}

private:
    /// Configuration variables per flavor, not used directly.
    ConfigFlavorVector variablesPerFlavor_;
    /// Definitions of supported variables with metadata and default values.
    ConfigVariableDefinitionMap definitions_;

    /// Current state of variables.
    StringVariantMap variables_;
};


}
