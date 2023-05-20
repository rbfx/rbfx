using System.Text.Json.Serialization;

namespace ActionGenerator.Model
{
    public class Parameter
    {
        [JsonPropertyName("name")]
        public string Name { get; set; }

        [JsonPropertyName("type")]
        public ParameterType Type { get; set; }

        [JsonPropertyName("default")] public string DefaultValue { get; set; } = "";

        [JsonPropertyName("inherited")]
        public bool Inherited { get; set; }
    }
}
