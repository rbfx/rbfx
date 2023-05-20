using ActionGenerator.Model;

namespace ActionGenerator.Templates
{
    public partial class CppTemplate
    {
        protected readonly Definition _definition;

        public CppTemplate(Definition definition)
        {
            _definition = definition;
        }
    }

    public partial class HeaderTemplate
    {
        protected readonly Definition _definition;

        public HeaderTemplate(Definition definition)
        {
            _definition = definition;
        }
    }
}
