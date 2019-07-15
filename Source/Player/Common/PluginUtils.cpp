//
// Copyright (c) 2017-2019 the rbfx project.
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
#if __APPLE__
struct mach_header
{
    uint32_t magic;
    uint32_t cputype;
    uint32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
#if URHO3D_64BIT
    uint32_t reserved;
#endif
};


#if URHO3D_64BIT
const unsigned MACHO_MAGIC = 0xFEEDFACF;
#else
const unsigned MACHO_MAGIC = 0xFEEDFACE;
#endif

struct nlist
{
    int32_t n_strx;
    uint8_t n_type;
    uint8_t n_sect;
    int16_t n_desc;
#if URHO3D_64BIT
    uint64_t n_value;
#else
    uint32_t n_value;
#endif
};

struct load_command
{
    uint32_t cmd;
    uint32_t cmdsize;
};
struct symtab_command : load_command
{
    uint32_t symoff;
    uint32_t nsyms;
    uint32_t stroff;
    uint32_t strsize;
};

struct dysymtab_command : load_command
{
    uint32_t ilocalsym;
    uint32_t nlocalsym;
    uint32_t iextdefsym;
    uint32_t nextdefsym;
    uint32_t iundefsym;
    uint32_t nundefsym;
    uint32_t tocoff;
    uint32_t ntoc;
    uint32_t modtaboff;
    uint32_t nmodtab;
    uint32_t extrefsymoff;
    uint32_t nextrefsyms;
    uint32_t indirectsymoff;
    uint32_t nindirectsyms;
    uint32_t extreloff;
    uint32_t nextrel;
    uint32_t locreloff;
    uint32_t nlocrel;
};
#endif

namespace Urho3D
{

template<typename T>
static bool IsValidPtr(const ea::vector<unsigned char>& data, const T* p)
{
    return reinterpret_cast<std::uintptr_t>(p) >= reinterpret_cast<std::uintptr_t>(data.data()) &&
           reinterpret_cast<std::uintptr_t>(p) + sizeof(T) <= reinterpret_cast<std::uintptr_t>(data.data() + data.size());
}

template<typename T>
static bool IsValidPtr(const ea::vector<unsigned char>& data, const T* p, unsigned len)
{
    return reinterpret_cast<std::uintptr_t>(p) >= reinterpret_cast<std::uintptr_t>(data.data()) &&
           reinterpret_cast<std::uintptr_t>(p) + len <= reinterpret_cast<std::uintptr_t>(data.data() + data.size());
}

PluginType GetPluginType(Context* context, const ea::string& path)
{
#if URHO3D_PLUGINS
    // This function implements a naive check for plugin validity. Proper check would parse executable headers and look
    // for relevant exported function names.
    ea::vector<unsigned char> data;
    const char pluginEntryPoint[] = "cr_main";

#if __linux__
    // ELF magic
    if (path.ends_with(".so"))
    {
        File file(context);
        if (!file.Open(path, FILE_READ))
            return PLUGIN_INVALID;

        data.resize(file.GetSize());
        if (file.Read(data.data(), data.size()) != data.size())
            return PLUGIN_INVALID;

        // Elf header parsing code based on elfdump by Owen Klan.
        Elf_Ehdr* hdr = reinterpret_cast<Elf_Ehdr*>(data.data());
        if (!IsValidPtr(data, hdr) || strncmp(reinterpret_cast<const char*>(hdr->e_ident), ELFMAG, SELFMAG) != 0)
            // Not elf.
            return PLUGIN_INVALID;

        if (hdr->e_type != ET_DYN)
            return PLUGIN_INVALID;

        // Find symbol name table
        unsigned symNameTableOffset = 0;
        {
            Elf_Shdr* shdr = reinterpret_cast<Elf_Shdr*>(data.data() + hdr->e_shoff + sizeof(Elf_Shdr) * hdr->e_shstrndx);
            if (!IsValidPtr(data, shdr))
                return PLUGIN_INVALID;

            auto nameTableOffset = shdr->sh_offset;
            shdr = reinterpret_cast<Elf_Shdr*>(data.data() + hdr->e_shoff);

            for (auto i = 0; i < hdr->e_shnum; i++)
            {
                if (!IsValidPtr(data, shdr))
                    return PLUGIN_INVALID;

                const char* tabNamePtr = reinterpret_cast<const char*>(data.data() + nameTableOffset + shdr->sh_name);
                const char strTabName[] = ".strtab";

                if (!IsValidPtr(data, tabNamePtr, sizeof(strTabName)))
                    return PLUGIN_INVALID;

                if (strncmp(tabNamePtr, strTabName, sizeof(strTabName)) == 0)
                {
                    symNameTableOffset = shdr->sh_offset;
                    break;
                }
                else
                    ++shdr;
            }
        }

        if (symNameTableOffset == 0)
            return PLUGIN_INVALID;

        // Find cr_main symbol
        {
            auto shoff = hdr->e_shoff;
            Elf_Shdr* sectab = nullptr;
            do
            {
                sectab = reinterpret_cast<Elf_Shdr*>(data.data() + shoff);
                if (!IsValidPtr(data, sectab))
                    return PLUGIN_INVALID;
                shoff += sizeof(Elf_Shdr);
            } while (sectab->sh_type != SHT_SYMTAB);

            shoff = sectab->sh_offset;
            auto num = sectab->sh_size / sectab->sh_entsize;
            ea::string funcName;
            for (auto i = 0; i < num; i++)
            {
                Elf_Sym* symbol = reinterpret_cast<Elf_Sym*>(data.data() + shoff);
                if (!IsValidPtr(data, symbol))
                    return PLUGIN_INVALID;
                shoff += sizeof(Elf_Sym);

                const char* funcNamePtr = reinterpret_cast<const char*>(data.data() + symNameTableOffset + symbol->st_name);
                if (!IsValidPtr(data, funcNamePtr, sizeof(pluginEntryPoint)))
                    return PLUGIN_INVALID;

                if (strncmp(funcNamePtr, pluginEntryPoint, sizeof(pluginEntryPoint)) == 0)
                    return PLUGIN_NATIVE;
            }
        }
    }
#endif

#if _WIN32 || URHO3D_CSHARP
    if (path.ends_with(".dll"))
    {
        File file(context);
        if (!file.Open(path, FILE_READ))
            return PLUGIN_INVALID;

        data.resize(file.GetSize());
        if (file.Read(data.data(), data.size()) != data.size())
            return PLUGIN_INVALID;

        PIMAGE_DOS_HEADER dos = reinterpret_cast<PIMAGE_DOS_HEADER>(data.data());

        if (!IsValidPtr(data, dos) || dos->e_magic != IMAGE_DOS_SIGNATURE)
            return PLUGIN_INVALID;

        PIMAGE_NT_HEADERS nt = reinterpret_cast<PIMAGE_NT_HEADERS>(data.data() + dos->e_lfanew);
        if (!IsValidPtr(data, dos) || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
            return PLUGIN_INVALID;

        const auto& eatDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        const auto& netDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
        if (netDir.VirtualAddress != 0)
        {
#if URHO3D_CSHARP
            // Verify that plugin has a class that inherits from PluginApplication.
            if (context->GetSubsystem<Script>()->GetRuntimeApi()->VerifyAssembly(path))
                return PLUGIN_MANAGED;
#endif
        }
#if _WIN32
        else if (eatDir.VirtualAddress > 0)
        {
            // Verify that plugin has exported function named cr_main.
            // Find section that contains EAT.

            unsigned firstSectionOffset = dos->e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + nt->FileHeader.SizeOfOptionalHeader;
            uint32_t eatModifier = 0;
            for (auto i = 0; i < nt->FileHeader.NumberOfSections; i++)
            {
                PIMAGE_SECTION_HEADER section = reinterpret_cast<PIMAGE_SECTION_HEADER>(data.data() + firstSectionOffset + i * sizeof(IMAGE_SECTION_HEADER));
                if (!IsValidPtr(data, section))
                    return PLUGIN_INVALID;

                if (eatDir.VirtualAddress >= section->VirtualAddress && eatDir.VirtualAddress < (section->VirtualAddress + section->SizeOfRawData))
                {
                    eatModifier = section->VirtualAddress - section->PointerToRawData;
                    break;
                }
            }

            PIMAGE_EXPORT_DIRECTORY eat = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(data.data() + eatDir.VirtualAddress - eatModifier);
            if (!IsValidPtr(data, eat))
                return PLUGIN_INVALID;

            unsigned namesOffset = eat->AddressOfNames - eatModifier;
            for (auto i = 0; i < eat->NumberOfFunctions; i++)
            {
                uint32_t* nameOffset = reinterpret_cast<unsigned*>(data.data() + namesOffset + i * sizeof(uint32_t));
                if (!IsValidPtr(data, nameOffset))
                    return PLUGIN_INVALID;

                const char* funcNamePtr = reinterpret_cast<const char*>(data.data() + *nameOffset - eatModifier);
                if (!IsValidPtr(data, funcNamePtr, sizeof(pluginEntryPoint)))
                    return PLUGIN_INVALID;

                if (strncmp(funcNamePtr, pluginEntryPoint, sizeof(pluginEntryPoint)) == 0)
                    return PLUGIN_NATIVE;
            }
        }
#endif  // _WIN32
    }
#endif  // _WIN32
#if __APPLE__
    if (path.ends_with(".dylib"))
    {
        File file(context);
        if (!file.Open(path, FILE_READ))
            return PLUGIN_INVALID;

        data.resize(file.GetSize());
        if (file.Read(data.data(), data.size()) != data.size())
            return PLUGIN_INVALID;

        mach_header* hdr = reinterpret_cast<mach_header*>(data.data());

        if (!IsValidPtr(data, hdr) || hdr->magic != MACHO_MAGIC || hdr->filetype != 6 /*dylib*/)
            return PLUGIN_INVALID;

        symtab_command* symtab = nullptr;
        dysymtab_command* dysymtab = nullptr;
        unsigned offset = sizeof(mach_header);
        for (unsigned i = 0; i < hdr->ncmds; i++)
        {
            load_command* cmd = reinterpret_cast<load_command*>(data.data() + offset);
            if (!IsValidPtr(data, cmd))
                return PLUGIN_INVALID;

            if (cmd->cmd == 0x2)            // SYMTAB
            {
                symtab = reinterpret_cast<symtab_command*>(cmd);
                if (!IsValidPtr(data, symtab))
                    return PLUGIN_INVALID;
            }
            else if (cmd->cmd == 0xB)   // DYSYMTAB
            {
                dysymtab = reinterpret_cast<dysymtab_command*>(cmd);
                if (!IsValidPtr(data, dysymtab))
                    return PLUGIN_INVALID;
            }

            if (symtab && dysymtab)
                break;

            offset += cmd->cmdsize;
        }

        if (!symtab || !dysymtab)
            return PLUGIN_INVALID;

        const char* strtab = reinterpret_cast<const char*>(data.data() + symtab->stroff);
        const nlist* lists = reinterpret_cast<nlist*>(data.data() + symtab->symoff);

        if (!IsValidPtr(data, lists, sizeof(nlist) * symtab->nsyms))
            return PLUGIN_INVALID;

        if (!IsValidPtr(data, strtab, symtab->strsize))
            return PLUGIN_INVALID;

        for (unsigned i = dysymtab->ilocalsym; i < dysymtab->ilocalsym + dysymtab->nlocalsym; i++)
        {
            const char* name = strtab + lists[i].n_strx;
            if (strncmp(name, pluginEntryPoint, sizeof(pluginEntryPoint)) == 0)
                return PLUGIN_NATIVE;
        }
    }
#endif
#endif
    return PLUGIN_INVALID;
}


}
