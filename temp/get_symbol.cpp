#include <iostream>
#include <fstream>
#include <elf.h>
#include <vector>
#include <stdexcept>
#include <string>

constexpr uint64_t PAGE_SIZE = 0x1000;

uint64_t page_align(uint64_t addr) {
    return addr & ~(PAGE_SIZE - 1);
}

class ElfParseException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class FileNotFoundException : public ElfParseException {
public:
    FileNotFoundException(const std::string& file_name)
        : ElfParseException("File not found: " + file_name) {}
};

class ElfReadException : public ElfParseException {
public:
    ElfReadException(const std::string& message)
        : ElfParseException("ELF read error: " + message) {}
};

class SymbolTableNotFoundException : public ElfParseException {
public:
    SymbolTableNotFoundException()
        : ElfParseException("Could not find a .symtab section.") {}
};

class SymbolNotFoundException : public ElfParseException {
public:
    SymbolNotFoundException(const std::string& symbol_name)
        : ElfParseException("Symbol not found: " + symbol_name) {}
};

class MySymbol {
public:
    uint64_t address;
    uint8_t size;
    bool is_pie;
};


MySymbol get_symbol_from_elf(const char* file_name, const char* symbol_name) {
    MySymbol symbol;
    std::ifstream file(file_name, std::ios::binary);
    if (!file.is_open())
        throw FileNotFoundException(file_name);

    Elf64_Ehdr elf_header;
    if (!file.read(reinterpret_cast<char*>(&elf_header), sizeof(elf_header)))
        throw ElfReadException("Could not read ELF header.");

    // check if magic number and architecture checks out
    if (elf_header.e_ident[EI_MAG0] != ELFMAG0 || elf_header.e_ident[EI_MAG1] != ELFMAG1 ||
        elf_header.e_ident[EI_MAG2] != ELFMAG2 || elf_header.e_ident[EI_MAG3] != ELFMAG3)
        throw ElfReadException("Not a valid ELF file.");

    if (elf_header.e_ident[EI_CLASS] != ELFCLASS64)
       throw ElfReadException("Not a 64-bit ELF file.");

    std::cout << "Executable of type: ";
    switch (elf_header.e_type) {
        case ET_DYN:
            symbol.is_pie = true;
            std::cout << "ET_DYN." << std::endl;
            break;
        case ET_EXEC:
            symbol.is_pie = false;
            std::cout << "ET_EXEC." << std::endl;
            break;
        default:
            std::cout << "INVALID." << std::endl;
            throw ElfReadException("Wrong object file type.");
    }

    // read all section headers
    std::vector<Elf64_Shdr> section_headers(elf_header.e_shnum);
    file.seekg(elf_header.e_shoff);
    if (!file.read(reinterpret_cast<char*>(section_headers.data()), elf_header.e_shnum * sizeof(Elf64_Shdr)))
        throw ElfReadException("Could not read section headers.");

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
        throw SymbolTableNotFoundException();

    std::vector<Elf64_Sym> symbols(symtab_sh->sh_size / sizeof(Elf64_Sym));
    file.seekg(symtab_sh->sh_offset);
    file.read(reinterpret_cast<char*>(symbols.data()), symtab_sh->sh_size);

    std::vector<char> strtab(strtab_sh->sh_size);
    file.seekg(strtab_sh->sh_offset);
    file.read(strtab.data(), strtab_sh->sh_size);

    for (const auto& sym : symbols) {
        if (sym.st_name != 0 && std::string(&strtab[sym.st_name]).find(symbol_name) != std::string::npos) {
            std::cout << "Symbol '" << symbol_name << "' found!" << std::endl;
            std::cout << "  - Virtual Address: 0x" << std::hex << sym.st_value << std::dec << std::endl;
            std::cout << "  - Size: " << sym.st_size << " bytes" << std::endl;

            if (sym.st_shndx >= SHN_LORESERVE) {
                std::cout << "  - Section: Special (e.g., ABS, COMMON, UNDEF)" << std::endl;
                if (sym.st_shndx == SHN_ABS) {
                    std::cout << "  - Offset: N/A (Absolute Symbol)" << std::endl;
                }
            } else if (sym.st_shndx >= section_headers.size()) {
                    std::cerr << "  - Error: Invalid section index " << sym.st_shndx << std::endl;
            } else {
                const Elf64_Shdr& symbol_section = section_headers[sym.st_shndx];
                std::string_view section_name = &shstrtab[symbol_section.sh_name];
                Elf64_Addr offset = sym.st_value - symbol_section.sh_addr;
                Elf64_Addr adj_offset = offset + symbol_section.sh_addr - page_align(symbol_section.sh_addr);

                std::cout << "  - Section: " << section_name
                            << " (Index " << sym.st_shndx << ")" << std::endl;
                std::cout << "  - Section Base Address: 0x" << std::hex << symbol_section.sh_addr << std::dec << std::endl;
                std::cout << "  - Adjusted Base Address to page align: 0x" << std::hex << page_align(symbol_section.sh_addr) << std::dec << std::endl;
                std::cout << "  - Offset within section: 0x" << std::hex << offset << " (" << std::dec << offset  << " bytes)" << std::endl;
                std::cout << "  - Offset within adjusted section: 0x" << std::hex << adj_offset << " (" << std::dec << adj_offset << " bytes)" << std::endl;
            }

            symbol.address = sym.st_value;
            symbol.size = sym.st_size;
            return symbol;
        }
    }

    throw SymbolNotFoundException(symbol_name);
}

int main() {
    try {
        auto symbol_addr = get_symbol_from_elf("build/get_offset", "global_variable");
        (void) symbol_addr;
    } catch (const ElfParseException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
