using System.Text;
using ActionGenerator.Model;

namespace ActionGenerator;

public class Util
{
    public static string Camel(string str)
    {
        var sb = new StringBuilder();
        bool upper = true;
        foreach (char c in str)
        {
            if (char.IsWhiteSpace(c))
            {
                upper = true;
            }
            else if (upper)
            {
                sb.Append(char.ToUpper(c));
                upper = false;
            }
            else
            {
                sb.Append(c);
            }
        }
        return sb.ToString();
    }
    public static string ArgName(string str)
    {
        var sb = new StringBuilder();
        bool upper = false;
        foreach (char c in str)
        {
            if (char.IsWhiteSpace(c))
            {
                upper = true;
            }
            else if (upper)
            {
                sb.Append(char.ToUpper(c));
                upper = false;
            }
            else
            {
                sb.Append(c);
            }
        }

        return sb.ToString();
    }

    public static string CppType(ParameterType type)
    {
        switch (type)
        {
            case ParameterType.Unsigned:
                return "unsigned";
            case ParameterType.Float:
                return "float";
            case ParameterType.Vector2:
                return "Vector2";
            case ParameterType.Vector3:
                return "Vector3";
            case ParameterType.Quaternion:
                return "Quaternion";
            case ParameterType.String:
                return "ea::string";
            case ParameterType.Variant:
                return "Variant";
            case ParameterType.FiniteTimeAction:
                return "SharedPtr<FiniteTimeAction>";
            default:
                throw new ArgumentOutOfRangeException(nameof(type), type, null);
        }
    }

    public static string FromVariant(ParameterType type)
    {
        if (type == ParameterType.Variant)
            return "";
        return ".Get<" + CppType(type) + ">()";
    }

    public static string CppTypeRef(ParameterType type)
    {
        var name = CppType(type);

        switch (type)
        {
            case ParameterType.Unsigned:
            case ParameterType.Float:
                return name;
            case ParameterType.FiniteTimeAction:
                return "FiniteTimeAction*";
        }
        return "const "+name+"&";
    }

    public static string GetValue(string value, ParameterType type)
    {
        switch (type)
        {
            case ParameterType.String:
                return "\"" + value + "\"";
        }
        return value;
    }
}
