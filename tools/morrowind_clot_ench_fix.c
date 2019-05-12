#define _GNU_SOURCE

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
	int off;
	uint32_t len;
	uint8_t* data;
} datarec_t;

datarec_t readrec(const char* typ, uint8_t* buf, size_t buflen, int fixoff)
{
	datarec_t ret = {0};
	ret.off = -1;
	
	uint8_t* ptr = memmem(buf,buflen,typ,strlen(typ));
	if (!ptr) return ret;
	
	ptr += strlen(typ);
	ret.len = *((uint32_t*)ptr);
	//printf("%s record of length %u\n",typ,ret.len);
	
	if (ret.len > 0) ret.data = ptr + 4 + fixoff;
	
	ret.off = ptr - buf;
	return ret;
}

int main(int argc, char* argv[])
{
	assert(argc > 2);
	
	FILE* fi = fopen(argv[1],"rb");
	assert(fi);
	
	FILE* fo = fopen(argv[2],"wb");
	assert(fi);
	
	fseek(fi,0,SEEK_END);
	size_t inlen = ftell(fi);
	fseek(fi,0,SEEK_SET);
	
	uint8_t* inp = (uint8_t*)malloc(inlen);
	assert(fread(inp,inlen,1,fi));
	fclose(fi);
	
	uint8_t* pos = inp;
	size_t rem = inlen;
	int cnt = 0;
	
	for (;;cnt++) {
		datarec_t cp = readrec("CLOT",pos,rem,8);
		if (cp.off < 0) break;
		
		pos += cp.off;
		rem -= cp.off;
		assert(cp.data);
		
		datarec_t name = readrec("NAME",cp.data,cp.len,0);
		assert(name.off >= 0 && name.data && name.data[name.len-1] == 0);
		
		int soff = name.off + name.len;
		datarec_t ctdt = readrec("CTDT",cp.data+soff,cp.len-soff,8);
		assert(ctdt.data);
		uint16_t* price = (uint16_t*)ctdt.data;
		uint16_t* ench = (uint16_t*)(ctdt.data+2);
		uint16_t nench = (*ench > 100)? *ench : 200; // <----------------
		
		printf("Clothing record (%s) of length %u @ 0x%08X : price = %hu, ench = %hu (was %hu)\n",
				name.data, cp.len, (uint32_t)(pos-inp), *price, nench, *ench);
		
		*ench = nench;
	}
	
	assert(fwrite(inp,inlen,1,fo));
	
	free(inp);
	fclose(fo);
	
	return 0;
}
