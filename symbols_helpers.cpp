#include "symbols_helpers.hpp"
#include <elf.h>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <sstream>

MySymbol get_symbol_from_elf(const char* file_name, const char* symbol_name) {
    MySymbol symbol;
    std::ifstream file(file_name, std::ios::binary);
    if (!file.is_open())
        throw std::system_error(errno, std::generic_category(), std::string("Failed to open ") + file_name);

    Elf64_Ehdr elf_header;
    if (!file.read(reinterpret_cast<char*>(&elf_header), sizeof(elf_header)))
        throw std::system_error(errno, std::generic_category(), std::string("Failed to read ") + file_name);

    // check if magic number and architecture checks out
    if (elf_header.e_ident[EI_MAG0] != ELFMAG0 || elf_header.e_ident[EI_MAG1] != ELFMAG1 ||
        elf_header.e_ident[EI_MAG2] != ELFMAG2 || elf_header.e_ident[EI_MAG3] != ELFMAG3)
        throw std::runtime_error("Not a valid ELF file.");

    if (elf_header.e_ident[EI_CLASS] != ELFCLASS64)
       throw std::runtime_error("Not a 64-bit ELF file.");

    switch (elf_header.e_type) {
        case ET_DYN:
            symbol.is_pie = true;
            break;
        case ET_EXEC:
            symbol.is_pie = false;
            break;
        default:
            throw std::runtime_error("Wrong object file type.");
    }

    // read all section headers
    std::vector<Elf64_Shdr> section_headers(elf_header.e_shnum);
    file.seekg(elf_header.e_shoff);
    if (!file.read(reinterpret_cast<char*>(section_headers.data()), elf_header.e_shnum * sizeof(Elf64_Shdr)))
        throw std::runtime_error("Could not read section headers.");

    // find symbol table and string table
    const Elf64_Shdr* symtab_sh = nullptr;
    const Elf64_Shdr* strtab_sh = nullptr;

    const Elf64_Shdr& shstrtab_header = section_headers[elf_header.e_shstrndx];
    std::vector<char> shstrtab(shstrtab_header.sh_size);
    file.seekg(shstrtab_header.sh_offset);
    file.read(shstrtab.data(), shstrtab_header.sh_size);

    for (const auto& sh : section_headers) {
        if (sh.sh_type == SHT_SYMTAB) {
            symtab_sh = &sh;
            strtab_sh = &section_headers[sh.sh_link];
            break;
        }
    }

    if (!symtab_sh)
        throw std::runtime_error("Could not find .symtab section.");

    std::vector<Elf64_Sym> symbols(symtab_sh->sh_size / sizeof(Elf64_Sym));
    file.seekg(symtab_sh->sh_offset);
    file.read(reinterpret_cast<char*>(symbols.data()), symtab_sh->sh_size);

    std::vector<char> strtab(strtab_sh->sh_size);
    file.seekg(strtab_sh->sh_offset);
    file.read(strtab.data(), strtab_sh->sh_size);

    for (const auto& sym : symbols) {
        if (sym.st_name != 0 && std::string(&strtab[sym.st_name]).find(symbol_name) != std::string::npos) {
            symbol.address = sym.st_value;
            symbol.size = sym.st_size;
            return symbol;
        }
    }

    throw std::runtime_error("Could not find symbol in executable.");
}



uint64_t get_offset_from_maps(pid_t pid, const char *file_name) {
    std::stringstream ss;
    ss << "/proc/" << pid << "/maps";
    std::string maps_path = ss.str();

    std::ifstream maps(maps_path);
    if (!maps.is_open())
        throw std::system_error(errno, std::generic_category(), "Failed to open " + maps_path);

    std::string line;
    while (std::getline(maps, line)) {
        if (line.find(file_name) != std::string::npos) {
            auto dash_pos = line.find('-');
            if (dash_pos == std::string::npos)
                throw std::runtime_error("Malformed /proc/<pid>/maps line: " + line);
            std::string base_addr = line.substr(0, dash_pos);
            return std::stoull(base_addr, nullptr, 16);
        }
    }

    throw std::runtime_error("Mapping for '" + (file_name + ("' not found in " + maps_path)));
}