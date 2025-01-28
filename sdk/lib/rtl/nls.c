/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * FILE:              lib/rtl/nls.c
 * PURPOSE:           National Language Support (NLS) functions
 * PROGRAMMERS:       Emanuele Aliberti
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PUSHORT NlsUnicodeUpcaseTable = NULL;
PUSHORT NlsUnicodeLowercaseTable = NULL;

USHORT NlsAnsiCodePage = 0; /* exported */
BOOLEAN NlsMbCodePageTag = FALSE; /* exported */
PUSHORT NlsAnsiToUnicodeTable = NULL;
PCHAR NlsUnicodeToAnsiTable = NULL;
PUSHORT NlsUnicodeToMbAnsiTable = NULL;
PUSHORT NlsLeadByteInfo = NULL; /* exported */

USHORT NlsOemCodePage = 0;
BOOLEAN NlsMbOemCodePageTag = FALSE; /* exported */
PUSHORT NlsOemToUnicodeTable = NULL;
PCHAR NlsUnicodeToOemTable = NULL;
PUSHORT NlsUnicodeToMbOemTable = NULL;
PUSHORT NlsOemLeadByteInfo = NULL; /* exported */

USHORT NlsOemDefaultChar = '\0';
USHORT NlsUnicodeDefaultChar = 0;


/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlCustomCPToUnicodeN(IN PCPTABLEINFO CustomCP,
                      OUT PWCHAR UnicodeString,
                      IN ULONG UnicodeSize,
                      OUT PULONG ResultSize OPTIONAL,
                      IN PCHAR CustomString,
                      IN ULONG CustomSize)
{
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!CustomCP->DBCSCodePage)
    {
        /* single-byte code page */
        if (CustomSize > (UnicodeSize / sizeof(WCHAR)))
            Size = UnicodeSize / sizeof(WCHAR);
        else
            Size = CustomSize;

        if (ResultSize)
            *ResultSize = Size * sizeof(WCHAR);

        for (i = 0; i < Size; i++)
        {
            *UnicodeString = CustomCP->MultiByteTable[(UCHAR)*CustomString];
            UnicodeString++;
            CustomString++;
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */
        ASSERT(FALSE);
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
WCHAR NTAPI
RtlpDowncaseUnicodeChar(IN WCHAR Source)
{
    USHORT Offset;

    PAGED_CODE_RTL();

    if (Source < L'A')
        return Source;

    if (Source <= L'Z')
        return Source + (L'a' - L'A');

    if (Source < 0x80)
        return Source;

    Offset = ((USHORT)Source >> 8);
    DPRINT("Offset: %hx\n", Offset);

    Offset = NlsUnicodeLowercaseTable[Offset];
    DPRINT("Offset: %hx\n", Offset);

    Offset += (((USHORT)Source & 0x00F0) >> 4);
    DPRINT("Offset: %hx\n", Offset);

    Offset = NlsUnicodeLowercaseTable[Offset];
    DPRINT("Offset: %hx\n", Offset);

    Offset += ((USHORT)Source & 0x000F);
    DPRINT("Offset: %hx\n", Offset);

    Offset = NlsUnicodeLowercaseTable[Offset];
    DPRINT("Offset: %hx\n", Offset);

    DPRINT("Result: %hx\n", Source + (SHORT)Offset);

    return Source + (SHORT)Offset;
}

/*
 * @implemented
 */
WCHAR NTAPI
RtlDowncaseUnicodeChar(IN WCHAR Source)
{
    PAGED_CODE_RTL();

    return RtlpDowncaseUnicodeChar(Source);
}

/*
 * @implemented
 */
VOID NTAPI
RtlGetDefaultCodePage(OUT PUSHORT AnsiCodePage,
                      OUT PUSHORT OemCodePage)
{
    PAGED_CODE_RTL();

    *AnsiCodePage = NlsAnsiCodePage;
    *OemCodePage = NlsOemCodePage;
}

/*
 * @implemented
 */
VOID NTAPI
RtlInitCodePageTable(IN PUSHORT TableBase,
                     OUT PCPTABLEINFO CodePageTable)
{
    PNLS_FILE_HEADER NlsFileHeader;

    PAGED_CODE_RTL();

    DPRINT("RtlInitCodePageTable() called\n");

    NlsFileHeader = (PNLS_FILE_HEADER)TableBase;

    /* Copy header fields first */
    CodePageTable->CodePage = NlsFileHeader->CodePage;
    CodePageTable->MaximumCharacterSize = NlsFileHeader->MaximumCharacterSize;
    CodePageTable->DefaultChar = NlsFileHeader->DefaultChar;
    CodePageTable->UniDefaultChar = NlsFileHeader->UniDefaultChar;
    CodePageTable->TransDefaultChar = NlsFileHeader->TransDefaultChar;
    CodePageTable->TransUniDefaultChar = NlsFileHeader->TransUniDefaultChar;

    RtlCopyMemory(&CodePageTable->LeadByte,
                  &NlsFileHeader->LeadByte,
                  MAXIMUM_LEADBYTES);

    /* Offset to wide char table is after the header */
    CodePageTable->WideCharTable =
        TableBase + NlsFileHeader->HeaderSize + 1 + TableBase[NlsFileHeader->HeaderSize];

    /* Then multibyte table (256 wchars) follows */
    CodePageTable->MultiByteTable = TableBase + NlsFileHeader->HeaderSize + 1;

    /* Check the presence of glyph table (256 wchars) */
    if (!CodePageTable->MultiByteTable[256])
        CodePageTable->DBCSRanges = CodePageTable->MultiByteTable + 256 + 1;
    else
        CodePageTable->DBCSRanges = CodePageTable->MultiByteTable + 256 + 1 + 256;

    /* Is this double-byte code page? */
    if (*CodePageTable->DBCSRanges)
    {
        CodePageTable->DBCSCodePage = 1;
        CodePageTable->DBCSOffsets = CodePageTable->DBCSRanges + 1;
    }
    else
    {
        CodePageTable->DBCSCodePage = 0;
        CodePageTable->DBCSOffsets = NULL;
    }
}

/*
 * @implemented
 */
VOID NTAPI
RtlInitNlsTables(IN PUSHORT AnsiTableBase,
                 IN PUSHORT OemTableBase,
                 IN PUSHORT CaseTableBase,
                 OUT PNLSTABLEINFO NlsTable)
{
    PAGED_CODE_RTL();

    DPRINT("RtlInitNlsTables()called\n");

    if (AnsiTableBase && OemTableBase && CaseTableBase)
    {
        RtlInitCodePageTable(AnsiTableBase, &NlsTable->AnsiTableInfo);
        RtlInitCodePageTable(OemTableBase, &NlsTable->OemTableInfo);

        NlsTable->UpperCaseTable = (PUSHORT)CaseTableBase + 2;
        NlsTable->LowerCaseTable = (PUSHORT)CaseTableBase + *((PUSHORT)CaseTableBase + 1) + 2;
    }
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlMultiByteToUnicodeN(OUT PWCHAR UnicodeString,
                       IN ULONG UnicodeSize,
                       OUT PULONG ResultSize,
                       IN PCSTR MbString,
                       IN ULONG MbSize)
{
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!NlsMbCodePageTag)
    {
        /* single-byte code page */
        if (MbSize > (UnicodeSize / sizeof(WCHAR)))
            Size = UnicodeSize / sizeof(WCHAR);
        else
            Size = MbSize;

        if (ResultSize)
            *ResultSize = Size * sizeof(WCHAR);

        for (i = 0; i < Size; i++)
        {
            UnicodeString[i] = NlsAnsiToUnicodeTable[(UCHAR)MbString[i]];
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        UCHAR Char;
        USHORT LeadByteInfo;
        PCSTR MbEnd = MbString + MbSize;

        for (i = 0; i < UnicodeSize / sizeof(WCHAR) && MbString < MbEnd; i++)
        {
            Char = *(PUCHAR)MbString++;

            if (Char < 0x80)
            {
                *UnicodeString++ = Char;
                continue;
            }

            LeadByteInfo = NlsLeadByteInfo[Char];

            if (!LeadByteInfo)
            {
                *UnicodeString++ = NlsAnsiToUnicodeTable[Char];
                continue;
            }

            if (MbString < MbEnd)
                *UnicodeString++ = NlsLeadByteInfo[LeadByteInfo + *(PUCHAR)MbString++];
        }

        if (ResultSize)
            *ResultSize = i * sizeof(WCHAR);
    }

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlConsoleMultiByteToUnicodeN(OUT PWCHAR UnicodeString,
                              IN ULONG UnicodeSize,
                              OUT PULONG ResultSize,
                              IN PCSTR MbString,
                              IN ULONG MbSize,
                              OUT PULONG Unknown)
{
    PAGED_CODE_RTL();

    UNIMPLEMENTED;
    DPRINT1("RtlConsoleMultiByteToUnicodeN calling RtlMultiByteToUnicodeN\n");
    *Unknown = 1;
    return RtlMultiByteToUnicodeN(UnicodeString,
                                  UnicodeSize,
                                  ResultSize,
                                  MbString,
                                  MbSize);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlMultiByteToUnicodeSize(OUT PULONG UnicodeSize,
                          IN PCSTR MbString,
                          IN ULONG MbSize)
{
    ULONG Length = 0;

    PAGED_CODE_RTL();

    if (!NlsMbCodePageTag)
    {
        /* single-byte code page */
        *UnicodeSize = MbSize * sizeof(WCHAR);
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        while (MbSize--)
        {
            UCHAR Char = *(PUCHAR)MbString++;

            if (Char >= 0x80 && NlsLeadByteInfo[Char])
            {
                if (MbSize)
                {
                    /* Move on */
                    MbSize--;
                    MbString++;
                }
            }

            /* Increase returned size */
            Length++;
        }

        /* Return final size */
        *UnicodeSize = Length * sizeof(WCHAR);
    }

    /* Success */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlOemToUnicodeN(OUT PWCHAR UnicodeString,
                 IN ULONG UnicodeSize,
                 OUT PULONG ResultSize OPTIONAL,
                 IN PCCH OemString,
                 IN ULONG OemSize)
{
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!NlsMbOemCodePageTag)
    {
        /* single-byte code page */
        if (OemSize > (UnicodeSize / sizeof(WCHAR)))
            Size = UnicodeSize / sizeof(WCHAR);
        else
            Size = OemSize;

        if (ResultSize)
            *ResultSize = Size * sizeof(WCHAR);

        for (i = 0; i < Size; i++)
        {
            *UnicodeString = NlsOemToUnicodeTable[(UCHAR)*OemString];
            UnicodeString++;
            OemString++;
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        UCHAR Char;
        USHORT OemLeadByteInfo;
        PCCH OemEnd = OemString + OemSize;

        for (i = 0; i < UnicodeSize / sizeof(WCHAR) && OemString < OemEnd; i++)
        {
            Char = *(PUCHAR)OemString++;

            if (Char < 0x80)
            {
                *UnicodeString++ = Char;
                continue;
            }

            OemLeadByteInfo = NlsOemLeadByteInfo[Char];

            if (!OemLeadByteInfo)
            {
                *UnicodeString++ = NlsOemToUnicodeTable[Char];
                continue;
            }

            if (OemString < OemEnd)
                *UnicodeString++ =
                    NlsOemLeadByteInfo[OemLeadByteInfo + *(PUCHAR)OemString++];
        }

        if (ResultSize)
            *ResultSize = i * sizeof(WCHAR);
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID NTAPI
RtlResetRtlTranslations(IN PNLSTABLEINFO NlsTable)
{
    PAGED_CODE_RTL();

    DPRINT("RtlResetRtlTranslations() called\n");

    /* Set ANSI data */
    NlsAnsiToUnicodeTable = (PUSHORT)NlsTable->AnsiTableInfo.MultiByteTable;
    NlsUnicodeToAnsiTable = NlsTable->AnsiTableInfo.WideCharTable;
    NlsUnicodeToMbAnsiTable = (PUSHORT)NlsTable->AnsiTableInfo.WideCharTable;
    NlsMbCodePageTag = (NlsTable->AnsiTableInfo.DBCSCodePage != 0);
    NlsLeadByteInfo = NlsTable->AnsiTableInfo.DBCSOffsets;
    NlsAnsiCodePage = NlsTable->AnsiTableInfo.CodePage;
    DPRINT("Ansi codepage %hu\n", NlsAnsiCodePage);

    /* Set OEM data */
    NlsOemToUnicodeTable = (PUSHORT)NlsTable->OemTableInfo.MultiByteTable;
    NlsUnicodeToOemTable = NlsTable->OemTableInfo.WideCharTable;
    NlsUnicodeToMbOemTable = (PUSHORT)NlsTable->OemTableInfo.WideCharTable;
    NlsMbOemCodePageTag = (NlsTable->OemTableInfo.DBCSCodePage != 0);
    NlsOemLeadByteInfo = NlsTable->OemTableInfo.DBCSOffsets;
    NlsOemCodePage = NlsTable->OemTableInfo.CodePage;
    DPRINT("Oem codepage %hu\n", NlsOemCodePage);

    /* Set Unicode case map data */
    NlsUnicodeUpcaseTable = NlsTable->UpperCaseTable;
    NlsUnicodeLowercaseTable = NlsTable->LowerCaseTable;

    /* set the default characters for RtlpDidUnicodeToOemWork */
    NlsOemDefaultChar = NlsTable->OemTableInfo.DefaultChar;
    NlsUnicodeDefaultChar = NlsTable->OemTableInfo.TransDefaultChar;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUnicodeToCustomCPN(IN PCPTABLEINFO CustomCP,
                      OUT PCHAR CustomString,
                      IN ULONG CustomSize,
                      OUT PULONG ResultSize OPTIONAL,
                      IN PWCHAR UnicodeString,
                      IN ULONG UnicodeSize)
{
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!CustomCP->DBCSCodePage)
    {
        /* single-byte code page */
        if (UnicodeSize > (CustomSize * sizeof(WCHAR)))
            Size = CustomSize;
        else
            Size = UnicodeSize / sizeof(WCHAR);

        if (ResultSize)
            *ResultSize = Size;

        for (i = 0; i < Size; i++)
        {
            *CustomString = ((PCHAR)CustomCP->WideCharTable)[*UnicodeString];
            CustomString++;
            UnicodeString++;
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */
        ASSERT(FALSE);
    }

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUnicodeToMultiByteN(OUT PCHAR MbString,
                       IN ULONG MbSize,
                       OUT PULONG ResultSize OPTIONAL,
                       IN PCWCH UnicodeString,
                       IN ULONG UnicodeSize)
{
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!NlsMbCodePageTag)
    {
        /* single-byte code page */
        Size = (UnicodeSize > (MbSize * sizeof(WCHAR)))
                ? MbSize : (UnicodeSize / sizeof(WCHAR));

        if (ResultSize)
            *ResultSize = Size;

        for (i = 0; i < Size; i++)
        {
            *MbString++ = NlsUnicodeToAnsiTable[*UnicodeString++];
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        USHORT WideChar;
        USHORT MbChar;

        for (i = MbSize, Size = UnicodeSize / sizeof(WCHAR); i && Size; i--, Size--)
        {
            WideChar = *UnicodeString++;

            if (WideChar < 0x80)
            {
                *MbString++ = LOBYTE(WideChar);
                continue;
            }

            MbChar = NlsUnicodeToMbAnsiTable[WideChar];

            if (!HIBYTE(MbChar))
            {
                *MbString++ = LOBYTE(MbChar);
                continue;
            }

            if (i >= 2)
            {
                *MbString++ = HIBYTE(MbChar);
                *MbString++ = LOBYTE(MbChar);
                i--;
            }
            else break;
        }

        if (ResultSize)
            *ResultSize = MbSize - i;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlUnicodeToMultiByteSize(OUT PULONG MbSize,
                          IN PCWCH UnicodeString,
                          IN ULONG UnicodeSize)
{
    ULONG UnicodeLength = UnicodeSize / sizeof(WCHAR);
    ULONG MbLength = 0;

    PAGED_CODE_RTL();

    if (!NlsMbCodePageTag)
    {
        /* single-byte code page */
        *MbSize = UnicodeLength;
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        while (UnicodeLength--)
        {
            USHORT WideChar = *UnicodeString++;

            if (WideChar >= 0x80 && HIBYTE(NlsUnicodeToMbAnsiTable[WideChar]))
            {
                MbLength += sizeof(WCHAR);
            }
            else
            {
                MbLength++;
            }
        }

        *MbSize = MbLength;
    }

    /* Success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlUnicodeToOemN(OUT PCHAR OemString,
                 IN ULONG OemSize,
                 OUT PULONG ResultSize OPTIONAL,
                 IN PCWCH UnicodeString,
                 IN ULONG UnicodeSize)
{
    ULONG Size = 0;

    PAGED_CODE_RTL();

    /* Bytes -> chars */
    UnicodeSize /= sizeof(WCHAR);

    if (!NlsMbOemCodePageTag)
    {
        while (OemSize && UnicodeSize)
        {
            OemString[Size] = NlsUnicodeToOemTable[*UnicodeString++];
            Size++;
            OemSize--;
            UnicodeSize--;
        }
    }
    else
    {
        while (OemSize && UnicodeSize)
        {
            USHORT OemChar = NlsUnicodeToMbOemTable[*UnicodeString++];

            if (HIBYTE(OemChar))
            {
                if (OemSize < 2)
                    break;
                OemString[Size++] = HIBYTE(OemChar);
                OemSize--;
            }
            OemString[Size++] = LOBYTE(OemChar);
            OemSize--;
            UnicodeSize--;
        }
    }

    if (ResultSize)
        *ResultSize = Size;

    return UnicodeSize ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;
}

/*
 * @implemented
 */
WCHAR NTAPI
RtlpUpcaseUnicodeChar(IN WCHAR Source)
{
    USHORT Offset;

    if (Source < 'a')
        return Source;

    if (Source <= 'z')
        return (Source - ('a' - 'A'));

    Offset = ((USHORT)Source >> 8) & 0xFF;
    Offset = NlsUnicodeUpcaseTable[Offset];

    Offset += ((USHORT)Source >> 4) & 0xF;
    Offset = NlsUnicodeUpcaseTable[Offset];

    Offset += ((USHORT)Source & 0xF);
    Offset = NlsUnicodeUpcaseTable[Offset];

    return Source + (SHORT)Offset;
}

/*
 * @implemented
 */
WCHAR NTAPI
RtlUpcaseUnicodeChar(IN WCHAR Source)
{
    PAGED_CODE_RTL();

    return RtlpUpcaseUnicodeChar(Source);
}

/*
 * @implemented
 */
NTSTATUS NTAPI
RtlUpcaseUnicodeToCustomCPN(IN PCPTABLEINFO CustomCP,
                            OUT PCHAR CustomString,
                            IN ULONG CustomSize,
                            OUT PULONG ResultSize OPTIONAL,
                            IN PWCHAR UnicodeString,
                            IN ULONG UnicodeSize)
{
    WCHAR UpcaseChar;
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!CustomCP->DBCSCodePage)
    {
        /* single-byte code page */
        if (UnicodeSize > (CustomSize * sizeof(WCHAR)))
            Size = CustomSize;
        else
            Size = UnicodeSize / sizeof(WCHAR);

        if (ResultSize)
            *ResultSize = Size;

        for (i = 0; i < Size; i++)
        {
            UpcaseChar = RtlpUpcaseUnicodeChar(*UnicodeString);
            *CustomString = ((PCHAR)CustomCP->WideCharTable)[UpcaseChar];
            ++CustomString;
            ++UnicodeString;
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */
        ASSERT(FALSE);
    }

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUpcaseUnicodeToMultiByteN(OUT PCHAR MbString,
                             IN ULONG MbSize,
                             OUT PULONG ResultSize OPTIONAL,
                             IN PCWCH UnicodeString,
                             IN ULONG UnicodeSize)
{
    WCHAR UpcaseChar;
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!NlsMbCodePageTag)
    {
        /* single-byte code page */
        if (UnicodeSize > (MbSize * sizeof(WCHAR)))
            Size = MbSize;
        else
            Size = UnicodeSize / sizeof(WCHAR);

        if (ResultSize)
            *ResultSize = Size;

        for (i = 0; i < Size; i++)
        {
            UpcaseChar = RtlpUpcaseUnicodeChar(*UnicodeString);
            *MbString = NlsUnicodeToAnsiTable[UpcaseChar];
            MbString++;
            UnicodeString++;
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */
        ASSERT(FALSE);
    }

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUpcaseUnicodeToOemN(OUT PCHAR OemString,
                       IN ULONG OemSize,
                       OUT PULONG ResultSize OPTIONAL,
                       IN PCWCH UnicodeString,
                       IN ULONG UnicodeSize)
{
    WCHAR UpcaseChar;
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!NlsMbOemCodePageTag)
    {
        /* single-byte code page */
        if (UnicodeSize > (OemSize * sizeof(WCHAR)))
            Size = OemSize;
        else
            Size = UnicodeSize / sizeof(WCHAR);

        if (ResultSize)
            *ResultSize = Size;

        for (i = 0; i < Size; i++)
        {
            UpcaseChar = RtlpUpcaseUnicodeChar(*UnicodeString);
            *OemString = NlsUnicodeToOemTable[UpcaseChar];
            OemString++;
            UnicodeString++;
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        USHORT WideChar;
        USHORT OemChar;

        for (i = OemSize, Size = UnicodeSize / sizeof(WCHAR); i && Size; i--, Size--)
        {
            WideChar = RtlpUpcaseUnicodeChar(*UnicodeString++);

            if (WideChar < 0x80)
            {
                *OemString++ = LOBYTE(WideChar);
                continue;
            }

            OemChar = NlsUnicodeToMbOemTable[WideChar];

            if (!HIBYTE(OemChar))
            {
                *OemString++ = LOBYTE(OemChar);
                continue;
            }

            if (i >= 2)
            {
                *OemString++ = HIBYTE(OemChar);
                *OemString++ = LOBYTE(OemChar);
                i--;
            }
            else break;
        }

        if (ResultSize)
            *ResultSize = OemSize - i;
    }

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
CHAR NTAPI
RtlUpperChar(IN CHAR Source)
{
    WCHAR Unicode;
    CHAR Destination;

    PAGED_CODE_RTL();

    /* Check for simple ANSI case */
    if (Source <= 'z')
    {
        /* Check for simple downcase a-z case */
        if (Source >= 'a')
        {
            /* Just XOR with the difference */
            return Source ^ ('a' - 'A');
        }
        else
        {
            /* Otherwise return the same char, it's already upcase */
            return Source;
        }
    }
    else
    {
        if (!NlsMbCodePageTag)
        {
            /* single-byte code page */

            /* ansi->unicode */
            Unicode = NlsAnsiToUnicodeTable[(UCHAR)Source];

            /* upcase conversion */
            Unicode = RtlpUpcaseUnicodeChar (Unicode);

            /* unicode -> ansi */
            Destination = NlsUnicodeToAnsiTable[(USHORT)Unicode];
        }
        else
        {
            /* multi-byte code page */
            /* FIXME */
            Destination = Source;
        }
    }

    return Destination;
}

/* EOF */

 typedef struct
{
    UINT   id;                     /* 00 lcid */
    USHORT idx;                    /* 04 index in locales array */
    USHORT name;                   /* 06 locale name */
} NLS_LOCALE_LCID_INDEX;
typedef struct
{
    USHORT name;                   /* 00 locale name */
    USHORT idx;                    /* 02 index in locales array */
    UINT   id;                     /* 04 lcid */
} NLS_LOCALE_LCNAME_INDEX;
typedef struct
{
    UINT   sname;                  /* 000 LOCALE_SNAME */
    UINT   sopentypelanguagetag;   /* 004 LOCALE_SOPENTYPELANGUAGETAG */
    USHORT ilanguage;              /* 008 LOCALE_ILANGUAGE */
    USHORT unique_lcid;            /* 00a unique id if lcid == 0x1000 */
    USHORT idigits;                /* 00c LOCALE_IDIGITS */
    USHORT inegnumber;             /* 00e LOCALE_INEGNUMBER */
    USHORT icurrdigits;            /* 010 LOCALE_ICURRDIGITS*/
    USHORT icurrency;              /* 012 LOCALE_ICURRENCY */
    USHORT inegcurr;               /* 014 LOCALE_INEGCURR */
    USHORT ilzero;                 /* 016 LOCALE_ILZERO */
    USHORT inotneutral;            /* 018 LOCALE_INEUTRAL (inverted) */
    USHORT ifirstdayofweek;        /* 01a LOCALE_IFIRSTDAYOFWEEK (monday=0) */
    USHORT ifirstweekofyear;       /* 01c LOCALE_IFIRSTWEEKOFYEAR */
    USHORT icountry;               /* 01e LOCALE_ICOUNTRY */
    USHORT imeasure;               /* 020 LOCALE_IMEASURE */
    USHORT idigitsubstitution;     /* 022 LOCALE_IDIGITSUBSTITUTION */
    UINT   sgrouping;              /* 024 LOCALE_SGROUPING (as binary string) */
    UINT   smongrouping;           /* 028 LOCALE_SMONGROUPING  (as binary string) */
    UINT   slist;                  /* 02c LOCALE_SLIST */
    UINT   sdecimal;               /* 030 LOCALE_SDECIMAL */
    UINT   sthousand;              /* 034 LOCALE_STHOUSAND */
    UINT   scurrency;              /* 038 LOCALE_SCURRENCY */
    UINT   smondecimalsep;         /* 03c LOCALE_SMONDECIMALSEP */
    UINT   smonthousandsep;        /* 040 LOCALE_SMONTHOUSANDSEP */
    UINT   spositivesign;          /* 044 LOCALE_SPOSITIVESIGN */
    UINT   snegativesign;          /* 048 LOCALE_SNEGATIVESIGN */
    UINT   s1159;                  /* 04c LOCALE_S1159 */
    UINT   s2359;                  /* 050 LOCALE_S2359 */
    UINT   snativedigits;          /* 054 LOCALE_SNATIVEDIGITS (array of single digits) */
    UINT   stimeformat;            /* 058 LOCALE_STIMEFORMAT (array of formats) */
    UINT   sshortdate;             /* 05c LOCALE_SSHORTDATE (array of formats) */
    UINT   slongdate;              /* 060 LOCALE_SLONGDATE (array of formats) */
    UINT   syearmonth;             /* 064 LOCALE_SYEARMONTH (array of formats) */
    UINT   sduration;              /* 068 LOCALE_SDURATION (array of formats) */
    USHORT idefaultlanguage;       /* 06c LOCALE_IDEFAULTLANGUAGE */
    USHORT idefaultansicodepage;   /* 06e LOCALE_IDEFAULTANSICODEPAGE */
    USHORT idefaultcodepage;       /* 070 LOCALE_IDEFAULTCODEPAGE */
    USHORT idefaultmaccodepage;    /* 072 LOCALE_IDEFAULTMACCODEPAGE */
    USHORT idefaultebcdiccodepage; /* 074 LOCALE_IDEFAULTEBCDICCODEPAGE */
    USHORT old_geoid;              /* 076 LOCALE_IGEOID (older version?) */
    USHORT ipapersize;             /* 078 LOCALE_IPAPERSIZE */
    BYTE   islamic_cal[2];         /* 07a calendar id for islamic calendars (?) */
    UINT   scalendartype;          /* 07c string, first char is LOCALE_ICALENDARTYPE, next chars are LOCALE_IOPTIONALCALENDAR */
    UINT   sabbrevlangname;        /* 080 LOCALE_SABBREVLANGNAME */
    UINT   siso639langname;        /* 084 LOCALE_SISO639LANGNAME */
    UINT   senglanguage;           /* 088 LOCALE_SENGLANGUAGE */
    UINT   snativelangname;        /* 08c LOCALE_SNATIVELANGNAME */
    UINT   sengcountry;            /* 090 LOCALE_SENGCOUNTRY */
    UINT   snativectryname;        /* 094 LOCALE_SNATIVECTRYNAME */
    UINT   sabbrevctryname;        /* 098 LOCALE_SABBREVCTRYNAME */
    UINT   siso3166ctryname;       /* 09c LOCALE_SISO3166CTRYNAME */
    UINT   sintlsymbol;            /* 0a0 LOCALE_SINTLSYMBOL */
    UINT   sengcurrname;           /* 0a4 LOCALE_SENGCURRNAME */
    UINT   snativecurrname;        /* 0a8 LOCALE_SNATIVECURRNAME */
    UINT   fontsignature;          /* 0ac LOCALE_FONTSIGNATURE (binary string) */
    UINT   siso639langname2;       /* 0b0 LOCALE_SISO639LANGNAME2 */
    UINT   siso3166ctryname2;      /* 0b4 LOCALE_SISO3166CTRYNAME2 */
    UINT   sparent;                /* 0b8 LOCALE_SPARENT */
    UINT   sdayname;               /* 0bc LOCALE_SDAYNAME1 (array of days 1..7) */
    UINT   sabbrevdayname;         /* 0c0 LOCALE_SABBREVDAYNAME1  (array of days 1..7) */
    UINT   smonthname;             /* 0c4 LOCALE_SMONTHNAME1 (array of months 1..13) */
    UINT   sabbrevmonthname;       /* 0c8 LOCALE_SABBREVMONTHNAME1 (array of months 1..13) */
    UINT   sgenitivemonth;         /* 0cc equivalent of LOCALE_SMONTHNAME1 for genitive months */
    UINT   sabbrevgenitivemonth;   /* 0d0 equivalent of LOCALE_SABBREVMONTHNAME1 for genitive months */
    UINT   calnames;               /* 0d4 array of calendar names */
    UINT   customsorts;            /* 0d8 array of custom sort names */
    USHORT inegativepercent;       /* 0dc LOCALE_INEGATIVEPERCENT */
    USHORT ipositivepercent;       /* 0de LOCALE_IPOSITIVEPERCENT */
    USHORT unknown1;               /* 0e0 */
    USHORT ireadinglayout;         /* 0e2 LOCALE_IREADINGLAYOUT */
    USHORT unknown2[2];            /* 0e4 */
    UINT   unused1;                /* 0e8 unused? */
    UINT   sengdisplayname;        /* 0ec LOCALE_SENGLISHDISPLAYNAME */
    UINT   snativedisplayname;     /* 0f0 LOCALE_SNATIVEDISPLAYNAME */
    UINT   spercent;               /* 0f4 LOCALE_SPERCENT */
    UINT   snan;                   /* 0f8 LOCALE_SNAN */
    UINT   sposinfinity;           /* 0fc LOCALE_SPOSINFINITY */
    UINT   sneginfinity;           /* 100 LOCALE_SNEGINFINITY */
    UINT   unused2;                /* 104 unused? */
    UINT   serastring;             /* 108 CAL_SERASTRING */
    UINT   sabbreverastring;       /* 10c CAL_SABBREVERASTRING */
    UINT   unused3;                /* 110 unused? */
    UINT   sconsolefallbackname;   /* 114 LOCALE_SCONSOLEFALLBACKNAME */
    UINT   sshorttime;             /* 118 LOCALE_SSHORTTIME (array of formats) */
    UINT   sshortestdayname;       /* 11c LOCALE_SSHORTESTDAYNAME1 (array of days 1..7) */
    UINT   unused4;                /* 120 unused? */
    UINT   ssortlocale;            /* 124 LOCALE_SSORTLOCALE */
    UINT   skeyboardstoinstall;    /* 128 LOCALE_SKEYBOARDSTOINSTALL */
    UINT   sscripts;               /* 12c LOCALE_SSCRIPTS */
    UINT   srelativelongdate;      /* 130 LOCALE_SRELATIVELONGDATE */
    UINT   igeoid;                 /* 134 LOCALE_IGEOID */
    UINT   sshortestam;            /* 138 LOCALE_SSHORTESTAM */
    UINT   sshortestpm;            /* 13c LOCALE_SSHORTESTPM */
    UINT   smonthday;              /* 140 LOCALE_SMONTHDAY (array of formats) */
    UINT   keyboard_layout;        /* 144 keyboard layouts */
} NLS_LOCALE_DATA;
typedef struct
{
    UINT   offset;                 /* 00 offset to version, always 8? */
    UINT   unknown1;               /* 04 */
    UINT   version;                /* 08 file format version */
    UINT   magic;                  /* 0c magic 'NSDS' */
    UINT   unknown2[3];            /* 10 */
    USHORT header_size;            /* 1c size of this header (?) */
    USHORT nb_lcids;               /* 1e number of lcids in index */
    USHORT nb_locales;             /* 20 number of locales in array */
    USHORT locale_size;            /* 22 size of NLS_LOCALE_DATA structure */
    UINT   locales_offset;         /* 24 offset of locales array */
    USHORT nb_lcnames;             /* 28 number of lcnames in index */
    USHORT pad;                    /* 2a */
    UINT   lcids_offset;           /* 2c offset of lcids index */
    UINT   lcnames_offset;         /* 30 offset of lcnames index */
    UINT   unknown3;               /* 34 */
    USHORT nb_calendars;           /* 38 number of calendars in array */
    USHORT calendar_size;          /* 3a size of calendar structure */
    UINT   calendars_offset;       /* 3c offset of calendars array */
    UINT   strings_offset;         /* 40 offset of strings data */
    USHORT unknown4[4];            /* 44 */
} NLS_LOCALE_HEADER;
#define MUI_FULL_LANGUAGE             0x01
#define MUI_LANGUAGE_ID               0x04
#define MUI_LANGUAGE_NAME             0x08
#define MUI_MERGE_SYSTEM_FALLBACK     0x10
#define MUI_MERGE_USER_FALLBACK       0x20
#define MUI_UI_FALLBACK               MUI_MERGE_SYSTEM_FALLBACK | MUI_MERGE_USER_FALLBACK
#define MUI_MACHINE_LANGUAGE_SETTINGS 0x400
 
#define LANG_SYSTEM_DEFAULT       MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT)
#define LANG_USER_DEFAULT         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
#define LOCALE_SYSTEM_DEFAULT     MAKELCID(LANG_SYSTEM_DEFAULT, SORT_DEFAULT)
#define LOCALE_USER_DEFAULT       MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT)
#define LOCALE_CUSTOM_DEFAULT     MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_CUSTOM_DEFAULT), SORT_DEFAULT)
#define LOCALE_CUSTOM_UNSPECIFIED MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_CUSTOM_UNSPECIFIED), SORT_DEFAULT)
#define LOCALE_CUSTOM_UI_DEFAULT  MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_UI_CUSTOM_DEFAULT), SORT_DEFAULT)
#define LOCALE_NEUTRAL            MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT)
#define LOCALE_INVARIANT          MAKELCID(MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL), SORT_DEFAULT)
#define LOCALE_CUSTOM_UNSPECIFIED MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_CUSTOM_UNSPECIFIED), SORT_DEFAULT)
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

 
static LCID system_lcid;
static const NLS_LOCALE_LCID_INDEX *lcids_index;
static const NLS_LOCALE_HEADER *locale_table;
static const WCHAR *locale_strings;
static const NLS_LOCALE_DATA *get_locale_data( UINT idx )
{
    ULONG offset = locale_table->locales_offset + idx * locale_table->locale_size;
    return (const NLS_LOCALE_DATA *)((const char *)locale_table + offset);
}
 static const NLS_LOCALE_LCID_INDEX *find_lcid_entry( LCID lcid )
{
    int min = 0, max = locale_table->nb_lcids - 1;
    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (lcid < lcids_index[pos].id) max = pos - 1;
        else if (lcid > lcids_index[pos].id) min = pos + 1;
        else return &lcids_index[pos];
    }
    return NULL;
}
/******************************************************************
 *      RtlLcidToLocaleName   (NTDLL.@)
 */
NTSTATUS WINAPI RtlLcidToLocaleName( LCID lcid, UNICODE_STRING *str, ULONG flags, BOOLEAN alloc )
{
    const NLS_LOCALE_LCID_INDEX *entry;
    const WCHAR *name;
    ULONG len;
    if (!str) return STATUS_INVALID_PARAMETER_2;
    switch (lcid)
    {
    case LOCALE_USER_DEFAULT:
        NtQueryDefaultLocale( TRUE, &lcid );
        break;
    case LOCALE_SYSTEM_DEFAULT:
    case LOCALE_CUSTOM_DEFAULT:
        lcid = system_lcid;
        break;
    case LOCALE_CUSTOM_UI_DEFAULT:
        return STATUS_UNSUCCESSFUL;
    case LOCALE_CUSTOM_UNSPECIFIED:
        return STATUS_INVALID_PARAMETER_1;
    }
    if (!(entry = find_lcid_entry(lcid ))) return STATUS_INVALID_PARAMETER_1;
    /* reject neutral locale unless flag 2 is set */
    if (!(flags & 2) && !get_locale_data(entry->idx )->inotneutral)
        return STATUS_INVALID_PARAMETER_1;
    name = locale_strings + entry->name;
    len = *name++;
    if (alloc)
    {
        if (!(str->Buffer = RtlAllocateHeap( RtlGetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
            return STATUS_NO_MEMORY;
        str->MaximumLength = (len + 1) * sizeof(WCHAR);
    }
    else if (str->MaximumLength < (len + 1) * sizeof(WCHAR)) return STATUS_BUFFER_TOO_SMALL;
    wcscpy( str->Buffer, name );
    str->Length = len * sizeof(WCHAR);
   // TRACE( "%04lx -> %s\n", lcid, debugstr_us(str) );
    return STATUS_SUCCESS;
}
static NTSTATUS get_dummy_preferred_ui_language( DWORD flags, LANGID lang, ULONG *count,
                                                 WCHAR *buffer, ULONG *size )
{
    WCHAR name[LOCALE_NAME_MAX_LENGTH + 2];
    NTSTATUS status;
    ULONG len;
   // FIXME("(0x%lx %#x %p %p %p) returning a dummy value (current locale)\n", flags, lang, count, buffer, size);
    if (flags & MUI_LANGUAGE_ID) swprintf( name, L"%04lX", lang );
    else
    {
        UNICODE_STRING str;
        if (lang == LOCALE_CUSTOM_UNSPECIFIED)
            NtQueryInstallUILanguage( &lang );
        str.Buffer = name;
        str.MaximumLength = sizeof(name);
        status = RtlLcidToLocaleName( lang, &str, 0, FALSE );
        if (status) return status;
    }
    len = wcslen( name ) + 2;
    name[len - 1] = 0;
    if (buffer)
    {
        if (len > *size)
        {
            *size = len;
            return STATUS_BUFFER_TOO_SMALL;
        }
        memcpy( buffer, name, len * sizeof(WCHAR) );
    }
    *size = len;
    *count = 1;
//    TRACE("returned variable content: %ld, \"%s\", %ld\n", *count, debugstr_w(buffer), *size);
    return STATUS_SUCCESS;
}
/**************************************************************************
 *      RtlGetThreadPreferredUILanguages   (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetThreadPreferredUILanguages( DWORD flags, ULONG *count, WCHAR *buffer, ULONG *size )
{
    LANGID ui_language;
 //   FIXME( "%08lx, %p, %p %p\n", flags, count, buffer, size );
    NtQueryDefaultUILanguage( &ui_language );
    return get_dummy_preferred_ui_language( flags, ui_language, count, buffer, size );
}
