using System.Text.Json.Serialization;

namespace ActionGenerator.Model
{
    public class Action
    {
        [JsonPropertyName("parent")]
        public string Parent { get; set; }

        [JsonPropertyName("comment")]
        public string Comment { get; set; }

        [JsonPropertyName("parameters")]
        public IList<Parameter> Parameters { get; set; } = new List<Parameter>();

        [JsonIgnore]
        public IList<Parameter> ThisParameters { get; set; } = new List<Parameter>();

        [JsonPropertyName("customReverse")]
        public bool CustomReverse { get; set; }
    }
}
