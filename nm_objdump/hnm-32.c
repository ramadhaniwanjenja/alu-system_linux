#include "hnm.h"

/**
 * get_symbol_type32 - determines the symbol type for a 32-bit ELF symbol
 * @symbol: the symbol entry
 * @section_headers: array of section headers
 * Return: symbol type character
 */
char get_symbol_type32(Elf32_Sym symbol, Elf32_Shdr *section_headers)
{
	char symbol_type = '?';
	Elf32_Shdr section;

	if (ELF32_ST_BIND(symbol.st_info) == STB_WEAK)
	{
		if (symbol.st_shndx == SHN_UNDEF)
			return ('w');
		if (ELF32_ST_TYPE(symbol.st_info) == STT_OBJECT)
			return ('V');
		return ('W');
	}
	if (symbol.st_shndx == SHN_UNDEF)
		return ('U');
	if (symbol.st_shndx == SHN_ABS)
		return ('A');
	if (symbol.st_shndx == SHN_COMMON)
		return ('C');
	if (symbol.st_shndx < SHN_LORESERVE)
	{
		section = section_headers[symbol.st_shndx];
		if (ELF32_ST_BIND(symbol.st_info) == STB_GNU_UNIQUE)
			symbol_type = 'u';
		else if (section.sh_type == SHT_NOBITS &&
			 section.sh_flags == (SHF_ALLOC | SHF_WRITE))
			symbol_type = 'B';
		else if (section.sh_type == SHT_PROGBITS)
		{
			if (section.sh_flags == (SHF_ALLOC | SHF_EXECINSTR))
				symbol_type = 'T';
			else if (section.sh_flags == SHF_ALLOC)
				symbol_type = 'R';
			else if (section.sh_flags == (SHF_ALLOC | SHF_WRITE))
				symbol_type = 'D';
		}
		else if (section.sh_type == SHT_DYNAMIC)
			symbol_type = 'D';
		else
			symbol_type = 't';
	}
	if (ELF32_ST_BIND(symbol.st_info) == STB_LOCAL)
		symbol_type = tolower(symbol_type);

	return (symbol_type);
}

/**
 * print_symbol_table32 - prints symbol table for 32-bit ELF file
 * @section_header: pointer to the section header of the symbol table
 * @symbol_table: pointer to the symbol table
 * @string_table: pointer to the string table
 * @section_headers: pointer to all section headers
 */
void print_symbol_table32(Elf32_Shdr *section_header, Elf32_Sym *symbol_table,
			  char *string_table, Elf32_Shdr *section_headers)
{
	int i;
	int symbol_count = section_header->sh_size / sizeof(Elf32_Sym);
	char *symbol_name;
	char symbol_type;

	for (i = 0; i < symbol_count; i++)
	{
		Elf32_Sym symbol = symbol_table[i];
		symbol_name = string_table + symbol.st_name;

		if (symbol.st_name != 0 &&
		    ELF32_ST_TYPE(symbol.st_info) != STT_FILE)
		{
			symbol_type = get_symbol_type32(symbol, section_headers);
			if (symbol_type != 'U' && symbol_type != 'w')
				printf("%08x %c %s\n",
				       symbol.st_value, symbol_type, symbol_name);
			else
				printf("         %c %s\n",
				       symbol_type, symbol_name);
		}
	}
}

/**
 * load_section_headers32 - loads ELF section headers from file
 * @file: pointer to the ELF file
 * @elf_header: ELF header structure
 * Return: pointer to section headers or NULL on failure
 */
Elf32_Shdr *load_section_headers32(FILE *file, Elf32_Ehdr elf_header)
{
	Elf32_Shdr *section_headers;

	section_headers = malloc(elf_header.e_shentsize * elf_header.e_shnum);
	if (section_headers == NULL)
		return (NULL);

	fseek(file, elf_header.e_shoff, SEEK_SET);
	fread(section_headers,
	      elf_header.e_shentsize, elf_header.e_shnum, file);

	return (section_headers);
}

/**
 * process_elf_file32 - processes a 32-bit ELF file and prints symbols
 * @file_path: path to ELF file
 */
void process_elf_file32(char *file_path)
{
	FILE *file;
	Elf32_Ehdr elf_header;
	Elf32_Shdr *section_headers;
	Elf32_Shdr symtab_header, strtab_header;
	Elf32_Sym *symbol_table;
	char *string_table;
	int i, sym_index = -1, str_index;

	file = fopen(file_path, "rb");
	if (!file)
	{
		fprintf(stderr, "./hnm: %s: failed to open file\n", file_path);
		return;
	}
	fread(&elf_header, sizeof(Elf32_Ehdr), 1, file);

	if (elf_header.e_ident[EI_CLASS] != ELFCLASS32)
	{
		fprintf(stderr, "./hnm: %s: not a 32-bit ELF file\n", file_path);
		fclose(file);
		return;
	}
	section_headers = load_section_headers32(file, elf_header);
	if (!section_headers)
	{
		fprintf(stderr, "./hnm: %s: section load error\n", file_path);
		fclose(file);
		return;
	}

	for (i = 0; i < elf_header.e_shnum; i++)
	{
		if (section_headers[i].sh_type == SHT_SYMTAB)
		{
			sym_index = i;
			break;
		}
	}
	if (sym_index == -1)
	{
		fprintf(stderr, "./hnm: %s: no symbols\n", file_path);
		fclose(file);
		free(section_headers);
		return;
	}

	symtab_header = section_headers[sym_index];
	symbol_table = malloc(symtab_header.sh_size);
	fseek(file, symtab_header.sh_offset, SEEK_SET);
	fread(symbol_table, symtab_header.sh_size, 1, file);

	str_index = symtab_header.sh_link;
	strtab_header = section_headers[str_index];
	string_table = malloc(strtab_header.sh_size);
	fseek(file, strtab_header.sh_offset, SEEK_SET);
	fread(string_table, strtab_header.sh_size, 1, file);

	print_symbol_table32(&symtab_header,
			     symbol_table, string_table, section_headers);

	fclose(file);
	free(section_headers);
	free(symbol_table);
	free(string_table);
}
