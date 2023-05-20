using System.Text.Json.Serialization;

namespace ActionGenerator.Model
{
    public class Definition
    {
        [JsonPropertyName("actions")]
        public Dictionary<string, Action> Actions { get; set; } = new Dictionary<string, Action>();
    }
}
