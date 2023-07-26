%constant Urho3D::IntVector2 MousePositionOffscreen = Urho3D::MOUSE_POSITION_OFFSCREEN;
%ignore Urho3D::MOUSE_POSITION_OFFSCREEN;
%csconstvalue("1") Urho3D::MOUSEB_LEFT;
%csconstvalue("2") Urho3D::MOUSEB_MIDDLE;
%csconstvalue("4") Urho3D::MOUSEB_RIGHT;
%csconstvalue("8") Urho3D::MOUSEB_X1;
%csconstvalue("16") Urho3D::MOUSEB_X2;
%csconstvalue("4294967295") Urho3D::MOUSEB_ANY;
%typemap(csattributes) Urho3D::MouseButton "[global::System.Flags]";
using MouseButtonFlags = Urho3D::MouseButton;
%typemap(ctype) MouseButtonFlags "size_t";
%typemap(out) MouseButtonFlags "$result = (size_t)$1.AsInteger();"
%csconstvalue("0") Urho3D::QUAL_NONE;
%csconstvalue("1") Urho3D::QUAL_SHIFT;
%csconstvalue("2") Urho3D::QUAL_CTRL;
%csconstvalue("4") Urho3D::QUAL_ALT;
%csconstvalue("8") Urho3D::QUAL_ANY;
%typemap(csattributes) Urho3D::Qualifier "[global::System.Flags]";
using QualifierFlags = Urho3D::Qualifier;
%typemap(ctype) QualifierFlags "size_t";
%typemap(out) QualifierFlags "$result = (size_t)$1.AsInteger();"
%csconstvalue("0") Urho3D::KEY_UNKNOWN;
%csconstvalue("97") Urho3D::KEY_A;
%csconstvalue("98") Urho3D::KEY_B;
%csconstvalue("99") Urho3D::KEY_C;
%csconstvalue("100") Urho3D::KEY_D;
%csconstvalue("101") Urho3D::KEY_E;
%csconstvalue("102") Urho3D::KEY_F;
%csconstvalue("103") Urho3D::KEY_G;
%csconstvalue("104") Urho3D::KEY_H;
%csconstvalue("105") Urho3D::KEY_I;
%csconstvalue("106") Urho3D::KEY_J;
%csconstvalue("107") Urho3D::KEY_K;
%csconstvalue("108") Urho3D::KEY_L;
%csconstvalue("109") Urho3D::KEY_M;
%csconstvalue("110") Urho3D::KEY_N;
%csconstvalue("111") Urho3D::KEY_O;
%csconstvalue("112") Urho3D::KEY_P;
%csconstvalue("113") Urho3D::KEY_Q;
%csconstvalue("114") Urho3D::KEY_R;
%csconstvalue("115") Urho3D::KEY_S;
%csconstvalue("116") Urho3D::KEY_T;
%csconstvalue("117") Urho3D::KEY_U;
%csconstvalue("118") Urho3D::KEY_V;
%csconstvalue("119") Urho3D::KEY_W;
%csconstvalue("120") Urho3D::KEY_X;
%csconstvalue("121") Urho3D::KEY_Y;
%csconstvalue("122") Urho3D::KEY_Z;
%csconstvalue("48") Urho3D::KEY_0;
%csconstvalue("49") Urho3D::KEY_1;
%csconstvalue("50") Urho3D::KEY_2;
%csconstvalue("51") Urho3D::KEY_3;
%csconstvalue("52") Urho3D::KEY_4;
%csconstvalue("53") Urho3D::KEY_5;
%csconstvalue("54") Urho3D::KEY_6;
%csconstvalue("55") Urho3D::KEY_7;
%csconstvalue("56") Urho3D::KEY_8;
%csconstvalue("57") Urho3D::KEY_9;
%csconstvalue("8") Urho3D::KEY_BACKSPACE;
%csconstvalue("9") Urho3D::KEY_TAB;
%csconstvalue("13") Urho3D::KEY_RETURN;
%csconstvalue("1073741982") Urho3D::KEY_RETURN2;
%csconstvalue("1073741912") Urho3D::KEY_KP_ENTER;
%csconstvalue("1073742049") Urho3D::KEY_SHIFT;
%csconstvalue("1073742048") Urho3D::KEY_CTRL;
%csconstvalue("1073742050") Urho3D::KEY_ALT;
%csconstvalue("1073742051") Urho3D::KEY_GUI;
%csconstvalue("1073741896") Urho3D::KEY_PAUSE;
%csconstvalue("1073741881") Urho3D::KEY_CAPSLOCK;
%csconstvalue("27") Urho3D::KEY_ESCAPE;
%csconstvalue("32") Urho3D::KEY_SPACE;
%csconstvalue("1073741899") Urho3D::KEY_PAGEUP;
%csconstvalue("1073741902") Urho3D::KEY_PAGEDOWN;
%csconstvalue("1073741901") Urho3D::KEY_END;
%csconstvalue("1073741898") Urho3D::KEY_HOME;
%csconstvalue("1073741904") Urho3D::KEY_LEFT;
%csconstvalue("1073741906") Urho3D::KEY_UP;
%csconstvalue("1073741903") Urho3D::KEY_RIGHT;
%csconstvalue("1073741905") Urho3D::KEY_DOWN;
%csconstvalue("1073741943") Urho3D::KEY_SELECT;
%csconstvalue("1073741894") Urho3D::KEY_PRINTSCREEN;
%csconstvalue("1073741897") Urho3D::KEY_INSERT;
%csconstvalue("127") Urho3D::KEY_DELETE;
%csconstvalue("1073742051") Urho3D::KEY_LGUI;
%csconstvalue("1073742055") Urho3D::KEY_RGUI;
%csconstvalue("1073741925") Urho3D::KEY_APPLICATION;
%csconstvalue("1073741922") Urho3D::KEY_KP_0;
%csconstvalue("1073741913") Urho3D::KEY_KP_1;
%csconstvalue("1073741914") Urho3D::KEY_KP_2;
%csconstvalue("1073741915") Urho3D::KEY_KP_3;
%csconstvalue("1073741916") Urho3D::KEY_KP_4;
%csconstvalue("1073741917") Urho3D::KEY_KP_5;
%csconstvalue("1073741918") Urho3D::KEY_KP_6;
%csconstvalue("1073741919") Urho3D::KEY_KP_7;
%csconstvalue("1073741920") Urho3D::KEY_KP_8;
%csconstvalue("1073741921") Urho3D::KEY_KP_9;
%csconstvalue("1073741909") Urho3D::KEY_KP_MULTIPLY;
%csconstvalue("1073741911") Urho3D::KEY_KP_PLUS;
%csconstvalue("1073741910") Urho3D::KEY_KP_MINUS;
%csconstvalue("1073741923") Urho3D::KEY_KP_PERIOD;
%csconstvalue("1073741908") Urho3D::KEY_KP_DIVIDE;
%csconstvalue("1073741882") Urho3D::KEY_F1;
%csconstvalue("1073741883") Urho3D::KEY_F2;
%csconstvalue("1073741884") Urho3D::KEY_F3;
%csconstvalue("1073741885") Urho3D::KEY_F4;
%csconstvalue("1073741886") Urho3D::KEY_F5;
%csconstvalue("1073741887") Urho3D::KEY_F6;
%csconstvalue("1073741888") Urho3D::KEY_F7;
%csconstvalue("1073741889") Urho3D::KEY_F8;
%csconstvalue("1073741890") Urho3D::KEY_F9;
%csconstvalue("1073741891") Urho3D::KEY_F10;
%csconstvalue("1073741892") Urho3D::KEY_F11;
%csconstvalue("1073741893") Urho3D::KEY_F12;
%csconstvalue("1073741928") Urho3D::KEY_F13;
%csconstvalue("1073741929") Urho3D::KEY_F14;
%csconstvalue("1073741930") Urho3D::KEY_F15;
%csconstvalue("1073741931") Urho3D::KEY_F16;
%csconstvalue("1073741932") Urho3D::KEY_F17;
%csconstvalue("1073741933") Urho3D::KEY_F18;
%csconstvalue("1073741934") Urho3D::KEY_F19;
%csconstvalue("1073741935") Urho3D::KEY_F20;
%csconstvalue("1073741936") Urho3D::KEY_F21;
%csconstvalue("1073741937") Urho3D::KEY_F22;
%csconstvalue("1073741938") Urho3D::KEY_F23;
%csconstvalue("1073741939") Urho3D::KEY_F24;
%csconstvalue("1073741907") Urho3D::KEY_NUMLOCKCLEAR;
%csconstvalue("1073741895") Urho3D::KEY_SCROLLLOCK;
%csconstvalue("1073742049") Urho3D::KEY_LSHIFT;
%csconstvalue("1073742053") Urho3D::KEY_RSHIFT;
%csconstvalue("1073742048") Urho3D::KEY_LCTRL;
%csconstvalue("1073742052") Urho3D::KEY_RCTRL;
%csconstvalue("1073742050") Urho3D::KEY_LALT;
%csconstvalue("1073742054") Urho3D::KEY_RALT;
%csconstvalue("1073742094") Urho3D::KEY_AC_BACK;
%csconstvalue("1073742098") Urho3D::KEY_AC_BOOKMARKS;
%csconstvalue("1073742095") Urho3D::KEY_AC_FORWARD;
%csconstvalue("1073742093") Urho3D::KEY_AC_HOME;
%csconstvalue("1073742097") Urho3D::KEY_AC_REFRESH;
%csconstvalue("1073742092") Urho3D::KEY_AC_SEARCH;
%csconstvalue("1073742096") Urho3D::KEY_AC_STOP;
%csconstvalue("1073741945") Urho3D::KEY_AGAIN;
%csconstvalue("1073741977") Urho3D::KEY_ALTERASE;
%csconstvalue("38") Urho3D::KEY_AMPERSAND;
%csconstvalue("42") Urho3D::KEY_ASTERISK;
%csconstvalue("64") Urho3D::KEY_AT;
%csconstvalue("1073742086") Urho3D::KEY_AUDIOMUTE;
%csconstvalue("1073742082") Urho3D::KEY_AUDIONEXT;
%csconstvalue("1073742085") Urho3D::KEY_AUDIOPLAY;
%csconstvalue("1073742083") Urho3D::KEY_AUDIOPREV;
%csconstvalue("1073742084") Urho3D::KEY_AUDIOSTOP;
%csconstvalue("96") Urho3D::KEY_BACKQUOTE;
%csconstvalue("92") Urho3D::KEY_BACKSLASH;
%csconstvalue("1073742099") Urho3D::KEY_BRIGHTNESSDOWN;
%csconstvalue("1073742100") Urho3D::KEY_BRIGHTNESSUP;
%csconstvalue("1073742090") Urho3D::KEY_CALCULATOR;
%csconstvalue("1073741979") Urho3D::KEY_CANCEL;
%csconstvalue("94") Urho3D::KEY_CARET;
%csconstvalue("1073741980") Urho3D::KEY_CLEAR;
%csconstvalue("1073741986") Urho3D::KEY_CLEARAGAIN;
%csconstvalue("58") Urho3D::KEY_COLON;
%csconstvalue("44") Urho3D::KEY_COMMA;
%csconstvalue("1073742091") Urho3D::KEY_COMPUTER;
%csconstvalue("1073741948") Urho3D::KEY_COPY;
%csconstvalue("1073741987") Urho3D::KEY_CRSEL;
%csconstvalue("1073742005") Urho3D::KEY_CURRENCYSUBUNIT;
%csconstvalue("1073742004") Urho3D::KEY_CURRENCYUNIT;
%csconstvalue("1073741947") Urho3D::KEY_CUT;
%csconstvalue("1073742003") Urho3D::KEY_DECIMALSEPARATOR;
%csconstvalue("1073742101") Urho3D::KEY_DISPLAYSWITCH;
%csconstvalue("36") Urho3D::KEY_DOLLAR;
%csconstvalue("1073742105") Urho3D::KEY_EJECT;
%csconstvalue("61") Urho3D::KEY_EQUALS;
%csconstvalue("33") Urho3D::KEY_EXCLAIM;
%csconstvalue("1073741988") Urho3D::KEY_EXSEL;
%csconstvalue("1073741950") Urho3D::KEY_FIND;
%csconstvalue("62") Urho3D::KEY_GREATER;
%csconstvalue("35") Urho3D::KEY_HASH;
%csconstvalue("1073741941") Urho3D::KEY_HELP;
%csconstvalue("1073742103") Urho3D::KEY_KBDILLUMDOWN;
%csconstvalue("1073742102") Urho3D::KEY_KBDILLUMTOGGLE;
%csconstvalue("1073742104") Urho3D::KEY_KBDILLUMUP;
%csconstvalue("1073742000") Urho3D::KEY_KP_00;
%csconstvalue("1073742001") Urho3D::KEY_KP_000;
%csconstvalue("1073742012") Urho3D::KEY_KP_A;
%csconstvalue("1073742023") Urho3D::KEY_KP_AMPERSAND;
%csconstvalue("1073742030") Urho3D::KEY_KP_AT;
%csconstvalue("1073742013") Urho3D::KEY_KP_B;
%csconstvalue("1073742011") Urho3D::KEY_KP_BACKSPACE;
%csconstvalue("1073742042") Urho3D::KEY_KP_BINARY;
%csconstvalue("1073742014") Urho3D::KEY_KP_C;
%csconstvalue("1073742040") Urho3D::KEY_KP_CLEAR;
%csconstvalue("1073742041") Urho3D::KEY_KP_CLEARENTRY;
%csconstvalue("1073742027") Urho3D::KEY_KP_COLON;
%csconstvalue("1073741957") Urho3D::KEY_KP_COMMA;
%csconstvalue("1073742015") Urho3D::KEY_KP_D;
%csconstvalue("1073742024") Urho3D::KEY_KP_DBLAMPERSAND;
%csconstvalue("1073742026") Urho3D::KEY_KP_DBLVERTICALBAR;
%csconstvalue("1073742044") Urho3D::KEY_KP_DECIMAL;
%csconstvalue("1073742016") Urho3D::KEY_KP_E;
%csconstvalue("1073741927") Urho3D::KEY_KP_EQUALS;
%csconstvalue("1073741958") Urho3D::KEY_KP_EQUALSAS400;
%csconstvalue("1073742031") Urho3D::KEY_KP_EXCLAM;
%csconstvalue("1073742017") Urho3D::KEY_KP_F;
%csconstvalue("1073742022") Urho3D::KEY_KP_GREATER;
%csconstvalue("1073742028") Urho3D::KEY_KP_HASH;
%csconstvalue("1073742045") Urho3D::KEY_KP_HEXADECIMAL;
%csconstvalue("1073742008") Urho3D::KEY_KP_LEFTBRACE;
%csconstvalue("1073742006") Urho3D::KEY_KP_LEFTPAREN;
%csconstvalue("1073742021") Urho3D::KEY_KP_LESS;
%csconstvalue("1073742035") Urho3D::KEY_KP_MEMADD;
%csconstvalue("1073742034") Urho3D::KEY_KP_MEMCLEAR;
%csconstvalue("1073742038") Urho3D::KEY_KP_MEMDIVIDE;
%csconstvalue("1073742037") Urho3D::KEY_KP_MEMMULTIPLY;
%csconstvalue("1073742033") Urho3D::KEY_KP_MEMRECALL;
%csconstvalue("1073742032") Urho3D::KEY_KP_MEMSTORE;
%csconstvalue("1073742036") Urho3D::KEY_KP_MEMSUBTRACT;
%csconstvalue("1073742043") Urho3D::KEY_KP_OCTAL;
%csconstvalue("1073742020") Urho3D::KEY_KP_PERCENT;
%csconstvalue("1073742039") Urho3D::KEY_KP_PLUSMINUS;
%csconstvalue("1073742019") Urho3D::KEY_KP_POWER;
%csconstvalue("1073742009") Urho3D::KEY_KP_RIGHTBRACE;
%csconstvalue("1073742007") Urho3D::KEY_KP_RIGHTPAREN;
%csconstvalue("1073742029") Urho3D::KEY_KP_SPACE;
%csconstvalue("1073742010") Urho3D::KEY_KP_TAB;
%csconstvalue("1073742025") Urho3D::KEY_KP_VERTICALBAR;
%csconstvalue("1073742018") Urho3D::KEY_KP_XOR;
%csconstvalue("91") Urho3D::KEY_LEFTBRACKET;
%csconstvalue("40") Urho3D::KEY_LEFTPAREN;
%csconstvalue("60") Urho3D::KEY_LESS;
%csconstvalue("1073742089") Urho3D::KEY_MAIL;
%csconstvalue("1073742087") Urho3D::KEY_MEDIASELECT;
%csconstvalue("1073741942") Urho3D::KEY_MENU;
%csconstvalue("45") Urho3D::KEY_MINUS;
%csconstvalue("1073742081") Urho3D::KEY_MODE;
%csconstvalue("1073741951") Urho3D::KEY_MUTE;
%csconstvalue("1073741985") Urho3D::KEY_OPER;
%csconstvalue("1073741984") Urho3D::KEY_OUT;
%csconstvalue("1073741949") Urho3D::KEY_PASTE;
%csconstvalue("37") Urho3D::KEY_PERCENT;
%csconstvalue("46") Urho3D::KEY_PERIOD;
%csconstvalue("43") Urho3D::KEY_PLUS;
%csconstvalue("1073741926") Urho3D::KEY_POWER;
%csconstvalue("1073741981") Urho3D::KEY_PRIOR;
%csconstvalue("63") Urho3D::KEY_QUESTION;
%csconstvalue("39") Urho3D::KEY_QUOTE;
%csconstvalue("34") Urho3D::KEY_QUOTEDBL;
%csconstvalue("93") Urho3D::KEY_RIGHTBRACKET;
%csconstvalue("41") Urho3D::KEY_RIGHTPAREN;
%csconstvalue("59") Urho3D::KEY_SEMICOLON;
%csconstvalue("1073741983") Urho3D::KEY_SEPARATOR;
%csconstvalue("47") Urho3D::KEY_SLASH;
%csconstvalue("1073742106") Urho3D::KEY_SLEEP;
%csconstvalue("1073741944") Urho3D::KEY_STOP;
%csconstvalue("1073741978") Urho3D::KEY_SYSREQ;
%csconstvalue("1073742002") Urho3D::KEY_THOUSANDSSEPARATOR;
%csconstvalue("95") Urho3D::KEY_UNDERSCORE;
%csconstvalue("1073741946") Urho3D::KEY_UNDO;
%csconstvalue("1073741953") Urho3D::KEY_VOLUMEDOWN;
%csconstvalue("1073741952") Urho3D::KEY_VOLUMEUP;
%csconstvalue("1073742088") Urho3D::KEY_WWW;
%csconstvalue("0") Urho3D::SCANCODE_UNKNOWN;
%csconstvalue("224") Urho3D::SCANCODE_CTRL;
%csconstvalue("225") Urho3D::SCANCODE_SHIFT;
%csconstvalue("226") Urho3D::SCANCODE_ALT;
%csconstvalue("227") Urho3D::SCANCODE_GUI;
%csconstvalue("4") Urho3D::SCANCODE_A;
%csconstvalue("5") Urho3D::SCANCODE_B;
%csconstvalue("6") Urho3D::SCANCODE_C;
%csconstvalue("7") Urho3D::SCANCODE_D;
%csconstvalue("8") Urho3D::SCANCODE_E;
%csconstvalue("9") Urho3D::SCANCODE_F;
%csconstvalue("10") Urho3D::SCANCODE_G;
%csconstvalue("11") Urho3D::SCANCODE_H;
%csconstvalue("12") Urho3D::SCANCODE_I;
%csconstvalue("13") Urho3D::SCANCODE_J;
%csconstvalue("14") Urho3D::SCANCODE_K;
%csconstvalue("15") Urho3D::SCANCODE_L;
%csconstvalue("16") Urho3D::SCANCODE_M;
%csconstvalue("17") Urho3D::SCANCODE_N;
%csconstvalue("18") Urho3D::SCANCODE_O;
%csconstvalue("19") Urho3D::SCANCODE_P;
%csconstvalue("20") Urho3D::SCANCODE_Q;
%csconstvalue("21") Urho3D::SCANCODE_R;
%csconstvalue("22") Urho3D::SCANCODE_S;
%csconstvalue("23") Urho3D::SCANCODE_T;
%csconstvalue("24") Urho3D::SCANCODE_U;
%csconstvalue("25") Urho3D::SCANCODE_V;
%csconstvalue("26") Urho3D::SCANCODE_W;
%csconstvalue("27") Urho3D::SCANCODE_X;
%csconstvalue("28") Urho3D::SCANCODE_Y;
%csconstvalue("29") Urho3D::SCANCODE_Z;
%csconstvalue("30") Urho3D::SCANCODE_1;
%csconstvalue("31") Urho3D::SCANCODE_2;
%csconstvalue("32") Urho3D::SCANCODE_3;
%csconstvalue("33") Urho3D::SCANCODE_4;
%csconstvalue("34") Urho3D::SCANCODE_5;
%csconstvalue("35") Urho3D::SCANCODE_6;
%csconstvalue("36") Urho3D::SCANCODE_7;
%csconstvalue("37") Urho3D::SCANCODE_8;
%csconstvalue("38") Urho3D::SCANCODE_9;
%csconstvalue("39") Urho3D::SCANCODE_0;
%csconstvalue("40") Urho3D::SCANCODE_RETURN;
%csconstvalue("41") Urho3D::SCANCODE_ESCAPE;
%csconstvalue("42") Urho3D::SCANCODE_BACKSPACE;
%csconstvalue("43") Urho3D::SCANCODE_TAB;
%csconstvalue("44") Urho3D::SCANCODE_SPACE;
%csconstvalue("45") Urho3D::SCANCODE_MINUS;
%csconstvalue("46") Urho3D::SCANCODE_EQUALS;
%csconstvalue("47") Urho3D::SCANCODE_LEFTBRACKET;
%csconstvalue("48") Urho3D::SCANCODE_RIGHTBRACKET;
%csconstvalue("49") Urho3D::SCANCODE_BACKSLASH;
%csconstvalue("50") Urho3D::SCANCODE_NONUSHASH;
%csconstvalue("51") Urho3D::SCANCODE_SEMICOLON;
%csconstvalue("52") Urho3D::SCANCODE_APOSTROPHE;
%csconstvalue("53") Urho3D::SCANCODE_GRAVE;
%csconstvalue("54") Urho3D::SCANCODE_COMMA;
%csconstvalue("55") Urho3D::SCANCODE_PERIOD;
%csconstvalue("56") Urho3D::SCANCODE_SLASH;
%csconstvalue("57") Urho3D::SCANCODE_CAPSLOCK;
%csconstvalue("58") Urho3D::SCANCODE_F1;
%csconstvalue("59") Urho3D::SCANCODE_F2;
%csconstvalue("60") Urho3D::SCANCODE_F3;
%csconstvalue("61") Urho3D::SCANCODE_F4;
%csconstvalue("62") Urho3D::SCANCODE_F5;
%csconstvalue("63") Urho3D::SCANCODE_F6;
%csconstvalue("64") Urho3D::SCANCODE_F7;
%csconstvalue("65") Urho3D::SCANCODE_F8;
%csconstvalue("66") Urho3D::SCANCODE_F9;
%csconstvalue("67") Urho3D::SCANCODE_F10;
%csconstvalue("68") Urho3D::SCANCODE_F11;
%csconstvalue("69") Urho3D::SCANCODE_F12;
%csconstvalue("70") Urho3D::SCANCODE_PRINTSCREEN;
%csconstvalue("71") Urho3D::SCANCODE_SCROLLLOCK;
%csconstvalue("72") Urho3D::SCANCODE_PAUSE;
%csconstvalue("73") Urho3D::SCANCODE_INSERT;
%csconstvalue("74") Urho3D::SCANCODE_HOME;
%csconstvalue("75") Urho3D::SCANCODE_PAGEUP;
%csconstvalue("76") Urho3D::SCANCODE_DELETE;
%csconstvalue("77") Urho3D::SCANCODE_END;
%csconstvalue("78") Urho3D::SCANCODE_PAGEDOWN;
%csconstvalue("79") Urho3D::SCANCODE_RIGHT;
%csconstvalue("80") Urho3D::SCANCODE_LEFT;
%csconstvalue("81") Urho3D::SCANCODE_DOWN;
%csconstvalue("82") Urho3D::SCANCODE_UP;
%csconstvalue("83") Urho3D::SCANCODE_NUMLOCKCLEAR;
%csconstvalue("84") Urho3D::SCANCODE_KP_DIVIDE;
%csconstvalue("85") Urho3D::SCANCODE_KP_MULTIPLY;
%csconstvalue("86") Urho3D::SCANCODE_KP_MINUS;
%csconstvalue("87") Urho3D::SCANCODE_KP_PLUS;
%csconstvalue("88") Urho3D::SCANCODE_KP_ENTER;
%csconstvalue("89") Urho3D::SCANCODE_KP_1;
%csconstvalue("90") Urho3D::SCANCODE_KP_2;
%csconstvalue("91") Urho3D::SCANCODE_KP_3;
%csconstvalue("92") Urho3D::SCANCODE_KP_4;
%csconstvalue("93") Urho3D::SCANCODE_KP_5;
%csconstvalue("94") Urho3D::SCANCODE_KP_6;
%csconstvalue("95") Urho3D::SCANCODE_KP_7;
%csconstvalue("96") Urho3D::SCANCODE_KP_8;
%csconstvalue("97") Urho3D::SCANCODE_KP_9;
%csconstvalue("98") Urho3D::SCANCODE_KP_0;
%csconstvalue("99") Urho3D::SCANCODE_KP_PERIOD;
%csconstvalue("100") Urho3D::SCANCODE_NONUSBACKSLASH;
%csconstvalue("101") Urho3D::SCANCODE_APPLICATION;
%csconstvalue("102") Urho3D::SCANCODE_POWER;
%csconstvalue("103") Urho3D::SCANCODE_KP_EQUALS;
%csconstvalue("104") Urho3D::SCANCODE_F13;
%csconstvalue("105") Urho3D::SCANCODE_F14;
%csconstvalue("106") Urho3D::SCANCODE_F15;
%csconstvalue("107") Urho3D::SCANCODE_F16;
%csconstvalue("108") Urho3D::SCANCODE_F17;
%csconstvalue("109") Urho3D::SCANCODE_F18;
%csconstvalue("110") Urho3D::SCANCODE_F19;
%csconstvalue("111") Urho3D::SCANCODE_F20;
%csconstvalue("112") Urho3D::SCANCODE_F21;
%csconstvalue("113") Urho3D::SCANCODE_F22;
%csconstvalue("114") Urho3D::SCANCODE_F23;
%csconstvalue("115") Urho3D::SCANCODE_F24;
%csconstvalue("116") Urho3D::SCANCODE_EXECUTE;
%csconstvalue("117") Urho3D::SCANCODE_HELP;
%csconstvalue("118") Urho3D::SCANCODE_MENU;
%csconstvalue("119") Urho3D::SCANCODE_SELECT;
%csconstvalue("120") Urho3D::SCANCODE_STOP;
%csconstvalue("121") Urho3D::SCANCODE_AGAIN;
%csconstvalue("122") Urho3D::SCANCODE_UNDO;
%csconstvalue("123") Urho3D::SCANCODE_CUT;
%csconstvalue("124") Urho3D::SCANCODE_COPY;
%csconstvalue("125") Urho3D::SCANCODE_PASTE;
%csconstvalue("126") Urho3D::SCANCODE_FIND;
%csconstvalue("127") Urho3D::SCANCODE_MUTE;
%csconstvalue("128") Urho3D::SCANCODE_VOLUMEUP;
%csconstvalue("129") Urho3D::SCANCODE_VOLUMEDOWN;
%csconstvalue("133") Urho3D::SCANCODE_KP_COMMA;
%csconstvalue("134") Urho3D::SCANCODE_KP_EQUALSAS400;
%csconstvalue("135") Urho3D::SCANCODE_INTERNATIONAL1;
%csconstvalue("136") Urho3D::SCANCODE_INTERNATIONAL2;
%csconstvalue("137") Urho3D::SCANCODE_INTERNATIONAL3;
%csconstvalue("138") Urho3D::SCANCODE_INTERNATIONAL4;
%csconstvalue("139") Urho3D::SCANCODE_INTERNATIONAL5;
%csconstvalue("140") Urho3D::SCANCODE_INTERNATIONAL6;
%csconstvalue("141") Urho3D::SCANCODE_INTERNATIONAL7;
%csconstvalue("142") Urho3D::SCANCODE_INTERNATIONAL8;
%csconstvalue("143") Urho3D::SCANCODE_INTERNATIONAL9;
%csconstvalue("144") Urho3D::SCANCODE_LANG1;
%csconstvalue("145") Urho3D::SCANCODE_LANG2;
%csconstvalue("146") Urho3D::SCANCODE_LANG3;
%csconstvalue("147") Urho3D::SCANCODE_LANG4;
%csconstvalue("148") Urho3D::SCANCODE_LANG5;
%csconstvalue("149") Urho3D::SCANCODE_LANG6;
%csconstvalue("150") Urho3D::SCANCODE_LANG7;
%csconstvalue("151") Urho3D::SCANCODE_LANG8;
%csconstvalue("152") Urho3D::SCANCODE_LANG9;
%csconstvalue("153") Urho3D::SCANCODE_ALTERASE;
%csconstvalue("154") Urho3D::SCANCODE_SYSREQ;
%csconstvalue("155") Urho3D::SCANCODE_CANCEL;
%csconstvalue("156") Urho3D::SCANCODE_CLEAR;
%csconstvalue("157") Urho3D::SCANCODE_PRIOR;
%csconstvalue("158") Urho3D::SCANCODE_RETURN2;
%csconstvalue("159") Urho3D::SCANCODE_SEPARATOR;
%csconstvalue("160") Urho3D::SCANCODE_OUT;
%csconstvalue("161") Urho3D::SCANCODE_OPER;
%csconstvalue("162") Urho3D::SCANCODE_CLEARAGAIN;
%csconstvalue("163") Urho3D::SCANCODE_CRSEL;
%csconstvalue("164") Urho3D::SCANCODE_EXSEL;
%csconstvalue("176") Urho3D::SCANCODE_KP_00;
%csconstvalue("177") Urho3D::SCANCODE_KP_000;
%csconstvalue("178") Urho3D::SCANCODE_THOUSANDSSEPARATOR;
%csconstvalue("179") Urho3D::SCANCODE_DECIMALSEPARATOR;
%csconstvalue("180") Urho3D::SCANCODE_CURRENCYUNIT;
%csconstvalue("181") Urho3D::SCANCODE_CURRENCYSUBUNIT;
%csconstvalue("182") Urho3D::SCANCODE_KP_LEFTPAREN;
%csconstvalue("183") Urho3D::SCANCODE_KP_RIGHTPAREN;
%csconstvalue("184") Urho3D::SCANCODE_KP_LEFTBRACE;
%csconstvalue("185") Urho3D::SCANCODE_KP_RIGHTBRACE;
%csconstvalue("186") Urho3D::SCANCODE_KP_TAB;
%csconstvalue("187") Urho3D::SCANCODE_KP_BACKSPACE;
%csconstvalue("188") Urho3D::SCANCODE_KP_A;
%csconstvalue("189") Urho3D::SCANCODE_KP_B;
%csconstvalue("190") Urho3D::SCANCODE_KP_C;
%csconstvalue("191") Urho3D::SCANCODE_KP_D;
%csconstvalue("192") Urho3D::SCANCODE_KP_E;
%csconstvalue("193") Urho3D::SCANCODE_KP_F;
%csconstvalue("194") Urho3D::SCANCODE_KP_XOR;
%csconstvalue("195") Urho3D::SCANCODE_KP_POWER;
%csconstvalue("196") Urho3D::SCANCODE_KP_PERCENT;
%csconstvalue("197") Urho3D::SCANCODE_KP_LESS;
%csconstvalue("198") Urho3D::SCANCODE_KP_GREATER;
%csconstvalue("199") Urho3D::SCANCODE_KP_AMPERSAND;
%csconstvalue("200") Urho3D::SCANCODE_KP_DBLAMPERSAND;
%csconstvalue("201") Urho3D::SCANCODE_KP_VERTICALBAR;
%csconstvalue("202") Urho3D::SCANCODE_KP_DBLVERTICALBAR;
%csconstvalue("203") Urho3D::SCANCODE_KP_COLON;
%csconstvalue("204") Urho3D::SCANCODE_KP_HASH;
%csconstvalue("205") Urho3D::SCANCODE_KP_SPACE;
%csconstvalue("206") Urho3D::SCANCODE_KP_AT;
%csconstvalue("207") Urho3D::SCANCODE_KP_EXCLAM;
%csconstvalue("208") Urho3D::SCANCODE_KP_MEMSTORE;
%csconstvalue("209") Urho3D::SCANCODE_KP_MEMRECALL;
%csconstvalue("210") Urho3D::SCANCODE_KP_MEMCLEAR;
%csconstvalue("211") Urho3D::SCANCODE_KP_MEMADD;
%csconstvalue("212") Urho3D::SCANCODE_KP_MEMSUBTRACT;
%csconstvalue("213") Urho3D::SCANCODE_KP_MEMMULTIPLY;
%csconstvalue("214") Urho3D::SCANCODE_KP_MEMDIVIDE;
%csconstvalue("215") Urho3D::SCANCODE_KP_PLUSMINUS;
%csconstvalue("216") Urho3D::SCANCODE_KP_CLEAR;
%csconstvalue("217") Urho3D::SCANCODE_KP_CLEARENTRY;
%csconstvalue("218") Urho3D::SCANCODE_KP_BINARY;
%csconstvalue("219") Urho3D::SCANCODE_KP_OCTAL;
%csconstvalue("220") Urho3D::SCANCODE_KP_DECIMAL;
%csconstvalue("221") Urho3D::SCANCODE_KP_HEXADECIMAL;
%csconstvalue("224") Urho3D::SCANCODE_LCTRL;
%csconstvalue("225") Urho3D::SCANCODE_LSHIFT;
%csconstvalue("226") Urho3D::SCANCODE_LALT;
%csconstvalue("227") Urho3D::SCANCODE_LGUI;
%csconstvalue("228") Urho3D::SCANCODE_RCTRL;
%csconstvalue("229") Urho3D::SCANCODE_RSHIFT;
%csconstvalue("230") Urho3D::SCANCODE_RALT;
%csconstvalue("231") Urho3D::SCANCODE_RGUI;
%csconstvalue("257") Urho3D::SCANCODE_MODE;
%csconstvalue("258") Urho3D::SCANCODE_AUDIONEXT;
%csconstvalue("259") Urho3D::SCANCODE_AUDIOPREV;
%csconstvalue("260") Urho3D::SCANCODE_AUDIOSTOP;
%csconstvalue("261") Urho3D::SCANCODE_AUDIOPLAY;
%csconstvalue("262") Urho3D::SCANCODE_AUDIOMUTE;
%csconstvalue("263") Urho3D::SCANCODE_MEDIASELECT;
%csconstvalue("264") Urho3D::SCANCODE_WWW;
%csconstvalue("265") Urho3D::SCANCODE_MAIL;
%csconstvalue("266") Urho3D::SCANCODE_CALCULATOR;
%csconstvalue("267") Urho3D::SCANCODE_COMPUTER;
%csconstvalue("268") Urho3D::SCANCODE_AC_SEARCH;
%csconstvalue("269") Urho3D::SCANCODE_AC_HOME;
%csconstvalue("270") Urho3D::SCANCODE_AC_BACK;
%csconstvalue("271") Urho3D::SCANCODE_AC_FORWARD;
%csconstvalue("272") Urho3D::SCANCODE_AC_STOP;
%csconstvalue("273") Urho3D::SCANCODE_AC_REFRESH;
%csconstvalue("274") Urho3D::SCANCODE_AC_BOOKMARKS;
%csconstvalue("275") Urho3D::SCANCODE_BRIGHTNESSDOWN;
%csconstvalue("276") Urho3D::SCANCODE_BRIGHTNESSUP;
%csconstvalue("277") Urho3D::SCANCODE_DISPLAYSWITCH;
%csconstvalue("278") Urho3D::SCANCODE_KBDILLUMTOGGLE;
%csconstvalue("279") Urho3D::SCANCODE_KBDILLUMDOWN;
%csconstvalue("280") Urho3D::SCANCODE_KBDILLUMUP;
%csconstvalue("281") Urho3D::SCANCODE_EJECT;
%csconstvalue("282") Urho3D::SCANCODE_SLEEP;
%csconstvalue("283") Urho3D::SCANCODE_APP1;
%csconstvalue("284") Urho3D::SCANCODE_APP2;
%csconstvalue("0") Urho3D::HAT_CENTER;
%csconstvalue("1") Urho3D::HAT_UP;
%csconstvalue("2") Urho3D::HAT_RIGHT;
%csconstvalue("4") Urho3D::HAT_DOWN;
%csconstvalue("8") Urho3D::HAT_LEFT;
%csconstvalue("3") Urho3D::HAT_RIGHTUP;
%csconstvalue("6") Urho3D::HAT_RIGHTDOWN;
%csconstvalue("9") Urho3D::HAT_LEFTUP;
%csconstvalue("12") Urho3D::HAT_LEFTDOWN;
%csconstvalue("0") Urho3D::CONTROLLER_BUTTON_A;
%csconstvalue("1") Urho3D::CONTROLLER_BUTTON_B;
%csconstvalue("2") Urho3D::CONTROLLER_BUTTON_X;
%csconstvalue("3") Urho3D::CONTROLLER_BUTTON_Y;
%csconstvalue("4") Urho3D::CONTROLLER_BUTTON_BACK;
%csconstvalue("5") Urho3D::CONTROLLER_BUTTON_GUIDE;
%csconstvalue("6") Urho3D::CONTROLLER_BUTTON_START;
%csconstvalue("7") Urho3D::CONTROLLER_BUTTON_LEFTSTICK;
%csconstvalue("8") Urho3D::CONTROLLER_BUTTON_RIGHTSTICK;
%csconstvalue("9") Urho3D::CONTROLLER_BUTTON_LEFTSHOULDER;
%csconstvalue("10") Urho3D::CONTROLLER_BUTTON_RIGHTSHOULDER;
%csconstvalue("11") Urho3D::CONTROLLER_BUTTON_DPAD_UP;
%csconstvalue("12") Urho3D::CONTROLLER_BUTTON_DPAD_DOWN;
%csconstvalue("13") Urho3D::CONTROLLER_BUTTON_DPAD_LEFT;
%csconstvalue("14") Urho3D::CONTROLLER_BUTTON_DPAD_RIGHT;
%csconstvalue("0") Urho3D::CONTROLLER_AXIS_LEFTX;
%csconstvalue("1") Urho3D::CONTROLLER_AXIS_LEFTY;
%csconstvalue("2") Urho3D::CONTROLLER_AXIS_RIGHTX;
%csconstvalue("3") Urho3D::CONTROLLER_AXIS_RIGHTY;
%csconstvalue("4") Urho3D::CONTROLLER_AXIS_TRIGGERLEFT;
%csconstvalue("5") Urho3D::CONTROLLER_AXIS_TRIGGERRIGHT;
%csconstvalue("0") Urho3D::JOYSTICK_TYPE_UNKNOWN;
%csconstvalue("1") Urho3D::JOYSTICK_TYPE_GAMECONTROLLER;
%csconstvalue("2") Urho3D::JOYSTICK_TYPE_WHEEL;
%csconstvalue("3") Urho3D::JOYSTICK_TYPE_ARCADE_STICK;
%csconstvalue("4") Urho3D::JOYSTICK_TYPE_FLIGHT_STICK;
%csconstvalue("5") Urho3D::JOYSTICK_TYPE_DANCE_PAD;
%csconstvalue("6") Urho3D::JOYSTICK_TYPE_GUITAR;
%csconstvalue("7") Urho3D::JOYSTICK_TYPE_DRUM_KIT;
%csconstvalue("8") Urho3D::JOYSTICK_TYPE_ARCADE_PAD;
%csconstvalue("9") Urho3D::JOYSTICK_TYPE_THROTTLE;
%csconstvalue("0") Urho3D::MM_ABSOLUTE;
%csconstvalue("0") Urho3D::DirectionAggregatorMask::None;
%csconstvalue("1") Urho3D::DirectionAggregatorMask::Keyboard;
%csconstvalue("2") Urho3D::DirectionAggregatorMask::Joystick;
%csconstvalue("4") Urho3D::DirectionAggregatorMask::Touch;
%csconstvalue("7") Urho3D::DirectionAggregatorMask::All;
%typemap(csattributes) Urho3D::DirectionAggregatorMask "[global::System.Flags]";
using DirectionAggregatorFlags = Urho3D::DirectionAggregatorMask;
%typemap(ctype) DirectionAggregatorFlags "size_t";
%typemap(out) DirectionAggregatorFlags "$result = (size_t)$1.AsInteger();"
%csconstvalue("0") Urho3D::DirectionAggregator::InputType::External;
%csconstvalue("1") Urho3D::DirectionAggregator::InputType::Keyboard;
%csconstvalue("2") Urho3D::DirectionAggregator::InputType::Touch;
%csconstvalue("3") Urho3D::DirectionAggregator::InputType::JoystickAxis;
%csconstvalue("4") Urho3D::DirectionAggregator::InputType::JoystickDPad;
%csconstvalue("0") Urho3D::DirectionalPadAdapterMask::None;
%csconstvalue("1") Urho3D::DirectionalPadAdapterMask::Keyboard;
%csconstvalue("2") Urho3D::DirectionalPadAdapterMask::Joystick;
%csconstvalue("4") Urho3D::DirectionalPadAdapterMask::KeyRepeat;
%csconstvalue("7") Urho3D::DirectionalPadAdapterMask::All;
%typemap(csattributes) Urho3D::DirectionalPadAdapterMask "[global::System.Flags]";
using DirectionalPadAdapterFlags = Urho3D::DirectionalPadAdapterMask;
%typemap(ctype) DirectionalPadAdapterFlags "size_t";
%typemap(out) DirectionalPadAdapterFlags "$result = (size_t)$1.AsInteger();"
%csconstvalue("0") Urho3D::DirectionalPadAdapter::InputType::External;
%csconstvalue("1") Urho3D::DirectionalPadAdapter::InputType::Keyboard;
%csconstvalue("100") Urho3D::DirectionalPadAdapter::InputType::JoystickAxis;
%csconstvalue("200") Urho3D::DirectionalPadAdapter::InputType::JoystickDPad;
%csconstvalue("0") Urho3D::PointerAdapterMask::None;
%csconstvalue("1") Urho3D::PointerAdapterMask::Mouse;
%csconstvalue("2") Urho3D::PointerAdapterMask::Touch;
%csconstvalue("4") Urho3D::PointerAdapterMask::Keyboard;
%csconstvalue("8") Urho3D::PointerAdapterMask::Joystick;
%csconstvalue("15") Urho3D::PointerAdapterMask::All;
%typemap(csattributes) Urho3D::PointerAdapterMask "[global::System.Flags]";
using PointerAdapterFlags = Urho3D::PointerAdapterMask;
%typemap(ctype) PointerAdapterFlags "size_t";
%typemap(out) PointerAdapterFlags "$result = (size_t)$1.AsInteger();"
%csattribute(Urho3D::JoystickState, %arg(unsigned int), NumButtons, GetNumButtons);
%csattribute(Urho3D::JoystickState, %arg(unsigned int), NumAxes, GetNumAxes);
%csattribute(Urho3D::JoystickState, %arg(unsigned int), NumHats, GetNumHats);
%csattribute(Urho3D::Input, %arg(Urho3D::QualifierFlags), Qualifiers, GetQualifiers);
%csattribute(Urho3D::Input, %arg(Urho3D::IntVector2), MousePosition, GetMousePosition, SetMousePosition);
%csattribute(Urho3D::Input, %arg(Urho3D::Vector2), RelativeMousePosition, GetRelativeMousePosition);
%csattribute(Urho3D::Input, %arg(Urho3D::IntVector2), MouseMove, GetMouseMove);
%csattribute(Urho3D::Input, %arg(int), MouseMoveX, GetMouseMoveX);
%csattribute(Urho3D::Input, %arg(int), MouseMoveY, GetMouseMoveY);
%csattribute(Urho3D::Input, %arg(int), MouseMoveWheel, GetMouseMoveWheel);
%csattribute(Urho3D::Input, %arg(Urho3D::Vector2), SystemToBackbufferScale, GetSystemToBackbufferScale);
%csattribute(Urho3D::Input, %arg(unsigned int), NumTouches, GetNumTouches);
%csattribute(Urho3D::Input, %arg(unsigned int), NumJoysticks, GetNumJoysticks);
%csattribute(Urho3D::Input, %arg(bool), ToggleFullscreen, GetToggleFullscreen, SetToggleFullscreen);
%csattribute(Urho3D::Input, %arg(bool), ScreenKeyboardSupport, GetScreenKeyboardSupport);
%csattribute(Urho3D::Input, %arg(bool), IsScreenKeyboardVisible, IsScreenKeyboardVisible, SetScreenKeyboardVisible);
%csattribute(Urho3D::Input, %arg(bool), TouchEmulation, GetTouchEmulation, SetTouchEmulation);
%csattribute(Urho3D::Input, %arg(bool), IsMouseLocked, IsMouseLocked);
%csattribute(Urho3D::Input, %arg(bool), IsMinimized, IsMinimized);
%csattribute(Urho3D::Input, %arg(bool), Enabled, GetEnabled, SetEnabled);
%csattribute(Urho3D::Input, %arg(Urho3D::IntVector2), GlobalWindowPosition, GetGlobalWindowPosition);
%csattribute(Urho3D::Input, %arg(Urho3D::IntVector2), GlobalWindowSize, GetGlobalWindowSize);
%csattribute(Urho3D::Input, %arg(Urho3D::IntVector2), BackbufferSize, GetBackbufferSize);
%csattribute(Urho3D::AxisAdapter, %arg(float), DeadZone, GetDeadZone, SetDeadZone);
%csattribute(Urho3D::AxisAdapter, %arg(float), PositiveSensitivity, GetPositiveSensitivity, SetPositiveSensitivity);
%csattribute(Urho3D::AxisAdapter, %arg(float), NegativeSensitivity, GetNegativeSensitivity, SetNegativeSensitivity);
%csattribute(Urho3D::AxisAdapter, %arg(float), NeutralValue, GetNeutralValue, SetNeutralValue);
%csattribute(Urho3D::AxisAdapter, %arg(bool), IsInverted, IsInverted, SetInverted);
%csattribute(Urho3D::DirectionAggregator, %arg(bool), IsEnabled, IsEnabled, SetEnabled);
%csattribute(Urho3D::DirectionAggregator, %arg(Urho3D::DirectionAggregatorFlags), SubscriptionMask, GetSubscriptionMask, SetSubscriptionMask);
%csattribute(Urho3D::DirectionAggregator, %arg(Urho3D::UIElement *), UIElement, GetUIElement, SetUIElement);
%csattribute(Urho3D::DirectionAggregator, %arg(float), DeadZone, GetDeadZone, SetDeadZone);
%csattribute(Urho3D::DirectionAggregator, %arg(Urho3D::Vector2), Direction, GetDirection);
%csattribute(Urho3D::DirectionalPadAdapter, %arg(bool), IsEnabled, IsEnabled, SetEnabled);
%csattribute(Urho3D::DirectionalPadAdapter, %arg(Urho3D::DirectionalPadAdapterFlags), SubscriptionMask, GetSubscriptionMask, SetSubscriptionMask);
%csattribute(Urho3D::DirectionalPadAdapter, %arg(float), AxisUpperThreshold, GetAxisUpperThreshold, SetAxisUpperThreshold);
%csattribute(Urho3D::DirectionalPadAdapter, %arg(float), AxisLowerThreshold, GetAxisLowerThreshold, SetAxisLowerThreshold);
%csattribute(Urho3D::DirectionalPadAdapter, %arg(float), RepeatDelay, GetRepeatDelay, SetRepeatDelay);
%csattribute(Urho3D::DirectionalPadAdapter, %arg(float), RepeatInterval, GetRepeatInterval, SetRepeatInterval);
%csattribute(Urho3D::DirectionalPadAdapter::AggregatedState, %arg(bool), IsActive, IsActive);
%csattribute(Urho3D::MultitouchAdapter, %arg(bool), IsEnabled, IsEnabled, SetEnabled);
%csattribute(Urho3D::FreeFlyController, %arg(float), Speed, GetSpeed, SetSpeed);
%csattribute(Urho3D::FreeFlyController, %arg(float), AcceleratedSpeed, GetAcceleratedSpeed, SetAcceleratedSpeed);
%csattribute(Urho3D::FreeFlyController, %arg(float), MinPitch, GetMinPitch, SetMinPitch);
%csattribute(Urho3D::FreeFlyController, %arg(float), MaxPitch, GetMaxPitch, SetMaxPitch);
%csattribute(Urho3D::PointerAdapter, %arg(bool), IsEnabled, IsEnabled, SetEnabled);
%csattribute(Urho3D::PointerAdapter, %arg(Urho3D::PointerAdapterFlags), SubscriptionMask, GetSubscriptionMask, SetSubscriptionMask);
%csattribute(Urho3D::PointerAdapter, %arg(Urho3D::UIElement *), UIElement, GetUIElement, SetUIElement);
%csattribute(Urho3D::PointerAdapter, %arg(bool), IsButtonDown, IsButtonDown);
%csattribute(Urho3D::PointerAdapter, %arg(float), CursorVelocity, GetCursorVelocity);
%csattribute(Urho3D::PointerAdapter, %arg(float), CursorAcceleration, GetCursorAcceleration, SetCursorAcceleration);
%csattribute(Urho3D::PointerAdapter, %arg(Urho3D::IntVector2), PointerPosition, GetPointerPosition);
%csattribute(Urho3D::PointerAdapter, %arg(Urho3D::IntVector2), UIPointerPosition, GetUIPointerPosition);
%csattribute(Urho3D::PointerAdapter, %arg(Urho3D::DirectionAggregator *), DirectionAggregator, GetDirectionAggregator);
%csattribute(Urho3D::Detail::ControllerButtonMapping, %arg(Urho3D::ControllerButton), ControllerButton, GetControllerButton);
%csattribute(Urho3D::Detail::ControllerButtonMapping, %arg(unsigned int), GenericButton, GetGenericButton);
%csattribute(Urho3D::Detail::ControllerAxisMapping, %arg(Urho3D::ControllerAxis), ControllerAxis, GetControllerAxis);
%csattribute(Urho3D::Detail::ControllerAxisMapping, %arg(unsigned int), GenericAxis, GetGenericAxis);
%csattribute(Urho3D::InputMap, %arg(float), DeadZone, GetDeadZone, SetDeadZone);
%csattribute(Urho3D::InputMap, %arg(ea::unordered_map<ea::string, Detail::ActionMapping>), Mappings, GetMappings);
%csattribute(Urho3D::MoveAndOrbitComponent, %arg(Urho3D::Vector3), Velocity, GetVelocity);
%csattribute(Urho3D::MoveAndOrbitController, %arg(Urho3D::InputMap *), InputMap, GetInputMap, SetInputMap);
%csattribute(Urho3D::MoveAndOrbitController, %arg(Urho3D::ResourceRef), InputMapAttr, GetInputMapAttr, SetInputMapAttr);
%csattribute(Urho3D::MoveAndOrbitController, %arg(Urho3D::UIElement *), MovementUIElement, GetMovementUIElement, SetMovementUIElement);
%csattribute(Urho3D::MoveAndOrbitController, %arg(Urho3D::UIElement *), RotationUIElement, GetRotationUIElement, SetRotationUIElement);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class MouseButtonDownEvent {
        private StringHash _event = new StringHash("MouseButtonDown");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public StringHash Clicks = new StringHash("Clicks");
        public MouseButtonDownEvent() { }
        public static implicit operator StringHash(MouseButtonDownEvent e) { return e._event; }
    }
    public static MouseButtonDownEvent MouseButtonDown = new MouseButtonDownEvent();
    public class MouseButtonUpEvent {
        private StringHash _event = new StringHash("MouseButtonUp");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public MouseButtonUpEvent() { }
        public static implicit operator StringHash(MouseButtonUpEvent e) { return e._event; }
    }
    public static MouseButtonUpEvent MouseButtonUp = new MouseButtonUpEvent();
    public class MouseMoveEvent {
        private StringHash _event = new StringHash("MouseMove");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash DX = new StringHash("DX");
        public StringHash DY = new StringHash("DY");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public MouseMoveEvent() { }
        public static implicit operator StringHash(MouseMoveEvent e) { return e._event; }
    }
    public static MouseMoveEvent MouseMove = new MouseMoveEvent();
    public class MouseWheelEvent {
        private StringHash _event = new StringHash("MouseWheel");
        public StringHash Wheel = new StringHash("Wheel");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public MouseWheelEvent() { }
        public static implicit operator StringHash(MouseWheelEvent e) { return e._event; }
    }
    public static MouseWheelEvent MouseWheel = new MouseWheelEvent();
    public class KeyDownEvent {
        private StringHash _event = new StringHash("KeyDown");
        public StringHash Key = new StringHash("Key");
        public StringHash Scancode = new StringHash("Scancode");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public StringHash Repeat = new StringHash("Repeat");
        public KeyDownEvent() { }
        public static implicit operator StringHash(KeyDownEvent e) { return e._event; }
    }
    public static KeyDownEvent KeyDown = new KeyDownEvent();
    public class KeyUpEvent {
        private StringHash _event = new StringHash("KeyUp");
        public StringHash Key = new StringHash("Key");
        public StringHash Scancode = new StringHash("Scancode");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public KeyUpEvent() { }
        public static implicit operator StringHash(KeyUpEvent e) { return e._event; }
    }
    public static KeyUpEvent KeyUp = new KeyUpEvent();
    public class TextInputEvent {
        private StringHash _event = new StringHash("TextInput");
        public StringHash Text = new StringHash("Text");
        public TextInputEvent() { }
        public static implicit operator StringHash(TextInputEvent e) { return e._event; }
    }
    public static TextInputEvent TextInput = new TextInputEvent();
    public class TextEditingEvent {
        private StringHash _event = new StringHash("TextEditing");
        public StringHash Composition = new StringHash("Composition");
        public StringHash Cursor = new StringHash("Cursor");
        public StringHash SelectionLength = new StringHash("SelectionLength");
        public TextEditingEvent() { }
        public static implicit operator StringHash(TextEditingEvent e) { return e._event; }
    }
    public static TextEditingEvent TextEditing = new TextEditingEvent();
    public class JoystickConnectedEvent {
        private StringHash _event = new StringHash("JoystickConnected");
        public StringHash JoystickID = new StringHash("JoystickID");
        public JoystickConnectedEvent() { }
        public static implicit operator StringHash(JoystickConnectedEvent e) { return e._event; }
    }
    public static JoystickConnectedEvent JoystickConnected = new JoystickConnectedEvent();
    public class JoystickDisconnectedEvent {
        private StringHash _event = new StringHash("JoystickDisconnected");
        public StringHash JoystickID = new StringHash("JoystickID");
        public JoystickDisconnectedEvent() { }
        public static implicit operator StringHash(JoystickDisconnectedEvent e) { return e._event; }
    }
    public static JoystickDisconnectedEvent JoystickDisconnected = new JoystickDisconnectedEvent();
    public class JoystickButtonDownEvent {
        private StringHash _event = new StringHash("JoystickButtonDown");
        public StringHash JoystickID = new StringHash("JoystickID");
        public StringHash Button = new StringHash("Button");
        public JoystickButtonDownEvent() { }
        public static implicit operator StringHash(JoystickButtonDownEvent e) { return e._event; }
    }
    public static JoystickButtonDownEvent JoystickButtonDown = new JoystickButtonDownEvent();
    public class JoystickButtonUpEvent {
        private StringHash _event = new StringHash("JoystickButtonUp");
        public StringHash JoystickID = new StringHash("JoystickID");
        public StringHash Button = new StringHash("Button");
        public JoystickButtonUpEvent() { }
        public static implicit operator StringHash(JoystickButtonUpEvent e) { return e._event; }
    }
    public static JoystickButtonUpEvent JoystickButtonUp = new JoystickButtonUpEvent();
    public class JoystickAxisMoveEvent {
        private StringHash _event = new StringHash("JoystickAxisMove");
        public StringHash JoystickID = new StringHash("JoystickID");
        public StringHash Button = new StringHash("Button");
        public StringHash Position = new StringHash("Position");
        public JoystickAxisMoveEvent() { }
        public static implicit operator StringHash(JoystickAxisMoveEvent e) { return e._event; }
    }
    public static JoystickAxisMoveEvent JoystickAxisMove = new JoystickAxisMoveEvent();
    public class JoystickHatMoveEvent {
        private StringHash _event = new StringHash("JoystickHatMove");
        public StringHash JoystickID = new StringHash("JoystickID");
        public StringHash Button = new StringHash("Button");
        public StringHash Position = new StringHash("Position");
        public JoystickHatMoveEvent() { }
        public static implicit operator StringHash(JoystickHatMoveEvent e) { return e._event; }
    }
    public static JoystickHatMoveEvent JoystickHatMove = new JoystickHatMoveEvent();
    public class TouchBeginEvent {
        private StringHash _event = new StringHash("TouchBegin");
        public StringHash TouchID = new StringHash("TouchID");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash Pressure = new StringHash("Pressure");
        public TouchBeginEvent() { }
        public static implicit operator StringHash(TouchBeginEvent e) { return e._event; }
    }
    public static TouchBeginEvent TouchBegin = new TouchBeginEvent();
    public class TouchEndEvent {
        private StringHash _event = new StringHash("TouchEnd");
        public StringHash TouchID = new StringHash("TouchID");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public TouchEndEvent() { }
        public static implicit operator StringHash(TouchEndEvent e) { return e._event; }
    }
    public static TouchEndEvent TouchEnd = new TouchEndEvent();
    public class TouchMoveEvent {
        private StringHash _event = new StringHash("TouchMove");
        public StringHash TouchID = new StringHash("TouchID");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash DX = new StringHash("DX");
        public StringHash DY = new StringHash("DY");
        public StringHash Pressure = new StringHash("Pressure");
        public TouchMoveEvent() { }
        public static implicit operator StringHash(TouchMoveEvent e) { return e._event; }
    }
    public static TouchMoveEvent TouchMove = new TouchMoveEvent();
    public class GestureRecordedEvent {
        private StringHash _event = new StringHash("GestureRecorded");
        public StringHash GestureID = new StringHash("GestureID");
        public GestureRecordedEvent() { }
        public static implicit operator StringHash(GestureRecordedEvent e) { return e._event; }
    }
    public static GestureRecordedEvent GestureRecorded = new GestureRecordedEvent();
    public class GestureInputEvent {
        private StringHash _event = new StringHash("GestureInput");
        public StringHash GestureID = new StringHash("GestureID");
        public StringHash CenterX = new StringHash("CenterX");
        public StringHash CenterY = new StringHash("CenterY");
        public StringHash NumFingers = new StringHash("NumFingers");
        public StringHash Error = new StringHash("Error");
        public GestureInputEvent() { }
        public static implicit operator StringHash(GestureInputEvent e) { return e._event; }
    }
    public static GestureInputEvent GestureInput = new GestureInputEvent();
    public class MultiGestureEvent {
        private StringHash _event = new StringHash("MultiGesture");
        public StringHash CenterX = new StringHash("CenterX");
        public StringHash CenterY = new StringHash("CenterY");
        public StringHash NumFingers = new StringHash("NumFingers");
        public StringHash DTheta = new StringHash("DTheta");
        public StringHash DDist = new StringHash("DDist");
        public MultiGestureEvent() { }
        public static implicit operator StringHash(MultiGestureEvent e) { return e._event; }
    }
    public static MultiGestureEvent MultiGesture = new MultiGestureEvent();
    public class DropFileEvent {
        private StringHash _event = new StringHash("DropFile");
        public StringHash FileName = new StringHash("FileName");
        public DropFileEvent() { }
        public static implicit operator StringHash(DropFileEvent e) { return e._event; }
    }
    public static DropFileEvent DropFile = new DropFileEvent();
    public class InputFocusEvent {
        private StringHash _event = new StringHash("InputFocus");
        public StringHash Focus = new StringHash("Focus");
        public StringHash Minimized = new StringHash("Minimized");
        public InputFocusEvent() { }
        public static implicit operator StringHash(InputFocusEvent e) { return e._event; }
    }
    public static InputFocusEvent InputFocus = new InputFocusEvent();
    public class MouseVisibleChangedEvent {
        private StringHash _event = new StringHash("MouseVisibleChanged");
        public StringHash Visible = new StringHash("Visible");
        public MouseVisibleChangedEvent() { }
        public static implicit operator StringHash(MouseVisibleChangedEvent e) { return e._event; }
    }
    public static MouseVisibleChangedEvent MouseVisibleChanged = new MouseVisibleChangedEvent();
    public class MouseModeChangedEvent {
        private StringHash _event = new StringHash("MouseModeChanged");
        public StringHash Mode = new StringHash("Mode");
        public StringHash MouseLocked = new StringHash("MouseLocked");
        public MouseModeChangedEvent() { }
        public static implicit operator StringHash(MouseModeChangedEvent e) { return e._event; }
    }
    public static MouseModeChangedEvent MouseModeChanged = new MouseModeChangedEvent();
    public class ExitRequestedEvent {
        private StringHash _event = new StringHash("ExitRequested");
        public ExitRequestedEvent() { }
        public static implicit operator StringHash(ExitRequestedEvent e) { return e._event; }
    }
    public static ExitRequestedEvent ExitRequested = new ExitRequestedEvent();
    public class SDLRawInputEvent {
        private StringHash _event = new StringHash("SDLRawInput");
        public StringHash SDLEvent = new StringHash("SDLEvent");
        public StringHash Consumed = new StringHash("Consumed");
        public SDLRawInputEvent() { }
        public static implicit operator StringHash(SDLRawInputEvent e) { return e._event; }
    }
    public static SDLRawInputEvent SDLRawInput = new SDLRawInputEvent();
    public class InputBeginEvent {
        private StringHash _event = new StringHash("InputBegin");
        public InputBeginEvent() { }
        public static implicit operator StringHash(InputBeginEvent e) { return e._event; }
    }
    public static InputBeginEvent InputBegin = new InputBeginEvent();
    public class InputEndEvent {
        private StringHash _event = new StringHash("InputEnd");
        public InputEndEvent() { }
        public static implicit operator StringHash(InputEndEvent e) { return e._event; }
    }
    public static InputEndEvent InputEnd = new InputEndEvent();
    public class MultitouchEvent {
        private StringHash _event = new StringHash("Multitouch");
        public StringHash NumFingers = new StringHash("NumFingers");
        public StringHash EventType = new StringHash("EventType");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash DX = new StringHash("DX");
        public StringHash DY = new StringHash("DY");
        public StringHash Size = new StringHash("Size");
        public StringHash DSize = new StringHash("DSize");
        public MultitouchEvent() { }
        public static implicit operator StringHash(MultitouchEvent e) { return e._event; }
    }
    public static MultitouchEvent Multitouch = new MultitouchEvent();
} %}
