
%rename(Equals) operator==;
%rename(Set) operator=;
%rename(At) operator[];
%ignore operator!=;


%define AddEqualityOperators(fqn)
  %extend fqn {
    %proxycode %{
        public static bool operator ==($typemap(cstype, fqn) a, $typemap(cstype, fqn) b)
        {
            if (!ReferenceEquals(a, null))
                return !ReferenceEquals(b, null) && a.Equals(b);     // Equality checked when both not null.
            return b is null;                                        // true when both are null
        }

        public static bool operator !=($typemap(cstype, fqn) a, $typemap(cstype, fqn) b)
        {
            return !(a == b);
        }

        public override bool Equals(object other)
        {
            return Equals(other as $typemap(cstype, fqn));
        }
    %}
  }
%enddef
