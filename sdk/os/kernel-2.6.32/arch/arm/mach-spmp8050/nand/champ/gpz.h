#ifndef GPZ_H
#define GPZ_H

// Block flag constant
#define		BLOCKFLG_PACKED			0
#define		BLOCKFLG_UNPACKED		1

#define GPZMAGIC 0x47505A46  //"GPZF"

typedef struct _BlockTableGPZip_
{
	unsigned int	Offset;			// Block offset on packed file
	unsigned short	Size;			// Block size
	unsigned short	Flags;			// Block flags
} BlockTableGPZip;

typedef struct _FileHeaderGPZip_
{
	unsigned int	GPZipFileID;	// Magic number: 0x47505A46  "GPZF"
	int				BlockSize;		// Unpack block size
	int				BlockNum;		// Total active block number
	int				HeaderSize;		// Header file size	
	int				BlockDataOffset;// Offset to Block data
	unsigned int	Reserved[3];	// For 32 bytes alignment
} FileHeaderGPZip;

#endif //GPZ_H

