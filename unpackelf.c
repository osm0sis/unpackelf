/*
 * unpackelf.c by tobias.waldvogel @ xda-developers
 * https://github.com/tobiaswaldvogel/and_boot_tools/blob/master/bootimg/unpackelf.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <libgen.h>

typedef uint16_t	__u16t;
typedef int16_t		__s16t;
typedef uint32_t	__u32t;
typedef int32_t		__s32t;
typedef uint64_t	__u64t;
typedef int64_t		__s64t;

typedef __u32t	Elf32_Addr;
typedef __u16t	Elf32_Half;
typedef __u32t	Elf32_Off;
typedef __s32t	Elf32_Sword;
typedef __u32t	Elf32_Word;

typedef __u64t	Elf64_Addr;
typedef __u16t	Elf64_Half;
typedef __s16t	Elf64_SHalf;
typedef __u64t	Elf64_Off;
typedef __s32t	Elf64_Sword;
typedef __u32t	Elf64_Word;
typedef __u64t	Elf64_Xword;
typedef __s64t	Elf64_Sxword;

#define EI_MAG0			0
#define EI_MAG1			1
#define EI_MAG2			2
#define EI_MAG3			3
#define EI_CLASS		4
#define EI_DATA			5
#define EI_VERSION		6
#define EI_OSABI		7
#define EI_PAD			8

#define ELFCLASSNONE	0
#define ELFCLASS32		1
#define ELFCLASS64		2
#define ELFCLASSNUM		3

#define ET_EXEC		2
#define EM_ARM		40
#define EI_NIDENT	16

typedef struct elf32_hdr{
	unsigned char	e_ident[EI_NIDENT];
	Elf32_Half		e_type;
	Elf32_Half		e_machine;
	Elf32_Word		e_version;
	Elf32_Addr		e_entry;	/* Entry point */
	Elf32_Off		e_phoff;
	Elf32_Off		e_shoff;
	Elf32_Word		e_flags;
	Elf32_Half		e_ehsize;
	Elf32_Half		e_phentsize;
	Elf32_Half		e_phnum;
	Elf32_Half		e_shentsize;
	Elf32_Half		e_shnum;
	Elf32_Half		e_shstrndx;
} Elf32_Ehdr;

typedef struct elf64_hdr {
	unsigned char	e_ident[EI_NIDENT];	/* ELF "magic number" */
	Elf64_Half		e_type;
	Elf64_Half		e_machine;
	Elf64_Word		e_version;
	Elf64_Addr		e_entry;	/* Entry point virtual address */
	Elf64_Off		e_phoff;	/* Program header table file offset */
	Elf64_Off		e_shoff;	/* Section header table file offset */
	Elf64_Word		e_flags;
	Elf64_Half		e_ehsize;
	Elf64_Half		e_phentsize;
	Elf64_Half		e_phnum;
	Elf64_Half		e_shentsize;
	Elf64_Half		e_shnum;
	Elf64_Half		e_shstrndx;
} Elf64_Ehdr;

typedef struct elf32_phdr{
	Elf32_Word	p_type;
	Elf32_Off	p_offset;
	Elf32_Addr	p_vaddr;
	Elf32_Addr	p_paddr;
	Elf32_Word	p_filesz;
	Elf32_Word	p_memsz;
	Elf32_Word	p_flags;
	Elf32_Word	p_align;
} Elf32_Phdr;

typedef struct elf64_phdr {
	Elf64_Word	p_type;
	Elf64_Word	p_flags;
	Elf64_Off	p_offset;		/* Segment file offset */
	Elf64_Addr	p_vaddr;		/* Segment virtual address */
	Elf64_Addr	p_paddr;		/* Segment physical address */
	Elf64_Xword	p_filesz;		/* Segment size in file */
	Elf64_Xword	p_memsz;		/* Segment size in memory */
	Elf64_Xword	p_align;		/* Segment alignment, file & memory */
} Elf64_Phdr;

typedef struct elf32_shdr {
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
} Elf32_Shdr;

typedef struct elf64_shdr {
	Elf64_Word	sh_name;		/* Section name, index in string tbl */
	Elf64_Word	sh_type;		/* Type of section */
	Elf64_Xword	sh_flags;		/* Miscellaneous section attributes */
	Elf64_Addr	sh_addr;		/* Section virtual addr at execution */
	Elf64_Off	sh_offset;		/* Section file offset */
	Elf64_Xword	sh_size;		/* Size of section in bytes */
	Elf64_Word	sh_link;		/* Index of another section */
	Elf64_Word	sh_info;		/* Additional section information */
	Elf64_Xword	sh_addralign;	/* Section alignment */
	Elf64_Xword	sh_entsize;		/* Entry size if section holds table */
} Elf64_Shdr;

FILE			*f = NULL;
static char		*obj_name[] = { "KERNEL", "RAMDISK", "TAGS" };
static char		*off_name[] = { "kernel_offset", "ramdisk_offset", "tags_offset" };
unsigned char	*obj[4] = { NULL, NULL, NULL, NULL };
size_t			obj_len[4];
__u32t			obj_off[4];

unsigned char	buffer[4096];	
int				unpackelf_quiet = 0;
int				unpackelf_headeronly = 0;

void cleanup()
{
	int i;

	for (i=0; i<sizeof(obj)/sizeof(obj[0]); i++)
		if (obj[i])
			free(obj[i]);
}

void die(int rc, const char *why, ...)
{
	va_list	ap;

	if (why && !unpackelf_quiet) {
		va_start(ap, why);
		fprintf(stderr,"error: ");
		vfprintf(stderr, why, ap);
		fprintf(stderr,"\n\n");
		va_end(ap);
	}

	cleanup();
	exit(rc);
}

void read_elf(char *kernelimg)
{
	unsigned char	elfclass = ELFCLASSNONE;
	int				i;

	f = fopen(kernelimg, "rb");
	if (!f)
		die(1, "Could not open kernel image %s", kernelimg);

	fread(buffer, 1, sizeof(buffer), f);
	if (buffer[EI_MAG0] != 0x7f ||
		buffer[EI_MAG1] != 'E' ||
		buffer[EI_MAG2] != 'L' ||
		buffer[EI_MAG3] != 'F' )
		die(1, "No ELF header found");

	fseek(f, 0, SEEK_SET);

	elfclass = buffer[EI_CLASS];

	if (elfclass == ELFCLASS32) {
		Elf32_Ehdr		elf32;
		Elf32_Phdr		ph32[4];
		Elf32_Shdr		sh32;

		fread(&elf32, 1, sizeof(elf32), f);

		if (elf32.e_ehsize != sizeof(elf32))
			die(1, "Header size not %d", sizeof(elf32));

		if (elf32.e_machine != EM_ARM)
			die(1, "ELF machine is not ARM");

		if (elf32.e_version != 1)
			die(1, "Unknown ELF version");

		if (elf32.e_phentsize != sizeof(ph32[0]))
			die(1, "Program header size not %d", sizeof(ph32[0]));

		if (elf32.e_phnum < 2 || elf32.e_phnum > 4)
			die(1, "Unexpected number of elements: %d", elf32.e_phnum);

		if (elf32.e_shnum && elf32.e_shentsize != sizeof(sh32))
			die(1, "Section header size not %d", sizeof(sh32));

		if (elf32.e_shnum > 1)
			die(1, "More than one section header");

		fseek(f, elf32.e_phoff, SEEK_SET);
		fread(ph32, 1, sizeof(ph32), f);

		for (i=0; i<elf32.e_phnum; i++) {
			obj_len[i] = (size_t)ph32[i].p_filesz;
			obj[i] = (unsigned char*)malloc(obj_len[i]);
			fseek(f, ph32[i].p_offset, SEEK_SET);
			fread(obj[i], 1, obj_len[i], f);
			obj_off[i] = ph32[i].p_paddr;
		}

		if (elf32.e_shnum) {
			fseek(f, elf32.e_shoff, SEEK_SET);
			fread(&sh32, 1, sizeof(sh32), f);

			obj_len[3] = (size_t)sh32.sh_size;
			obj[3] = (unsigned char*)malloc(obj_len[3]+1);
			fseek(f, sh32.sh_offset, SEEK_SET);
			fread(obj[3], 1, obj_len[3], f);
			obj[3] = obj[3]+8;
		}

		fclose(f);
		f = NULL;
		return;
	}

	if (elfclass == ELFCLASS64) {
		Elf64_Ehdr		elf64;
		Elf64_Phdr		ph64[4];
		Elf64_Shdr		sh64;

		fread(&elf64, 1, sizeof(elf64), f);

		if (elf64.e_ehsize != sizeof(elf64))
			die(1, "Header size not %d", sizeof(elf64));

		if (elf64.e_machine != EM_ARM)
			die(1, "ELF machine is not ARM");

		if (elf64.e_version != 1)
			die(1, "Unknown ELF version");

		if (elf64.e_phentsize != sizeof(ph64[0]))
			die(1, "Program header size not %d", sizeof(ph64[0]));

		if (elf64.e_phnum < 2 || elf64.e_phnum > 4)
			die(1, "Unexpected number of elements: %d", elf64.e_phnum);

		if (elf64.e_shnum && elf64.e_shentsize != sizeof(sh64))
			die(1, "Section header size not %d", sizeof(sh64));

		if (elf64.e_shnum > 1)
			die(1, "More than one section header");

		fseek(f, (long)elf64.e_phoff, SEEK_SET);
		fread(ph64, 1, sizeof(ph64), f);

		for (i=0; i<elf64.e_phnum; i++) {
			obj_len[i] = (size_t)ph64[i].p_filesz;
			obj[i] = (unsigned char*)malloc(obj_len[i]);
			fseek(f, (long)ph64[i].p_offset, SEEK_SET);
			fread(obj[i], 1, obj_len[i], f);
			obj_off[i] = (__u32t)ph64[i].p_paddr;
		}

		if (elf64.e_shnum) {
			fseek(f, (long)elf64.e_shoff, SEEK_SET);
			fread(&sh64, 1, sizeof(sh64), f);

			obj_len[3] = (size_t)sh64.sh_size;
			obj[3] = (unsigned char*)malloc(obj_len[3]+1);
			fseek(f, (long)sh64.sh_offset, SEEK_SET);
			fread(obj[3], 1, obj_len[3], f);
			obj[3] = obj[3]+8;
		}

		fclose(f);
		f = NULL;
		return;
	}
	
	die(1, "Unknown ELF class %d", elfclass);
}

int unpackelf_usage()
{
	fprintf(stderr, "Usage: unpackelf -i kernel_elf_image [ -o output_dir | -k kernel | -r ramdisk | -d dt | -h | -q ]\n\n");
	return 200;
}

void fwrite_str(char* file, char* output)
{
	f = fopen(file, "w");
	if (!f)
		die(1, "Could not open file %s for writing", file);
	fwrite(output, strlen(output), 1, f);
	fwrite("\n", 1, 1, f);
	fclose(f);
}

int main(int argc, char** argv)
{
	char	*image_file = NULL;
	char	*out_file[3] = { "kernel", "ramdisk", "dt" };
	char	*out_dir = "./";
	char	out_name[PATH_MAX];
	char	out_tmp[200];
	int		found_cmdline = 0;
	int		pagesize = 4096;	/* hardcoded as there is no way to determine it from the ELF header */
	int		base = 0;
	int		i;

	argc--;
	argv++;
	while(argc > 0){
		char *arg = argv[0];
		if (!strcmp(arg, "-q")) {
			unpackelf_quiet = 1;
			argc -= 1;
			argv += 1;
		} else if (!strcmp(arg, "-h")) {
			unpackelf_headeronly = 1;
			argc -= 1;
			argv += 1;
		} else if (argc >= 2) {
			char *val = argv[1];
			argc -= 2;
			argv += 2;
			if (!strcmp(arg, "-i"))
				image_file = val;
			else if (!strcmp(arg, "-o"))
				out_dir = val;
			else if (!strcmp(arg, "-k"))
				out_file[0] = val;
			else if (!strcmp(arg, "-r"))
				out_file[1] = val;
			else if (!strcmp(arg, "-d"))
				out_file[2] = val;
			else
				return unpackelf_usage();
		} else {
			return unpackelf_usage();
		}
	}

	if (!image_file)
		return unpackelf_usage();

	read_elf(image_file);
	if (obj_off[1] > 0x10000000)
		base = obj_off[0] - 0x00008000;
	sprintf(out_tmp, "0x%08x", base);
	sprintf(out_name, "%s/%s-base", out_dir, basename(image_file));
	if (!unpackelf_headeronly)
		fwrite_str(out_name, out_tmp);
	if (!unpackelf_quiet)
		printf("BOARD_KERNEL_BASE=\"0x%08x\"\n", base);

	for (i=0; i<=3; i++) {
		if ((!obj_len[i]) || (found_cmdline))
			continue;
		if (obj_len[i] <= 4096) {
			obj[i][strcspn((const char*)obj[i], "\n")] = 0;
			sprintf(out_name, "%s/%s-cmdline", out_dir, basename(image_file));
			if (!unpackelf_headeronly)
				fwrite_str(out_name, (char*)obj[i]);
			if (!unpackelf_quiet)
				printf("BOARD_KERNEL_CMDLINE=\"%s\"\n", obj[i]);
			found_cmdline = 1;
			continue;
		}

		sprintf(out_name, "%s/%s-%s", out_dir, basename(image_file), out_file[i]);
		if (!unpackelf_headeronly) {
			f = fopen(out_name, "wb+");
			if (!f)
				die(1, "Could not open file %s for writing", out_name);
			fwrite(obj[i], 1, obj_len[i], f);
			fclose(f);
		}

		sprintf(out_tmp, "0x%08x", obj_off[i] - base);
		sprintf(out_name, "%s/%s-%s", out_dir, basename(image_file), off_name[i]);
		if (!unpackelf_headeronly)
			fwrite_str(out_name, out_tmp);
		if (!unpackelf_quiet)
			printf("BOARD_%s_OFFSET=\"0x%08x\"\n", obj_name[i], obj_off[i] - base);
	}

	sprintf(out_tmp, "%d", pagesize);
	sprintf(out_name, "%s/%s-pagesize", out_dir, basename(image_file));
	if (!unpackelf_headeronly)
		fwrite_str(out_name, out_tmp);
	if (!unpackelf_quiet)
		printf("BOARD_PAGE_SIZE=\"%d\"\n", pagesize);
	return 0;
}
