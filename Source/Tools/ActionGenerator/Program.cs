using System.Collections;
using System.CommandLine;
using System.Text.Json;
using System.Text.Json.Serialization;
using ActionGenerator.Model;
using ActionGenerator.Templates;
using Action = ActionGenerator.Model.Action;

namespace ActionGenerator
{
    public static class Program
    {
        public static async Task Main(string[] args)
        {
            var definitionJson = new Option<FileInfo>(new[] { "-d", "--definition" },
                "Path to Actions.json file including file name");
            var cppOutputFolder = new Option<DirectoryInfo>(new[] { "-o", "--output" },
                "Path to output folder where source code will be generated");
            var rootCommand = new RootCommand("Generate actions source code")
            {
                definitionJson, cppOutputFolder
            };

            rootCommand.SetHandler((definitionJsonValue, cppOutputFolderValue) =>
            {
                Generate(definitionJsonValue, cppOutputFolderValue);
            }, definitionJson, cppOutputFolder);

            await rootCommand.InvokeAsync(args);
        }

        private static void Generate(FileInfo definitionJson, DirectoryInfo cppOutputFolder)
        {
            if (!cppOutputFolder.Exists) cppOutputFolder.Create();

            var json = File.ReadAllText(definitionJson.FullName);
            var options = new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true,
            };
            var converter = new JsonStringEnumConverter(null, allowIntegerValues: true);
            options.Converters.Add(converter);
            var definition = JsonSerializer.Deserialize<Definition>(json, options);

            MarkInheritedParams(definition);
            File.WriteAllText(Path.Combine(cppOutputFolder.FullName, "Actions.cpp"), new CppTemplate(definition).TransformText());
            File.WriteAllText(Path.Combine(cppOutputFolder.FullName, "Actions.h"), new HeaderTemplate(definition).TransformText());
        }

        private static void MarkInheritedParams(Definition definition)
        {
            foreach (var definitionAction in definition.Actions)
            {
                var visited = new Dictionary<string, int>();
                var filteredParams = new List<Parameter>();
                foreach (var param in GetParams(definitionAction.Value, definition))
                {
                    if (visited.TryGetValue(param.Name, out var p))
                    {
                        filteredParams[p] = new Parameter()
                        {
                            Name = filteredParams[p].Name,
                            DefaultValue = param.DefaultValue,
                            Type = filteredParams[p].Type,
                            Inherited = true
                        };
                    }
                    else
                    {
                        visited.Add(param.Name, filteredParams.Count);
                        filteredParams.Add(param);
                    }
                }

                definitionAction.Value.Parameters = filteredParams;
                definitionAction.Value.ThisParameters = filteredParams.Where(_ => !_.Inherited).ToArray();
            }
        }

        private static IEnumerable<Parameter> GetParams(Action action, Definition definitions)
        {
            if (action.Parent == "AttributeAction")
            {
                yield return new Parameter()
                {
                    Name = "duration",
                    Type = ParameterType.Float,
                    Inherited = true,
                };
                yield return new Parameter()
                {
                    Name = "attribute name",
                    Type = ParameterType.String,
                    Inherited = true,
                };
            }
            else if (action.Parent == "AttributeActionInstant")
            {
                yield return new Parameter()
                {
                    Name = "attribute name",
                    Type = ParameterType.String,
                    Inherited = true,
                };
            }
            else if (action.Parent == "ActionInstant")
            {
            }
            else if (action.Parent == "DynamicAction")
            {
            }
            else if (action.Parent == "FiniteTimeAction")
            {
                yield return new Parameter()
                {
                    Name = "duration",
                    Type = ParameterType.Float,
                    Inherited = true,
                };
            }
            else
            {
                var parent = definitions.Actions[action.Parent];
                foreach (var parameter in GetParams(parent, definitions))
                {
                    yield return new Parameter()
                    {
                        Name = parameter.Name,
                        Type = parameter.Type,
                        DefaultValue = parameter.DefaultValue,
                        Inherited = true
                    };
                }
            }

            foreach (var parameter in action.Parameters)
            {
                yield return parameter;
            }
        }
    }
}
