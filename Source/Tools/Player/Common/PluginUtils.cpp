//
// Copyright (c) 2018 Rokas Kupstys
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/IO/File.h>
#if URHO3D_CSHARP
#   include <Urho3D/Script/Script.h>
#endif
#include "PluginUtils.h"
#if _WIN32
#   include <windows.h>
#else
#   include "PE.h"
#endif
#if __linux__
#   include <elf.h>
#   if URHO3D_64BIT
using Elf_Ehdr = Elf64_Ehdr;
using Elf_Phdr = Elf64_Phdr;
using Elf_Shdr = Elf64_Shdr;
using Elf_Sym = Elf64_Sym;
#   else
using Elf_Ehdr = Elf32_Ehdr;
using Elf_Phdr = Elf32_Phdr;
using Elf_Shdr = Elf32_Shdr;
using Elf_Sym = Elf32_Sym;
#   endif
#endif


namespace Urho3D
{


PluginType GetPluginType(Context* context, const String& path)
{
#if URHO3D_PLUGINS
    // This function implements a naive check for plugin validity. Proper check would parse executable headers and look
    // for relevant exported function names.
#if __linux__
    // ELF magic
    if (path.EndsWith(".so"))
    {
        File file(context);
        if (!file.Open(path, FILE_READ))
            return PLUGIN_INVALID;

        String buf{};
        buf.Resize(file.GetSize());
        file.Seek(0);
        file.Read(&buf[0], file.GetSize());
        file.Close();

        // Elf header parsing code based on elfdump by Owen Klan.
        const auto base = reinterpret_cast<const char*>(buf.CString());
        const auto hdr = reinterpret_cast<const Elf_Ehdr*>(base);
        if (strncmp(reinterpret_cast<const char*>(hdr->e_ident), ELFMAG, SELFMAG) != 0)
            // Not elf.
            return PLUGIN_INVALID;

        if (hdr->e_type != ET_DYN)
            return PLUGIN_INVALID;

        // Find symbol name table
        const char* symNameTable = nullptr;
        {
            const Elf_Shdr* ptr = reinterpret_cast<const Elf_Shdr*>(base + hdr->e_shoff);

            for (int i = 0; i < hdr->e_shstrndx; i++)
                ptr++;

            const char* nameTable = base + ptr->sh_offset;

            ptr = reinterpret_cast<const Elf_Shdr*>(base + hdr->e_shoff);
            for (auto i = 0; i < hdr->e_shnum; i++)
            {
                if (strncmp((nameTable + ptr->sh_name), ".strtab", 7) == 0)
                {
                    symNameTable = base + ptr->sh_offset;
                    break;
                }
                else
                    ptr++;
            }
        }

        if (symNameTable == nullptr)
            return PLUGIN_INVALID;

        // Find cr_main symbol
        {
            const Elf_Shdr* sectab = reinterpret_cast<const Elf_Shdr*>(base + hdr->e_shoff);
            const Elf_Shdr* ptr = sectab;
            while (ptr->sh_type != SHT_SYMTAB)
                ptr++;
            const Elf_Shdr* sym_tab = reinterpret_cast<const Elf_Shdr*>(base + ptr->sh_offset);
            const Elf_Sym* symbol = reinterpret_cast<const Elf_Sym*>(sym_tab);
            auto num = ptr->sh_size / ptr->sh_entsize;
            for (auto i = 0; i < num; i++)
            {
                const char* name = symNameTable + symbol->st_name;
                if (strncmp(name, "cr_main", 7) == 0)
                    return PLUGIN_NATIVE;
                symbol++;
            }
        }
    }
#endif
    if (path.EndsWith(".dll"))
    {
        File file(context);
        if (!file.Open(path, FILE_READ))
            return PLUGIN_INVALID;

        if (file.ReadShort() == IMAGE_DOS_SIGNATURE)
        {
            String buf{};
            buf.Resize(file.GetSize());
            file.Seek(0);
            file.Read(&buf[0], file.GetSize());
            file.Close();

            const auto base = reinterpret_cast<const char*>(buf.CString());
            const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
            const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);

            if (nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
                return PLUGIN_INVALID;

            const auto& eatDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
            const auto& netDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
            if (netDir.VirtualAddress != 0)
            {
#if URHO3D_CSHARP
                // Verify that plugin has a class that inherits from PluginApplication.
                if (context->GetSubsystem<Script>()->VerifyAssembly(path))
                    return PLUGIN_MANAGED;
#endif
            }
            else if (eatDir.VirtualAddress > 0)
            {
                // Verify that plugin has exported function named cr_main.
                // Find section that contains EAT.
                const auto* section = IMAGE_FIRST_SECTION(nt);
                uint32_t eatModifier = 0;
                for (auto i = 0; i < nt->FileHeader.NumberOfSections; i++, section++)
                {
                    if (eatDir.VirtualAddress >= section->VirtualAddress && eatDir.VirtualAddress < (section->VirtualAddress + section->SizeOfRawData))
                    {
                        eatModifier = section->VirtualAddress - section->PointerToRawData;
                        break;
                    }
                }

                const auto eat = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base + eatDir.VirtualAddress - eatModifier);
                const auto names = reinterpret_cast<const uint32_t*>(base + eat->AddressOfNames - eatModifier);
                for (auto i = 0; i < eat->NumberOfFunctions; i++)
                {
                    if (strcmp(base + names[i] - eatModifier, "cr_main") == 0)
                        return PLUGIN_NATIVE;
                }
            }
        }
    }

    if (path.EndsWith(".dylib"))
    {
        // TODO: MachO file support.
    }
#endif
    return PLUGIN_INVALID;
}


}
